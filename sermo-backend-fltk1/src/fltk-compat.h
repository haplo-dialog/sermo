/* SPDX-License-Identifier: GPL-2.0-or-later */
/*
 * Copyright (C) 2026 S. Cage
 * haplo-dialog <devel@haplo-dialog.fr>
 */
/* fltk-compat.h — Couche de compatibilité GLib → stdlib/FLTK
 * Force-inclus dans chaque unité de compilation via AM_CFLAGS.
 *
 * haplo-dialog / fltk1dialog 1.0.0
 * Contact : devel@haplo-dialog.fr
 *
 * Ce fichier sert deux objectifs :
 *  1. Remplacer les appels GLib du core (variables.c, actions.c, etc.)
 *     par des équivalents stdlib C standard quand GLib n'est pas disponible.
 *  2. Déclarer les includes FLTK communs à tous les widget_*.cpp.
 */

#ifndef FLTK_COMPAT_H
#define FLTK_COMPAT_H

#ifdef __cplusplus
/* === Includes FLTK de base (C++ uniquement) === */
#include <FL/Fl.H>
#include <FL/Fl_Window.H>
#include <FL/Fl_Double_Window.H>
#include <FL/Fl_Box.H>
#include <FL/Fl_Button.H>
#include <FL/Fl_Light_Button.H>
#include <FL/Fl_Check_Button.H>
#include <FL/Fl_Round_Button.H>
#include <FL/Fl_Toggle_Button.H>
#include <FL/Fl_Group.H>
#include <FL/Fl_Pack.H>
#include <FL/Fl_Flex.H>
#include <FL/Fl_Scroll.H>
#include <FL/Fl_Tabs.H>
#include <FL/Fl_Widget.H>
#include <FL/Fl_Input.H>
#include <FL/Fl_Multiline_Input.H>
#include <FL/Fl_Multiline_Output.H>
#include <FL/Fl_Output.H>
#include <FL/Fl_Value_Slider.H>
#include <FL/Fl_Spinner.H>
#include <FL/Fl_Choice.H>
#include <FL/Fl_Browser.H>
#include <FL/Fl_Progress.H>
#include <FL/Fl_Color_Chooser.H>
#include <FL/Fl_Shared_Image.H>
#include <FL/fl_ask.H>
#include <FL/fl_draw.H>

#if HAVE_FL_TERMINAL
#include <FL/Fl_Terminal.H>
#endif

/* === Typedef GtkWidget → Fl_Widget (pont core C ↔ widgets C++) === */
typedef Fl_Widget GtkWidget;

/* === Stubs types GDK référencés dans signals.h ===
 * signals.h déclare des prototypes utilisant GdkEvent*, GdkEventButton*, etc.
 * Ces types ne sont pas définis par FLTK ; on les stub ici comme structures
 * opaques, avant que gtkdialog.h → signals.h soit inclus. */
struct _GdkEventButton    {};  typedef struct _GdkEventButton    GdkEventButton;
struct _GdkEventConfigure {};  typedef struct _GdkEventConfigure GdkEventConfigure;
struct _GdkEventCrossing  {};  typedef struct _GdkEventCrossing  GdkEventCrossing;
struct _GdkEventFocus     {};  typedef struct _GdkEventFocus     GdkEventFocus;
struct _GdkEvent          {};  typedef struct _GdkEvent          GdkEvent;
struct _GdkEventKey       {};  typedef struct _GdkEventKey       GdkEventKey;
struct _GdkEventScroll    {};  typedef struct _GdkEventScroll    GdkEventScroll;
struct _GdkEventMotion    {};  typedef struct _GdkEventMotion    GdkEventMotion;
struct _GdkScreen         {};  typedef struct _GdkScreen         GdkScreen;
struct _GdkDisplay        {};  typedef struct _GdkDisplay        GdkDisplay;

/* Types GTK supplémentaires référencés dans signals.h */
typedef int GtkEntryIconPosition;
struct _GtkTreePath       {};  typedef struct _GtkTreePath       GtkTreePath;
struct _GtkTreeViewColumn {};  typedef struct _GtkTreeViewColumn GtkTreeViewColumn;
typedef int GdkInputCondition;
typedef int GFileMonitorEvent;
struct _GFileMonitor      {};  typedef struct _GFileMonitor      GFileMonitor;
struct _GFile             {};  typedef struct _GFile             GFile;

#endif /* __cplusplus */

/* ------------------------------------------------------------------ */
/* Typedefs GTK → void* pour les fichiers C du core                   */
/* (En C++, GtkWidget est typedef'd vers Fl_Widget dans le bloc C++)  */
/* Ces typedefs sont actifs SEULEMENT en C (pas en C++ où              */
/* Fl_Widget est une vraie classe).                                    */
/* ------------------------------------------------------------------ */
#ifndef __cplusplus
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
typedef struct { int stamp; void *user_data; void *user_data2; void *user_data3; } GtkTreeIter;
typedef void GtkTreeViewColumn;
typedef void GdkColor;
typedef void GdkRGBA;
typedef void GdkScreen;
typedef void GdkPixbuf;
typedef void GdkWindow;
typedef int  GtkWindowPosition;
typedef int  GtkOrientation;
typedef int  GtkPackType;
typedef int  GtkShadowType;
typedef int  GtkPolicyType;
typedef int  GtkSortType;
typedef int  GtkWrapMode;
typedef int  GtkJustification;
typedef int  GtkResponseType;
/* Constantes GTK fréquentes */
#define GTK_WIN_POS_CENTER        1
#define GTK_WIN_POS_NONE          0
#define GTK_ORIENTATION_HORIZONTAL 0
#define GTK_ORIENTATION_VERTICAL   1
#define GTK_PACK_START            0
#define GTK_PACK_END              1
#define GTK_SHADOW_NONE           0
#define GTK_POLICY_AUTOMATIC      1
#define GTK_JUSTIFY_LEFT          0
#define GTK_JUSTIFY_CENTER        2

