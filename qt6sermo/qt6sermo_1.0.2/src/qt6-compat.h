/* qt6-compat.h — Couche de compatibilité GTK/GLib → Qt6
 * Force-inclus dans les unités C du core via -include dans CMakeLists.txt.
 *
 * haplo-dialog / qt6sermo 1.0.0
 * Contact : devel@haplo-dialog.fr
 */

#ifndef QT6_COMPAT_H
#define QT6_COMPAT_H

/* ─── _GNU_SOURCE avant tout include ──────────────────────────────────────── */
#ifndef _GNU_SOURCE
#define _GNU_SOURCE
#endif

#include <stdlib.h>
#include <string.h>
#include <stdio.h>
#include <stdint.h>

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
typedef signed char   gint8;
typedef unsigned char guint8;
typedef int32_t       gint32;
typedef uint32_t      guint32;
typedef int64_t       gint64;
typedef uint64_t      guint64;
typedef unsigned long GType;
#include <stddef.h>
#include <stdarg.h>     /* g_build_filename */
#include <errno.h>      /* rapport d'échec d'exec (g_spawn_*) */
#include <fcntl.h>      /* pipe2 O_CLOEXEC */
#include <sys/stat.h>   /* g_file_test */
#include <unistd.h>     /* access, pipe */
typedef size_t       gsize;
typedef ptrdiff_t    gssize;

/* static assertion portable C/C++ */
#if defined(__cplusplus)
#  define QT6_STATIC_ASSERT(cond, msg) static_assert(cond, msg)
#elif defined(__STDC_VERSION__) && __STDC_VERSION__ >= 201112L
#  define QT6_STATIC_ASSERT(cond, msg) _Static_assert(cond, msg)
#else
#  define QT6_STATIC_ASSERT(cond, msg) typedef char qt6_sa_##__LINE__[(cond) ? 1 : -1]
#endif
QT6_STATIC_ASSERT(sizeof(gint32) == 4, "gint32 must be 4 bytes");
QT6_STATIC_ASSERT(sizeof(gint64) == 8, "gint64 must be 8 bytes");
#ifndef TRUE
#define TRUE  1
#endif
#ifndef FALSE
#define FALSE 0
#endif
#ifndef NULL
#define NULL ((void*)0)
#endif

/* ─── Mémoire ─────────────────────────────────────────────────────────────
 * ⚠️ Contrat GLib : g_malloc et famille AVORTENT sur échec d'allocation et ne
 * rendent jamais NULL — le code écrit en style GLib ne teste donc pas les
 * retours. Shimmer sur malloc nu inversait ce contrat : sous pression mémoire,
 * déréférencement de NULL au lieu d'un abort net. (Audit du 2026-09-03.) */
static inline void *_qt6_malloc(size_t n)  { void *p = malloc(n ? n : 1);      if (!p) abort(); return p; }
static inline void *_qt6_malloc0(size_t n) { void *p = calloc(1, n ? n : 1);   if (!p) abort(); return p; }
static inline void *_qt6_realloc(void *q, size_t n) { void *p = realloc(q, n ? n : 1); if (!p) abort(); return p; }
#define g_malloc(n)          _qt6_malloc(n)
#define g_malloc0(n)         _qt6_malloc0(n)
#define g_realloc(p, n)      _qt6_realloc((p), (n))
#define g_free(p)            free(p)
#define g_new(t, n)          ((t*)_qt6_malloc(sizeof(t) * (n)))
#define g_new0(t, n)         ((t*)_qt6_malloc0((size_t)(n) * sizeof(t)))
#define g_renew(t, p, n)     ((t*)_qt6_realloc((p), sizeof(t) * (n)))

/* ─── Chaînes ───────────────────────────────────────────────────────────── */
static inline __attribute__((warn_unused_result, malloc))
char *_qt6_strdup(const char *s) {
    if (!s) return NULL;
    char *r = strdup(s); if (!r) abort();   /* contrat mémoire GLib */
    return r;
}
#define g_strdup(s) _qt6_strdup(s)
/* GLib rend NULL pour une entrée NULL ; strndup(NULL, n) segfaulte.
 * g_strdup avait reçu cette garde, g_strndup juste en dessous non. */
static inline __attribute__((warn_unused_result, malloc))
char *_qt6_strndup(const char *s, size_t n) {
    if (!s) return NULL;
    char *r = strndup(s, n); if (!r) abort();
    return r;
}
#define g_strndup(s, n)           _qt6_strndup((s), (n))
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
/* ⚠️ PAS strtod() nu : gtkdialog.c appelle setlocale(LC_ALL, ""), donc sous une
 * locale française strtod("2.5") s'arrête au point et rend 2 — la partie
 * décimale disparaît EN SILENCE. Le contrat de g_ascii_strtod est justement
 * d'être indépendant de la locale ; le shim le trahissait. Mesuré le
 * 2026-09-03 : <timer interval="2.5"> déclenchait à 2000 ms sous fr_FR.UTF-8
 * et à 2500 ms sous C — même binaire, même XML. Le correctif force la locale
 * « C » via strtod_l, sans toucher la locale du processus. */
#include <locale.h>
static inline double _compat_ascii_strtod(const char *s, char **end) {
    static locale_t loc_c = (locale_t)0;
    if (loc_c == (locale_t)0)
        loc_c = newlocale(LC_ALL_MASK, "C", (locale_t)0);
    if (loc_c != (locale_t)0)
        return strtod_l(s, end, loc_c);
    return strtod(s, end);   /* repli si newlocale échoue — garde:primitive-locale */
}
/* g_strtod ≠ g_ascii_strtod : GLib essaie la locale COURANTE puis la locale C
 * et garde la conversion la plus longue — sous fr_FR, g_strtod("2,5") vaut 2.5.
 * L'aliaser sur le parseur C-only trahissait ce second contrat. */
static inline double _qt6_strtod_dual(const char *s, char **end) {
    char *e1 = NULL, *e2 = NULL;
    double v1 = strtod(s, &e1);          /* volontairement locale — garde:primitive-locale */
    double v2 = _compat_ascii_strtod(s, &e2);
    if (e2 > e1) { if (end) *end = e2; return v2; }
    if (end) *end = e1;
    return v1;
}
#define g_strtod(s, e)            _qt6_strtod_dual((s), (e))
#define g_ascii_strtod(s, e)      _compat_ascii_strtod((s), (e))
#define g_ascii_strtoull(s,e,b)   strtoull((s),(e),(b))
/* Contrat GLib : NULL trie AVANT toute chaîne (g_strcmp0(NULL,"") = -1) ;
 * mapper NULL sur "" rendait NULL et chaîne vide indiscernables, et la macro
 * double-évaluait ses arguments. */
static inline int _qt6_strcmp0(const char *a, const char *b) {
    if (!a) return (b != NULL) ? -1 : 0;
    if (!b) return 1;
    return strcmp(a, b);
}
#define g_strcmp0(a, b)           _qt6_strcmp0((a), (b))
#define g_str_has_prefix(s,p)     (strncmp((s),(p),strlen(p))==0)
#define g_str_has_suffix(s,x) \
    (strlen(s)>=strlen(x) && strcmp((s)+strlen(s)-strlen(x),(x))==0)
