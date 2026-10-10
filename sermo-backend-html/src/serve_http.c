/* SPDX-License-Identifier: GPL-2.0-or-later */
/*
 * Copyright (C) 2026 S. Cage
 * haplo-dialog <devel@haplo-dialog.fr>
 */
/*
 * serve_http.c — micro-serveur local et éphémère du backend HTML.
 * sermo — haplo-dialog <devel@haplo-dialog.fr>
 *
 * POURQUOI. Une page exportée ne peut pas répondre : elle part dans le
 * navigateur et rien ne revient. Pour que `sermo` rende ses valeurs comme les
 * autres backends, il faut un canal de retour. Celui-ci est le plus
 * court possible : une socket sur la boucle locale, une page servie, un
 * formulaire reçu, les valeurs imprimées, et le processus sort.
 *
 * C'EST LE PREMIER ÉCOUTEUR RÉSEAU DU PROJET. Il n'est actif qu'en
 * SERMO_HTML_MODE=serve ; en export et en interactive, aucune socket n'est
 * créée. Les gardes, toutes non négociables :
 *
 *   - Écoute sur 127.0.0.1 UNIQUEMENT. Jamais 0.0.0.0 : la page n'est pas
 *     joignable depuis le réseau, seulement depuis cette machine.
 *   - Port éphémère tiré par le noyau (bind sur 0), pas un port fixe
 *     prévisible.
 *   - Jeton à usage unique de 128 bits dans le chemin, tiré de getrandom()
 *     (repli /dev/urandom). Une requête sans le bon jeton reçoit 404 et ne
 *     révèle rien. Comparaison à temps constant.
 *   - Ne sert QUE cette page : aucun accès au système de fichiers, aucun
 *     chemin traversable, aucune autre méthode que GET et POST.
 *   - Requête bornée (SERVE_MAX_REQ) : un envoi plus gros est coupé.
 *   - Délai d'expiration (SERMO_HTML_TIMEOUT, défaut 300 s) : le processus ne
 *     reste jamais en écoute indéfiniment.
 *   - Une seule réponse de formulaire : après elle, on sort.
 */
#ifndef _GNU_SOURCE
#define _GNU_SOURCE
#endif
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <errno.h>
#include <time.h>
#include <fcntl.h>
#include <netinet/in.h>
#include <arpa/inet.h>
#include <sys/socket.h>
#include <sys/select.h>

#include "serve_http.h"

#define SERVE_MAX_REQ   (256 * 1024)   /* requête au-delà = coupée */
#define NONCE_BYTES     16             /* 128 bits */

/* ─── Jeton ──────────────────────────────────────────────────────────────── */
static int nonce_make(char *hex, size_t hexsz)
{
    unsigned char raw[NONCE_BYTES];
    FILE *f = fopen("/dev/urandom", "rb");
    if (!f) return -1;
    size_t got = fread(raw, 1, sizeof raw, f);
    fclose(f);
    if (got != sizeof raw) return -1;
    if (hexsz < sizeof(raw) * 2 + 1) return -1;
    for (size_t i = 0; i < sizeof raw; i++)
        snprintf(hex + i * 2, 3, "%02x", raw[i]);
    return 0;
}

/* Comparaison à temps constant : ne pas donner d'indice par la durée. */
static int nonce_equal(const char *a, const char *b, size_t n)
{
    unsigned char d = 0;
    for (size_t i = 0; i < n; i++) d |= (unsigned char)(a[i] ^ b[i]);
    return d == 0;
}

/* ─── Décodage du corps de formulaire ────────────────────────────────────── */
static void url_decode(char *s)
{
    char *o = s;
    for (char *p = s; *p; p++) {
        if (*p == '+') *o++ = ' ';
        else if (*p == '%' && p[1] && p[2]) {
            int hi = p[1], lo = p[2];
            hi = (hi >= '0' && hi <= '9') ? hi - '0' : (hi | 32) - 'a' + 10;
            lo = (lo >= '0' && lo <= '9') ? lo - '0' : (lo | 32) - 'a' + 10;
            if (hi >= 0 && hi < 16 && lo >= 0 && lo < 16) { *o++ = (char)(hi * 16 + lo); p += 2; }
            else *o++ = *p;
        } else *o++ = *p;
    }
    *o = '\0';
}