/* Types GDK avec les champs réellement utilisés (signals.c, automaton.c) */
typedef int GdkEventType;
#define GDK_NOTHING     (-1)
#define GDK_CONFIGURE   13
#define GDK_EXPOSE      14
#define GDK_ALL_EVENTS_MASK 0x3FFFFE

typedef struct { GdkEventType type; double x, y; unsigned int state; unsigned int button; double x_root, y_root; } GdkEventButton;
typedef struct { GdkEventType type; int x, y, width, height; } GdkEventConfigure;
typedef struct { GdkEventType type; } GdkEventCrossing;
typedef struct { GdkEventType type; } GdkEventFocus;
typedef struct { GdkEventType type; unsigned int keyval; unsigned int state; unsigned int hardware_keycode; } GdkEventKey;
typedef struct { GdkEventType type; double x, y; unsigned int state; } GdkEventScroll;
typedef struct { GdkEventType type; double x, y; unsigned int state; } GdkEventMotion;
typedef struct { GdkEventType type; } GdkEvent;
typedef void GdkDisplay;
typedef int  GdkInputCondition;
typedef int  GFileMonitorEvent;
typedef void GFileMonitor;
typedef void GFile;
typedef int  GtkEntryIconPosition;
typedef void GtkTreePath;

/* GObject minimal — stubs pour tag_attributes.c */
typedef unsigned long GType;

/* GParamSpec — structure minimale pour tag_attributes.c */
typedef struct _GParamSpec {
    GType  value_type;
    int    flags;
} GParamSpec;
typedef void GObjectClass;

/* GType constants — valeurs arbitraires (GLib non disponible) */
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

/* GParam flags */
#define G_PARAM_READABLE   1
#define G_PARAM_WRITABLE   2
#define G_PARAM_READWRITE  3

/* GObject stubs — no-op sous FLTK */
#define G_OBJECT_GET_CLASS(o)              NULL
#define g_object_class_find_property(c,n)  NULL
#define G_OBJECT_CLASS(c)                  ((GObjectClass*)(c))
#define G_OBJECT(w)                        ((void*)(w))
#define g_object_set(obj, ...)             /* no-op */
#define g_object_set_data(o,k,v)           /* no-op */
#define g_object_get_data(o,k)            NULL
#define g_object_unref(o)                  /* no-op */
#define G_IS_OBJECT(o)                     ((o) != NULL)

/* GTK_CONTAINER — macro-ifiée pour les fichiers C */
#define GTK_CONTAINER(w)                   ((void*)(w))
#define gtk_container_remove(c,w)          /* no-op */

/* strncasecmp / strdup — exposés via _GNU_SOURCE sur Linux */
#ifndef _GNU_SOURCE
#define _GNU_SOURCE
#endif
#include <string.h>

#endif /* !__cplusplus */

/* === Shims GLib → stdlib (actifs si GLib absent) === */
#if !HAVE_GLIB

#include <stdlib.h>
#include <string.h>
#include <stdio.h>
#include <locale.h>   /* newlocale, strtod_l, uselocale, LC_ALL_MASK */

/* Mémoire */
#define g_malloc(n)          malloc(n)
#define g_malloc0(n)         calloc(1, n)
#define g_realloc(p, n)      realloc((p), (n))
#define g_free(p)            free(p)
#define g_new(t, n)          ((t*)malloc(sizeof(t) * (n)))
#define g_new0(t, n)         ((t*)calloc((n), sizeof(t)))
#define g_renew(t, p, n)     ((t*)realloc((p), sizeof(t) * (n)))

/* Chaînes */
static inline __attribute__((warn_unused_result, malloc))
char *_fltk_strdup(const char *s) { return s ? strdup(s) : NULL; }
#define g_strdup(s) _fltk_strdup(s)
#define g_strndup(s, n)      strndup((s), (n))
/* g_ascii_strtod : toujours en locale C (le point décimal, jamais la virgule
 * de la locale courante ne doit décider du sens d'un nombre lu dans un XML). */
static inline double _sermo_ascii_strtod(const char *s, char **end) {
    static locale_t loc_c = (locale_t)0;
    if (loc_c == (locale_t)0) loc_c = newlocale(LC_ALL_MASK, "C", (locale_t)0);
    return strtod_l(s, end, loc_c ? loc_c : uselocale((locale_t)0));
}
/* g_strtod : sémantique GLib — accepte le séparateur de la locale courante,
 * sinon retombe sur l'interprétation en locale C (celle qui consomme le plus). */
