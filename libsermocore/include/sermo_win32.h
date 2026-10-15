/*
 * sermo_win32.h — fonctions POSIX absentes du CRT de Windows (mingw-w64).
 *
 * Portage Windows natif : port ncurses seulement.
 * Inclus par le shim du cœur neutre (sermocore-shim.h) et par le shim du port
 * ncurses, sous _WIN32 seulement : sur les autres systèmes, il est vide.
 *
 * Ce qui N'EST PAS ici : le lancement de commandes (fork, exec, pipe, waitpid).
 * Il n'a pas d'équivalent simple ; un lanceur par CreateProcess reste à écrire.
 *
 * SPDX-License-Identifier: GPL-2.0-or-later
 */
#ifndef SERMO_WIN32_H
#define SERMO_WIN32_H
#ifdef _WIN32

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <errno.h>
#include <sys/types.h>

/* setenv / unsetenv : _putenv_s copie la chaîne, comme setenv. */
static inline int _sermo_setenv(const char *k, const char *v, int ecraser) {
    if (!k || !*k || strchr(k, '=')) { errno = EINVAL; return -1; }
    if (!ecraser && getenv(k)) return 0;
    return _putenv_s(k, v ? v : "") == 0 ? 0 : -1;
}
static inline int _sermo_unsetenv(const char *k) {
    if (!k || !*k || strchr(k, '=')) { errno = EINVAL; return -1; }
    /* Une valeur vide retire la variable de l'environnement du CRT. */
    return _putenv_s(k, "") == 0 ? 0 : -1;
}
/* sermo_win32.c (compilé sans le shim) :
 * - caractères, console et sortie standard en UTF-8, au démarrage ;
 * - lancement d'un programme (CreateProcess) : attendre=1 synchrone (statut
 *   au format de waitpid), 0 détaché (sortie par un tube si fd_sortie) ;
 *   errno = ENOENT quand Windows ne trouve pas le programme ;
 * - est-ce cmd.exe ? (son « /C » ne suit pas les règles des autres) ;
 * - octets prêts sur un tube (-1 : fermé, -2 : pas un tube) ;
 * - ouverture d'un lien http, https ou mailto, rien d'autre. */
void sermo_win32_utf8(void);
int  sermo_win32_lance(char *const argv[], char *const envp[], int attendre,
                       int *statut, int *fd_sortie, int *pid, char **erreur);
int  sermo_win32_est_cmd(const char *chemin);
long sermo_win32_octets_prets(int fd);
int  sermo_win32_ouvrir_uri(const char *uri);

#define setenv(k, v, o)  _sermo_setenv((k), (v), (o))
#define unsetenv(k)      _sermo_unsetenv(k)

static inline char *_sermo_stpcpy(char *dst, const char *src) {
    size_t n = strlen(src);
    memcpy(dst, src, n + 1);
    return dst + n;
}
#define stpcpy(d, s)     _sermo_stpcpy((d), (s))

static inline char *_sermo_strndup(const char *s, size_t n) {
    size_t len = strnlen(s, n);
    char *r = (char *)malloc(len + 1);
    if (!r) return NULL;
    memcpy(r, s, len); r[len] = '\0';
    return r;
}
#define strndup(s, n)    _sermo_strndup((s), (n))

/* getline : même contrat que POSIX (tampon agrandi par realloc, -1 à la fin). */
static inline ssize_t _sermo_getline(char **ligne, size_t *taille, FILE *f) {
    if (!ligne || !taille || !f) { errno = EINVAL; return -1; }
    if (!*ligne || *taille == 0) {
        *taille = 128;
        char *t = (char *)realloc(*ligne, *taille);
        if (!t) { errno = ENOMEM; return -1; }
        *ligne = t;
    }
    size_t n = 0;
    int c;
    while ((c = fgetc(f)) != EOF) {
        if (n + 2 > *taille) {
            if (*taille > ((size_t)-1) / 2) { errno = EOVERFLOW; return -1; }
            size_t neuve = *taille * 2;
            char *t = (char *)realloc(*ligne, neuve);
            if (!t) { errno = ENOMEM; return -1; }
            *ligne = t; *taille = neuve;
        }
        (*ligne)[n++] = (char)c;
        if (c == '\n') break;
    }
    if (n == 0) return -1;
    (*ligne)[n] = '\0';
    return (ssize_t)n;
}
#define getline(l, t, f) _sermo_getline((l), (t), (f))

/* scandir / alphasort : opendir + readdir de mingw-w64, puis tri. Le filtre
 * est appelé comme sous POSIX. Chaque entrée est une copie à libérer par free. */
#include <dirent.h>
static inline int _sermo_alphasort(const struct dirent **a, const struct dirent **b) {
    return strcoll((*a)->d_name, (*b)->d_name);
}
#define alphasort _sermo_alphasort
static inline int _sermo_scandir(const char *chemin, struct dirent ***liste,
        int (*filtre)(const struct dirent *),
        int (*tri)(const struct dirent **, const struct dirent **)) {
    DIR *d = opendir(chemin);
    if (!d) return -1;
    struct dirent **t = NULL, *e;
    size_t n = 0, cap = 0;
    while ((e = readdir(d)) != NULL) {
        if (filtre && !filtre(e)) continue;
        if (n == cap) {
            size_t neuve = cap ? cap * 2 : 32;
            struct dirent **g = (struct dirent **)realloc(t, neuve * sizeof *g);
            if (!g) goto echec;
            t = g; cap = neuve;
        }
        struct dirent *copie = (struct dirent *)malloc(sizeof *copie);
        if (!copie) goto echec;
        memcpy(copie, e, sizeof *copie);
        t[n++] = copie;
    }
    closedir(d);
    if (tri && n > 1)
        qsort(t, n, sizeof *t, (int (*)(const void *, const void *))tri);
    *liste = t;
    return (int)n;
echec:
    for (size_t i = 0; i < n; i++) free(t[i]);
    free(t);
    closedir(d);
    errno = ENOMEM;
    return -1;
}
#define scandir(c, l, f, t) _sermo_scandir((c), (l), (f), (t))

#endif /* _WIN32 */
#endif /* SERMO_WIN32_H */