/* Échappement pour un contexte VAR="…" de shell, comme le cœur. */
static void print_shell_escaped(const char *v)
{
    for (const unsigned char *p = (const unsigned char *)v; *p; p++) {
        if (*p == '\\' || *p == '"' || *p == '$' || *p == '`') putchar('\\');
        putchar((int)*p);
    }
}

/* Un nom de variable est-il un identifiant shell ? Le nom vient du formulaire :
 * on n'imprime jamais une chaine arbitraire a gauche d'un « = ». */
static int name_ok(const char *name)
{
    if (!(*name == '_' || (*name >= 'A' && *name <= 'Z') || (*name >= 'a' && *name <= 'z')))
        return 0;
    for (const char *q = name; *q; q++)
        if (!(*q == '_' || (*q >= '0' && *q <= '9') ||
              (*q >= 'A' && *q <= 'Z') || (*q >= 'a' && *q <= 'z'))) return 0;
    return 1;
}

#define FORM_MAX_FIELDS 512

/* Imprime le corps `application/x-www-form-urlencoded` en VAR="valeur".
 *
 * DEDOUBLONNAGE, LA DERNIERE GAGNE. Une case a cocher est precedee d'un champ
 * cache homonyme valant « false » : si la case est cochee, le navigateur envoie
 * les DEUX, « true » en second. Garder la derniere occurrence donne donc
 * « true » quand c'est coche et « false » sinon — la convention de l'etalon
 * gtk3sermo, ou une case decochee rend bien CB="false" et non rien du tout. */
static void print_form(char *body)
{
    char *names[FORM_MAX_FIELDS], *vals[FORM_MAX_FIELDS];
    int n = 0;

    for (char *pair = strtok(body, "&"); pair && n < FORM_MAX_FIELDS; pair = strtok(NULL, "&")) {
        char *eq = strchr(pair, '=');
        if (!eq) continue;
        *eq = '\0';
        char *name = pair, *val = eq + 1;
        url_decode(name); url_decode(val);
        if (!*name || !name_ok(name)) continue;
        int seen = -1;
        for (int i = 0; i < n; i++) if (!strcmp(names[i], name)) { seen = i; break; }
        if (seen >= 0) vals[seen] = val;        /* la derniere gagne */
        else { names[n] = name; vals[n] = val; n++; }
    }
    for (int i = 0; i < n; i++) {
        printf("%s=\"", names[i]);
        print_shell_escaped(vals[i]);
        printf("\"\n");
    }
    fflush(stdout);
}

/* ─── Réponses ───────────────────────────────────────────────────────────── */
static void send_all(int fd, const char *buf, size_t len)
{
    while (len) {
        ssize_t w = write(fd, buf, len);
        if (w <= 0) return;
        buf += w; len -= (size_t)w;
    }
}

static void send_simple(int fd, const char *status, const char *body)
{
    char head[256];
    int n = snprintf(head, sizeof head,
        "HTTP/1.1 %s\r\nContent-Type: text/html; charset=utf-8\r\n"
        "Content-Length: %zu\r\nConnection: close\r\n\r\n", status, strlen(body));
    if (n > 0) send_all(fd, head, (size_t)n);
    send_all(fd, body, strlen(body));
}

