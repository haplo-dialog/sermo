/* SPDX-License-Identifier: GPL-2.0-or-later */
/*
 * Copyright (C) 2026 S. Cage
 * haplo-dialog <devel@haplo-dialog.fr>
 */
/* efl-compat.h v2 — Couche de compatibilité GTK/GLib → EFL/Elementary
 *
 * haplo-dialog / efl1dialog 1.0.0
 * Licence : GPL-2.0-or-later
 * Contact : devel@haplo-dialog.fr
 *
 * Force-inclus via AM_CFLAGS: -include $(top_srcdir)/src/efl-compat.h
 *
 * Changements v2 :
 *  - snprintf_safe() remplace le statement-expression VLA g_strdup_printf
 *  - Attributs GCC __attribute__((nonnull, warn_unused_result, malloc))
 *  - _Static_assert sur les tailles critiques
 *  - Correction pclose→fclose sur FILE* issus de safe_popen/widget_opencommand
 *  - Ajout EFL_WIN_GET() macro sécurisée pour récupérer la fenêtre parente
 *  - Types stdint cohérents (gint8→int8_t, etc.)
 *  - g_debug/g_info conditionnels sur -DDEBUG
 */

#ifndef EFL_COMPAT_H
#define EFL_COMPAT_H

/* ─── config.h en premier ─────────────────────────────────────────────────── */
/*
 * efl-compat.h est force-inclus (-include) AVANT que l'unité de compilation
 * n'inclue config.h. Sans cela, HAVE_GLIB (et les autres HAVE_*) seraient
 * indéfinis ici, et « #if HAVE_GLIB » prendrait à tort la branche de repli.
 * -DHAVE_CONFIG_H est fourni par Automake (DEFS) ; config.h est dans
 * $(top_builddir), résolu via DEFAULT_INCLUDES (-I..).
 */
#ifdef HAVE_CONFIG_H
#  include "config.h"
#endif

/* ─── Standards ───────────────────────────────────────────────────────────── */
#ifndef _GNU_SOURCE
#  define _GNU_SOURCE
#endif
#ifndef _POSIX_C_SOURCE
#  define _POSIX_C_SOURCE 200809L
#endif

/* ─── Includes EFL ────────────────────────────────────────────────────────── */
#include <Elementary.h>
#include <Evas.h>
#include <Ecore.h>
#include <Eina.h>

#if HAVE_ECORE_EXE
#  include <Ecore_Exe.h>
#endif

/* ─── Includes C standard ─────────────────────────────────────────────────── */
#include <stdint.h>
#include <stddef.h>
#include <stdarg.h>
#include <stdlib.h>
#include <stdio.h>
#include <string.h>
#include <errno.h>
#include <assert.h>
#include <locale.h>   /* newlocale, strtod_l, uselocale, LC_ALL_MASK */

/* ─── Conversion numérique insensible à la locale ─────────────────────────── */
/*
 * atof()/strtod() nus lisent le séparateur décimal de la LOCALE : sous fr_FR,
 * strtod("0.1") s'arrête au point et rend 0. Un nombre écrit dans un XML doit
 * TOUJOURS être lu en locale « C » (point décimal). Ces deux inline remplacent
 * g_ascii_strtod (toujours C) et g_strtod (locale courante, avec repli C si
 * elle consomme davantage), y compris quand la vraie GLib est absente.
 */
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

/* ─── GLib : réelle si disponible (HAVE_GLIB), sinon repli Eina ───────────── */
/*
 * Le cœur partagé (variables.c, signals.c, automaton.c, tag_attributes.c…) est
 * un code GLib d'origine. Quand la vraie GLib est liée (HAVE_GLIB — posé par
 * CMakeLists.txt), on l'utilise telle quelle : elle fournit toutes
 * les utilitaires g_* (chaînes, listes, GString, g_utf8_*, g_spawn_*,
 * g_shell_*, GOption…) avec une sémantique exacte. Les shims Eina ci-dessous
 * ne servent que de repli minimal lorsque GLib est absente.
 *
 * Seule la couche WIDGET (GtkWidget, gtk_*, GTK_*, g_object_*, GdkEvent, GIO)
 * est shimmée vers EFL/Elementary, plus bas, en zone toujours-active.
 */
#if HAVE_GLIB
#  include <glib.h>
#else

typedef int8_t   gint8;
typedef uint8_t  guint8;
typedef int16_t  gint16;
typedef uint16_t guint16;
typedef int32_t  gint32;
typedef uint32_t guint32;
typedef int64_t  gint64;
typedef uint64_t guint64;
typedef int      gint;
typedef unsigned int   guint;
typedef char           gchar;
typedef unsigned char  guchar;
typedef int            gboolean;
typedef long           glong;
typedef unsigned long  gulong;
typedef void          *gpointer;
typedef const void    *gconstpointer;
typedef double         gdouble;
typedef float          gfloat;
typedef size_t         gsize;