/* snprintf_safe() — deux passes va_list, remplace l'expression-statement VLA */
#include <stdarg.h>
static inline char *snprintf_safe(const char *fmt, ...) {
    va_list ap1, ap2;
    va_start(ap1, fmt);
    va_copy(ap2, ap1);
    int n = vsnprintf(NULL, 0, fmt, ap1);
    va_end(ap1);
    if (n < 0) { va_end(ap2); return NULL; }
    char *s = (char *)malloc((size_t)n + 1);
    if (s) vsnprintf(s, (size_t)n + 1, fmt, ap2);
    va_end(ap2);
    return s;
}
#define g_strdup_printf(fmt, ...) snprintf_safe(fmt, ##__VA_ARGS__)
static inline char **g_strsplit(const char *string, const char *delimiter, int max_tokens) {
    /* Contrat GLib mesuré : string NULL → NULL ; chaîne VIDE → vecteur vide
     * (premier élément NULL), pas [""]. (Audit 2026-09-03.) */
    if (!string) return NULL;
    if (!*string) { char **r=(char**)_qt6_malloc(sizeof(char*)); r[0]=NULL; return r; }
    if (!delimiter || !*delimiter) {
        char **r=(char**)malloc(2*sizeof(char*)); r[0]=strdup(string); r[1]=NULL; return r; }
    int cap=8,n=0; char **out=(char**)malloc((size_t)cap*sizeof(char*));
    size_t dlen=strlen(delimiter); const char *q=string;
    for(;;){
        const char *hit=strstr(q,delimiter);
        if(max_tokens>0 && n==max_tokens-1) hit=NULL;
        size_t seg = hit ? (size_t)(hit-q) : strlen(q);
        if(n+1>=cap){ cap*=2; out=(char**)realloc(out,(size_t)cap*sizeof(char*)); }
        char *t=(char*)malloc(seg+1); memcpy(t,q,seg); t[seg]='\0'; out[n++]=t;
        if(!hit) break;
        q=hit+dlen;
    }
    out[n]=NULL; return out;
}
static inline void g_strfreev(char **v) {
    if (!v) return;
    for (char **p = v; *p; p++) free(*p);
    free(v);
}
/* Ces quatre-là rendaient des CONSTANTES ("" , ".", 0) — zéro appelant
 * aujourd'hui, mais exportés à tout le cœur : le prochain appelant aurait
 * hérité d'un mensonge silencieux. Implémentés pour de vrai (audit 2026-09-03). */
static inline char *g_strjoinv(const char *sep, char **v) {
    if (!v || !v[0]) return _qt6_strdup("");
    size_t seplen = sep ? strlen(sep) : 0, total = 0;
    int n = 0;
    for (; v[n]; n++) total += strlen(v[n]);
    char *r = (char *)_qt6_malloc(total + seplen * (size_t)(n - 1) + 1);
    char *w = r;
    for (int i = 0; i < n; i++) {
        size_t l = strlen(v[i]); memcpy(w, v[i], l); w += l;
        if (v[i + 1] && seplen) { memcpy(w, sep, seplen); w += seplen; }
    }
    *w = '\0';
    return r;
}
static inline char *g_path_get_dirname(const char *file_name) {
    if (!file_name) return _qt6_strdup(".");
    const char *slash = strrchr(file_name, '/');
    if (!slash) return _qt6_strdup(".");
    while (slash > file_name && slash[-1] == '/') slash--;
    if (slash == file_name) return _qt6_strdup("/");
    size_t l = (size_t)(slash - file_name);
    char *r = (char *)_qt6_malloc(l + 1); memcpy(r, file_name, l); r[l] = '\0';
    return r;
}
static inline char *g_path_get_basename(const char *file_name) {
    if(!file_name || !*file_name){ char *r=(char*)malloc(2); r[0]='.'; r[1]='\0'; return r; }
    size_t len=strlen(file_name);
    while(len>1 && file_name[len-1]=='/') len--;
    const char *base=file_name;
    for(size_t i=0;i<len;i++) if(file_name[i]=='/') base=file_name+i+1;
    size_t blen=(size_t)(file_name+len-base);
    char *r=(char*)malloc(blen+1); memcpy(r,base,blen); r[blen]='\0'; return r;
}
static inline char *_qt6_build_filename(const char *first, ...) {
    if (!first) return _qt6_strdup("");
    size_t rl = strlen(first);
    char *r = (char *)_qt6_malloc(rl + 1); memcpy(r, first, rl + 1);
    va_list ap; va_start(ap, first);
    const char *seg;
    while ((seg = va_arg(ap, const char *)) != NULL) {
        while (*seg == '/') seg++;
        size_t sl = strlen(seg);
        r = (char *)_qt6_realloc(r, rl + 1 + sl + 1);
        while (rl && r[rl - 1] == '/') rl--;   /* un seul séparateur */
        r[rl++] = '/';
        memcpy(r + rl, seg, sl + 1); rl += sl;
    }
    va_end(ap);
    return r;
}
/* le NULL final est ajouté ici ; un NULL déjà passé par l'appelant arrête
 * simplement la lecture plus tôt — sans danger. */
#define g_build_filename(...)     _qt6_build_filename(__VA_ARGS__, (const char *)NULL)
typedef int GFileTest;
#define G_FILE_TEST_IS_REGULAR    (1 << 0)
#define G_FILE_TEST_IS_SYMLINK    (1 << 1)
#define G_FILE_TEST_IS_DIR        (1 << 2)
#define G_FILE_TEST_IS_EXECUTABLE (1 << 3)
#define G_FILE_TEST_EXISTS        (1 << 4)
static inline gboolean g_file_test(const char *f, GFileTest t) {
    struct stat st;
    if ((t & G_FILE_TEST_IS_SYMLINK) && lstat(f, &st) == 0 && S_ISLNK(st.st_mode))
        return TRUE;
    if (stat(f, &st) != 0) return FALSE;
    if (t & G_FILE_TEST_EXISTS) return TRUE;
    if ((t & G_FILE_TEST_IS_REGULAR) && S_ISREG(st.st_mode)) return TRUE;
    if ((t & G_FILE_TEST_IS_DIR) && S_ISDIR(st.st_mode)) return TRUE;
    if ((t & G_FILE_TEST_IS_EXECUTABLE) && access(f, X_OK) == 0) return TRUE;
    return FALSE;
}

/* ─── Helpers chaîne / UTF-8 (stringman.c, attributes.c…) ────────────────────
 * Le cœur n'utilise que de l'ASCII (préfixes de commandes, noms de widgets),
 * donc les fonctions « utf8 » se ramènent à leurs équivalents octet. */
#include <strings.h>
typedef uint32_t gunichar;
/* ⚠️ Contrat g_ascii_* : repli de casse ASCII, INDÉPENDANT de la locale.
 * str[n]casecmp suit LC_CTYPE (sous tr_TR, 'I'/'i' ne se replient plus l'un
 * sur l'autre) — même famille de trahison que g_ascii_strtod→strtod, payée le
 * 2026-09-03. Repli manuel A-Z, rien d'autre. */
