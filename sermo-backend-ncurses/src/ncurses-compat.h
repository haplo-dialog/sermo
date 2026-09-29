/* SPDX-License-Identifier: GPL-2.0-or-later */
/*
 * Copyright (C) 2026 S. Cage
 * haplo-dialog <devel@haplo-dialog.fr>
 */
/* ncurses-compat.h — Couche de compatibilité GTK/GLib → ncurses (terminal)  v2
 * Force-inclus dans les unités C du core via -include dans CMakeLists.txt.
 *
 * sermo
 * Contact : devel@haplo-dialog.fr
 * Licence : GPL-2.0-or-later
 *
 * v2 — 2026-05-28 : g_strdup_printf sans GCC statement-expression (C11),
 *      snprintf_safe(), g_debug/g_info, g_ascii_strtod, g_str_has_prefix,
 *      g_utf8_validate stub, g_key_file stub, GSList/GList implémentés.
 */

#ifndef SDL3_COMPAT_H
#define SDL3_COMPAT_H

#ifdef __cplusplus
extern "C" {
#endif

#ifndef _GNU_SOURCE
#  define _GNU_SOURCE 1   /* strtod_l */
#endif
#define _POSIX_C_SOURCE 200809L
#include <stdlib.h>
#include <string.h>
#include <stdio.h>
#include <stdarg.h>
#include <stdint.h>
#include <stdbool.h>
#include <ctype.h>
#include <strings.h>   /* strcasecmp / strncasecmp */
#include <math.h>
#include <time.h>
#include <locale.h>    /* newlocale, strtod_l, uselocale, LC_ALL_MASK */

/* ─── Types GLib de base ────────────────────────────────────────────────── */
typedef int           gint;
typedef unsigned int  guint;
typedef char          gchar;
typedef unsigned char guchar;
typedef int           gboolean;
typedef long          glong;
typedef unsigned long gulong;
typedef void*         gpointer;
typedef const void*   gconstpointer;
typedef double        gdouble;
typedef float         gfloat;
typedef int8_t        gint8;
typedef uint8_t       guint8;
typedef int16_t       gint16;
typedef uint16_t      guint16;
typedef int32_t       gint32;
typedef uint32_t      guint32;
typedef int64_t       gint64;
typedef uint64_t      guint64;
typedef size_t        gsize;
typedef unsigned long GType;

#ifndef TRUE
#  define TRUE  1
#  define FALSE 0
#endif

/* ─── Mémoire ───────────────────────────────────────────────────────────── */
static inline void *_sdl3_malloc_check(size_t n) {
    void *p = malloc(n);
    if (!p) { fprintf(stderr, "FATAL: out of memory (%zu bytes)\n", n); abort(); }
    return p;
}
#define g_malloc(n)      _sdl3_malloc_check(n)
#define g_malloc0(n)     ({ void *_p = calloc(1,(n)); if(!_p){fprintf(stderr,"FATAL: OOM\n");abort();} _p; })
#define g_realloc(p,n)   realloc((p),(n))
#define g_free(p)        free(p)
#define g_new(t,n)       ((t*)_sdl3_malloc_check(sizeof(t)*(n)))
#define g_new0(t,n)      ((t*)calloc((n),sizeof(t)))
#define g_try_malloc(n)  malloc(n)
#define g_try_new(t,n)   ((t*)malloc(sizeof(t)*(n)))

/* ─── Chaînes ───────────────────────────────────────────────────────────── */
#define g_strdup(s)       strdup(s)
#define g_strndup(s,n)    strndup((s),(n))
#define g_strcmp0(a,b)    strcmp((a)?(a):"",(b)?(b):"")
/* Analyse numérique indépendante de la locale : un nombre écrit dans un XML
 * utilise le point décimal « C », jamais la virgule de LC_NUMERIC. */
static inline double _sermo_ascii_strtod(const char *s, char **end) {
    static locale_t loc_c = (locale_t)0;
    if (loc_c == (locale_t)0) loc_c = newlocale(LC_ALL_MASK, "C", (locale_t)0);
    return strtod_l(s, end, loc_c ? loc_c : uselocale((locale_t)0));
}
static inline double _sermo_strtod_current(const char *s, char **end) {
    char *e1 = NULL, *e2 = NULL;
    double v1 = strtod_l(s, &e1, uselocale((locale_t)0)); /* locale courante */
    double v2 = _sermo_ascii_strtod(s, &e2);              /* locale C */
    if (e2 > e1) { if (end) *end = e2; return v2; }
    if (end) *end = e1; return v1;
}
#define g_strtod(s,e)       _sermo_strtod_current((s),(e))
#define g_ascii_strtod(s,e) _sermo_ascii_strtod((s),(e))
/* Écriture d'un nombre SANS que la locale s'en mêle — miroir exact de
 * g_ascii_strtod ci-dessus. Sous fr_FR, snprintf("%g", 0.5) écrit « 0,5 » :
 * la valeur exportée dépendrait alors de la machine, et un script qui compare
 * « 0.5 » ne reconnaîtrait plus rien. snprintf_l fige la locale C. */
static inline char *_sermo_ascii_formatd(char *buf, size_t n, const char *fmt, double d) {
    static locale_t loc_c = (locale_t)0;
    locale_t prec;
    if (loc_c == (locale_t)0) loc_c = newlocale(LC_ALL_MASK, "C", (locale_t)0);
    /* ⚠️ snprintf_l n'existe pas dans la glibc (c'est une API BSD) : on bascule
     * la locale du THREAD le temps de l'écriture, puis on la rend. */
    prec = loc_c ? uselocale(loc_c) : (locale_t)0;
    snprintf(buf, n, fmt, d);
    if (prec) uselocale(prec);
    return buf;
}
#define g_ascii_formatd(b,n,f,d) _sermo_ascii_formatd((b),(n),(f),(d))
#define g_str_has_prefix(s,p) (strncmp((s),(p),strlen(p))==0)
#define g_str_has_suffix(s,x) (strlen(s)>=strlen(x) && strcmp((s)+strlen(s)-strlen(x),(x))==0)
#define g_ascii_tolower(c)  tolower((unsigned char)(c))
#define g_ascii_toupper(c)  toupper((unsigned char)(c))
#define g_utf8_validate(s,l,e) (1)  /* stub — SDL3 assume UTF-8 */
#define g_utf8_strlen(s,l)   ((glong)strlen(s))

/* snprintf_safe — retourne un malloc'd string */
static inline char *snprintf_safe(const char *fmt, ...) {
    va_list ap1, ap2;
    va_start(ap1, fmt);
    va_copy(ap2, ap1);
    int n = vsnprintf(NULL, 0, fmt, ap1);
    va_end(ap1);
    if (n < 0) { va_end(ap2); return strdup(""); }
    char *buf = (char *)malloc((size_t)n + 1);
    if (!buf) { va_end(ap2); return strdup(""); }
    vsnprintf(buf, (size_t)n + 1, fmt, ap2);
    va_end(ap2);
    return buf;
}
#define g_strdup_printf(fmt, ...)  snprintf_safe(fmt, ##__VA_ARGS__)
#define g_strdup_vprintf(fmt, ap)  ({ int _n=vsnprintf(NULL,0,fmt,ap)+1; char *_s=(char*)malloc(_n); vsnprintf(_s,_n,fmt,ap); _s; })

/* Concaténation */
static inline gsize g_strlcat(char *dst, const char *src, gsize size); /* fwd */
static inline char *_g_strconcat_impl(const char *s, ...) {
    va_list ap;
    size_t total = s ? strlen(s) : 0;
    va_start(ap, s);
    const char *p;
    while ((p = va_arg(ap, const char *)) != NULL) total += strlen(p);
    va_end(ap);
    char *out = (char *)malloc(total + 1);
    if (!out) return NULL;
    out[0] = '\0';
    if (s) g_strlcat(out, s, total + 1);
    va_start(ap, s);
    while ((p = va_arg(ap, const char *)) != NULL) g_strlcat(out, p, total + 1);
    va_end(ap);
    return out;
}
#define g_strconcat(s, ...)  _g_strconcat_impl((s), ##__VA_ARGS__, NULL)
#define g_strjoinv(sep, v)   ({     char *_r = strdup("");     if (v) { for (int _i=0; (v)[_i]; _i++) {         char *_t = g_strconcat(_r, _i?sep:"", (v)[_i], NULL); free(_r); _r=_t;     }} _r; })

/* Split */
static inline char **g_strsplit(const char *str, const char *delim, int max) {
    if (!str) return NULL;
    int cap = 8, n = 0;
    char **res = (char **)malloc(cap * sizeof(char *));
    char *copy = strdup(str);
    char *tok = strtok(copy, delim);
    while (tok && (max <= 0 || n < max - 1)) {
        if (n >= cap - 1) { cap *= 2; res = (char **)realloc(res, cap * sizeof(char *)); }
        res[n++] = strdup(tok);
        tok = strtok(NULL, delim);
    }
    if (tok) res[n++] = strdup(tok);
    res[n] = NULL;
    free(copy);
    return res;
}
static inline void g_strfreev(char **v) {
    if (!v) return;
    for (int i = 0; v[i]; i++) free(v[i]);
    free(v);
}
static inline int g_strv_length(char **v) {
    int n = 0; if (v) while (v[n]) n++; return n;
}

/* ─── Helpers chaîne bornés / ASCII / UTF-8 ──────────────────────────────────
 * Le cœur n'utilise que de l'ASCII (préfixes de commandes, noms de widgets) ;
 * les variantes « utf8 » se ramènent donc à leurs équivalents octet. */
#ifndef g_ascii_strcasecmp
#  define g_ascii_strcasecmp(a,b)    strcasecmp((a),(b))
#endif
#ifndef g_ascii_strncasecmp
#  define g_ascii_strncasecmp(a,b,n) strncasecmp((a),(b),(n))
#endif
#ifndef g_snprintf
#  define g_snprintf(buf,n,fmt,...)  snprintf((buf),(n),(fmt),##__VA_ARGS__)
#endif
/* g_strlcpy / g_strlcat — bornées, toujours null-terminées, retournent
 * la longueur de la source (resp. longueur combinée) comme dans GLib. */
static inline gsize g_strlcpy(char *dst, const char *src, gsize size) {
    gsize srclen = strlen(src);
    if (size != 0) {
        gsize n = (srclen >= size) ? size - 1 : srclen;
        memcpy(dst, src, n);
        dst[n] = '\0';
    }
    return srclen;
}
static inline gsize g_strlcat(char *dst, const char *src, gsize size) {
    gsize dstlen = strnlen(dst, size);
    gsize srclen = strlen(src);
    if (dstlen == size) return size + srclen;
    if (srclen < size - dstlen) {
        memcpy(dst + dstlen, src, srclen + 1);
    } else {
        memcpy(dst + dstlen, src, size - dstlen - 1);
        dst[size - 1] = '\0';
    }
    return dstlen + srclen;
}
static inline long g_utf8_pointer_to_offset(const char *str, const char *pos) {
    return (long)(pos - str);
}
/* g_strchug : supprime les blancs en tête, en place, renvoie la chaîne. */
static inline char *g_strchug(char *s) {
    if (!s) return s;
    char *p = s;
    while (*p == ' ' || *p == '\t' || *p == '\n' || *p == '\r' ||
           *p == '\f' || *p == '\v') p++;
    if (p != s) memmove(s, p, strlen(p) + 1);
    return s;
}
/* g_strchomp : supprime les blancs en fin, en place, renvoie la chaîne. */
static inline char *g_strchomp(char *s) {
    if (!s) return s;
    size_t n = strlen(s);
    while (n > 0) {
        char c = s[n - 1];
        if (c == ' ' || c == '\t' || c == '\n' || c == '\r' ||
            c == '\f' || c == '\v') s[--n] = '\0';
        else break;
    }
    return s;
}
#ifndef g_strstrip
#  define g_strstrip(s)  g_strchomp(g_strchug(s))
#endif

/* ─── Messages ──────────────────────────────────────────────────────────── */
#ifndef g_print
#define g_print(fmt,...)     printf(fmt, ##__VA_ARGS__)
#endif
#ifndef g_printerr
#define g_printerr(fmt,...)  fprintf(stderr, fmt, ##__VA_ARGS__)
#endif
#ifndef g_warning
#define g_warning(fmt,...)   fprintf(stderr, "WARNING [ncursessermo]: " fmt "\n", ##__VA_ARGS__)
#endif
#ifndef g_critical
#define g_critical(fmt,...)  fprintf(stderr, "CRITICAL [ncursessermo]: " fmt "\n", ##__VA_ARGS__)
#endif
#ifndef g_error
#define g_error(fmt,...)     do { fprintf(stderr, "ERROR [ncursessermo]: " fmt "\n", ##__VA_ARGS__); abort(); } while(0)
#endif
#ifdef DEBUG
#ifndef g_debug
#  define g_debug(fmt,...)   fprintf(stderr, "DEBUG [ncursessermo]: " fmt "\n", ##__VA_ARGS__)
#endif
#ifndef g_info
#  define g_info(fmt,...)    fprintf(stderr, "INFO [ncursessermo]: " fmt "\n", ##__VA_ARGS__)
#endif
#else
#ifndef g_debug
#  define g_debug(fmt,...)   ((void)0)
#endif
#ifndef g_info
#  define g_info(fmt,...)    ((void)0)
#endif
#endif

/* ─── Assertions / messages de contrôle ─────────────────────────────────────── */
#ifndef g_message
#  define g_message(fmt,...)  fprintf(stderr, fmt "\n", ##__VA_ARGS__)
#endif
#ifndef g_assert
#  define g_assert(expr) \
    do { if (!(expr)) { fprintf(stderr, "ASSERT FAILED: %s (%s:%d)\n", \
         #expr, __FILE__, __LINE__); abort(); } } while (0)
#endif
#ifndef g_assert_not_reached
#  define g_assert_not_reached() \
    do { fprintf(stderr, "ASSERT: code unreachable (%s:%d)\n", \
         __FILE__, __LINE__); abort(); } while (0)
#endif
#ifndef g_return_if_fail
#  define g_return_if_fail(expr)      do { if (!(expr)) return; } while (0)
#endif
#ifndef g_return_val_if_fail
#  define g_return_val_if_fail(expr,v) do { if (!(expr)) return (v); } while (0)
#endif

/* ─── GSList ─────────────────────────────────────────────────────────────── */
typedef struct _GSList { void *data; struct _GSList *next; } GSList;
static inline GSList *g_slist_append(GSList *l, void *d) {
    GSList *n = (GSList *)malloc(sizeof(GSList));
    n->data = d; n->next = NULL;
    if (!l) return n;
    GSList *p = l; while (p->next) p = p->next; p->next = n; return l;
}
static inline GSList *g_slist_prepend(GSList *l, void *d) {
    GSList *n = (GSList *)malloc(sizeof(GSList));
    n->data = d; n->next = l; return n;
}
static inline void g_slist_free(GSList *l) {
    while (l) { GSList *n = l->next; free(l); l = n; }
}
static inline guint g_slist_length(GSList *l) {
    guint c = 0; while (l) { c++; l = l->next; } return c;
}

/* ─── GList ──────────────────────────────────────────────────────────────── */
typedef struct _GList { void *data; struct _GList *next; struct _GList *prev; } GList;
static inline GList *g_list_append(GList *l, void *d) {
    GList *n = (GList *)malloc(sizeof(GList));
    n->data = d; n->next = NULL; n->prev = NULL;
    if (!l) return n;
    GList *p = l; while (p->next) p = p->next;
    p->next = n; n->prev = p; return l;
}
static inline void g_list_free(GList *l) {
    while (l) { GList *n = l->next; free(l); l = n; }
}
static inline guint g_list_length(GList *l) {
    guint c = 0; while (l) { c++; l = l->next; } return c;
}
#define g_list_next(l)       ((l)?((GList*)(l))->next:NULL)
#define g_list_previous(l)   ((l)?((GList*)(l))->prev:NULL)
#define g_list_last(l)       ({ GList*_l=(l);while(_l&&_l->next)_l=_l->next;_l; })
#define g_list_first(l)      ({ GList*_l=(l);while(_l&&_l->prev)_l=_l->prev;_l; })
#define g_list_prepend(l,d)  ({ GList*_n=(GList*)malloc(sizeof(GList));_n->data=(d);_n->next=(l);_n->prev=NULL;if(l)((GList*)(l))->prev=_n;_n; })
#define g_list_foreach(l,f,d) do{GList*_e=(l);while(_e){(f)(_e->data,(d));_e=_e->next;}}while(0)
#define g_list_nth_data(l,n) ({ GList*_e=(l);guint _i=0;while(_e&&_i<(n)){_e=_e->next;_i++;}_e?_e->data:NULL; })
#define g_list_nth(l,n)      ({ GList*_e=(l);guint _i=0;while(_e&&_i<(n)){_e=_e->next;_i++;}_e; })
#define g_list_find(l,d)     ({ GList*_e=(l);while(_e&&_e->data!=(d))_e=_e->next;_e; })
#define g_list_remove(l,d)   ({ GList*_l=(l),*_e=_l;while(_e&&_e->data!=(d))_e=_e->next;if(_e){if(_e->prev)_e->prev->next=_e->next;else _l=_e->next;if(_e->next)_e->next->prev=_e->prev;free(_e);}_l; })
#define g_list_copy(l)       ({ GList*_r=NULL,*_e=(l);while(_e){_r=g_list_append(_r,_e->data);_e=_e->next;}_r; })
#define g_list_delete_link(l,lnk) ({ GList*_l=(l),*_e=(GList*)(lnk);if(_e->prev)_e->prev->next=_e->next;else _l=_e->next;if(_e->next)_e->next->prev=_e->prev;free(_e);_l; })

/* ─── GString minimal (variables.c, stringman.c) ─────────────────────────────*/
typedef struct { char *str; size_t len, allocated_len; } GString;
static inline GString *g_string_sized_new(size_t reserve) {
    GString *s = (GString *)calloc(1, sizeof(GString));
    if (!s) return NULL;
    s->allocated_len = (reserve < 16 ? 16 : reserve) + 1;
    s->str = (char *)malloc(s->allocated_len);
    if (!s->str) { free(s); return NULL; }
    s->str[0] = '\0';
    return s;
}
static inline GString *g_string_new(const char *init) {
    size_t n = init ? strlen(init) : 0;
    GString *s = g_string_sized_new(n);
    if (s && init) { memcpy(s->str, init, n + 1); s->len = n; }
    return s;
}
static inline GString *g_string_append_c(GString *s, char c) {
    if (s->len + 2 > s->allocated_len) {
        while (s->len + 2 > s->allocated_len) s->allocated_len *= 2;
        s->str = (char *)realloc(s->str, s->allocated_len);
    }
    s->str[s->len++] = c;
    s->str[s->len] = '\0';
    return s;
}
static inline GString *g_string_append(GString *s, const char *str) {
    if (!s || !str) return s;
    size_t add = strlen(str);
    if (s->len + add + 1 > s->allocated_len) {
        while (s->len + add + 1 > s->allocated_len) s->allocated_len *= 2;
        s->str = (char *)realloc(s->str, s->allocated_len);
    }
    memcpy(s->str + s->len, str, add + 1);
    s->len += add;
    return s;
}
/* free_segment != 0 : libère aussi le tampon (retourne NULL) ;
 * sinon retourne le tampon, à la charge de l'appelant. */
static inline char *g_string_free(GString *s, int free_segment) {
    if (!s) return NULL;
    char *ret = NULL;
    if (free_segment) free(s->str);
    else ret = s->str;
    free(s);
    return ret;
}

/* ─── Threads — no-op ────────────────────────────────────────────────────── */
#define g_thread_init(v)    ((void)0)
#define gdk_threads_init()  ((void)0)
#define gdk_threads_enter() ((void)0)
#define gdk_threads_leave() ((void)0)

/* ─── GTK stubs pour le core C ───────────────────────────────────────────── */
#define gtk_init(a,v)  ((void)0)
#define gtk_main()     ((void)0)
#define gtk_main_quit() ((void)0)
#define HAVE_GLADE_LIB 0

/* ─── Gdk / GObject — types opaques & helpers pour le core C ─────────────────
 * Le port SDL3/ImGui n'a ni événements Gdk ni introspection GObject. On fournit
 * des types opaques pour préserver les signatures des gestionnaires de signaux
 * (signals.c/.h) et un magasin clé/valeur minimal pour g_object_{set,get}_data.
 */
typedef struct { int type; double x, y; double x_root, y_root;
                 unsigned int state, button, keyval, hardware_keycode; }
        GdkEvent;
typedef GdkEvent GdkEventButton;
typedef GdkEvent GdkEventConfigure;
typedef GdkEvent GdkEventCrossing;
typedef GdkEvent GdkEventFocus;
typedef GdkEvent GdkEventKey;
typedef int      GdkInputCondition;
typedef int      GtkEntryIconPosition;
typedef struct _GFile        GFile;
typedef struct _GFileMonitor GFileMonitor;
typedef int      GFileMonitorEvent;
typedef struct _GtkTreePath        GtkTreePath;
typedef struct _GtkTreeViewColumn  GtkTreeViewColumn;
typedef struct _GError             GError;  /* def complete plus bas */

#define GTK_IS_WIDGET(w)       ((w) != NULL)
#define G_OBJECT(x)            ((void *)(x))
#define GPOINTER_TO_INT(p)     ((int)(long)(p))
#define GINT_TO_POINTER(i)     ((void *)(long)(i))
#define GPOINTER_TO_UINT(p)    ((unsigned int)(unsigned long)(p))
#define GUINT_TO_POINTER(u)    ((void *)(unsigned long)(u))

/* Association clé(chaîne)/valeur par objet — remplace g_object_set/get_data.
 * Implémentation linéaire suffisante pour le faible volume d'usages du core. */
typedef struct _Sdl3ObjData {
    void *obj; char *key; void *val; struct _Sdl3ObjData *next;
} Sdl3ObjData;
extern Sdl3ObjData *sdl3_objdata_head;
static inline void g_object_set_data(void *obj, const char *key, void *val) {
    Sdl3ObjData *p = sdl3_objdata_head;
    for (; p; p = p->next)
        if (p->obj == obj && p->key && strcmp(p->key, key) == 0) { p->val = val; return; }
    p = (Sdl3ObjData *)malloc(sizeof(*p));
    if (!p) return;
    p->obj = obj; p->key = strdup(key); p->val = val;
    p->next = sdl3_objdata_head; sdl3_objdata_head = p;
}
static inline void *g_object_get_data(void *obj, const char *key) {
    Sdl3ObjData *p = sdl3_objdata_head;
    for (; p; p = p->next)
        if (p->obj == obj && p->key && strcmp(p->key, key) == 0) return p->val;
    return NULL;
}
/* gdk_input_add : surveillance de fd non supportée ici (no-op, renvoie un tag). */
static inline int gdk_input_add(int source, int condition,
        void (*func)(void *, int, int), void *data) {
    (void)source; (void)condition; (void)func; (void)data; return 0;
}
#define gdk_input_remove(tag)  ((void)(tag))

/* Signaux GObject : le port immediate-mode ne connecte rien dynamiquement.
 * g_signal_connect ignore (et n'évalue donc pas) ses arguments, ce qui rend
 * G_CALLBACK inerte ; on le définit tout de même pour les usages isolés. */
#ifndef G_CALLBACK
#  define G_CALLBACK(f)                  ((void *)(f))
#endif
#ifndef g_signal_connect
#  define g_signal_connect(o,s,c,d)        ((void)0)
#endif
#ifndef g_signal_connect_after
#  define g_signal_connect_after(o,s,c,d)  ((void)0)
#endif
#ifndef g_signal_connect_swapped
#  define g_signal_connect_swapped(o,s,c,d) ((void)0)
#endif
#ifndef g_signal_emit_by_name
#  define g_signal_emit_by_name(o,s,...)   ((void)0)
#endif

/* Touches Gdk — pas de table de symboles X ici ; suffisant pour le core. */
static inline const char *gdk_keyval_name(unsigned int keyval) { (void)keyval; return ""; }
static inline unsigned int gdk_keyval_to_unicode(unsigned int keyval) { return keyval; }

/* Environnement */
#ifndef g_setenv
#  define g_setenv(k,v,ow)  ((void)setenv((k),(v),(ow)))
#endif
#ifndef g_unsetenv
#  define g_unsetenv(k)     unsetenv(k)
#endif

/* ─── GIO (GFile/GFileMonitor) — stubs no-op ─────────────────────────────────
 * La surveillance de fichiers passe par inotify sur Linux (HAVE_SYS_INOTIFY_H).
 * La branche GIO est conservée pour portabilité mais inerte dans le port SDL3. */
#define G_FILE_MONITOR_NONE                    0
#define G_FILE_MONITOR_EVENT_CHANGED           0
#define G_FILE_MONITOR_EVENT_CHANGES_DONE_HINT 1
#define G_FILE_MONITOR_EVENT_DELETED           2
#define G_FILE_MONITOR_EVENT_CREATED           3
#define G_FILE_MONITOR_EVENT_ATTRIBUTE_CHANGED 4
static inline GFile *g_file_new_for_path(const char *p) { (void)p; return NULL; }
static inline char  *g_file_get_path(GFile *f) { (void)f; return NULL; }
static inline GFileMonitor *g_file_monitor_file(GFile *f, int flags,
        int cancellable, GError **error) {
    (void)f; (void)flags; (void)cancellable; (void)error; return NULL;
}
static inline void g_file_monitor_set_rate_limit(GFileMonitor *m, int ms) {
    (void)m; (void)ms;
}
static inline int  g_file_monitor_cancel(GFileMonitor *m) { (void)m; return 1; }
static inline void g_object_unref(void *o) { (void)o; }

/* ─── printf GLib ────────────────────────────────────────────────────────── */
#define g_vprintf(fmt, args)   vprintf((fmt), (args))
#define g_printf(...)          printf(__VA_ARGS__)

/* ─── GError minimal ─────────────────────────────────────────────────────── */
#ifndef SDL3_COMPAT_GERROR
#define SDL3_COMPAT_GERROR
typedef struct _GError { int domain; int code; char *message; } GError;
static inline void g_error_free(GError *e) { if (e) { free(e->message); free(e); } }
#endif

/* ─── Exécution de processus (g_spawn / g_shell) — POSIX ──────────────────────
 * Le port SDL3 n'a pas de boucle GLib : safe_exec.c (durcissement sécurité) est
 * réimplémenté au-dessus de fork/exec/pipe. g_child_watch_add est inutile ici —
 * le double fork de g_spawn_async_with_pipes réattache l'enfant à init, qui le
 * récupère automatiquement (aucun zombie, même sans boucle d'événements). */
#include <unistd.h>
#include <sys/types.h>
#include <sys/wait.h>

#define g_getenv(name)             getenv(name)
typedef int GPid;
#define G_SPAWN_SEARCH_PATH        (1 << 2)
#define G_SPAWN_DO_NOT_REAP_CHILD  (1 << 1)
#define g_spawn_close_pid(pid)     ((void)(pid))

static inline void _sdl3_set_gerror(GError **error, const char *msg) {
    if (!error) return;
    GError *e = (GError *)calloc(1, sizeof(GError));
    if (e) { e->message = strdup(msg ? msg : "error"); *error = e; }
}

/* Découpe une ligne SANS métacaractères en argv (séparateurs : blancs).
 * safe_exec.c ne l'appelle que pour des commandes déjà filtrées : pas de
 * guillemets ni d'expansion à gérer ici. */
static inline gboolean g_shell_parse_argv(const char *line, int *argcp,
        char ***argvp, GError **error) {
    if (!line || !argvp) { _sdl3_set_gerror(error, "empty command"); return FALSE; }
    int cap = 8, n = 0;
    char **argv = (char **)malloc((size_t)cap * sizeof(char *));
    char *copy  = strdup(line);
    if (!argv || !copy) { free(argv); free(copy);
        _sdl3_set_gerror(error, "out of memory"); return FALSE; }
    char *save = NULL;
    for (char *tok = strtok_r(copy, " \t\n\r\f\v", &save); tok;
             tok = strtok_r(NULL, " \t\n\r\f\v", &save)) {
        if (n + 1 >= cap) { cap *= 2;
            argv = (char **)realloc(argv, (size_t)cap * sizeof(char *)); }
        argv[n++] = strdup(tok);
    }
    argv[n] = NULL;
    free(copy);
    if (n == 0) { free(argv); _sdl3_set_gerror(error, "empty command"); return FALSE; }
    if (argcp) *argcp = n;
    *argvp = argv;
    return TRUE;
}

/* Exécution synchrone : statut wait() brut renvoyé dans *exit_status. */
static inline gboolean g_spawn_sync(const char *wd, char **argv, char **envp,
        int flags, void *setup, void *data,
        char **out_str, char **err_str, int *exit_status, GError **error) {
    (void)wd; (void)envp; (void)flags; (void)setup; (void)data;
    (void)out_str; (void)err_str;
    if (!argv || !argv[0]) { _sdl3_set_gerror(error, "no argv"); return FALSE; }
    pid_t pid = fork();
    if (pid < 0) { _sdl3_set_gerror(error, "fork failed"); return FALSE; }
    if (pid == 0) { execvp(argv[0], argv); _exit(127); }
    int status = 0;
    while (waitpid(pid, &status, 0) < 0) { /* retry on EINTR */ }
    if (exit_status) *exit_status = status;
    return TRUE;
}

/* Exécution asynchrone avec pipe stdout. Double fork → l'enfant réel est
 * réattaché à init (réapé automatiquement, zéro zombie). */
static inline gboolean g_spawn_async_with_pipes(const char *wd, char **argv,
        char **envp, int flags, void *setup, void *data, GPid *child_pid,
        int *stdin_fd, int *stdout_fd, int *stderr_fd, GError **error) {
    (void)wd; (void)envp; (void)flags; (void)setup; (void)data;
    if (!argv || !argv[0]) { _sdl3_set_gerror(error, "no argv"); return FALSE; }
    int outpipe[2] = { -1, -1 };
    if (stdout_fd && pipe(outpipe) != 0) {
        _sdl3_set_gerror(error, "pipe failed"); return FALSE; }
    pid_t pid = fork();
    if (pid < 0) {
        if (stdout_fd) { close(outpipe[0]); close(outpipe[1]); }
        _sdl3_set_gerror(error, "fork failed");
        return FALSE;
    }
    if (pid == 0) {
        pid_t grandchild = fork();
        if (grandchild == 0) {
            if (stdout_fd) {
                dup2(outpipe[1], STDOUT_FILENO);
                close(outpipe[0]); close(outpipe[1]);
            }
            execvp(argv[0], argv);
            _exit(127);
        }
        _exit(0);
    }
    if (stdout_fd) { close(outpipe[1]); *stdout_fd = outpipe[0]; }
    if (stdin_fd)  *stdin_fd  = -1;
    if (stderr_fd) *stderr_fd = -1;
    int dummy = 0;
    while (waitpid(pid, &dummy, 0) < 0) { /* reap intermediate */ }
    if (child_pid) *child_pid = (GPid)pid;
    return TRUE;
}

static inline unsigned g_child_watch_add(GPid pid,
        void (*cb)(GPid, int, void *), void *data) {
    (void)pid; (void)cb; (void)data;  /* double fork → rien à surveiller */
    return 0;
}

/* ─── UTF-8 helpers manquants ────────────────────────────────────────────── */
/* Suffisant pour l'ASCII employé par le parsing d'options. */
static inline char *g_utf8_strchr(const char *p, long len, int c) {
    (void)len;
    /* strchr() renvoie const char* en C++ pour une entrée const ; on caste
     * pour conserver la signature GLib (gchar*). */
    return p ? (char *)strchr(p, c) : NULL;
}
static inline char **g_strsplit_set(const char *str, const char *delims, int max) {
    /* Comme g_strsplit mais coupe sur n'importe quel caractère de delims. */
    int n = 0, cap = 8;
    char **out = (char **)malloc(sizeof(char *) * cap);
    const char *start = str;
    if (!out || !str) { if (out) out[0] = NULL; return out; }
    for (const char *p = str; ; p++) {
        int is_delim = (*p && strchr(delims, *p) != NULL);
        if (is_delim || *p == '\0') {
            if (max > 0 && n == max - 1 && *p) {
                /* dernier champ : conserver le reste tel quel */
                continue;
            }
            size_t seglen = (size_t)(p - start);
            char *seg = (char *)malloc(seglen + 1);
            memcpy(seg, start, seglen); seg[seglen] = '\0';
            if (n + 1 >= cap) { cap *= 2; out = (char **)realloc(out, sizeof(char *) * cap); }
            out[n++] = seg;
            if (*p == '\0') break;
            start = p + 1;
        }
    }
    out[n] = NULL;
    return out;
}

/* ─── GOption — implémenté au-dessus de getopt_long ──────────────────────── */
#include <getopt.h>

#define G_OPTION_ARG_NONE     0
#define G_OPTION_ARG_STRING   1
#define G_OPTION_FLAG_IN_MAIN 0

typedef struct {
    const char *long_name;
    char        short_name;
    int         flags;
    int         arg;            /* G_OPTION_ARG_* */
    void       *arg_data;       /* gboolean* ou gchar** selon arg */
    const char *description;
    const char *arg_description;
} GOptionEntry;

typedef struct {
    const char         *summary;
    const GOptionEntry *entries;
} GOptionContext;

static inline GOptionContext *g_option_context_new(const char *summary) {
    GOptionContext *c = (GOptionContext *)calloc(1, sizeof(GOptionContext));
    if (c) c->summary = summary;
    return c;
}
static inline void g_option_context_add_main_entries(
        GOptionContext *c, const GOptionEntry *entries, const char *domain) {
    (void)domain;
    if (c) c->entries = entries;
}
/* gtk_get_option_group : aucun groupe GTK en mode SDL3. */
static inline void *gtk_get_option_group(int open_default_display) {
    (void)open_default_display; return NULL;
}
static inline void g_option_context_add_group(GOptionContext *c, void *group) {
    (void)c; (void)group;
}
static inline void g_option_context_free(GOptionContext *c) { free(c); }

static inline int g_option_context_parse(
        GOptionContext *c, int *argc, char ***argv, GError **error) {
    if (!c || !c->entries) return 1;
    const GOptionEntry *e;
    int n = 0;
    for (e = c->entries; e->long_name; e++) n++;

    struct option *lo = (struct option *)calloc((size_t)n + 1, sizeof(struct option));
    /* chaîne d'options courtes : "vd" ou "p:" selon arg */
    char *so = (char *)malloc((size_t)n * 2 + 2);
    int soi = 0;
    so[soi++] = ':';   /* signaler les arguments manquants via ':' */
    for (int i = 0; i < n; i++) {
        e = &c->entries[i];
        lo[i].name    = e->long_name;
        lo[i].has_arg = (e->arg == G_OPTION_ARG_STRING) ? required_argument : no_argument;
        lo[i].flag    = NULL;
        lo[i].val     = (e->short_name) ? e->short_name : (1000 + i);
        if (e->short_name) {
            so[soi++] = e->short_name;
            if (e->arg == G_OPTION_ARG_STRING) so[soi++] = ':';
        }
    }
    so[soi] = '\0';

    optind = 1;
    int opt, longidx = 0, ok = 1;
    while ((opt = getopt_long(*argc, *argv, so, lo, &longidx)) != -1) {
        const GOptionEntry *match = NULL;
        if (opt == '?' || opt == ':') { ok = 0; break; }
        for (int i = 0; i < n; i++) {
            int val = c->entries[i].short_name ? c->entries[i].short_name : (1000 + i);
            if (opt == val) { match = &c->entries[i]; break; }
        }
        if (!match) continue;
        if (match->arg == G_OPTION_ARG_STRING) {
            if (match->arg_data) *(char **)match->arg_data = strdup(optarg ? optarg : "");
        } else {
            if (match->arg_data) *(int *)match->arg_data = 1;
        }
    }
    /* compacter argv : retirer les options consommées, garder argv[0] + reste */
    if (ok) {
        int w = 1;
        for (int r = optind; r < *argc; r++) (*argv)[w++] = (*argv)[r];
        (*argv)[w] = NULL;
        *argc = w;
    } else if (error) {
        GError *err = (GError *)calloc(1, sizeof(GError));
        if (err) { err->message = strdup("invalid command line option"); *error = err; }
    }
    free(lo); free(so);
    return ok;
}

/* ─── Macros ──────────────────────────────────────────────────────────────── */
#define SDL3_NO_MENU      0   /* ImGui::BeginMenuBar implémenté v2 */
#define SDL3_NO_TERMINAL  0   /* Console ImGui avec scrollback implémentée v2 */
#define SDL3_HAS_COLORPICKER 1
#define SDL3_HAS_TABLE    1
#define SDL3_HAS_CALENDAR 1

#ifdef __cplusplus
}
#endif

/* ─── GtkWidget = WidgetNode (pont immediate-mode) ──────────────────────── */
struct WidgetNode;
typedef struct WidgetNode GtkWidget;


/* Trace de mise au point de sermo. Remplace une macro homonyme de l'amont dont
 * le nom portait le pseudonyme de son auteur : aucune raison de le garder dans
 * du code qui ne lui appartient pas. Silencieuse sauf si SERMO_TRACE_ON. */
#ifndef SERMO_TRACE
#  ifdef SERMO_TRACE_ON
#    define SERMO_TRACE(...) fprintf(stderr, __VA_ARGS__)
#  else
#    define SERMO_TRACE(...) ((void)0)
#  endif
#endif

#endif /* SDL3_COMPAT_H */