#ifndef TRUE
#  define TRUE  EINA_TRUE
#endif
#ifndef FALSE
#  define FALSE EINA_FALSE
#endif

/* Vérifications statiques */
_Static_assert(sizeof(gint32) == 4,  "gint32 must be 4 bytes");
_Static_assert(sizeof(gint64) == 8,  "gint64 must be 8 bytes");

/* ─── Mémoire ─────────────────────────────────────────────────────────────── */
#define g_malloc(n)      malloc(n)
#define g_malloc0(n)     calloc(1, (n))
#define g_realloc(p, n)  realloc((p), (n))
#define g_free(p)        free(p)
#define g_new(t, n)      ((t *)malloc(sizeof(t) * (size_t)(n)))
#define g_new0(t, n)     ((t *)calloc((size_t)(n), sizeof(t)))

/* ─── Chaînes ─────────────────────────────────────────────────────────────── */
#define g_strdup(s)        strdup(s)
#define g_strndup(s, n)    strndup((s), (n))
#define g_strcmp0(a, b)    strcmp((a) ? (a) : "", (b) ? (b) : "")
#define g_strtod(s, e)      _sermo_strtod_current((s), (e))
#define g_ascii_strtod(s,e) _sermo_ascii_strtod((s), (e))

/* g_strlcat de repli (GLib absente) : concatène en bornant à la taille du
 * tampon destination, contrairement à strcat qui déborde en silence. */
static inline gsize g_strlcat(gchar *dest, const gchar *src, gsize dest_size) {
    gsize dl = strlen(dest), sl = strlen(src);
    if (dl >= dest_size) return dest_size + sl;
    gsize room = dest_size - dl - 1;
    gsize n = sl < room ? sl : room;
    memcpy(dest + dl, src, n);
    dest[dl + n] = '\0';
    return dl + sl;
}

/* snprintf_safe() est défini en zone toujours-active (plus bas) : il fournit
 * g_strdup_printf ICI (repli sans GLib) et est aussi appelé directement par
 * widget_table.c, y compris quand la vraie GLib offre déjà g_strdup_printf. */
#define g_strdup_printf(fmt, ...) snprintf_safe(fmt, ##__VA_ARGS__)

/* ─── Messages ────────────────────────────────────────────────────────────── */
#ifndef g_print
#define g_print(fmt, ...)     printf(fmt, ##__VA_ARGS__)
#endif
#ifndef g_printerr
#define g_printerr(fmt, ...)  fprintf(stderr, fmt, ##__VA_ARGS__)
#endif
#ifndef g_warning
#define g_warning(fmt, ...)   EINA_LOG_WARN(fmt, ##__VA_ARGS__)
#endif
#ifndef g_critical
#define g_critical(fmt, ...)  EINA_LOG_CRIT(fmt, ##__VA_ARGS__)
#endif
#ifndef g_message
#define g_message(fmt, ...)   EINA_LOG_INFO(fmt, ##__VA_ARGS__)
#endif
#ifndef g_error
#define g_error(fmt, ...)     do { EINA_LOG_CRIT(fmt, ##__VA_ARGS__); abort(); } while(0)
#endif

#ifdef DEBUG
#ifndef g_debug
#  define g_debug(fmt, ...)   EINA_LOG_DBG(fmt, ##__VA_ARGS__)
#endif
#ifndef g_info
#  define g_info(fmt, ...)    EINA_LOG_INFO(fmt, ##__VA_ARGS__)
#endif
#else
#ifndef g_debug
#  define g_debug(fmt, ...)   ((void)0)
#endif
#ifndef g_info
#  define g_info(fmt, ...)    ((void)0)
#endif
#endif

/* ─── GSList / GList → Eina_List ─────────────────────────────────────────── */
typedef Eina_List GSList;
typedef Eina_List GList;
#define g_slist_append(l, d)      eina_list_append((l), (d))
#define g_slist_prepend(l, d)     eina_list_prepend((l), (d))
#define g_slist_free(l)           eina_list_free(l)
#define g_slist_length(l)         eina_list_count(l)
#define g_list_append(l, d)       eina_list_append((l), (d))
#define g_list_prepend(l, d)      eina_list_prepend((l), (d))
#define g_list_free(l)            eina_list_free(l)
#define g_list_length(l)          eina_list_count(l)
#define g_list_next(l)            eina_list_next(l)
#define g_list_nth_data(l, n)     eina_list_nth((l), (n))

/* ─── GError minimal ──────────────────────────────────────────────────────── */
typedef struct { int code; char *message; } GError;
static inline void g_error_free(GError *e) { if (e) { free(e->message); free(e); } }