static inline double _sermo_strtod_current(const char *s, char **end) {
    char *e1 = NULL, *e2 = NULL;
    double v1 = strtod_l(s, &e1, uselocale((locale_t)0)); /* locale courante */
    double v2 = _sermo_ascii_strtod(s, &e2);              /* locale C */
    if (e2 > e1) { if (end) *end = e2; return v2; }
    if (end) *end = e1; return v1;
}
#define g_strtod(s, e)       _sermo_strtod_current((s), (e))
#define g_ascii_strtod(s, e) _sermo_ascii_strtod((s), (e))
#define g_ascii_strtoull(s, e, b) strtoull((s), (e), (b))
#define g_strcmp0(a, b)      strcmp((a) ? (a) : "", (b) ? (b) : "")
#define g_str_has_prefix(s, p) (strncmp((s), (p), strlen(p)) == 0)
#define g_str_has_suffix(s, x) \
    (strlen(s) >= strlen(x) && strcmp((s) + strlen(s) - strlen(x), (x)) == 0)

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

/* Messages */
#ifndef g_print
#define g_print(fmt, ...)    printf(fmt, ##__VA_ARGS__)
#endif
#ifndef g_printerr
#define g_printerr(fmt, ...) fprintf(stderr, fmt, ##__VA_ARGS__)
#endif
#ifndef g_warning
#define g_warning(fmt, ...)  fprintf(stderr, "WARNING: " fmt "\n", ##__VA_ARGS__)
#endif
#ifndef g_critical
#define g_critical(fmt, ...) fprintf(stderr, "CRITICAL: " fmt "\n", ##__VA_ARGS__)
#endif
#ifndef g_message
#define g_message(fmt, ...)  fprintf(stderr, fmt "\n", ##__VA_ARGS__)
#endif

/* Types de base */
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
#include <stdint.h>
typedef int32_t       gint32;
typedef uint32_t      guint32;
typedef int64_t       gint64;
typedef uint64_t      guint64;
_Static_assert(sizeof(gint32) == 4, "gint32 must be 4 bytes");
_Static_assert(sizeof(gint64) == 8, "gint64 must be 8 bytes");
#define TRUE  1
#define FALSE 0
#define NULL  ((void*)0)

/* GSList minimal — implémentation stub dans fltk-compat.cpp */
typedef struct _GSList { void *data; struct _GSList *next; } GSList;
GSList* g_slist_append(GSList *list, void *data);
GSList* g_slist_prepend(GSList *list, void *data);
void    g_slist_free(GSList *list);
guint   g_slist_length(GSList *list);

/* GList minimal */
typedef struct _GList { void *data; struct _GList *next; struct _GList *prev; } GList;
GList*  g_list_append(GList *list, void *data);
void    g_list_free(GList *list);
guint   g_list_length(GList *list);

/* Environnement */
#define g_setenv(k, v, ow)   setenv((k), (v), (ow))
#define g_getenv(k)          getenv(k)
#define g_unsetenv(k)        unsetenv(k)

/* GList navigation */
#define g_list_next(l)       ((l) ? ((GList*)(l))->next : NULL)
#define g_list_previous(l)   ((l) ? ((GList*)(l))->prev : NULL)
#define g_list_last(l)       ({ GList *_l = (l); while (_l && _l->next) _l = _l->next; _l; })
#define g_list_prepend(l, d) ({ GList *_n = (GList*)malloc(sizeof(GList)); \
    _n->data = (d); _n->next = (l); _n->prev = NULL; \
    if (l) ((GList*)(l))->prev = _n; _n; })
#define g_list_foreach(l, f, d) \
    do { GList *_e = (l); while (_e) { (f)(_e->data, (d)); _e = _e->next; } } while(0)
#define g_list_remove(l, d) \
    ({ GList *_l = (l), *_e = _l; \
       while (_e && _e->data != (d)) _e = _e->next; \
       if (_e) { if (_e->prev) _e->prev->next = _e->next; \
                 else _l = _e->next; \
                 if (_e->next) _e->next->prev = _e->prev; \
                 free(_e); } _l; })
#define g_list_nth_data(l, n) \
    ({ GList *_e = (l); guint _i = 0; \
       while (_e && _i < (n)) { _e = _e->next; _i++; } \
       _e ? _e->data : NULL; })
#define g_list_copy(l)       ({ GList *_r = NULL; GList *_e = (l); \
    while (_e) { _r = g_list_append(_r, _e->data); _e = _e->next; } _r; })
#define g_list_find(l, d)    \
    ({ GList *_e = (l); while (_e && _e->data != (d)) _e = _e->next; _e; })
#define g_list_delete_link(l, lnk) \
    ({ GList *_l = (l), *_e = (GList*)(lnk); \
       if (_e->prev) _e->prev->next = _e->next; else _l = _e->next; \
       if (_e->next) _e->next->prev = _e->prev; \
       free(_e); _l; })
#define g_list_length(l)     ({ guint _n = 0; GList *_e = (l); \
    while (_e) { _n++; _e = _e->next; } _n; })

/* GSList navigation */
#define g_slist_next(l)      ((l) ? ((GSList*)(l))->next : NULL)
#define g_slist_last(l)      ({ GSList *_l = (l); while (_l && _l->next) _l = _l->next; _l; })
#define g_slist_nth_data(l,n) \
    ({ GSList *_e=(l); guint _i=0; while(_e&&_i<(n)){_e=_e->next;_i++;} _e?_e->data:NULL; })
