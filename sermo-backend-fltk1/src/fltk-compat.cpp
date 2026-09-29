/* SPDX-License-Identifier: GPL-2.0-or-later */
/*
 * Copyright (C) 2026 S. Cage
 * haplo-dialog <devel@haplo-dialog.fr>
 */
/* fltk-compat.cpp — Implémentations runtime de la couche de compatibilité
 *
 * haplo-dialog / fltk1dialog 1.0.0
 * Contact : devel@haplo-dialog.fr
 */

#include "fltk-compat.h"
#include <FL/Fl_Shared_Image.H>
#include <FL/Fl_Image_Surface.H>
#include <FL/Fl_PNG_Image.H>   /* fl_write_png */
#include <FL/Fl_RGB_Image.H>
#include <stdlib.h>
#include <string>
#include <ctype.h>
#include <stdio.h>
#include <string.h>

#if !HAVE_GLIB

/* ─── GSList minimal ──────────────────────────────────────────────────────── */

GSList* g_slist_append(GSList *list, void *data) {
    GSList *node = (GSList*)malloc(sizeof(GSList));
    node->data = data;
    node->next = NULL;
    if (!list) return node;
    GSList *last = list;
    while (last->next) last = last->next;
    last->next = node;
    return list;
}

GSList* g_slist_prepend(GSList *list, void *data) {
    GSList *node = (GSList*)malloc(sizeof(GSList));
    node->data = data;
    node->next = list;
    return node;
}

void g_slist_free(GSList *list) {
    while (list) {
        GSList *next = list->next;
        free(list);
        list = next;
    }
}

guint g_slist_length(GSList *list) {
    guint n = 0;
    while (list) { n++; list = list->next; }
    return n;
}

/* ─── GList minimal ───────────────────────────────────────────────────────── */

GList* g_list_append(GList *list, void *data) {
    GList *node = (GList*)malloc(sizeof(GList));
    node->data = data;
    node->next = NULL;
    node->prev = NULL;
    if (!list) return node;
    GList *last = list;
    while (last->next) last = last->next;
    last->next = node;
    node->prev = last;
    return list;
}

void g_list_free(GList *list) {
    while (list) {
        GList *next = list->next;
        free(list);
        list = next;
    }
}

guint g_list_length(GList *list) {
    guint n = 0;
    while (list) { n++; list = list->next; }
    return n;
}

#endif /* !HAVE_GLIB */

/* ─── Wrappers C pour les fichiers core (.c) ─────────────────────────────── *
 *                                                                             *
 * Les fichiers core (variables.c, stack.c, automaton.c, signals.c, …) sont  *
 * compilés en C et ne peuvent pas appeler directement les méthodes C++ de    *
 * Fl_Widget. fltk-compat.h définit des macros qui appellent ces wrappers.    *
 * ─────────────────────────────────────────────────────────────────────────── */