/* ─── Threads — no-op sous EFL (Ecore_Thread si besoin) ──────────────────── */
#define g_thread_init(v)   ((void)0)
#define gdk_threads_init() ((void)0)
#define gdk_threads_enter()((void)0)
#define gdk_threads_leave()((void)0)

/* ─── g_strfreev ─────────────────────────────────────────────────────────── */
static inline void g_strfreev(char **v) {
    if (!v) return;
    for (char **p = v; *p; p++) free(*p);
    free(v);
}

/* ─── g_strsplit ─────────────────────────────────────────────────────────── */
static inline char **g_strsplit(const char *s, const char *delim, int max) {
    if (!s || !delim) return NULL;
    size_t dlen = strlen(delim);
    int count = 0; const char *p = s;
    while ((p = strstr(p, delim)) && (max <= 0 || count < max - 1)) {
        count++; p += dlen;
    }
    char **res = (char **)malloc(sizeof(char *) * (size_t)(count + 2));
    if (!res) return NULL;
    int i = 0; p = s;
    while (i < count) {
        const char *end = strstr(p, delim);
        res[i++] = strndup(p, (size_t)(end - p));
        p = end + dlen;
    }
    res[i++] = strdup(p);
    res[i] = NULL;
    return res;
}

/* ─── g_strconcat ────────────────────────────────────────────────────────── */
static inline char * __attribute__((warn_unused_result, malloc))
g_strconcat(const char *s, ...) {
    va_list ap; size_t total = s ? strlen(s) : 0;
    va_start(ap, s);
    const char *t;
    while ((t = va_arg(ap, const char *))) total += strlen(t);
    va_end(ap);
    char *out = (char *)malloc(total + 1); if (!out) return NULL;
    out[0] = '\0';
    if (s) g_strlcat(out, s, total + 1);
    va_start(ap, s);
    while ((t = va_arg(ap, const char *))) g_strlcat(out, t, total + 1);
    va_end(ap);
    return out;
}

#endif /* HAVE_GLIB */

/* ─── Shims GTK → EFL (main loop) ───────────────────────────────────────── */
#define gtk_init(argc, argv)  elm_init(*(argc), *(argv))
#define gtk_main()            elm_run()
#define gtk_main_quit()       elm_exit(0)

/* Glade/GtkBuilder — non applicable EFL */
#define HAVE_GLADE_LIB 0

/* ─── EFL_WIN_GET — récupère la fenêtre parente de manière sécurisée ────── */
/*
 * Tous les widgets doivent appeler EFL_WIN_GET(parent) au lieu de
 * evas_object_data_get(NULL, "main_win").
 * La variable globale g_efl_main_win est initialisée par widget_window_create().
 */
extern Evas_Object *g_efl_main_win;

#define EFL_WIN_GET(var)  \
    Evas_Object *(var) = g_efl_main_win;

/* ─── FCLOSE_SAFE — fclose sur FILE* issu de safe_popen/widget_opencommand ─ */
/*
 * CRITIQUE : safe_popen() retourne un FILE* via fdopen(), pas via popen().
 * Il FAUT utiliser fclose(), jamais pclose() — pclose() sur un fdopen()
 * est un comportement indéfini (POSIX).
 */
#define EFL_PIPE_CLOSE(fp)  do { if (fp) { fclose(fp); (fp) = NULL; } } while(0)

/* ─── Macros EFL_TODO résiduelles ───────────────────────────────────────── */
/* GtkSocket/GtkPlug n'existe pas sous EFL */
#define EFL_NO_SOCKET 1

/* ═══════════════════════════════════════════════════════════════════════════
 *  snprintf_safe — toujours disponible (g_strdup_printf de repli + widget_table.c)
 *  Remplacement portable de g_strdup_printf sans VLA : deux passes (calcul de
 *  longueur puis allocation exacte). Chaîne allouée par malloc() — free()/g_free().
 * ═════════════════════════════════════════════════════════════════════════ */
static inline char * __attribute__((warn_unused_result, malloc))
snprintf_safe(const char *fmt, ...)
{
    va_list ap1, ap2;
    va_start(ap1, fmt);
    va_copy(ap2, ap1);
    int n = vsnprintf(NULL, 0, fmt, ap1);
    va_end(ap1);
    if (n < 0) { va_end(ap2); return strdup(""); }
    char *buf = (char *)malloc((size_t)n + 1);
    if (!buf) { va_end(ap2); return NULL; }
    vsnprintf(buf, (size_t)n + 1, fmt, ap2);
    va_end(ap2);
    return buf;
}