#define g_slist_foreach(l,f,d) \
    do { GSList *_e=(l); while(_e){(f)(_e->data,(d));_e=_e->next;} } while(0)
#define g_slist_remove(l,d) \
    ({ GSList *_l=(l),*_p=NULL,*_e=_l; \
       while(_e&&_e->data!=(d)){_p=_e;_e=_e->next;} \
       if(_e){if(_p)_p->next=_e->next;else _l=_e->next;free(_e);} _l; })

/* g_spawn stubs — safe_exec.c nécessite GLib si HAVE_GLIB=1.
 * Sans GLib, on stub avec popen/system. */
#define G_SPAWN_SEARCH_PATH        1
#define G_SPAWN_STDOUT_TO_DEV_NULL 4
#define G_SPAWN_STDERR_TO_DEV_NULL 8
#define GSpawnFlags int
typedef int GPid;
#define g_spawn_close_pid(pid)     /* no-op */
/* g_spawn_sync et g_spawn_async_with_pipes sont trop complexes pour être
 * macro-ifiés ; safe_exec.c nécessite GLib — forcer HAVE_GLIB=1 dans configure. */

/* Divers GLib */
#define g_strsplit(s, d, n)        NULL    /* stub — non utilisé dans les chemins portés */
#define g_strfreev(v)              /* no-op */
#define g_strjoinv(sep, v)         g_strdup("")
#define g_path_get_dirname(p)      g_strdup(".")
#define g_path_get_basename(p)     g_strdup(p)
#define g_build_filename(...)      g_strdup("")
#define g_file_test(f, t)          0
#define GFileTest int

/* Threads — no-op (single-threaded FLTK event loop) */
#define g_thread_init(vtable)   /* no-op */
#define gdk_threads_init()      /* no-op */
#define gdk_threads_enter()     /* no-op */
#define gdk_threads_leave()     /* no-op */

#else
/* GLib disponible — inclure normalement */
#include <glib.h>
#endif /* !HAVE_GLIB */

/* === Shims GTK → FLTK communs (toujours actifs) === */

/* gtk_init remplacé par Fl — appelé dans main.cpp */
#define gtk_init(argc, argv)    /* no-op — Fl s'initialise à Fl::run() */
#define gtk_main()              fltk_run()
#define gtk_main_quit()         /* géré par la fenêtre principale */
/* Pas d'équivalent FLTK aux groupes d'options GTK */
#define gtk_get_option_group(x) (NULL)

/* Wrappers C des appels FLTK C++ (implémentés dans fltk-compat.cpp) */
#ifdef __cplusplus
extern "C" {
#endif
void fltk_run(void);
void fltk_visual_init(void);
#ifdef __cplusplus
}
#endif

/* Stub glade — non applicable sous FLTK */
#define HAVE_GLADE_LIB 0

/* ------------------------------------------------------------------ */
/* Shims widget GTK → FLTK                                            */
/* Ces macros permettent aux fichiers core (variables.c, stack.c,     */
/* automaton.c, signals.c, actions.c...) de compiler sans             */
/* modification, en mappant les appels GTK vers des équivalents FLTK. */
/* ------------------------------------------------------------------ */

#ifdef __cplusplus

/* Visibilité */
#define gtk_widget_show(w)          ((Fl_Widget*)(w))->show()
#define gtk_widget_hide(w)          ((Fl_Widget*)(w))->hide()
#define gtk_widget_show_all(w)      ((Fl_Widget*)(w))->show()
#define gtk_widget_set_visible(w,v) ((v) ? ((Fl_Widget*)(w))->show() : ((Fl_Widget*)(w))->hide())

/* Sensibilité */
#define gtk_widget_set_sensitive(w,s) \
    do { if (s) ((Fl_Widget*)(w))->activate(); \
         else   ((Fl_Widget*)(w))->deactivate(); } while(0)

/* Rendu */
#define gtk_widget_queue_draw(w)    ((Fl_Widget*)(w))->redraw()

/* Taille */
#define gtk_widget_set_size_request(w,ww,hh) \
    ((Fl_Widget*)(w))->size((ww) > 0 ? (ww) : ((Fl_Widget*)(w))->w(), \
                            (hh) > 0 ? (hh) : ((Fl_Widget*)(w))->h())

/* Conteneurs — no-op ou équivalent FLTK */
#define gtk_container_add(c, w)    \
    do { Fl_Group *_g = (Fl_Group*)(c); \
         if (_g) { _g->add((Fl_Widget*)(w)); } } while(0)

/* Vérification de type — toujours vraie en FLTK (pas de GObject) */
#define GTK_IS_WIDGET(w)           ((w) != NULL)

/* Casts — transparents sous FLTK (GtkWidget = Fl_Widget*) */
#define GTK_WIDGET(w)              ((Fl_Widget*)(w))
#define GTK_WINDOW(w)              ((Fl_Window*)(w))

/* GtkBin — dans FLTK un Fl_Group avec 1 enfant ; on accède au child(0) */
#define GTK_BIN(w)                 ((Fl_Group*)(w))
#define gtk_bin_get_child(b)       (((Fl_Group*)(b))->children() > 0 ? \
                                    ((Fl_Group*)(b))->child(0) : NULL)

/* Scrolled window — pas de wrapper natif simple ; placeholder no-op */
#define GTK_SCROLLED_WINDOW(w)     ((Fl_Scroll*)(w))
#define gtk_scrolled_window_add_with_viewport(s, w) \
    ((Fl_Group*)(s))->add((Fl_Widget*)(w))

