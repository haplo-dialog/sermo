/* SPDX-License-Identifier: GPL-2.0-or-later */
/*
 * sermo_icon_theme.c — résolution d'icônes de thème freedesktop, sans GTK
 * sermo — cœur partagé
 * Licence : GPL-2.0-or-later
 *
 * C pur, sans GLib : le port sdl3 n'en a pas.
 */
#ifndef _GNU_SOURCE
#define _GNU_SOURCE
#endif
#include "sermo_icon_theme.h"

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <strings.h>
#include <sys/stat.h>

#define MAX_DIRS      1024
#define MAX_INHERITS  16
#define MAX_DEPTH     8

typedef struct {
    char  path[160];     /* "48x48/apps" */
    int   size;          /* Size= */
    int   min, max;      /* MinSize/MaxSize ou Size±Threshold */
    int   scalable;      /* Type=Scalable */
} IconDir;

typedef struct {
    IconDir dirs[MAX_DIRS];
    int     ndirs;
    char    inherits[MAX_INHERITS][64];
    int     ninherits;
} IconTheme;

static const char *k_bases[8];
static int         k_nbases = 0;

static int file_exists(const char *p)
{
    struct stat st;
    return stat(p, &st) == 0 && S_ISREG(st.st_mode);
}

static void bases_init(void)
{
    static char home_icons[512], home_dot[512], home_local[512];
    const char *home = getenv("HOME");
    const char *xdg  = getenv("XDG_DATA_HOME");
    if (k_nbases) return;
    if (xdg && *xdg) {
        snprintf(home_local, sizeof(home_local), "%s/icons", xdg);
        k_bases[k_nbases++] = home_local;
    } else if (home && *home) {
        snprintf(home_local, sizeof(home_local), "%s/.local/share/icons", home);
        k_bases[k_nbases++] = home_local;
    }
    if (home && *home) {
        snprintf(home_dot, sizeof(home_dot), "%s/.icons", home);
        k_bases[k_nbases++] = home_dot;
    }
    k_bases[k_nbases++] = "/usr/local/share/icons";
    k_bases[k_nbases++] = "/usr/share/icons";
    (void)home_icons;
}

static void rstrip(char *s)
{
    size_t n = strlen(s);
    while (n && (s[n-1] == '\n' || s[n-1] == '\r' || s[n-1] == ' ' || s[n-1] == '\t'))
        s[--n] = '\0';
}

/* Lit index.theme du thème `theme` dans l'un des dossiers de base. */
static int theme_load(const char *theme, IconTheme *t)
{
    char path[1024], line[2048];
    FILE *f = NULL;
    int i;
    memset(t, 0, sizeof(*t));
    for (i = 0; i < k_nbases && !f; i++) {
        snprintf(path, sizeof(path), "%s/%s/index.theme", k_bases[i], theme);
        f = fopen(path, "r");
    }
    if (!f) return 0;

    /* Passe 1 : Directories= et Inherits= de la section [Icon Theme] ;
     * passe 2 : les sections par dossier. On lit tout en une passe en
     * gardant la section courante. */
    IconDir *cur = NULL;
    while (fgets(line, sizeof(line), f)) {
        rstrip(line);
        if (line[0] == '[') {
            char *e = strchr(line, ']');
            if (e) *e = '\0';
            cur = NULL;
            if (strcmp(line + 1, "Icon Theme") != 0) {
                /* section d'un dossier : la retrouver (créée par Directories=)
                 * ou l'ajouter si Directories= vient après */
                for (i = 0; i < t->ndirs; i++)
                    if (strcmp(t->dirs[i].path, line + 1) == 0) { cur = &t->dirs[i]; break; }
                if (!cur && t->ndirs < MAX_DIRS) {
                    cur = &t->dirs[t->ndirs++];
                    snprintf(cur->path, sizeof(cur->path), "%s", line + 1);
                    cur->min = cur->max = cur->size = 0;
                }
            }
            continue;
        }
        char *eq = strchr(line, '=');
        if (!eq) continue;
        *eq = '\0';
        const char *key = line, *val = eq + 1;
        if (!cur) {
            if (strcmp(key, "Inherits") == 0) {
                char buf[1024]; snprintf(buf, sizeof(buf), "%s", val);
                char *sv = NULL;
                for (char *tok = strtok_r(buf, ",", &sv); tok && t->ninherits < MAX_INHERITS;
                     tok = strtok_r(NULL, ",", &sv)) {
                    while (*tok == ' ') tok++;
                    snprintf(t->inherits[t->ninherits++], 64, "%s", tok);
                }
            } else if (strcmp(key, "Directories") == 0 || strcmp(key, "ScaledDirectories") == 0) {
                char *buf = strdup(val), *sv = NULL;
                for (char *tok = strtok_r(buf, ",", &sv); tok && t->ndirs < MAX_DIRS;
                     tok = strtok_r(NULL, ",", &sv)) {
                    int dup = 0;
                    for (i = 0; i < t->ndirs; i++)
                        if (strcmp(t->dirs[i].path, tok) == 0) { dup = 1; break; }
                    if (dup) continue;
                    IconDir *d = &t->dirs[t->ndirs++];
                    snprintf(d->path, sizeof(d->path), "%s", tok);
                    d->min = d->max = d->size = 0; d->scalable = 0;
                }
                free(buf);
            }
        } else {
            if      (strcmp(key, "Size") == 0)    cur->size = atoi(val);
            else if (strcmp(key, "MinSize") == 0) cur->min  = atoi(val);
            else if (strcmp(key, "MaxSize") == 0) cur->max  = atoi(val);
            else if (strcmp(key, "Type") == 0)    cur->scalable = (strcasecmp(val, "Scalable") == 0);
            else if (strcmp(key, "Threshold") == 0 && cur->size) {
                int th = atoi(val);
                if (!cur->min) cur->min = cur->size - th;
                if (!cur->max) cur->max = cur->size + th;
            }
        }
    }
    fclose(f);
    /* compléter les bornes */
    for (i = 0; i < t->ndirs; i++) {
        IconDir *d = &t->dirs[i];
        if (!d->size) {
            /* pas de section : deviner depuis le nom "48x48/..." */
            int a = 0, b = 0;
            if (sscanf(d->path, "%dx%d", &a, &b) == 2) d->size = a;
            else if (strncmp(d->path, "scalable", 8) == 0) { d->scalable = 1; d->size = 48; }
        }
        if (!d->min) d->min = d->scalable ? 1 : d->size - 2;
        if (!d->max) d->max = d->scalable ? 4096 : d->size + 2;
    }
    return 1;
}