/* ═══════════════════════════════════════════════════════════════════════════
 *  COUCHE WIDGET GTK → EFL/Elementary
 *  ----------------------------------
 *  La GLib réelle (ci-dessus) fournit déjà toutes les utilitaires g_*. Il ne
 *  reste donc à shimmer que la couche widget, car les objets EFL sont des
 *  Evas_Object et NON des GObject/GtkWidget :
 *    1. types/objets widget GTK   → Evas_Object
 *    2. macros de cast GTK_*()      → identité
 *    3. prédicats GTK_IS_*()        → evas_object_type_get + strstr
 *    4. fonctions gtk_*()           → evas_/elm_ ou stubs (création réelle
 *                                     dans les widget_*.c, déjà portés)
 *    5. g_object_ et g_signal_      → données evas / no-op
 *    6. famille GdkEvent + gdk_*    → structs plats + stubs
 *    7. GFile/GFileMonitor (GIO)    → stubs (pas de GMainLoop sous EFL)
 *    8. introspection GObject       → code mort (paramspec toujours NULL),
 *                                     shimmé uniquement pour compiler
 * ═════════════════════════════════════════════════════════════════════════ */

/* ── 1. Types objets : tout widget GTK est un Evas_Object ─────────────────── */
typedef Evas_Object GtkWidget;
typedef Evas_Object GtkWindow;
typedef Evas_Object GtkContainer;
typedef Evas_Object GtkBin;
typedef Evas_Object GtkBox;
typedef Evas_Object GtkButton;
typedef Evas_Object GtkToggleButton;
typedef Evas_Object GtkCheckMenuItem;
typedef Evas_Object GtkEntry;
typedef Evas_Object GtkLabel;
typedef Evas_Object GtkMisc;
typedef Evas_Object GtkItem;
typedef Evas_Object GtkExpander;
typedef Evas_Object GtkProgress;
typedef Evas_Object GtkProgressBar;
typedef Evas_Object GtkRange;
typedef Evas_Object GtkScale;
typedef Evas_Object GtkSpinButton;
typedef Evas_Object GtkSpinner;
typedef Evas_Object GtkScrolledWindow;
typedef Evas_Object GtkNotebook;
typedef Evas_Object GtkMenuBar;
typedef Evas_Object GtkMenuItem;
typedef Evas_Object GtkList;
typedef Evas_Object GtkListBox;
typedef Evas_Object GtkListBoxRow;
typedef Evas_Object GtkCList;
typedef Evas_Object GtkCombo;
typedef Evas_Object GtkColorButton;
typedef Evas_Object GtkFontButton;
typedef Evas_Object GtkTreeView;
typedef Evas_Object GtkTreeViewColumn;
typedef Evas_Object GtkTreeModel;
typedef Evas_Object GtkTreePath;
typedef Evas_Object GtkSocket;
typedef Evas_Object GtkPlug;
typedef Evas_Object GtkFileChooser;
typedef Evas_Object GtkFileFilter;
typedef Evas_Object GtkFontChooserDialog;
typedef Evas_Object GtkBuilder;

/* Types « valeur » réellement déréférencés par le cœur */
typedef struct { gint   width, height; }            GtkRequisition;
typedef struct { gint16 left, right, top, bottom; } GtkBorder;
typedef struct { gint stamp; gpointer user_data, user_data2, user_data3; } GtkTreeIter;

typedef enum {
    GTK_FILE_CHOOSER_ACTION_OPEN,
    GTK_FILE_CHOOSER_ACTION_SAVE,
    GTK_FILE_CHOOSER_ACTION_SELECT_FOLDER,
    GTK_FILE_CHOOSER_ACTION_CREATE_FOLDER
} GtkFileChooserAction;

typedef enum {
    GTK_ENTRY_ICON_PRIMARY,
    GTK_ENTRY_ICON_SECONDARY
} GtkEntryIconPosition;

/* ── 2. Macros de cast : identité (tout est Evas_Object) ──────────────────── */
#define GTK_WIDGET(x)          ((GtkWidget *)(x))
#define GTK_WINDOW(x)          ((GtkWidget *)(x))
#define GTK_CONTAINER(x)       ((GtkWidget *)(x))
#define GTK_BIN(x)             ((GtkWidget *)(x))
#define GTK_BOX(x)             ((GtkWidget *)(x))
#define GTK_ENTRY(x)           ((GtkWidget *)(x))
#define GTK_LABEL(x)           ((GtkWidget *)(x))
#define GTK_BUTTON(x)          ((GtkWidget *)(x))
#define GTK_TOGGLE_BUTTON(x)   ((GtkWidget *)(x))
#define GTK_CHECK_MENU_ITEM(x) ((GtkWidget *)(x))
#define GTK_EXPANDER(x)        ((GtkWidget *)(x))
#define GTK_PROGRESS_BAR(x)    ((GtkWidget *)(x))
#define GTK_SPINNER(x)         ((GtkWidget *)(x))
#define GTK_SCALE(x)           ((GtkWidget *)(x))
#define GTK_RANGE(x)           ((GtkWidget *)(x))
#define GTK_SCROLLED_WINDOW(x) ((GtkWidget *)(x))
#define GTK_VIEWPORT(x)        ((GtkWidget *)(x))
#define GTK_TEXT_VIEW(x)       ((GtkWidget *)(x))
#define GTK_TREE_VIEW(x)       ((GtkWidget *)(x))
#define GTK_TREE_STORE(x)      ((GtkWidget *)(x))
#define GTK_TREE_MODEL(x)      ((GtkWidget *)(x))
#define GTK_CLIST(x)           ((GtkWidget *)(x))
#define GTK_LIST(x)            ((GtkWidget *)(x))
#define GTK_LIST_BOX(x)        ((GtkWidget *)(x))
#define GTK_NOTEBOOK(x)        ((GtkWidget *)(x))
#define GTK_DIALOG(x)          ((GtkWidget *)(x))
#define GTK_FILE_CHOOSER(x)    ((GtkWidget *)(x))
#define GTK_SOCKET(x)          ((GtkWidget *)(x))
#define GTK_OBJECT(x)          ((GtkWidget *)(x))