/* Socket/X11 embed — non supporté sous FLTK, no-op silencieux */
#define GTK_SOCKET(w)              (w)
#define gtk_socket_new()           NULL
#define gtk_socket_get_id(s)       0

/* Signaux GTK — ignorés (FLTK utilise des callbacks directs) */
#define g_signal_connect(obj, sig, cb, data)       /* no-op */
#define g_signal_connect_after(obj, sig, cb, data) /* no-op */
#define g_signal_handler_block(obj, id)            /* no-op */
#define g_signal_handler_unblock(obj, id)          /* no-op */
#define g_signal_emit_by_name(obj, sig, ...)       /* no-op */

/* g_object */
#define g_object_set_data(obj, key, val)           /* no-op */
#define g_object_get_data(obj, key)                NULL
#define g_object_unref(obj)                        /* no-op */
#undef G_OBJECT
#define G_OBJECT(w)                                (w)

/* Asserts GTK — ne redéfinir que si GLib absent (GLib les définit déjà) */
#if !HAVE_GLIB
#define g_assert(expr)   do { if (!(expr)) { \
    fprintf(stderr, "ASSERT FAILED: %s:%d: %s\n", __FILE__, __LINE__, #expr); \
    abort(); } } while(0)
#define g_return_if_fail(expr)       do { if (!(expr)) return; } while(0)
#define g_return_val_if_fail(expr,v) do { if (!(expr)) return (v); } while(0)
#endif /* !HAVE_GLIB */

/* gtk_widget_get/set_name — on stocke le nom dans le label() de FLTK
 * (uniquement comme repère interne, pas affiché) */
#define gtk_widget_get_name(w)      ((Fl_Widget*)(w))->label()
#define gtk_widget_set_name(w, n)   /* no-op — nom géré par variable->Name */

/* Geometry / position — no-op (FLTK gère sa propre géométrie) */
#define gtk_window_move(w, x, y)    ((Fl_Window*)(w))->position((x),(y))
#define gtk_window_resize(w, ww, hh) ((Fl_Window*)(w))->size((ww),(hh))
#define gtk_window_set_title(w, t)  ((Fl_Window*)(w))->copy_label(t)
#define gtk_window_set_position(w, pos) /* no-op — centrage géré dans widget_window_create */

/* Incluion gtk/gtk.h interceptée : on définit la garde pour qu'elle soit no-op */
#define __GTK_H__
#define __GTKX_H__

#else /* C files (non-C++) */

/* En C, on n'a pas accès aux méthodes FLTK.
 * On définit des fonctions inline déclarées dans fltk-compat.cpp */
typedef void Fl_Widget_C;

/* Les macros C appellent des wrappers C déclarés plus bas */
#define gtk_widget_show(w)          fltk_widget_show((Fl_Widget_C*)(w))
#define gtk_widget_hide(w)          fltk_widget_hide((Fl_Widget_C*)(w))
#define gtk_widget_show_all(w)      fltk_widget_show((Fl_Widget_C*)(w))
#define gtk_widget_set_sensitive(w,s) fltk_widget_set_sensitive((Fl_Widget_C*)(w),(s))
#define gtk_widget_queue_draw(w)    fltk_widget_redraw((Fl_Widget_C*)(w))
#define gtk_container_add(c,w)      fltk_group_add((Fl_Widget_C*)(c),(Fl_Widget_C*)(w))
#define GTK_IS_WIDGET(w)            ((w) != NULL)
#define GTK_WIDGET(w)               ((void*)(w))
#define GTK_WINDOW(w)               ((void*)(w))
#define GTK_BIN(w)                  ((void*)(w))
#define gtk_bin_get_child(b)        fltk_group_child0((Fl_Widget_C*)(b))
#define GTK_SCROLLED_WINDOW(w)      ((void*)(w))
#define gtk_scrolled_window_add_with_viewport(s,w) fltk_group_add((Fl_Widget_C*)(s),(Fl_Widget_C*)(w))
#define GTK_SOCKET(w)               (w)
#define gtk_socket_new()            NULL
#define gtk_socket_get_id(s)        0
#define g_signal_connect(o,s,c,d)           /* no-op */
#define g_signal_connect_after(o,s,c,d)     /* no-op */
#define g_signal_handler_block(o,i)         /* no-op */
#define g_signal_handler_unblock(o,i)       /* no-op */
#define g_signal_emit_by_name(o,s,...)      /* no-op */
#define g_object_set_data(o,k,v)            /* no-op */
#define g_object_get_data(o,k)              NULL
#define g_object_unref(o)                   /* no-op */
#undef G_OBJECT
#define G_OBJECT(w)                         (w)
/* g_assert / g_return_* : ne redéfinir que si GLib absent */
#if !HAVE_GLIB
#define g_assert(expr)  do { if (!(expr)) { \
    fprintf(stderr, "ASSERT FAILED: %s:%d: %s\n", __FILE__, __LINE__, #expr); \
    abort(); } } while(0)