/* Cherche name dans un dossier de thème ; renvoie 1 et remplit out. */
static int try_file(const char *base, const char *theme, const char *dir,
                    const char *name, char *out, size_t outlen)
{
    static const char *exts[] = { "svg", "png", "xpm" };
    for (unsigned e = 0; e < sizeof(exts)/sizeof(*exts); e++) {
        snprintf(out, outlen, "%s/%s/%s/%s.%s", base, theme, dir, name, exts[e]);
        if (file_exists(out)) return 1;
    }
    return 0;
}

static char *lookup_in_theme(const char *theme, const char *name, int size, int depth,
                             char visited[MAX_DEPTH][64])
{
    IconTheme *t;
    char out[1400];
    int i, b;
    if (depth >= MAX_DEPTH) return NULL;
    for (i = 0; i < depth; i++) if (strcmp(visited[i], theme) == 0) return NULL;
    snprintf(visited[depth], 64, "%s", theme);

    t = (IconTheme *)calloc(1, sizeof(IconTheme));
    if (!t) return NULL;
    if (!theme_load(theme, t)) { free(t); return NULL; }

    /* 1. dossier dont l'intervalle contient la taille (le plus proche d'abord) */
    int best = -1, bestdist = 1 << 30;
    for (i = 0; i < t->ndirs; i++) {
        IconDir *d = &t->dirs[i];
        if (strstr(d->path, "symbolic")) continue;   /* monochromes : pas ici */
        int dist = abs(d->size - size);
        if (size >= d->min && size <= d->max) dist = d->scalable ? 1 : 0;
        if (dist < bestdist) { bestdist = dist; best = i; }
    }
    /* essayer les dossiers par distance croissante, toutes bases */
    for (int pass = 0; pass < t->ndirs; pass++) {
        int pick = -1, pd = 1 << 30;
        for (i = 0; i < t->ndirs; i++) {
            IconDir *d = &t->dirs[i];
            if (d->size < 0) continue;              /* déjà essayé */
            if (strstr(d->path, "symbolic")) continue;
            int dist = abs(d->size - size);
            if (size >= d->min && size <= d->max) dist = d->scalable ? 1 : 0;
            if (dist < pd) { pd = dist; pick = i; }
        }
        if (pick < 0) break;
        for (b = 0; b < k_nbases; b++)
            if (try_file(k_bases[b], theme, t->dirs[pick].path, name, out, sizeof(out))) {
                free(t);
                return strdup(out);
            }
        t->dirs[pick].size = -1;
    }
    (void)best;

    /* 2. parents */
    for (i = 0; i < t->ninherits; i++) {
        char *r = lookup_in_theme(t->inherits[i], name, size, depth + 1, visited);
        if (r) { free(t); return r; }
    }
    free(t);
    return NULL;
}