/* Constantes (valeurs indicatives — la plupart des cibles sont des stubs
 * variadiques qui les ignorent) */
#define GTK_WINDOW_TOPLEVEL    0
#define GTK_WINDOW_POPUP       1
#define GTK_TYPE_WINDOW        0
#define GTK_RESPONSE_OK        (-5)
#define GTK_RESPONSE_CANCEL    (-6)
#define GTK_POLICY_ALWAYS      0
#define GTK_POLICY_AUTOMATIC   1
#define GTK_POLICY_NEVER       2
#define GTK_SHADOW_NONE        0
#define GTK_SHADOW_IN          1
#define GTK_SHADOW_OUT         2
#define GTK_MESSAGE_INFO       0
#define GTK_MESSAGE_WARNING    1
#define GTK_MESSAGE_QUESTION   2
#define GTK_MESSAGE_ERROR      3
#define GTK_BUTTONS_NONE       0
#define GTK_BUTTONS_OK         1
#define GTK_BUTTONS_CLOSE      7
#define GTK_DIALOG_DESTROY_WITH_PARENT 1

/* ── 3. Prédicats GTK_IS_* via le type Evas ───────────────────────────────── */
static inline gboolean efl_obj_is(const Evas_Object *o, const char *needle)
{
    if (!o || !needle) return FALSE;
    const char *t = evas_object_type_get((Evas_Object *)o);
    return (t && strstr(t, needle)) ? TRUE : FALSE;
}
#define GTK_IS_WIDGET(o)          ((o) != NULL)
#define GTK_IS_TOGGLE_BUTTON(o)   efl_obj_is((o), "check")
#define GTK_IS_CHECK_MENU_ITEM(o) efl_obj_is((o), "check")
#define GTK_IS_RADIO_MENU_ITEM(o) efl_obj_is((o), "radio")
#define GTK_IS_BUTTON(o)          efl_obj_is((o), "button")
#define GTK_IS_ENTRY(o)           efl_obj_is((o), "entry")
#define GTK_IS_LABEL(o)           efl_obj_is((o), "label")
#define GTK_IS_PROGRESS_BAR(o)    efl_obj_is((o), "progressbar")
#define GTK_IS_SCALE(o)           efl_obj_is((o), "slider")
#define GTK_IS_SPIN_BUTTON(o)     efl_obj_is((o), "spinner")
#define GTK_IS_SPINNER(o)         efl_obj_is((o), "progressbar")
#define GTK_IS_COMBO_BOX(o)       (efl_obj_is((o), "combobox") || efl_obj_is((o), "hoversel"))
#define GTK_IS_LIST(o)            efl_obj_is((o), "list")
#define GTK_IS_LIST_BOX(o)        efl_obj_is((o), "list")
#define GTK_IS_TREE_VIEW(o)       efl_obj_is((o), "genlist")
#define GTK_IS_SCROLLED_WINDOW(o) efl_obj_is((o), "scroller")
#define GTK_IS_VIEWPORT(o)        efl_obj_is((o), "scroller")
#define GTK_IS_HBOX(o)            efl_obj_is((o), "box")
#define GTK_IS_VBOX(o)            efl_obj_is((o), "box")
#define GTK_IS_COLOR_BUTTON(o)    efl_obj_is((o), "button")
#define GTK_IS_FONT_BUTTON(o)     efl_obj_is((o), "button")
#define GTK_IS_EXPANDER(o)        FALSE
#define GTK_IS_MENU_ITEM(o)       FALSE