#define g_return_if_fail(expr)       do { if (!(expr)) return; } while(0)
#define g_return_val_if_fail(expr,v) do { if (!(expr)) return (v); } while(0)
#endif
#define gtk_widget_get_name(w)      ""
#define gtk_widget_set_name(w,n)    /* no-op */
#define gtk_window_move(w,x,y)      fltk_window_move((Fl_Widget_C*)(w),(x),(y))
#define gtk_window_resize(w,ww,hh)  /* no-op */
#define gtk_window_set_title(w,t)   /* no-op */
#define gtk_window_set_position(w,p) /* no-op */
#define __GTK_H__
#define __GTKX_H__

/* Déclarations des wrappers C (implémentés dans fltk-compat.cpp) */
#ifdef __cplusplus
extern "C" {
#endif
void  fltk_widget_show(Fl_Widget_C *w);
void  fltk_widget_hide(Fl_Widget_C *w);
void  fltk_widget_redraw(Fl_Widget_C *w);
void  fltk_widget_set_sensitive(Fl_Widget_C *w, int sensitive);
void  fltk_group_add(Fl_Widget_C *group, Fl_Widget_C *child);
void *fltk_group_child0(Fl_Widget_C *group);
void  fltk_window_move(Fl_Widget_C *w, int x, int y);
#ifdef __cplusplus
}
#endif

#endif /* __cplusplus */

/* ------------------------------------------------------------------ */
/* Shims GTK widget-spécifiques — no-op pour widgets non portés       */
/* Actifs en C et C++ (communs aux deux).                             */
/* ------------------------------------------------------------------ */

/* gtk_widget_get_parent — retourne NULL (pas de GObject parent chain en FLTK) */
#define gtk_widget_get_parent(w)            NULL

/* Progress bar GTK — non portée via ces appels (widget_progressbar.cpp gère) */
#define GTK_PROGRESS_BAR(w)                 (w)
#define gtk_progress_bar_pulse(w)           /* no-op */
#define gtk_progress_bar_set_fraction(w,f)  /* no-op */
#define gtk_progress_bar_set_text(w,t)      /* no-op */

/* Spinner GTK (différent de Fl_Spinner qui est un spinbutton) */
#define GTK_SPINNER(w)                      (w)
#define gtk_spinner_start(w)                /* no-op */
#define gtk_spinner_stop(w)                 /* no-op */

/* Expander GTK */
#define GTK_EXPANDER(w)                     (w)
#define gtk_expander_get_expanded(w)        0
#define gtk_expander_set_expanded(w,v)      /* no-op */

/* GtkLabel */
#define GTK_LABEL(w)                        (w)
#define gtk_label_new(t)                    NULL
#define gtk_label_set_text(w,t)             /* no-op */
#define gtk_label_get_text(w)               ""
#define gtk_label_set_xalign(w,a)           /* no-op */

/* GtkListBox (GTK3 – remplacé par Fl_Browser dans FLTK) */
#define GTK_LIST_BOX(w)                     (w)
#define gtk_list_box_row_new()              NULL
#define gtk_list_box_prepend(lb, row)       /* no-op */
#define gtk_list_box_select_row(lb, row)    /* no-op */
#define gtk_list_box_get_row_at_index(lb,i) NULL

/* GtkTextView / GtkTextBuffer */
#define GTK_TEXT_VIEW(w)                    (w)
#define GTK_TEXT_BUFFER(w)                  (w)
#define gtk_text_view_get_buffer(w)         NULL
#define gtk_text_buffer_insert_at_cursor(b,t,l) /* no-op */
#define gtk_text_buffer_set_text(b,t,l)     /* no-op */
#define gtk_text_buffer_get_text(b,s,e,i)   g_strdup("")

/* GtkTreeView / GtkTreeStore / GtkTreeModel */
#define GTK_TREE_VIEW(w)                    (w)
#define GTK_TREE_STORE(w)                   (w)
#define GTK_LIST_STORE(w)                   (w)
#define GTK_TREE_MODEL(w)                   (w)
#define gtk_tree_view_get_model(w)          NULL
#define gtk_tree_store_append(m,i,p)        /* no-op */
#define gtk_tree_store_set(m,i,...)         /* no-op */
#define gtk_list_store_append(m,i)          /* no-op */
#define gtk_list_store_set(m,i,...)         /* no-op */

/* GtkFileChooser */
typedef int  GtkFileChooserAction;
typedef void GtkFileFilter;
#define GTK_FILE_CHOOSER(w)                         (w)
#define GTK_FILE_CHOOSER_ACTION_OPEN                0
#define GTK_FILE_CHOOSER_ACTION_SAVE                1
#define GTK_FILE_CHOOSER_ACTION_SELECT_FOLDER       2
#define GTK_FILE_CHOOSER_ACTION_CREATE_FOLDER       3
/* Pas de hiérarchie de widgets / fenêtres GTK sous FLTK */
#define GTK_TYPE_WINDOW                             0
#define gtk_widget_get_ancestor(w, t)               (w)
#define gtk_window_present(w)                       /* no-op */
#define gtk_file_chooser_widget_new(a)              NULL
#define gtk_file_chooser_dialog_new(t,p,a,...)      NULL
#define gtk_file_chooser_set_current_folder(w,p)    /* no-op */
#define gtk_file_chooser_get_filename(w)            g_strdup("")
#define gtk_file_chooser_get_file(w)                NULL
#define gtk_file_chooser_add_filter(w,f)            /* no-op */
#define gtk_file_chooser_add_shortcut_folder(w,p,e) /* no-op */
#define gtk_file_filter_new()                       NULL
#define gtk_file_filter_set_name(f,n)               /* no-op */
#define gtk_file_filter_add_pattern(f,p)            /* no-op */
#define gtk_file_filter_add_mime_type(f,m)          /* no-op */