static inline int _qt6_ascii_lower(int c) { return (c >= 'A' && c <= 'Z') ? c + 32 : c; }
static inline int _qt6_ascii_strcasecmp(const char *a, const char *b) {
    int ca, cb;
    do {
        ca = _qt6_ascii_lower((unsigned char)*a++);
        cb = _qt6_ascii_lower((unsigned char)*b++);
    } while (ca && ca == cb);
    return ca - cb;
}
static inline int _qt6_ascii_strncasecmp(const char *a, const char *b, size_t n) {
    while (n--) {
        int ca = _qt6_ascii_lower((unsigned char)*a++);
        int cb = _qt6_ascii_lower((unsigned char)*b++);
        if (ca != cb) return ca - cb;
        if (!ca) return 0;
    }
    return 0;
}
#define g_ascii_strcasecmp(a,b)    _qt6_ascii_strcasecmp((a),(b))
#define g_ascii_strncasecmp(a,b,n) _qt6_ascii_strncasecmp((a),(b),(n))
#define g_snprintf(buf,n,fmt,...) snprintf((buf),(n),(fmt),##__VA_ARGS__)
/* Vrai décodage UTF-8 (audit 2026-09-03) : les versions « octet » rendaient
 * 0xC3 pour « é », comptaient les octets au lieu des caractères, et tronquaient
 * le gunichar cherché à un char. Faux pour tout libellé accentué — c'est-à-dire
 * la langue du projet. */