/* ── 4. Fonctions gtk_* : mappées EFL ou stubbées ─────────────────────────── */
/* Conteneur générique : box → pack_end ; sinon content_set */
static inline void efl_container_add(Evas_Object *c, Evas_Object *child)
{
    if (!c || !child) return;
    const char *t = evas_object_type_get(c);
    if (evas_object_data_get(c, "sw_rect")) {
        evas_object_size_hint_weight_set(child, EVAS_HINT_EXPAND, EVAS_HINT_EXPAND);
        evas_object_size_hint_align_set(child, EVAS_HINT_FILL, EVAS_HINT_FILL);
        /* fond demande par l'enfant (background="#rrggbb") : le rectangle
         * de gabarit, sous lui, le porte */
        const char *bg = (const char *)evas_object_data_get(child, "sermo_bg");
        unsigned v;
        if (bg && bg[0] == '#' && sscanf(bg + 1, "%6x", &v) == 1) {
            Evas_Object *rect = (Evas_Object *)evas_object_data_get(c, "sw_rect");
            evas_object_color_set(rect, (v >> 16) & 0xff, (v >> 8) & 0xff, v & 0xff, 255);
        }
        elm_table_pack(c, child, 0, 0, 1, 1);
        evas_object_show(child);
    }
    else if (t && strstr(t, "box")) elm_box_pack_end(c, child);
    else                            elm_object_content_set(c, child);
}

/* Enveloppe « scrolled window » : une elm_table portant un rectangle
 * invisible qui impose le gabarit (les widgets elm recalculent leur propre
 * min, souvent 0 pour une entry multiligne ou une genlist vide). Le stub
 * NULL d'origine faisait empiler NULL pour edit/list/tree/table/terminal :
 * jamais affiches. */
Evas_Object *efl_main_win_get(void);
static inline Evas_Object *efl_scrolled_new(void)
{
    Evas_Object *win = efl_main_win_get();
    Evas_Object *tb  = elm_table_add(win ? win : elm_win_add(NULL, "tmp", ELM_WIN_BASIC));
    Evas_Object *rect = evas_object_rectangle_add(evas_object_evas_get(tb));
    evas_object_color_set(rect, 0, 0, 0, 0);
    evas_object_size_hint_min_set(rect, 200, 100);
    evas_object_size_hint_weight_set(rect, EVAS_HINT_EXPAND, EVAS_HINT_EXPAND);
    evas_object_size_hint_align_set(rect, EVAS_HINT_FILL, EVAS_HINT_FILL);
    elm_table_pack(tb, rect, 0, 0, 1, 1);
    evas_object_show(rect);   /* invisible (alpha 0) sauf fond demande */
    evas_object_data_set(tb, "sw_rect", rect);
    evas_object_size_hint_weight_set(tb, EVAS_HINT_EXPAND, EVAS_HINT_EXPAND);
    evas_object_size_hint_align_set(tb, EVAS_HINT_FILL, EVAS_HINT_FILL);
    evas_object_show(tb);
    return tb;
}
static inline void efl_size_request(Evas_Object *w, int ww, int hh)
{
    if (!w) return;
    Evas_Object *rect = (Evas_Object *)evas_object_data_get(w, "sw_rect");
    evas_object_size_hint_min_set(rect ? rect : w, ww > 0 ? ww : 0, hh > 0 ? hh : 0);
}

/* Opérations courantes — sur widgets EFL réels */
#define gtk_widget_show(w)              evas_object_show((Evas_Object *)(w))
#define gtk_widget_show_all(w)          evas_object_show((Evas_Object *)(w))
#define gtk_widget_hide(w)              evas_object_hide((Evas_Object *)(w))
#define gtk_widget_destroy(w)           evas_object_del((Evas_Object *)(w))
#define gtk_widget_get_visible(w)       evas_object_visible_get((Evas_Object *)(w))
#define gtk_widget_set_size_request(w, ww, hh) \
        efl_size_request((Evas_Object *)(w), (ww), (hh))
#define gtk_widget_set_sensitive(w, b)  elm_object_disabled_set((Evas_Object *)(w), !(b))
#define gtk_widget_get_sensitive(w)     (!elm_object_disabled_get((Evas_Object *)(w)))
#define gtk_widget_grab_focus(w)        elm_object_focus_set((Evas_Object *)(w), EINA_TRUE)
#define gtk_widget_get_parent(w)        elm_object_parent_widget_get((Evas_Object *)(w))
#define gtk_widget_get_toplevel(w)      elm_object_top_widget_get((Evas_Object *)(w))
#define gtk_widget_get_ancestor(w, t)   elm_object_top_widget_get((Evas_Object *)(w))
#define gtk_bin_get_child(w)            ((GtkWidget *)(w))
#define gtk_container_add(c, w)         efl_container_add((Evas_Object *)(c), (Evas_Object *)(w))
#define gtk_window_move(w, x, y)        evas_object_move((Evas_Object *)(w), (x), (y))
#define gtk_window_present(w)           evas_object_show((Evas_Object *)(w))
#define gtk_window_set_transient_for(w, p)  ((void)0)
#define gtk_toggle_button_get_active(w)     elm_check_state_get((Evas_Object *)(w))
#define gtk_check_menu_item_get_active(w)   elm_check_state_get((Evas_Object *)(w))
#define gtk_entry_set_text(w, t)        elm_object_text_set((Evas_Object *)(w), (t))
#define gtk_progress_bar_pulse(w)       elm_progressbar_pulse((Evas_Object *)(w), EINA_TRUE)
#define gtk_label_set_xalign(w, x)      ((void)0)
#define gtk_expander_get_expanded(w)    (FALSE)
#define gtk_spinner_start(w)            ((void)0)
#define gtk_spinner_stop(w)             ((void)0)
/* Renvoie un gboolean : pas d'action « activate » par défaut sous EFL. */
#define gtk_widget_activate(w)          ((void)(w), FALSE)