extern "C" {

void fltk_widget_show(void *w)
{
    if (w) ((Fl_Widget *)w)->show();
}

void fltk_widget_hide(void *w)
{
    if (w) ((Fl_Widget *)w)->hide();
}

void fltk_widget_redraw(void *w)
{
    if (w) ((Fl_Widget *)w)->redraw();
}

void fltk_widget_set_sensitive(void *w, int sensitive)
{
    if (!w) return;
    if (sensitive) ((Fl_Widget *)w)->activate();
    else           ((Fl_Widget *)w)->deactivate();
}

void fltk_group_add(void *group, void *child)
{
    if (!group || !child) return;
    Fl_Group *g = (Fl_Group *)group;
    Fl_Widget *c = (Fl_Widget *)child;
    g->add(c);
}

void *fltk_group_child0(void *group)
{
    if (!group) return NULL;
    Fl_Group *g = (Fl_Group *)group;
    if (g->children() == 0) return NULL;
    return (void *)g->child(0);
}

void fltk_window_move(void *w, int x, int y)
{
    if (w) ((Fl_Window *)w)->position(x, y);
}

/* ─── Constructeurs de widgets FLTK appelables depuis C ──────────────── */

/* Enveloppe « scrolled window » : son enfant unique (zone de texte, liste,
 * arbre — qui defilent deja par eux-memes) prend TOUTE sa surface. Fl_Scroll
 * nu laissait l'enfant a sa taille de creation (400 px) au milieu du cadre,
 * avec une deuxieme paire de barres. */
class SermoScroll : public Fl_Scroll {
public:
    SermoScroll(int X, int Y, int W, int H) : Fl_Scroll(X, Y, W, H) { type(0); }
    void resize(int X, int Y, int W, int H) override {
        Fl_Widget::resize(X, Y, W, H);
        for (int i = 0; i < children(); i++) {
            Fl_Widget *c = child(i);
            if (c == &scrollbar || c == &hscrollbar) continue;
            c->resize(X, Y, W, H);
        }
    }
};

/* Crée l'enveloppe (équivalent GtkScrolledWindow) */
void *fltk_scroll_new(int w, int h)
{
    Fl_Scroll *sc = new SermoScroll(0, 0, w, h);
    sc->end();
    return (void *)sc;
}

/* Boucle d'événements FLTK (équivalent gtk_main) */
void fltk_run(void)
{
    Fl::run();
}

/* ─── Rendu offscreen déterministe en PNG (--render-png FILE) ─────────────── *
 *                                                                             *
 * Dessine la fenêtre top-level dans un Fl_Image_Surface (offscreen), en tire  *
 * un Fl_RGB_Image et l'écrit en PNG via fl_write_png. C'est le backend qui    *
 * produit l'image lui-même (pas de capture « import » externe fragile) — le   *
 * pendant FLTK de QWidget::grab() du port qt6.                                *
 *                                                                             *
 * Prérequis : la fenêtre a déjà été montée (widget_show_all() de l'automate   *
 * l'a fait avant gtk_main → sermo_be_run_loop) donc Fl::first_window() la retourne. *
 * FLTK/X11 exige une connexion d'affichage pour l'offscreen : headless total  *
 * sans DISPLAY impossible, mais « xvfb-run -a » suffit (aucun screenshot).    *
 *                                                                             *
 * Retour : 1 si un PNG non vide a été écrit, 0 sinon.                          */
int fltk_render_png(const char *file)
{
    if (!file || !*file) return 0;

    /* La fenêtre racine : première fenêtre montrée par l'automate. */
    Fl_Window *win = Fl::first_window();
    if (!win) return 0;

    /* S'assurer qu'elle est réalisée et dimensionnée, puis laisser FLTK
     * traiter les événements de mappage/dessin en attente. */
    win->show();
    for (int i = 0; i < 8; i++) { Fl::check(); Fl::flush(); }

    int w = win->w(), h = win->h();
    if (w <= 0) w = 1;
    if (h <= 0) h = 1;

    Fl_Image_Surface *surf = new Fl_Image_Surface(w, h);
    Fl_Surface_Device::push_current(surf);

    /* Fond opaque (la fenêtre peut ne pas peindre chaque pixel). */
    fl_color(FL_BACKGROUND_COLOR);
    fl_rectf(0, 0, w, h);

    /* Dessiner la fenêtre et toute sa descendance dans l'offscreen. */
    surf->draw(win);

    Fl_RGB_Image *img = surf->image();
    Fl_Surface_Device::pop_current();

    int ok = 0;
    if (img) {
        ok = (fl_write_png(file, img) == 0);
        delete img;
    }
    delete surf;
    return ok;
}

/* Le bureau est-il en theme sombre ? Lu sans sous-processus : GTK_THEME,
 * puis ~/.config/gtk-3.0/settings.ini, puis les xsettings XFCE. */
static bool fltk_desktop_is_dark(void)
{
    const char *e = getenv("SERMO_DARK");
    if (e) return *e == '1';
    auto has_dark = [](const char *txt) {
        if (!txt) return false;
        std::string t(txt);
        for (auto &c : t) c = (char)tolower((unsigned char)c);
        return t.find("dark") != std::string::npos;
    };
    if (has_dark(getenv("GTK_THEME"))) return true;
    const char *home = getenv("HOME");
    if (!home) return false;
    const char *rel[] = { "/.config/gtk-3.0/settings.ini",
                          "/.config/xfce4/xfconf/xfce-perchannel-xml/xsettings.xml" };
    for (const char *r : rel) {
        std::string path = std::string(home) + r;
        FILE *f = fopen(path.c_str(), "r");
        if (!f) continue;
        char line[512];
        bool dark = false;
        while (fgets(line, sizeof(line), f)) {
            if (strstr(line, "gtk-theme-name") || strstr(line, "ThemeName")) {
                if (has_dark(line)) { dark = true; break; }
            }
        }
        fclose(f);
        if (dark) return true;
    }
    return false;
}

/* Initialisation du visuel FLTK (double buffer) + habillage systeme :
 * sans cela le port gardait le look FLTK d'usine (gris 1990, relief)
 * quel que soit le theme du bureau. FLTK_SCHEME reste prioritaire. */
void fltk_visual_init(void)
{
    Fl::visual(FL_DOUBLE | FL_INDEX);
    fl_register_images();   /* png/svg/xpm pour les icones de theme */
    /* sans DISPLAY (--print-ir, bancs) : get_system_colors() ouvrirait
     * l'affichage et avorterait (« Can't open display ») */
    const char *disp = getenv("DISPLAY");
    if (!disp || !*disp) return;
    Fl::get_system_colors();
    if (!getenv("FLTK_SCHEME")) Fl::scheme("gtk+");
    if (fltk_desktop_is_dark()) {
        Fl::background(0x2e, 0x30, 0x36);    /* fond des widgets */
        Fl::background2(0x24, 0x26, 0x2b);   /* fond des zones de saisie */
        Fl::foreground(0xe6, 0xe6, 0xe6);    /* texte */
        Fl::set_color(FL_SELECTION_COLOR, 0x5e, 0x81, 0xac);
    }
}

} /* extern "C" */