static inline gunichar g_utf8_get_char(const char *p) {
    const unsigned char *u = (const unsigned char *)p;
    if (u[0] < 0x80)          return u[0];
    if ((u[0] & 0xE0) == 0xC0) return ((gunichar)(u[0] & 0x1F) << 6)  |  (u[1] & 0x3F);
    if ((u[0] & 0xF0) == 0xE0) return ((gunichar)(u[0] & 0x0F) << 12) | ((gunichar)(u[1] & 0x3F) << 6)  |  (u[2] & 0x3F);
    if ((u[0] & 0xF8) == 0xF0) return ((gunichar)(u[0] & 0x07) << 18) | ((gunichar)(u[1] & 0x3F) << 12) | ((gunichar)(u[2] & 0x3F) << 6) | (u[3] & 0x3F);
    return 0xFFFD;   /* octet invalide : caractère de remplacement */
}
static inline const char *_qt6_utf8_next(const char *p) {
    p++;
    while ((*p & 0xC0) == 0x80) p++;
    return p;
}
static inline char *g_utf8_strchr(const char *p, gssize len, gunichar c) {
    if (!p) return NULL;
    const char *end = (len < 0) ? NULL : p + len;
    while (*p && (!end || p < end)) {
        if (g_utf8_get_char(p) == c) return (char *)p;
        p = _qt6_utf8_next(p);
    }
    return NULL;
}
static inline long g_utf8_strlen(const char *p, gssize max) {
    if (!p) return 0;
    long n = 0; gssize i = 0;
    while (p[i] && (max < 0 || i < max)) {
        if (((unsigned char)p[i] & 0xC0) != 0x80) n++;
        i++;
    }
    return n;
}
static inline long g_utf8_pointer_to_offset(const char *str, const char *pos) {
    long n = 0;
    while (str < pos) {
        if (((unsigned char)*str & 0xC0) != 0x80) n++;
        str++;
    }
    return n;
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
#define g_strstrip(s)             g_strchomp(g_strchug(s))

/* ─── Messages ──────────────────────────────────────────────────────────── */
#ifndef g_print
#define g_print(fmt, ...)     printf(fmt, ##__VA_ARGS__)
#endif
#ifndef g_printerr
#define g_printerr(fmt, ...)  fprintf(stderr, fmt, ##__VA_ARGS__)
#endif
#ifndef g_warning
#define g_warning(fmt, ...)   fprintf(stderr, "WARNING: " fmt "\n", ##__VA_ARGS__)
#endif
#ifndef g_critical
#define g_critical(fmt, ...)  fprintf(stderr, "CRITICAL: " fmt "\n", ##__VA_ARGS__)
#endif
#ifndef g_message
#define g_message(fmt, ...)   fprintf(stderr, fmt "\n", ##__VA_ARGS__)
#endif
#ifndef g_error
#define g_error(fmt, ...)     do { fprintf(stderr, "ERROR: " fmt "\n", ##__VA_ARGS__); abort(); } while(0)
#endif

/* ─── Debug (conditionnel -DDEBUG) ──────────────────────────────────────── */
#ifdef DEBUG
#ifndef g_debug
#define g_debug(fmt, ...)  fprintf(stderr, "DEBUG: " fmt "\n", ##__VA_ARGS__)
#endif
#ifndef g_info
#define g_info(fmt, ...)   fprintf(stderr, "INFO: "  fmt "\n", ##__VA_ARGS__)
#endif
#else
#ifndef g_debug
#define g_debug(fmt, ...)  do {} while (0)
#endif
#ifndef g_info
#define g_info(fmt, ...)   do {} while (0)
#endif
#endif

/* ─── Asserts ───────────────────────────────────────────────────────────── */
#define g_assert(expr)              do { if (!(expr)) { fprintf(stderr, "ASSERT FAILED: %s\n", #expr); abort(); } } while(0)
#define g_return_if_fail(expr)      do { if (!(expr)) return; } while(0)
#define g_return_val_if_fail(expr,v) do { if (!(expr)) return (v); } while(0)

/* ─── Environnement ─────────────────────────────────────────────────────── */
#define g_setenv(k,v,ow)    setenv((k),(v),(ow))
#define g_getenv(k)         getenv(k)
#define g_unsetenv(k)       unsetenv(k)

/* ─── GSList / GList minimaux ────────────────────────────────────────────── */
typedef struct _GSList { void *data; struct _GSList *next; } GSList;
#ifdef __cplusplus
extern "C" {
#endif
GSList* g_slist_append(GSList *list, void *data);
GSList* g_slist_prepend(GSList *list, void *data);
void    g_slist_free(GSList *list);
guint   g_slist_length(GSList *list);
#ifdef __cplusplus
}
#endif
#define g_slist_next(l)       ((l)?((GSList*)(l))->next:NULL)
#define g_slist_last(l)       ({ GSList *_l=(l); while(_l&&_l->next)_l=_l->next; _l; })
#define g_slist_foreach(l,f,d) do{GSList*_e=(l);while(_e){(f)(_e->data,(d));_e=_e->next;}}while(0)
#define g_slist_nth_data(l,n) ({ GSList*_e=(l);guint _i=0;while(_e&&_i<(n)){_e=_e->next;_i++;}_e?_e->data:NULL; })
#define g_slist_remove(l,d)   ({ GSList*_l=(l),*_p=NULL,*_e=_l; while(_e&&_e->data!=(d)){_p=_e;_e=_e->next;} if(_e){if(_p)_p->next=_e->next;else _l=_e->next;free(_e);} _l; })

typedef struct _GList { void *data; struct _GList *next; struct _GList *prev; } GList;
#ifdef __cplusplus
extern "C" {
#endif
GList* g_list_append(GList *list, void *data);
void   g_list_free(GList *list);
guint  g_list_length(GList *list);
#ifdef __cplusplus
}
#endif
#define g_list_next(l)       ((l)?((GList*)(l))->next:NULL)
#define g_list_previous(l)   ((l)?((GList*)(l))->prev:NULL)
#define g_list_last(l)       ({ GList*_l=(l);while(_l&&_l->next)_l=_l->next;_l; })
#define g_list_prepend(l,d)  ({ GList*_n=(GList*)malloc(sizeof(GList));_n->data=(d);_n->next=(l);_n->prev=NULL;if(l)((GList*)(l))->prev=_n;_n; })
#define g_list_foreach(l,f,d) do{GList*_e=(l);while(_e){(f)(_e->data,(d));_e=_e->next;}}while(0)
#define g_list_nth_data(l,n) ({ GList*_e=(l);guint _i=0;while(_e&&_i<(n)){_e=_e->next;_i++;}_e?_e->data:NULL; })
#define g_list_find(l,d)     ({ GList*_e=(l);while(_e&&_e->data!=(d))_e=_e->next;_e; })
#define g_list_remove(l,d)   ({ GList*_l=(l),*_e=_l;while(_e&&_e->data!=(d))_e=_e->next;if(_e){if(_e->prev)_e->prev->next=_e->next;else _l=_e->next;if(_e->next)_e->next->prev=_e->prev;free(_e);}_l; })
#define g_list_copy(l)       ({ GList*_r=NULL,*_e=(l);while(_e){_r=g_list_append(_r,_e->data);_e=_e->next;}_r; })
#define g_list_delete_link(l,lnk) ({ GList*_l=(l),*_e=(GList*)(lnk);if(_e->prev)_e->prev->next=_e->next;else _l=_e->next;if(_e->next)_e->next->prev=_e->prev;free(_e);_l; })

/* ─── GParamSpec minimal (tag_attributes.c) ─────────────────────────────── */
typedef struct _GParamSpec { GType value_type; int flags; } GParamSpec;
typedef void GObjectClass;
#define G_TYPE_INVALID    0
#define G_TYPE_NONE       1
#define G_TYPE_INTERFACE  2
#define G_TYPE_CHAR       3
#define G_TYPE_UCHAR      4
#define G_TYPE_BOOLEAN    5
#define G_TYPE_INT        6
#define G_TYPE_UINT       7
#define G_TYPE_LONG       8
#define G_TYPE_ULONG      9
#define G_TYPE_INT64      10
#define G_TYPE_UINT64     11
#define G_TYPE_ENUM       12
#define G_TYPE_FLAGS      13
#define G_TYPE_FLOAT      14
#define G_TYPE_DOUBLE     15
#define G_TYPE_STRING     16
#define G_TYPE_POINTER    17
#define G_TYPE_BOXED      18
#define G_TYPE_PARAM      19
#define G_TYPE_OBJECT     20
#define G_PARAM_READABLE  1
#define G_PARAM_WRITABLE  2
#define G_PARAM_READWRITE 3
#define G_OBJECT_GET_CLASS(o)             NULL
#define g_object_class_find_property(c,n) NULL
#define G_OBJECT_CLASS(c)                 ((GObjectClass*)(c))
#define G_OBJECT(w)                       ((void*)(w))
#define G_IS_OBJECT(o)                    ((o)!=NULL)
#define g_object_set(obj, ...)            /* no-op */
#define g_object_set_data(o,k,v)          /* no-op */
#define g_object_get_data(o,k)            NULL
#define g_object_unref(o)                 /* no-op */

/* ─── Exécution de processus (g_spawn / g_shell) — POSIX ──────────────────────
 * Le port Qt6 utilise QApplication, pas une boucle GLib : safe_exec.c
 * (durcissement sécurité) est réimplémenté au-dessus de fork/exec/pipe.
 * g_child_watch_add est inutile — le double fork de g_spawn_async_with_pipes
 * réattache l'enfant à init, qui le récupère automatiquement (zéro zombie). */
#include <unistd.h>
#include <sys/types.h>
#include <sys/wait.h>

#define G_SPAWN_SEARCH_PATH         (1 << 2)
#define G_SPAWN_STDOUT_TO_DEV_NULL  (1 << 3)
#define G_SPAWN_STDERR_TO_DEV_NULL  (1 << 4)
#define G_SPAWN_DO_NOT_REAP_CHILD   (1 << 1)
#define GSpawnFlags int
typedef int GPid;
#define g_spawn_close_pid(pid)      ((void)(pid))

#ifndef QT6_COMPAT_GERROR
#define QT6_COMPAT_GERROR
typedef struct _GError { int domain; int code; char *message; } GError;
static inline void g_error_free(GError *e) { if (e) { free(e->message); free(e); } }
#endif

static inline void _qt6_set_gerror(GError **error, const char *msg) {
    if (!error) return;
    GError *e = (GError *)calloc(1, sizeof(GError));
    if (e) { e->message = strdup(msg ? msg : "error"); *error = e; }
}

/* Découpe une ligne SANS métacaractères en argv (séparateurs : blancs).
 * safe_exec.c ne l'appelle que pour des commandes déjà filtrées : pas de
 * guillemets ni d'expansion à gérer ici. */
static inline gboolean g_shell_parse_argv(const char *line, int *argcp,
        char ***argvp, GError **error) {
    if (!line || !argvp) { _qt6_set_gerror(error, "empty command"); return FALSE; }
    int cap = 8, n = 0;
    char **argv = (char **)malloc((size_t)cap * sizeof(char *));
    char *copy  = strdup(line);
    if (!argv || !copy) { free(argv); free(copy);
        _qt6_set_gerror(error, "out of memory"); return FALSE; }
    char *save = NULL;
    for (char *tok = strtok_r(copy, " \t\n\r\f\v", &save); tok;
             tok = strtok_r(NULL, " \t\n\r\f\v", &save)) {
        /* Contrat GLib : un mot non cité commençant par '#' ouvre un
         * commentaire et TERMINE l'analyse. '#' n'est pas un métacaractère de
         * safe_exec : sans ce cas, « cmd arg # note » passait les '#' en
         * arguments réels là où le port de référence les élague. (2026-09-03) */
        if (tok[0] == '#') break;
        if (n + 1 >= cap) { cap *= 2;
            char **grown = (char **)realloc(argv, (size_t)cap * sizeof(char *));
            if (!grown) { for (int i = 0; i < n; i++) free(argv[i]);
                free(argv); free(copy);
                _qt6_set_gerror(error, "out of memory"); return FALSE; }
            argv = grown; }
        argv[n++] = strdup(tok);
    }
    argv[n] = NULL;
    free(copy);
    if (n == 0) { free(argv); _qt6_set_gerror(error, "empty command"); return FALSE; }
    if (argcp) *argcp = n;
    *argvp = argv;
    return TRUE;
}

/* Exécution synchrone : statut wait() brut renvoyé dans *exit_status. */
/* ─── Environnement & tableau de pointeurs (requis par le tronc durci 1.1.3) ─── */
extern char **environ;
static inline char **g_get_environ(void) {
    int n=0; while(environ && environ[n]) n++;
    char **out=(char**)malloc((size_t)(n+1)*sizeof(char*));
    for(int i=0;i<n;i++) out[i]=strdup(environ[i]);
    out[n]=NULL; return out;
}
typedef struct { void **pdata; unsigned len; unsigned _cap; } GPtrArray;
static inline GPtrArray *g_ptr_array_new(void) {
    GPtrArray *a=(GPtrArray*)malloc(sizeof(GPtrArray));
    a->_cap=8; a->len=0; a->pdata=(void**)malloc(a->_cap*sizeof(void*)); return a;
}
static inline void g_ptr_array_add(GPtrArray *a, void *ptr) {
    if(a->len+1>a->_cap){ a->_cap*=2; a->pdata=(void**)realloc(a->pdata,a->_cap*sizeof(void*)); }
    a->pdata[a->len++]=ptr;
}
static inline void *g_ptr_array_free(GPtrArray *a, int free_seg) {
    void **d=a->pdata; if(free_seg){ free(d); d=NULL; } free(a); return d;
}

static inline gboolean g_spawn_sync(const char *wd, char **argv, char **envp,
        int flags, void *setup, void *data,
        char **out_str, char **err_str, int *exit_status, GError **error) {
    (void)wd; (void)flags; (void)setup; (void)data;
    (void)out_str; (void)err_str;
    if (!argv || !argv[0]) { _qt6_set_gerror(error, "no argv"); return FALSE; }
    /* ⚠️ Contrat GLib : commande introuvable ⇒ FALSE + GError — l'appelant est
     * AVERTI. L'ancien shim rendait TRUE avec exit_status 127<<8, comme si la
     * commande avait tourné : l'échec d'exec était invisible. Technique GLib
     * reproduite : un tube O_CLOEXEC que l'enfant n'écrit QUE si exec échoue —
     * fermé tout seul (donc EOF côté parent) si exec réussit. (2026-09-03) */
    int ep[2];
    if (pipe2(ep, O_CLOEXEC) != 0) { _qt6_set_gerror(error, "pipe failed"); return FALSE; }
    pid_t pid = fork();
    if (pid < 0) { close(ep[0]); close(ep[1]);
        _qt6_set_gerror(error, "fork failed"); return FALSE; }
    if (pid == 0) {
        close(ep[0]);
        if (envp) execvpe(argv[0], argv, envp); else execvp(argv[0], argv);
        int err = errno;
        ssize_t w = write(ep[1], &err, sizeof err); (void)w;
        _exit(127);
    }
    close(ep[1]);
    int execerr = 0;
    ssize_t r;
    while ((r = read(ep[0], &execerr, sizeof execerr)) < 0 && errno == EINTR) { }
    close(ep[0]);
    int status = 0;
    while (waitpid(pid, &status, 0) < 0) { /* retry on EINTR */ }
    if (r > 0) { _qt6_set_gerror(error, strerror(execerr)); return FALSE; }
    if (exit_status) *exit_status = status;
    return TRUE;
}

/* Exécution asynchrone avec pipe stdout. Double fork → l'enfant réel est
 * réattaché à init (réapé automatiquement, zéro zombie). */
static inline gboolean g_spawn_async_with_pipes(const char *wd, char **argv,
        char **envp, int flags, void *setup, void *data, GPid *child_pid,
        int *stdin_fd, int *stdout_fd, int *stderr_fd, GError **error) {
    (void)wd; (void)envp; (void)flags; (void)setup; (void)data;
    if (!argv || !argv[0]) { _qt6_set_gerror(error, "no argv"); return FALSE; }
    int outpipe[2] = { -1, -1 };
    if (stdout_fd && pipe(outpipe) != 0) {
        _qt6_set_gerror(error, "pipe failed"); return FALSE; }
    pid_t pid = fork();
    if (pid < 0) {
        if (stdout_fd) { close(outpipe[0]); close(outpipe[1]); }
        _qt6_set_gerror(error, "fork failed");
        return FALSE;
    }
    /* Mêmes deux écarts de contrat que g_spawn_sync, corrigés (2026-09-03) :
     * (a) échec d'exec rapporté par un tube O_CLOEXEC (EOF = exec réussi) —
     *     l'ancien shim rendait TRUE et le lecteur du pipe voyait juste EOF ;
     * (b) *child_pid recevait le pid de l'INTERMÉDIAIRE, déjà moissonné — le
     *     vrai pid du petit-fils transite maintenant par le même tube. */
    int ep[2];
    if (pipe2(ep, O_CLOEXEC) != 0) {
        if (stdout_fd) { close(outpipe[0]); close(outpipe[1]); }
        _qt6_set_gerror(error, "pipe failed"); return FALSE;
    }
    if (pid == 0) {
        close(ep[0]);
        pid_t grandchild = fork();
        if (grandchild == 0) {
            if (stdout_fd) {
                dup2(outpipe[1], STDOUT_FILENO);
                close(outpipe[0]); close(outpipe[1]);
            }
            if(envp) execvpe(argv[0], argv, envp); else execvp(argv[0], argv);
            int err = errno;
            ssize_t w = write(ep[1], &err, sizeof err); (void)w;
            _exit(127);
        }
        /* pid du petit-fils vers le parent, par un canal qui survit à _exit */
        ssize_t w = write(ep[1], &grandchild, sizeof grandchild); (void)w;
        _exit(0);
    }
    close(ep[1]);
    if (stdout_fd) { close(outpipe[1]); *stdout_fd = outpipe[0]; }
    if (stdin_fd)  *stdin_fd  = -1;
    if (stderr_fd) *stderr_fd = -1;
    int dummy = 0;
    while (waitpid(pid, &dummy, 0) < 0) { /* reap intermediate */ }
    pid_t vrai_pid = -1;
    ssize_t r;
    while ((r = read(ep[0], &vrai_pid, sizeof vrai_pid)) < 0 && errno == EINTR) { }
    int execerr = 0; ssize_t r2;
    while ((r2 = read(ep[0], &execerr, sizeof execerr)) < 0 && errno == EINTR) { }
    close(ep[0]);
    if (r2 > 0) {   /* le petit-fils a écrit son errno : exec a échoué */
        if (stdout_fd) { close(*stdout_fd); *stdout_fd = -1; }
        _qt6_set_gerror(error, strerror(execerr));
        return FALSE;
    }
    if (child_pid) *child_pid = (GPid)(r > 0 ? vrai_pid : pid);
    return TRUE;
}

static inline unsigned g_child_watch_add(GPid pid,
        void (*cb)(GPid, int, void *), void *data) {
    (void)pid; (void)cb; (void)data;  /* double fork → rien à surveiller */
    return 0;
}

/* ─── Types GTK / GDK opaques (core C, signals.h) ──────────────────────── */
typedef void GtkWidget;
typedef void GtkWindow;
typedef void GtkContainer;
typedef void GtkBox;
typedef void GtkButton;
typedef void GtkLabel;
typedef void GtkEntry;
typedef void GtkTextView;
typedef void GtkCheckButton;
typedef void GtkRadioButton;
typedef void GtkToggleButton;
typedef void GtkComboBox;
typedef void GtkProgressBar;
typedef void GtkScrolledWindow;
typedef void GtkNotebook;
typedef void GtkImage;
typedef void GtkFileChooser;
typedef void GtkBuilder;
typedef void GtkSocket;
typedef void GtkCellRenderer;
typedef void GtkListStore;
typedef void GtkTreeView;
typedef void GtkTreeModel;
typedef struct { int stamp; void *user_data, *user_data2, *user_data3; } GtkTreeIter;
typedef void GtkTreeViewColumn;
typedef void GdkColor;
typedef void GdkRGBA;
typedef void GdkScreen;
typedef void GdkPixbuf;
typedef void GdkWindow;
/* GdkEvent : structures réelles. signals.c/automaton.c lisent quelques champs
 * et castent GdkEvent* vers les sous-types ; tous partagent « type » en tête
 * (initial common sequence) pour que les casts soient légaux. */
typedef int GdkEventType;
#define GDK_NOTHING        (-1)
#define GDK_EXPOSE         2          /* historique : automaton.c teste aussi ==2 */
#define GDK_CONFIGURE      13
#define GDK_ALL_EVENTS_MASK 0x3FFFFE
typedef struct { GdkEventType type; double x, y; unsigned int state, button;
                 double x_root, y_root; } GdkEventButton;
typedef struct { GdkEventType type; int x, y, width, height; } GdkEventConfigure;
typedef struct { GdkEventType type; unsigned int keyval, state, hardware_keycode;
               } GdkEventKey;
typedef struct { GdkEventType type; int mode, detail; } GdkEventCrossing;
typedef struct { GdkEventType type; int in; } GdkEventFocus;
typedef struct { GdkEventType type; int direction; double x, y; } GdkEventScroll;
typedef struct { GdkEventType type; double x, y; unsigned int state; } GdkEventMotion;
typedef union { GdkEventType type; GdkEventButton button; GdkEventConfigure configure;
                GdkEventKey key; GdkEventCrossing crossing; GdkEventFocus focus;
                GdkEventScroll scroll; GdkEventMotion motion; } GdkEvent;
typedef void GdkDisplay;
typedef void GFileMonitor;
typedef void GFile;
typedef void GtkTreePath;
typedef int  GtkWindowPosition;
typedef int  GtkOrientation;
typedef int  GtkPackType;
typedef int  GtkShadowType;
typedef int  GtkPolicyType;
typedef int  GtkSortType;
typedef int  GtkWrapMode;
typedef int  GtkJustification;
typedef int  GtkResponseType;
typedef int  GtkEntryIconPosition;
typedef int  GdkInputCondition;
typedef int  GFileMonitorEvent;

/* ─── Constantes GTK ────────────────────────────────────────────────────── */
#define GTK_WIN_POS_CENTER         1
#define GTK_WIN_POS_NONE           0
#define GTK_ORIENTATION_HORIZONTAL 0
#define GTK_ORIENTATION_VERTICAL   1
#define GTK_PACK_START             0
#define GTK_PACK_END               1
#define GTK_SHADOW_NONE            0
#define GTK_POLICY_AUTOMATIC       1
#define GTK_JUSTIFY_LEFT           0
#define GTK_JUSTIFY_CENTER         2
#define GTK_WINDOW_POPUP           0
#define GTK_WINDOW_TOPLEVEL        1

/* ─── Shims GTK → C wrappers (appelés depuis fichiers C) ───────────────── */
typedef void Qt6Widget_C;
#ifdef __cplusplus
extern "C" {
#endif
void  qt6_widget_show(Qt6Widget_C *w);
void  qt6_widget_hide(Qt6Widget_C *w);
void  qt6_widget_set_sensitive(Qt6Widget_C *w, int sensitive);
void  qt6_widget_redraw(Qt6Widget_C *w);
void  qt6_container_add(Qt6Widget_C *container, Qt6Widget_C *child);
void *qt6_container_child0(Qt6Widget_C *container);
void  qt6_window_move(Qt6Widget_C *w, int x, int y);
void *qt6_scroll_new(int w, int h);
#ifdef __cplusplus
}
#endif

#define gtk_widget_show(w)            qt6_widget_show((Qt6Widget_C*)(w))
#define gtk_widget_hide(w)            qt6_widget_hide((Qt6Widget_C*)(w))
#define gtk_widget_show_all(w)        qt6_widget_show((Qt6Widget_C*)(w))
#define gtk_widget_set_visible(w,v)   ((v)?qt6_widget_show((Qt6Widget_C*)(w)):qt6_widget_hide((Qt6Widget_C*)(w)))
#define gtk_widget_set_sensitive(w,s) qt6_widget_set_sensitive((Qt6Widget_C*)(w),(s))
#define gtk_widget_queue_draw(w)      qt6_widget_redraw((Qt6Widget_C*)(w))
#define gtk_widget_set_size_request(w,ww,hh) /* no-op */
#define gtk_container_add(c,w)        qt6_container_add((Qt6Widget_C*)(c),(Qt6Widget_C*)(w))
#define GTK_IS_WIDGET(w)              ((w)!=NULL)
#define GTK_WIDGET(w)                 ((void*)(w))
#define GTK_WINDOW(w)                 ((void*)(w))
#define GTK_BIN(w)                    ((void*)(w))
#define gtk_bin_get_child(b)          qt6_container_child0((Qt6Widget_C*)(b))
#define GTK_SCROLLED_WINDOW(w)        ((void*)(w))
#define gtk_scrolled_window_add_with_viewport(s,w) qt6_container_add((Qt6Widget_C*)(s),(Qt6Widget_C*)(w))
#define gtk_scrolled_window_new(h,v)  qt6_scroll_new(200,150)
#define GTK_SOCKET(w)                 (w)
#define gtk_socket_new()              NULL
#define gtk_socket_get_id(s)          0
#define gtk_widget_get_parent(w)      NULL
#define gtk_widget_get_name(w)        ""
#define gtk_widget_set_name(w,n)      /* no-op */
#define gtk_window_move(w,x,y)        qt6_window_move((Qt6Widget_C*)(w),(x),(y))
#define gtk_window_resize(w,ww,hh)    /* no-op */
#define gtk_window_set_title(w,t)     /* no-op */
#define gtk_window_set_position(w,p)  /* no-op */
#define GTK_CONTAINER(w)              ((void*)(w))
#define gtk_container_remove(c,w)     /* no-op */
#define g_signal_connect(o,s,c,d)     /* no-op */
#define g_signal_connect_after(o,s,c,d) /* no-op */
#define g_signal_handler_block(o,i)   /* no-op */
#define g_signal_handler_unblock(o,i) /* no-op */
#define g_signal_emit_by_name(o,s,...) /* no-op */
#define GTK_PROGRESS_BAR(w)           (w)
#define GTK_RANGE(w)                  (w)
#define GTK_SPIN_BUTTON(w)            (w)
#define GTK_ENTRY(w)                  (w)
#define GTK_LABEL(w)                  (w)
#define GTK_TEXT_VIEW(w)              (w)
#define GTK_CHECK_BUTTON(w)           (w)
#define GTK_RADIO_BUTTON(w)           (w)
#define GTK_TOGGLE_BUTTON(w)          (w)
#define GTK_COMBO_BOX(w)              (w)
#define GTK_IMAGE(w)                  (w)
#define GTK_TREE_VIEW(w)              (w)
#define GTK_CELL_RENDERER(w)          (w)
#define GTK_TREE_ITER(w)              (w)
#define GTK_TREE_PATH(w)              (w)
#define GTK_STATUSBAR(w)              (w)
#define GTK_EXPANDER(w)               (w)
#define GTK_FRAME(w)                  (w)
#define GTK_NOTEBOOK(w)               (w)
#define GTK_FILE_CHOOSER(w)           (w)
#define GTK_FILE_CHOOSER_BUTTON(w)    (w)
#define GTK_COLOR_BUTTON(w)           (w)
#define GTK_FONT_BUTTON(w)            (w)
#define GTK_BOX(w)                    (w)
#define GTK_SEPARATOR(w)              (w)
#define GTK_DIALOG(w)                 (w)
#define GTK_CELL_RENDERER_TEXT(w)     (w)
#define GTK_LIST_STORE(w)             (w)
#define GTK_TREE_MODEL(w)             (w)
#define GTK_TREE_VIEW_COLUMN(w)       (w)
#define GTK_BUILDER(w)                (w)
#define GTK_SOCKET(w)                 (w)

/* GTK widget-specific stubs */
#define gtk_progress_bar_set_fraction(w,f) /* no-op */
#define gtk_progress_bar_get_fraction(w)   0.0
#define gtk_range_get_value(w)             0.0
#define gtk_range_set_value(w,v)           /* no-op */
#define gtk_spin_button_get_value(w)       0.0
#define gtk_spin_button_set_value(w,v)     /* no-op */
#define gtk_entry_get_text(w)              ""
#define gtk_entry_set_text(w,t)            /* no-op */
#define gtk_label_set_text(w,t)            /* no-op */
#define gtk_label_get_text(w)              ""
#define gtk_toggle_button_get_active(w)    0
#define gtk_toggle_button_set_active(w,v)  /* no-op */
#define gtk_combo_box_get_active(w)        0
#define gtk_combo_box_set_active(w,i)      /* no-op */
#define gtk_combo_box_get_active_text(w)   g_strdup("")
#define gtk_list_store_new(n,...)          NULL
#define gtk_list_store_append(s,i)         /* no-op */
#define gtk_list_store_set(s,i,...)        /* no-op */
#define gtk_list_store_clear(s)            /* no-op */
#define gtk_tree_view_new_with_model(m)    NULL
#define gtk_tree_view_get_model(w)         NULL
#define gtk_tree_view_column_new()         NULL
#define gtk_tree_view_append_column(w,c)   0
#define gtk_cell_renderer_text_new()       NULL
#define gtk_tree_view_column_pack_start(c,r,b) /* no-op */
#define gtk_tree_view_column_add_attribute(c,r,a,n) /* no-op */
#define gtk_tree_model_get_iter_first(m,i) FALSE
#define gtk_tree_model_get_iter_next(m,i)  FALSE
#define gtk_tree_model_get(m,i,...)        /* no-op */
#define gtk_statusbar_push(w,c,t)          0
#define gtk_statusbar_pop(w,c)             /* no-op */
#define gtk_statusbar_get_context_id(w,d)  0
#define gtk_expander_get_expanded(w)       FALSE
#define gtk_expander_set_expanded(w,v)     /* no-op */
#define gtk_expander_set_label(w,t)        /* no-op */
#define gtk_frame_set_label(w,t)           /* no-op */
#define gtk_notebook_get_current_page(w)   0
#define gtk_notebook_set_current_page(w,n) /* no-op */
#define gtk_file_chooser_get_filename(w)   g_strdup("")
#define gtk_file_chooser_set_filename(w,f) FALSE

/* ─── File chooser / fenêtre (actions.c::action_fileselect) ──────────────────
 * Le port Qt6 ouvre les vrais dialogues via QFileDialog côté C++ ; ces stubs
 * permettent seulement de compiler le chemin GTK (mort) de actions.c. */
#define GTK_TYPE_WINDOW                          0
#define gtk_widget_get_ancestor(w,t)             ((GtkWidget*)NULL)
#define gtk_window_present(w)                    /* no-op */
typedef int  GtkFileChooserAction;
typedef void GtkFileFilter;
#define GTK_FILE_CHOOSER_ACTION_OPEN             0
#define GTK_FILE_CHOOSER_ACTION_SAVE             1
#define GTK_FILE_CHOOSER_ACTION_SELECT_FOLDER    2
#define GTK_FILE_CHOOSER_ACTION_CREATE_FOLDER    3
#define GTK_RESPONSE_NONE                        (-1)
#define GTK_RESPONSE_ACCEPT                      (-3)
#define GTK_RESPONSE_OK                          (-5)
#define GTK_RESPONSE_CANCEL                      (-6)
#define gtk_file_chooser_dialog_new(...)         ((GtkWidget*)NULL)
#define gtk_file_chooser_set_current_folder(c,f) /* no-op */
#define gtk_file_filter_new()                    ((GtkFileFilter*)NULL)
#define gtk_file_filter_set_name(f,n)            /* no-op */
#define gtk_file_filter_add_pattern(f,p)         /* no-op */
#define gtk_file_filter_add_mime_type(f,m)       /* no-op */
#define gtk_file_chooser_add_filter(c,f)         /* no-op */
#define gtk_file_chooser_get_file(c)             ((GFile*)NULL)
#define g_file_get_path(f)                       ((char*)NULL)
#define gtk_color_button_get_rgba(w,c)     /* no-op */
#define gtk_color_button_set_rgba(w,c)     /* no-op */
#define gtk_font_button_get_font_name(w)   ""
#define gtk_font_button_set_font_name(w,n) FALSE
#define gtk_dialog_run(w)                  0
#define gtk_dialog_add_button(w,t,r)       NULL
#define gtk_dialog_get_content_area(w)     NULL
#define gtk_text_view_get_buffer(w)        NULL
#define gtk_text_buffer_get_text(b,s,e,h)  g_strdup("")
#define gtk_text_buffer_set_text(b,t,l)    /* no-op */
#define gtk_text_buffer_get_start_iter(b,i) /* no-op */
#define gtk_text_buffer_get_end_iter(b,i)   /* no-op */
#define gtk_text_iter_forward_to_end(i)     /* no-op */
#define gtk_widget_get_toplevel(w)          (w)
#define gtk_window_new(t)                   NULL
#define gtk_widget_destroy(w)               /* no-op */
#define gtk_box_new(o,s)                    NULL
#define gtk_vbox_new(h,s)                   NULL
#define gtk_hbox_new(h,s)                   NULL
#define gtk_box_pack_start(b,w,e,f,p)       /* no-op */
#define gtk_box_pack_end(b,w,e,f,p)         /* no-op */
#define gtk_separator_new(o)                NULL
#define gtk_image_new_from_file(f)          NULL
#define gtk_image_new_from_pixbuf(p)        NULL
#define gtk_image_set_from_file(w,f)        /* no-op */
#define gtk_scrolled_window_set_policy(w,h,v) /* no-op */
#define gtk_accel_group_new()               NULL
#define gtk_window_add_accel_group(w,a)     /* no-op */
#define GdkColor void
#define GdkRGBA  void

/* ─── GString minimal (variables.c) ─────────────────────────────────────────*/
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
static inline GString *g_string_append_c(GString *s, char c) {
    if (s->len + 2 > s->allocated_len) {
        while (s->len + 2 > s->allocated_len) s->allocated_len *= 2;
        s->str = (char *)realloc(s->str, s->allocated_len);
    }
    s->str[s->len++] = c;
    s->str[s->len] = '\0';
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

/* ─── Conversions pointeur/entier ───────────────────────────────────────────*/
#define GPOINTER_TO_INT(p)   ((int)(intptr_t)(p))
#define GPOINTER_TO_UINT(p)  ((unsigned int)(uintptr_t)(p))
#define GINT_TO_POINTER(i)   ((void *)(intptr_t)(i))
#define GUINT_TO_POINTER(u)  ((void *)(uintptr_t)(u))

/* ─── g_strconcat / g_strsplit_set ──────────────────────────────────────────*/
static inline char *g_strconcat(const char *first, ...) {
    if (!first) return _qt6_strdup("");
    size_t total = strlen(first);
    va_list ap;
    va_start(ap, first);
    for (const char *s = va_arg(ap, const char *); s; s = va_arg(ap, const char *))
        total += strlen(s);
    va_end(ap);
    char *out = (char *)malloc(total + 1);
    if (!out) return NULL;
    char *p = out;
    p = stpcpy(p, first);
    va_start(ap, first);
    for (const char *s = va_arg(ap, const char *); s; s = va_arg(ap, const char *))
        p = stpcpy(p, s);
    va_end(ap);
    return out;
}
static inline char **g_strsplit_set(const char *str, const char *delims, int max) {
    /* mêmes cas de bord que g_strsplit : NULL → NULL, "" → vecteur vide. */
    if (!str) return NULL;
    int n = 0, cap = 8;
    char **out = (char **)_qt6_malloc(sizeof(char *) * cap);
    const char *start = str;
    if (!*str) { out[0] = NULL; return out; }
    for (const char *p = str; ; p++) {
        int is_delim = (*p && strchr(delims, *p) != NULL);
        if (is_delim || *p == '\0') {
            if (max > 0 && n == max - 1 && *p) continue;
            size_t seglen = (size_t)(p - start);
            char *seg = (char *)malloc(seglen + 1);
            if (!seg) { for (int i = 0; i < n; i++) free(out[i]);
                free(out); return NULL; }
            memcpy(seg, start, seglen); seg[seglen] = '\0';
            if (n + 1 >= cap) { cap *= 2;
                char **grown = (char **)realloc(out, sizeof(char *) * cap);
                if (!grown) { free(seg);
                    for (int i = 0; i < n; i++) free(out[i]);
                    free(out); return NULL; }
                out = grown; }
            out[n++] = seg;
            if (*p == '\0') break;
            start = p + 1;
        }
    }
    out[n] = NULL;
    return out;
}

/* ─── GOption — au-dessus de getopt_long (gtkdialog.c) ───────────────────────*/
#include <getopt.h>
#define G_OPTION_ARG_NONE     0
#define G_OPTION_ARG_STRING   1
#define G_OPTION_FLAG_IN_MAIN 0
typedef struct {
    const char *long_name;
    char        short_name;
    int         flags;
    int         arg;
    void       *arg_data;
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
static inline void *gtk_get_option_group(int open_default_display) {
    (void)open_default_display; return NULL;  /* pas de groupe GTK côté Qt6 */
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
    char *so = (char *)malloc((size_t)n * 2 + 2);
    int soi = 0;
    so[soi++] = ':';
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

/* ─── GdkKeyval helpers (signals.c) ──────────────────────────────────────────
 * Le clavier réel passe par Qt côté C++ ; ces stubs suffisent à compiler le
 * chemin GTK d'export des touches. */
static inline const char *gdk_keyval_name(unsigned int keyval) { (void)keyval; return ""; }
static inline unsigned int gdk_keyval_to_unicode(unsigned int keyval) {
    /* Contrat GDK : le code point correspondant, ou 0 s'il n'y en a pas.
     * Rendre keyval inchangé n'était correct que pour le Latin-1 : une flèche
     * (0xFF51…) formatait un caractère de zone privée dans KEY_UNI au lieu de
     * rien, là où le port de référence exporte 0. (Audit 2026-09-03.) */
    if (keyval < 0x100) return keyval;                       /* Latin-1 direct */
    if ((keyval & 0xFF000000u) == 0x01000000u)               /* forme U+xxxx de GDK */
        return keyval & 0x00FFFFFFu;
    return 0;                                                /* touche spéciale */
}

/* ─── GFileMonitor (signals.c — auto-refresh) ───────────────────────────────*/
typedef int GFileMonitorEvent;
#define G_FILE_MONITOR_NONE            0
#define G_FILE_MONITOR_EVENT_CHANGED   1
#define g_file_new_for_path(p)                  ((GFile *)NULL)
#define g_file_monitor_file(f,flags,c,err)      ((GFileMonitor *)NULL)
#define g_file_monitor_cancel(m)                (TRUE)
#define g_file_monitor_set_rate_limit(m,ms)     ((void)0)

/* ─── Requisition + prédicats / stubs GTK manquants ─────────────────────────*/
typedef struct { int width, height; } GtkRequisition;

#define GTK_ENTRY_ICON_PRIMARY   0

#define GTK_IS_BUTTON(w)            0
#define GTK_IS_CHECK_MENU_ITEM(w)   0
#define GTK_IS_COLOR_BUTTON(w)      0
#define GTK_IS_COMBO_BOX(w)         0
#define GTK_IS_ENTRY(w)             0
#define GTK_IS_EXPANDER(w)          0
#define GTK_IS_FONT_BUTTON(w)       0
#define GTK_IS_LABEL(w)             0
#define GTK_IS_LIST_BOX(w)          0
#define GTK_IS_MENU_ITEM(w)         0
#define GTK_IS_PROGRESS_BAR(w)      0
#define GTK_IS_RADIO_MENU_ITEM(w)   0
#define GTK_IS_SCALE(w)             0
#define GTK_IS_SCROLLED_WINDOW(w)   0
#define GTK_IS_SPIN_BUTTON(w)       0
#define GTK_IS_SPINNER(w)           0
#define GTK_IS_TOGGLE_BUTTON(w)     0
#define GTK_IS_TREE_VIEW(w)         0
#define GTK_IS_VIEWPORT(w)          0

#define GTK_CHECK_MENU_ITEM(w)      (w)
#define GTK_VIEWPORT(w)             (w)
#define GTK_SPINNER(w)              (w)
#define GTK_LIST_BOX(w)             (w)
#define GTK_TREE_STORE(w)           (w)

#define gtk_check_menu_item_get_active(w)   FALSE
#define gtk_widget_get_sensitive(w)         TRUE
#define gtk_widget_get_visible(w)           TRUE
#define gtk_drawing_area_new()              NULL
#define gtk_file_chooser_widget_new(...)    NULL
#define gtk_file_chooser_add_shortcut_folder(...) FALSE
#define gtk_label_new(s)                    NULL
#define gtk_list_box_row_new()              NULL
#define gtk_list_box_get_row_at_index(b,i)  NULL
#define gtk_label_set_xalign(w,x)           ((void)0)
#define gtk_list_box_prepend(b,w)           ((void)0)
#define gtk_list_box_select_row(b,r)        ((void)0)
#define gtk_progress_bar_pulse(w)           ((void)0)
#define gtk_spinner_start(w)                ((void)0)
#define gtk_spinner_stop(w)                 ((void)0)
#define gtk_text_buffer_insert_at_cursor(b,t,l) ((void)0)
#define gtk_tree_store_append(s,i,p)        ((void)0)
#define gtk_tree_store_set(...)             ((void)0)
#define gtk_viewport_set_shadow_type(w,t)   ((void)0)
#define gtk_widget_activate(w)              FALSE
#define gtk_widget_grab_focus(w)            ((void)0)
#define gtk_widget_realize(w)               ((void)0)
#define gtk_widget_set_events(w,e)          ((void)0)

/* Bloque gtk/gtk.h si on tente de l'inclure via gtk3d.h */
#define __GTK_H__
#define __GTKX_H__

/* ─── Threads ───────────────────────────────────────────────────────────── */
#define g_thread_init(v)    /* no-op */
#define gdk_threads_init()  /* no-op */
#define gdk_threads_enter() /* no-op */
#define gdk_threads_leave() /* no-op */

/* ─── GTK init / main → cycle de vie QApplication (qt6-compat.cpp) ───────── */
#ifdef __cplusplus
extern "C" {
#endif
void qt6_app_init(int *argc, char ***argv);   /* crée la QApplication (sauf --print-ir) */
int  qt6_app_run(void);                         /* boucle d'événements (QApplication::exec) */
void qt6_app_quit(void);                        /* QApplication::quit */
#ifdef __cplusplus
}
#endif
#define gtk_init(argc, argv)  qt6_app_init((argc), (argv))
#define gtk_main()            qt6_app_run()
#define gtk_main_quit()       qt6_app_quit()
#define HAVE_GLADE_LIB 0

#endif /* QT6_COMPAT_H */