/* === Shims de complétude GTK → FLTK (ports core) ================== */
/* Casts de type : sous FLTK tous les widgets sont des Fl_Widget* */
#define GTK_CHECK_BUTTON(w)      (w)
#define GTK_CHECK_MENU_ITEM(w)   (w)
#define GTK_COLOR_BUTTON(w)      (w)
#define GTK_COMBO_BOX(w)         (w)
#define GTK_ENTRY(w)             (w)
#define GTK_EXPANDER(w)          (w)
#define GTK_FONT_BUTTON(w)       (w)
#define GTK_LABEL(w)             (w)
#define GTK_LIST_BOX(w)          (w)
#define GTK_PROGRESS_BAR(w)      (w)
#define GTK_RADIO_BUTTON(w)      (w)
#undef GTK_SCROLLED_WINDOW
#define GTK_SCROLLED_WINDOW(w)   (w)
#define GTK_SPINNER(w)           (w)
#define GTK_TOGGLE_BUTTON(w)     (w)
#define GTK_TREE_VIEW(w)         (w)
#define GTK_VIEWPORT(w)          (w)

/* Tests de type : pas de RTTI GTK sous FLTK → faux (dispatch par Type) */
#define GTK_IS_BUTTON(w)           ((void)(w),0)
#define GTK_IS_CHECK_MENU_ITEM(w)  ((void)(w),0)
#define GTK_IS_COLOR_BUTTON(w)     ((void)(w),0)
#define GTK_IS_COMBO_BOX(w)        ((void)(w),0)
#define GTK_IS_ENTRY(w)            ((void)(w),0)
#define GTK_IS_EXPANDER(w)         ((void)(w),0)
#define GTK_IS_FONT_BUTTON(w)      ((void)(w),0)
#define GTK_IS_LABEL(w)            ((void)(w),0)
#define GTK_IS_LIST_BOX(w)         ((void)(w),0)
#define GTK_IS_MENU_ITEM(w)        ((void)(w),0)
#define GTK_IS_PROGRESS_BAR(w)     ((void)(w),0)
#define GTK_IS_RADIO_MENU_ITEM(w)  ((void)(w),0)
#define GTK_IS_SCALE(w)            ((void)(w),0)
#define GTK_IS_SCROLLED_WINDOW(w)  ((void)(w),0)
#define GTK_IS_SPIN_BUTTON(w)      ((void)(w),0)
#define GTK_IS_SPINNER(w)          ((void)(w),0)
#define GTK_IS_TOGGLE_BUTTON(w)    ((void)(w),0)
#define GTK_IS_TREE_VIEW(w)        ((void)(w),0)
#define GTK_IS_VIEWPORT(w)         ((void)(w),0)

/* Fonctions / accesseurs widget sans équivalent direct */
#define gtk_check_menu_item_get_active(w)  ((void)(w),0)
#define gtk_drawing_area_new()             (NULL)
#define gtk_viewport_set_shadow_type(w,s)  /* no-op */
#define gtk_widget_activate(w)             ((void)(w),0)
#undef gtk_widget_get_name
#define gtk_widget_get_name(w)             ("")
#undef gtk_widget_get_parent
#define gtk_widget_get_parent(w)           (NULL)
#define gtk_widget_get_sensitive(w)        (1)
#define gtk_widget_get_visible(w)          (1)
#define gtk_widget_grab_focus(w)           /* no-op */
#define gtk_widget_realize(w)              /* no-op */
#define gtk_widget_set_events(w,e)         /* no-op */
#define gtk_widget_set_name(w,n)           /* no-op */
#undef gtk_widget_set_size_request
#define gtk_widget_set_size_request(w,a,b) /* no-op */
#ifndef gtk_widget_hide
#define gtk_widget_hide(w)                 fltk_widget_hide((Fl_Widget_C*)(w))
#endif
#ifndef gtk_widget_set_sensitive
#define gtk_widget_set_sensitive(w,s)      fltk_widget_set_sensitive((Fl_Widget_C*)(w),(s))
#endif

/* GtkEntry icônes */
#define GTK_ENTRY_ICON_PRIMARY    0
#define GTK_ENTRY_ICON_SECONDARY  1

/* Requisition (taille demandée) */
typedef struct { int width; int height; } GtkRequisition;

/* GDK clavier */
#define gdk_keyval_name(k)         ("")
#define gdk_keyval_to_unicode(k)   ((unsigned int)(k))

/* GIO file monitor — non porté sous FLTK */
typedef int GFileMonitorEventType;
#define G_FILE_MONITOR_NONE              0
#define G_FILE_MONITOR_EVENT_CHANGED     1
#define g_file_new_for_path(p)           (NULL)
#define g_file_get_path(f)               (g_strdup(""))
#define g_file_monitor_file(f,fl,c,e)    (NULL)
#define g_file_monitor_cancel(m)         ((void)(m),0)
#define g_file_monitor_set_rate_limit(m,r) /* no-op */

/* GFile (GIO) */
#undef g_file_get_path
#define g_file_get_path(f)                  g_strdup("")