/* Pas d'équivalent EFL direct → stubs */
#define gtk_window_new(...)             (NULL)
#define gtk_drawing_area_new(...)       (NULL)
#define gtk_label_new(...)              (NULL)
#define gtk_scrolled_window_new(...)    efl_scrolled_new()
#define gtk_scrolled_window_set_policy(...)        ((void)0)
#define gtk_scrolled_window_add_with_viewport(sw, w) efl_container_add((Evas_Object *)(sw), (Evas_Object *)(w))
#define gtk_viewport_set_shadow_type(...)          ((void)0)
#define gtk_container_get_children(...) (NULL)
#define gtk_widget_realize(...)         ((void)0)
#define gtk_widget_reparent(...)        ((void)0)
#define gtk_widget_add_events(...)      ((void)0)
#define gtk_widget_set_events(...)      ((void)0)
#define gtk_widget_get_events(...)      (0)
#define gtk_widget_set_extension_events(...)       ((void)0)
#define gtk_socket_new(...)             (NULL)
#define gtk_socket_get_id(...)          (0)
#define gtk_clist_get_text(...)         ((void)0)
#define gtk_tree_view_get_model(...)    (NULL)
#define gtk_tree_store_append(...)      ((void)0)
#define gtk_tree_store_set(...)         ((void)0)
#define gtk_text_view_get_buffer(...)   (NULL)
#define gtk_text_buffer_insert_at_cursor(...)      ((void)0)
#define gtk_list_item_new_with_label(...)          (NULL)
#define gtk_list_prepend_items(...)     ((void)0)
#define gtk_list_box_new(...)           (NULL)
#define gtk_list_box_row_new(...)       (NULL)
#define gtk_list_box_prepend(...)       ((void)0)
#define gtk_list_box_get_row_at_index(...)         (NULL)
#define gtk_list_box_select_row(...)    ((void)0)
#define gtk_dialog_run(...)             (GTK_RESPONSE_CANCEL)
#define gtk_message_dialog_new(...)     (NULL)
#define gtk_file_chooser_dialog_new(...)           (NULL)
#define gtk_file_chooser_widget_new(...)           (NULL)
#define gtk_file_chooser_add_filter(...)           ((void)0)
#define gtk_file_chooser_add_shortcut_folder(...)  (FALSE)
#define gtk_file_chooser_get_file(...)  (NULL)
#define gtk_file_chooser_get_filename(...)         (NULL)
#define gtk_file_chooser_set_current_folder(...)   ((void)0)
#define gtk_file_chooser_set_preview_widget_active(...) ((void)0)
#define gtk_file_filter_new(...)        (NULL)
#define gtk_file_filter_add_mime_type(...)         ((void)0)
#define gtk_file_filter_add_pattern(...)           ((void)0)
#define gtk_file_filter_set_name(...)   ((void)0)
#define gtk_get_option_group(...)       (NULL)

/* ── 5. GObject : un widget EFL n'est pas un GObject ──────────────────────── */
#define G_OBJECT(x)              ((Evas_Object *)(x))
#define G_OBJECT_GET_CLASS(x)    ((void *)0)
#define g_object_set_data(o, k, v)  evas_object_data_set((Evas_Object *)(o), (k), (const void *)(v))
#define g_object_get_data(o, k)     evas_object_data_get((Evas_Object *)(o), (k))
#define g_object_set(...)           ((void)0)
#define g_object_unref(x)           ((void)(x))

#ifndef G_CALLBACK
typedef void (*GCallback)(void);
#define G_CALLBACK(f)            ((GCallback)(f))
#endif
#define GTK_SIGNAL_FUNC(f)       G_CALLBACK(f)
#define g_signal_connect(...)         ((void)0)
#define g_signal_connect_after(...)   ((void)0)
#define g_signal_connect_swapped(...) ((void)0)
#define gtk_signal_connect(...)       ((void)0)

/* Introspection GObject : code mort sous EFL (paramspec toujours NULL),
 * shimmé pour que le switch de tag_attributes.c compile. */