/* ─── Boucle ─────────────────────────────────────────────────────────────── */
int html_serve_once(const char *page, unsigned long len)
{
    char nonce[NONCE_BYTES * 2 + 1];
    if (nonce_make(nonce, sizeof nonce) != 0) {
        fprintf(stderr, "htmlsermo: impossible de tirer un jeton aleatoire\n");
        return 1;
    }

    int srv = socket(AF_INET, SOCK_STREAM, 0);
    if (srv < 0) { perror("htmlsermo: socket"); return 1; }
    int one = 1;
    setsockopt(srv, SOL_SOCKET, SO_REUSEADDR, &one, sizeof one);

    struct sockaddr_in a;
    memset(&a, 0, sizeof a);
    a.sin_family = AF_INET;
    a.sin_addr.s_addr = htonl(INADDR_LOOPBACK);   /* 127.0.0.1 — JAMAIS 0.0.0.0 */
    a.sin_port = 0;                                /* port ephemere, tire par le noyau */
    if (bind(srv, (struct sockaddr *)&a, sizeof a) < 0) { perror("htmlsermo: bind"); close(srv); return 1; }
    if (listen(srv, 4) < 0) { perror("htmlsermo: listen"); close(srv); return 1; }

    socklen_t al = sizeof a;
    if (getsockname(srv, (struct sockaddr *)&a, &al) < 0) { perror("htmlsermo: getsockname"); close(srv); return 1; }
    unsigned port = ntohs(a.sin_port);

    const char *tv = getenv("SERMO_HTML_TIMEOUT");
    long timeout = (tv && *tv) ? strtol(tv, NULL, 10) : 300;
    if (timeout <= 0) timeout = 300;

    fprintf(stderr, "htmlsermo: ouvrez http://127.0.0.1:%u/%s  (%ld s)\n", port, nonce, timeout);

    time_t deadline = time(NULL) + timeout;
    int rc = 1;

    for (;;) {
        long left = (long)(deadline - time(NULL));
        if (left <= 0) { fprintf(stderr, "htmlsermo: delai expire, rien recu\n"); break; }

        fd_set rf; FD_ZERO(&rf); FD_SET(srv, &rf);
        struct timeval tvsel = { .tv_sec = left, .tv_usec = 0 };
        int r = select(srv + 1, &rf, NULL, NULL, &tvsel);
        if (r < 0) { if (errno == EINTR) continue; perror("htmlsermo: select"); break; }
        if (r == 0) { fprintf(stderr, "htmlsermo: delai expire, rien recu\n"); break; }

        int c = accept(srv, NULL, NULL);
        if (c < 0) { if (errno == EINTR) continue; break; }

        char *req = (char *)malloc(SERVE_MAX_REQ + 1);
        if (!req) { close(c); break; }
        size_t used = 0;
        ssize_t n;
        long clen = -1;
        char *hdr_end = NULL;

        /* Lecture : d'abord les en-têtes, puis le corps annonce par Content-Length. */
        while (used < SERVE_MAX_REQ && (n = read(c, req + used, SERVE_MAX_REQ - used)) > 0) {
            used += (size_t)n;
            req[used] = '\0';
            if (!hdr_end) {
                hdr_end = strstr(req, "\r\n\r\n");
                if (hdr_end) {
                    const char *cl = strcasestr(req, "\r\ncontent-length:");
                    if (cl) clen = strtol(cl + 17, NULL, 10);
                }
            }
            if (hdr_end) {
                size_t have = used - (size_t)(hdr_end + 4 - req);
                if (clen < 0 || have >= (size_t)clen) break;
            }
        }
        if (!hdr_end) { send_simple(c, "400 Bad Request", "<p>requete incomplete</p>"); close(c); free(req); continue; }

        /* Ligne de requête : METHODE /chemin HTTP/x.y */
        char method[8] = {0}, path[256] = {0};
        if (sscanf(req, "%7s %255s", method, path) != 2) {
            send_simple(c, "400 Bad Request", "<p>requete illisible</p>"); close(c); free(req); continue;
        }

        /* Le jeton, et rien d'autre. Pas de chemin, pas de fichier, pas de « .. ». */
        const char *want = path[0] == '/' ? path + 1 : path;
        if (strlen(want) != strlen(nonce) || !nonce_equal(want, nonce, strlen(nonce))) {
            send_simple(c, "404 Not Found", "<p>404</p>");
            close(c); free(req); continue;
        }

        if (!strcmp(method, "GET")) {
            char head[256];
            int hn = snprintf(head, sizeof head,
                "HTTP/1.1 200 OK\r\nContent-Type: text/html; charset=utf-8\r\n"
                "Content-Length: %lu\r\nCache-Control: no-store\r\nConnection: close\r\n\r\n", len);
            if (hn > 0) send_all(c, head, (size_t)hn);
            send_all(c, page, (size_t)len);
            close(c); free(req); continue;
        }

        if (!strcmp(method, "POST")) {
            char *body = hdr_end + 4;
            if (clen >= 0) {
                size_t have = used - (size_t)(body - req);
                if ((size_t)clen < have) body[clen] = '\0';
            }
            print_form(body);
            send_simple(c, "200 OK",
                "<!doctype html><meta charset=utf-8><title>sermo</title>"
                "<p>Valeurs transmises. Vous pouvez fermer cet onglet.</p>");
            close(c); free(req);
            rc = 0;
            break;                       /* une seule reponse : on sort */
        }

        send_simple(c, "405 Method Not Allowed", "<p>405</p>");
        close(c); free(req);
    }

    close(srv);
    return rc;
}