/* GtkComboBox */
#define GTK_COMBO_BOX(w)                    (w)
#define GTK_COMBO_BOX_TEXT(w)               (w)
#define gtk_combo_box_get_active_text(w)    g_strdup("")
#define gtk_combo_box_get_active(w)         0
#define gtk_combo_box_set_active(w,i)       /* no-op */
#define gtk_combo_box_text_get_active_text(w) g_strdup("")

/* GtkNotebook */
#define GTK_NOTEBOOK(w)                     (w)
#define gtk_notebook_get_current_page(w)    0
#define gtk_notebook_set_current_page(w,p)  /* no-op */

/* GtkScrolledWindow — wrappé via Fl_Scroll (wrapper C dans fltk-compat.cpp) */
#define gtk_scrolled_window_new(h,v)          fltk_scroll_new(200, 150)
#define gtk_scrolled_window_set_policy(w,h,v) /* no-op */

/* Déclaration du constructeur C (implémenté dans fltk-compat.cpp) */
#ifdef __cplusplus
extern "C" {
#endif
void *fltk_scroll_new(int w, int h);
#ifdef __cplusplus
}
#endif

/* GtkBox */
#define GTK_BOX(w)                          (w)
#define gtk_box_pack_start(b,w,e,f,p)       /* no-op */
#define gtk_box_pack_end(b,w,e,f,p)         /* no-op */
#define gtk_box_set_spacing(b,s)            /* no-op */

/* GtkDialog */
#define GTK_DIALOG(w)                       (w)
#define gtk_dialog_run(w)                   0
#define gtk_dialog_get_content_area(w)      NULL
#define GTK_RESPONSE_OK                     (-5)
#define GTK_RESPONSE_CANCEL                 (-6)
#define GTK_RESPONSE_YES                    (-8)
#define GTK_RESPONSE_NO                     (-9)
#define GTK_RESPONSE_CLOSE                  (-7)
#define gtk_widget_destroy(w)               /* no-op */

/* GtkImage */
#define GTK_IMAGE(w)                        (w)
#define gtk_image_new_from_file(f)          NULL
#define gtk_image_set_from_file(w,f)        /* no-op */
#define gtk_image_new_from_icon_name(n,s)   NULL

/* GtkColorButton / GtkColorChooser */
#define GTK_COLOR_BUTTON(w)                 (w)
#define gtk_color_button_get_rgba(w,c)      /* no-op */
#define gtk_color_button_set_rgba(w,c)      /* no-op */

/* GtkFontButton */
#define GTK_FONT_BUTTON(w)                  (w)
#define gtk_font_button_get_font_name(w)    ""
#define gtk_font_button_set_font_name(w,f)  /* no-op */

/* GdkColor / GdkRGBA */
#define gdk_rgba_parse(c,s)                 FALSE
#define gdk_rgba_to_string(c)               g_strdup("#000000")

/* GtkStatusbar */
#define GTK_STATUSBAR(w)                    (w)
#define gtk_statusbar_push(w,i,t)           0
#define gtk_statusbar_pop(w,i)              /* no-op */

/* GtkRange (hscale / vscale) */
#define GTK_RANGE(w)                        (w)
#define gtk_range_get_value(w)              0.0
#define gtk_range_set_value(w,v)            /* no-op */

/* GtkEntry */
#define GTK_ENTRY(w)                        (w)
#define gtk_entry_get_text(w)               ""
#define gtk_entry_set_text(w,t)             /* no-op */

/* GtkToggleButton / CheckButton / RadioButton */
#define GTK_TOGGLE_BUTTON(w)                (w)
#define GTK_CHECK_BUTTON(w)                 (w)
#define GTK_RADIO_BUTTON(w)                 (w)
#define gtk_toggle_button_get_active(w)     0
#define gtk_toggle_button_set_active(w,v)   /* no-op */

/* GtkCellRenderer */
#define gtk_cell_renderer_text_new()        NULL
#define gtk_tree_view_column_new_with_attributes(t,r,...) NULL
#define gtk_tree_view_append_column(w,c)    0
#define gtk_tree_view_column_set_sort_column_id(c,i) /* no-op */

/* GtkSocket / XEmbed */
#define GTK_TYPE_SOCKET                     0
#define gtk_socket_add_id(s,id)             /* no-op */

/* GtkWindow extras */
#define GTK_WINDOW_TOPLEVEL                 0
#define gtk_window_new(t)                   NULL
#define gtk_window_set_transient_for(w,p)   /* no-op */
#define gtk_window_set_default_size(w,ww,hh) /* no-op */
#define gtk_window_get_size(w,ww,hh)        /* no-op */

/* GtkAccelGroup */
#define gtk_accel_group_new()               NULL
#define gtk_window_add_accel_group(w,ag)    /* no-op */

/* GtkBuilder (Glade) */
#define gtk_builder_new()                   NULL
#define gtk_builder_add_from_file(b,f,e)    0
#define gtk_builder_get_object(b,n)         NULL
#define gtk_builder_connect_signals(b,d)    /* no-op */

/* g_debug / g_info — conditionnels -DDEBUG */
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

/* g_error — fatal, on utilise abort() */
#ifndef g_error
#define g_error(fmt, ...) \
    do { fprintf(stderr, "ERROR: " fmt "\n", ##__VA_ARGS__); abort(); } while(0)
#endif

#endif /* FLTK_COMPAT_H */