typedef gsize GType;
typedef struct _GParamSpec { guint flags; GType value_type; } GParamSpec;
#define g_object_class_find_property(klass, name) ((GParamSpec *)0)
#define G_PARAM_WRITABLE           (1 << 1)
#define G_TYPE_FUNDAMENTAL_SHIFT   2
#define G_TYPE_MAKE_FUNDAMENTAL(x) ((GType)((x) << G_TYPE_FUNDAMENTAL_SHIFT))
#define G_TYPE_INVALID   G_TYPE_MAKE_FUNDAMENTAL(0)
#define G_TYPE_NONE      G_TYPE_MAKE_FUNDAMENTAL(1)
#define G_TYPE_INTERFACE G_TYPE_MAKE_FUNDAMENTAL(2)
#define G_TYPE_CHAR      G_TYPE_MAKE_FUNDAMENTAL(3)
#define G_TYPE_UCHAR     G_TYPE_MAKE_FUNDAMENTAL(4)
#define G_TYPE_BOOLEAN   G_TYPE_MAKE_FUNDAMENTAL(5)
#define G_TYPE_INT       G_TYPE_MAKE_FUNDAMENTAL(6)
#define G_TYPE_UINT      G_TYPE_MAKE_FUNDAMENTAL(7)
#define G_TYPE_LONG      G_TYPE_MAKE_FUNDAMENTAL(8)
#define G_TYPE_ULONG     G_TYPE_MAKE_FUNDAMENTAL(9)
#define G_TYPE_INT64     G_TYPE_MAKE_FUNDAMENTAL(10)
#define G_TYPE_UINT64    G_TYPE_MAKE_FUNDAMENTAL(11)
#define G_TYPE_ENUM      G_TYPE_MAKE_FUNDAMENTAL(12)
#define G_TYPE_FLAGS     G_TYPE_MAKE_FUNDAMENTAL(13)
#define G_TYPE_FLOAT     G_TYPE_MAKE_FUNDAMENTAL(14)
#define G_TYPE_DOUBLE    G_TYPE_MAKE_FUNDAMENTAL(15)
#define G_TYPE_STRING    G_TYPE_MAKE_FUNDAMENTAL(16)
#define G_TYPE_POINTER   G_TYPE_MAKE_FUNDAMENTAL(17)
#define G_TYPE_BOXED     G_TYPE_MAKE_FUNDAMENTAL(18)
#define G_TYPE_PARAM     G_TYPE_MAKE_FUNDAMENTAL(19)
#define G_TYPE_OBJECT    G_TYPE_MAKE_FUNDAMENTAL(20)

/* ── 6. Famille GdkEvent (structs plats) + gdk_* (stubs) ──────────────────── */
typedef int GdkEventType;
typedef int GdkInputCondition;
typedef struct _GdkEvent {
    GdkEventType type;
    guint        keyval;
    guint        state;
    guint16      hardware_keycode;
    guint        button;
    gdouble      x, y, x_root, y_root;
    gint         width, height;
} GdkEvent;
typedef GdkEvent GdkEventKey;
typedef GdkEvent GdkEventButton;
typedef GdkEvent GdkEventConfigure;
typedef GdkEvent GdkEventCrossing;
typedef GdkEvent GdkEventFocus;

#define GDK_EXPOSE                 2
#define GDK_CONFIGURE              13
#define GDK_INPUT_READ             1
#define GDK_ALL_EVENTS_MASK        0x3FFFFE
#define gdk_keyval_name(kv)        ("")
#define gdk_keyval_to_unicode(kv)  ((guint32)0)
#define gdk_input_add(fd, cond, cb, data)  (0)
#define gdk_input_remove(tag)      ((void)0)

/* ── 7. GFile / GFileMonitor (GIO) : stubs (pas de GMainLoop sous EFL) ────── */
typedef struct _GFile        GFile;
typedef struct _GFileMonitor GFileMonitor;
typedef int GFileMonitorEvent;
#define G_FILE_MONITOR_NONE                    0
#define G_FILE_MONITOR_EVENT_CHANGED           0
#define G_FILE_MONITOR_EVENT_CHANGES_DONE_HINT 1
#define G_FILE_MONITOR_EVENT_DELETED           2
#define G_FILE_MONITOR_EVENT_CREATED           3
#define G_FILE_MONITOR_EVENT_ATTRIBUTE_CHANGED 4
#define g_file_new_for_path(p)              ((GFile *)0)
#define g_file_get_path(f)                  (g_strdup(""))
#define g_file_monitor_file(f, fl, c, e)    ((GFileMonitor *)0)
#define g_file_monitor_cancel(m)            ((void)0)
#define g_file_monitor_set_rate_limit(m, r) ((void)0)

#endif /* EFL_COMPAT_H */