const char *sermo_icon_theme_name(void)
{
    static char name[128];
    if (name[0]) return name;
    const char *env = getenv("SERMO_ICON_THEME");
    if (env && *env) { snprintf(name, sizeof(name), "%s", env); return name; }

    const char *home = getenv("HOME");
    char path[1024], line[1024];
    FILE *f;
    if (home && *home) {
        snprintf(path, sizeof(path), "%s/.config/gtk-3.0/settings.ini", home);
        if ((f = fopen(path, "r"))) {
            while (fgets(line, sizeof(line), f)) {
                rstrip(line);
                if (strncmp(line, "gtk-icon-theme-name", 19) == 0) {
                    char *eq = strchr(line, '=');
                    if (eq) { eq++; while (*eq == ' ') eq++;
                        snprintf(name, sizeof(name), "%s", eq); }
                }
            }
            fclose(f);
            if (name[0]) return name;
        }
        snprintf(path, sizeof(path),
                 "%s/.config/xfce4/xfconf/xfce-perchannel-xml/xsettings.xml", home);
        if ((f = fopen(path, "r"))) {
            while (fgets(line, sizeof(line), f)) {
                if (strstr(line, "\"IconThemeName\"")) {
                    char *v = strstr(line, "value=\"");
                    if (v) {
                        v += 7;
                        char *e = strchr(v, '"');
                        if (e) { *e = '\0'; snprintf(name, sizeof(name), "%s", v); }
                    }
                }
            }
            fclose(f);
            if (name[0]) return name;
        }
    }
    snprintf(name, sizeof(name), "hicolor");
    return name;
}

/* « dark » (insensible a la casse) present dans la chaine ? */
static int str_has_dark(const char *s)
{
    if (!s) return 0;
    for (; *s; s++)
        if ((s[0] == 'd' || s[0] == 'D') && (s[1] == 'a' || s[1] == 'A') &&
            (s[2] == 'r' || s[2] == 'R') && (s[3] == 'k' || s[3] == 'K'))
            return 1;
    return 0;
}

int sermo_desktop_is_dark(void)
{
    /* SERMO_DARK prime (0 = clair, autre = sombre) : override explicite. */
    const char *e = getenv("SERMO_DARK");
    if (e && *e) return e[0] != '0';
    /* Puis le theme du BUREAU, pour SUIVRE le systeme (defaut = clair, comme
     * GTK sans reglage) : GTK_THEME, settings.ini, xsettings XFCE. */
    if (str_has_dark(getenv("GTK_THEME"))) return 1;

    const char *home = getenv("HOME");
    if (!home || !*home) return 0;
    char path[1024], line[1024];
    FILE *f;

    snprintf(path, sizeof(path), "%s/.config/gtk-3.0/settings.ini", home);
    if ((f = fopen(path, "r"))) {
        int dark = 0;
        while (fgets(line, sizeof(line), f)) {
            if (strncmp(line, "gtk-theme-name", 14) == 0 && str_has_dark(line)) { dark = 1; break; }
            if (strstr(line, "gtk-application-prefer-dark-theme")) {
                char *eq = strchr(line, '=');
                if (eq && (strchr(eq, '1') || strstr(eq, "true"))) { dark = 1; break; }
            }
        }
        fclose(f);
        if (dark) return 1;
    }

    snprintf(path, sizeof(path),
             "%s/.config/xfce4/xfconf/xfce-perchannel-xml/xsettings.xml", home);
    if ((f = fopen(path, "r"))) {
        int dark = 0;
        while (fgets(line, sizeof(line), f))
            if (strstr(line, "\"ThemeName\"") && str_has_dark(line)) { dark = 1; break; }
        fclose(f);
        if (dark) return 1;
    }
    return 0;
}

char *sermo_icon_lookup(const char *name, int size)
{
    char visited[MAX_DEPTH][64];
    char *r;
    if (!name || !*name) return NULL;
    if (size <= 0) size = 24;
    bases_init();

    /* un chemin absolu est rendu tel quel */
    if (name[0] == '/') return file_exists(name) ? strdup(name) : NULL;

    memset(visited, 0, sizeof(visited));
    r = lookup_in_theme(sermo_icon_theme_name(), name, size, 0, visited);
    if (r) return r;
    memset(visited, 0, sizeof(visited));
    r = lookup_in_theme("hicolor", name, size, 0, visited);
    if (r) return r;

    /* repli : /usr/share/pixmaps */
    static const char *exts[] = { "svg", "png", "xpm" };
    char out[1024];
    for (unsigned e = 0; e < 3; e++) {
        snprintf(out, sizeof(out), "/usr/share/pixmaps/%s.%s", name, exts[e]);
        if (file_exists(out)) return strdup(out);
    }
    return NULL;
}

int sermo_icon_size_px(const char *s)
{
    if (!s || !*s) return 24;
    if (strcasecmp(s, "menu") == 0 || strcasecmp(s, "small") == 0 ||
        strcasecmp(s, "small-toolbar") == 0 || strcasecmp(s, "button") == 0) return 16;
    if (strcasecmp(s, "large") == 0 || strcasecmp(s, "large-toolbar") == 0) return 24;
    if (strcasecmp(s, "dnd") == 0) return 32;
    if (strcasecmp(s, "dialog") == 0) return 48;
    {
        int n = atoi(s);
        return n > 0 ? n : 24;
    }
}
