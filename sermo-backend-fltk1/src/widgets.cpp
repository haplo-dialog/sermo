// SPDX-License-Identifier: GPL-2.0-or-later
/*
 * widgets.cpp — dispatch des widgets FLTK (implémentation indépendante)
 *
 * Copyright (C) 2026 S. Cage
 *
 * Façade haplo-dialog <devel@haplo-dialog.fr>
 */

#include "fltk-compat.h"
#include "gtkdialog.h"
#include "safe_exec.h"
#include "widgets.h"
#include "stringman.h"
#include "tag_attributes.h"
#include "macros.h"
#include "signals.h"

#include "widget_button.h"
#include "widget_checkbox.h"
#include "widget_colorbutton.h"
#include "widget_combobox.h"
#include "widget_edit.h"
#include "widget_entry.h"
#include "widget_expander.h"
#include "widget_frame.h"
#include "widget_hbox.h"
#include "widget_hscale.h"
#include "widget_hseparator.h"
#include "widget_list.h"
#include "widget_notebook.h"
#include "widget_pixmap.h"
#include "widget_progressbar.h"
#include "widget_radiobutton.h"
#include "widget_spinbutton.h"
#include "widget_statusbar.h"
#include "widget_text.h"
#include "widget_timer.h"
#include "widget_togglebutton.h"
#include "widget_vbox.h"
#include "widget_vscale.h"
#include "widget_vseparator.h"
#include "widget_window.h"
#include "widget_menubar.h"
#include "widget_table.h"
#include "widget_comboboxtext.h"
#include "widget_fontbutton.h"
#include "widget_menuitem.h"
#include "widget_terminal.h"
#include "widget_tree.h"
#include "widget_treetable.h"
#include "widget_switch.h"
#include "widget_password.h"
#include "widget_searchentry.h"
#include "widget_calendar.h"
#include "widget_spinner.h"
#include "widget_eventbox.h"
#include "widget_filechooser.h"
#include "widget_grid.h"
#include "widget_paned.h"
#include "widget_toolbar.h"
#include "widget_stackpages.h"
#include "widget_wizard.h"
#include "widget_menubutton.h"
#include "widget_flowbox.h"
#include "widget_overlay.h"
#include "widget_revealer.h"
#include "widget_linkbutton.h"
#include "widget_infobar.h"
#include "widget_image.h"
#include "widget_pulse.h"
#include "widget_aspectframe.h"
#include "widget_menu.h"
#include "widget_levelbar.h"
#include "widget_drawingarea.h"
#include "sermo_input.h"

#include <set>
#include <FL/Fl_Input_Choice.H>
#include <cstring>
#include <cstdio>
#include <strings.h>

/* ------------------------------------------------------------------ */
/* Définitions globales exigées par ce fichier.                        */
/* ------------------------------------------------------------------ */

extern gchar *option_include_file;

GList *widget_show_list = NULL;
GList *widget_hide_list = NULL;

/* Seule l'adresse de ce marqueur compte ; sa valeur n'a pas de sens
 * particulier au-delà de son initialisation à 0. */
int SERMO_FLEX_TAG = 0;

/* ------------------------------------------------------------------ */
/* Fonction 1 — ouverture d'une commande shell via safe_popen.          */
/* ------------------------------------------------------------------ */

FILE *widget_opencommand(const char *command)
{
    FILE *stream = NULL;

    if (option_include_file != NULL) {
        gchar *full_command =
            g_strdup_printf(". %s; %s", option_include_file, command);

        PIP_DEBUG("widget_opencommand: %s", full_command);
        stream = safe_popen(full_command);
        g_free(full_command);
    } else {
        PIP_DEBUG("widget_opencommand: %s", command);
        stream = safe_popen(command);
    }

    if (stream == NULL) {
        g_warning("widget_opencommand: could not open command '%s'", command);
        return NULL;
    }

    return stream;
}

/* ------------------------------------------------------------------ */
/* Fonction 2 — résolution des directives <input>.                     */
/* ------------------------------------------------------------------ */

gchar *widget_input_text(AttributeSet *Attr)
{
    if (Attr == NULL)
        return NULL;

    GList *element = NULL;
    gchar *value = NULL;

    for (value = (gchar *)attributeset_get_first(&element, Attr, ATTR_INPUT);
         value != NULL;
         value = (gchar *)attributeset_get_next(&element, Attr, ATTR_INPUT)) {

        if (strncasecmp(value, "Command:", 8) == 0 && value[8] != '\0') {
            const char *cmd = value + 8;
            FILE *stream = widget_opencommand(cmd);

            if (stream == NULL)
                continue;

            GString *s = g_string_new(NULL);
            char buf[1024];

            while (fgets(buf, sizeof(buf), stream) != NULL)
                g_string_append(s, buf);

            fclose(stream);   /* safe_popen() rend un fdopen() : fclose, jamais pclose */

            return g_string_free(s, FALSE);
        }

        if (strncasecmp(value, "file:", 5) == 0 && value[5] != '\0') {
            /* 2.7.3 : lu par sermo_fopen_input(), plafonné comme les autres
             * ports. g_file_get_contents() lisait /dev/zero sans fin. */
            FILE *fp = sermo_fopen_input(value + 5);

            if (fp == NULL)
                continue;

            GString *s = g_string_new(NULL);
            char buf[1024];

            while (fgets(buf, sizeof(buf), fp) != NULL)
                g_string_append(s, buf);

            fclose(fp);

            return g_string_free(s, FALSE);
        }

        /* Valeur non reconnue : on passe à la suivante. */
    }

    return NULL;
}

/* ------------------------------------------------------------------ */
/* Fonction 2 bis — <input> découpé en lignes (widgets à éléments).     */
/* ------------------------------------------------------------------ */

gchar *widget_input_line(AttributeSet *Attr)
{
    /* Règle de l'étalon gtk3sermo pour les widgets à valeur unique : seule la
     * PREMIÈRE ligne compte, coupée au premier CR ou LF (l'étalon les efface en
     * place). Une ligne vide vaut "" ; rien de lisible (pas une ligne) : NULL. */
    gchar *texte = widget_input_text(Attr);

    if (texte == NULL)
        return NULL;
    if (texte[0] == '\0') {
        g_free(texte);
        return NULL;
    }
    texte[strcspn(texte, "\r\n")] = '\0';
    return texte;
}

gint widget_input_bool(AttributeSet *Attr)
{
    /* Règle de l'étalon gtk3sermo pour <checkbox>, <radiobutton>, <switch> et
     * <togglebutton> : la PREMIÈRE ligne, sans CR/LF, vaut vrai si elle dit
     * « true » ou « yes » (sans casse) ou si atoi() y lit 1 ; toute autre
     * ligne vaut faux. Rien de lisible (ni ligne) : -1, l'état ne change pas. */
    gchar *texte = widget_input_text(Attr);
    gint   vrai;

    if (texte == NULL)
        return -1;
    if (texte[0] == '\0') {
        g_free(texte);
        return -1;
    }
    texte[strcspn(texte, "\r\n")] = '\0';
    vrai = (g_ascii_strcasecmp(texte, "true") == 0 ||
            g_ascii_strcasecmp(texte, "yes") == 0 || atoi(texte) == 1);
    g_free(texte);
    return vrai ? 1 : 0;
}

gchar **widget_input_lines(AttributeSet *Attr)
{
    gchar *text = widget_input_text(Attr);

    if (text == NULL)
        return NULL;

    /* Découpage calqué sur la boucle fgets() de l'étalon gtk3sermo : un « \n »
     * final ne crée pas de ligne vide de plus, et chaque ligne s'arrête à son
     * premier CR (l'étalon remplace chaque CR/LF par un NUL). Un vecteur vide
     * (directive lue, sortie vide) reste distinct de NULL (aucune directive
     * lisible) : tree ne se vide que dans le premier cas. */
    GPtrArray *lines = g_ptr_array_new();
    const gchar *p = text;

    while (*p != '\0') {
        const gchar *lf = strchr(p, '\n');
        gsize len = lf ? (gsize)(lf - p) : strlen(p);
        const gchar *cr = (const gchar *)memchr(p, '\r', len);

        if (cr != NULL)
            len = (gsize)(cr - p);
        g_ptr_array_add(lines, g_strndup(p, len));
        if (lf == NULL)
            break;
        p = lf + 1;
    }
    g_ptr_array_add(lines, NULL);
    g_free(text);

    return (gchar **)g_ptr_array_free(lines, FALSE);
}

/* ------------------------------------------------------------------ */
/* Fonction 3 — extraction de la valeur texte d'un widget par type.     */
/* ------------------------------------------------------------------ */

char *widget_get_text_value(GtkWidget *widget, int type)
{
    if (widget == NULL) {
        g_warning("widget_get_text_value: NULL widget");
        return g_strdup("NULL WIDGET");
    }

    switch (type) {
    case WIDGET_CANCELBUTTON:
    case WIDGET_HELPBUTTON:
    case WIDGET_NOBUTTON:
    case WIDGET_OKBUTTON:
    case WIDGET_YESBUTTON:
    case WIDGET_BUTTON:
        return widget_button_envvar_construct(widget);

    case WIDGET_TOGGLEBUTTON:
        /* Fonction dédiée : un togglebutton exporte son état true/false,
         * pas son label. Ne pas regrouper avec les boutons. */
        return widget_togglebutton_envvar_construct(widget);

    case WIDGET_CHECKBOX:
        return widget_checkbox_envvar_construct(widget);

    case WIDGET_COLORBUTTON:
        return widget_colorbutton_envvar_construct(widget);

    case WIDGET_COMBOBOX:
    case WIDGET_COMBOBOXENTRY:
    case WIDGET_COMBOBOXTEXT:
        return widget_combobox_envvar_construct(widget);

    case WIDGET_EDIT:
        return widget_edit_envvar_construct(widget);

    case WIDGET_ENTRY:
        return widget_entry_envvar_construct(widget);

    case WIDGET_EXPANDER:
        return widget_expander_envvar_construct(widget);

    case WIDGET_FRAME:
        return widget_frame_envvar_construct(widget);

    case WIDGET_HBOX:
        return widget_hbox_envvar_construct(widget);

    case WIDGET_HSCALE:
        return widget_hscale_envvar_construct(widget);

    case WIDGET_VSCALE:
        return widget_vscale_envvar_construct(widget);

    case WIDGET_HSEPARATOR:
        return widget_hseparator_envvar_construct(widget);

    case WIDGET_VSEPARATOR:
        return widget_vseparator_envvar_construct(widget);

    case WIDGET_LIST:
        return widget_list_envvar_construct(widget);

    case WIDGET_NOTEBOOK:
        return widget_notebook_envvar_construct(widget);

    case WIDGET_PIXMAP:
        return widget_pixmap_envvar_construct(widget);

    case WIDGET_PROGRESSBAR:
        return widget_progressbar_envvar_construct(widget);

    case WIDGET_RADIOBUTTON:
        return widget_radiobutton_envvar_construct(widget);

    case WIDGET_SPINBUTTON:
        return widget_spinbutton_envvar_construct(widget);

    case WIDGET_STATUSBAR:
        return widget_statusbar_envvar_construct(widget);

    case WIDGET_TEXT:
        return widget_text_envvar_construct(widget);

    case WIDGET_TIMER:
        return widget_timer_envvar_construct(widget);

    case WIDGET_VBOX:
        return widget_vbox_envvar_construct(widget);

    case WIDGET_WINDOW:
        return widget_window_envvar_construct(widget);

    case WIDGET_EVENTBOX:
        return widget_eventbox_envvar_construct(widget);

    case WIDGET_SWITCH:
        return widget_switch_envvar_construct(widget);

    case WIDGET_FILECHOOSER:
        return widget_filechooser_envvar_construct(widget);

    case WIDGET_CALENDAR:
        return widget_calendar_envvar_construct(widget);

    case WIDGET_GRID:
        return widget_grid_envvar_construct(widget);

    case WIDGET_PANED:
        return widget_paned_envvar_construct(widget);

    case WIDGET_TOOLBAR:
        return widget_toolbar_envvar_construct(widget);

    case WIDGET_STACK_PAGES:
        return widget_stackpages_envvar_construct(widget);

    case WIDGET_WIZARD:
        return widget_wizard_envvar_construct(widget);

    case WIDGET_MENUBUTTON:
        return widget_menubutton_envvar_construct(widget);

    case WIDGET_FLOWBOX:
        return widget_flowbox_envvar_construct(widget);

    case WIDGET_OVERLAY:
        return widget_overlay_envvar_construct(widget);

    case WIDGET_REVEALER:
        return widget_revealer_envvar_construct(widget);

    case WIDGET_LINKBUTTON:
        return widget_linkbutton_envvar_construct(widget);

    case WIDGET_SEARCHENTRY:
        return widget_searchentry_envvar_construct(widget);

    case WIDGET_INFOBAR:
        return widget_infobar_envvar_construct(widget);

    case WIDGET_SPINNER:
        return widget_spinner_envvar_construct(widget);

    case WIDGET_IMAGE:
        return widget_image_envvar_construct(widget);

    case WIDGET_PULSE:
        return widget_pulse_envvar_construct(widget);

    case WIDGET_PASSWORD:
        return widget_password_envvar_construct(widget);

    case WIDGET_ASPECTFRAME:
        return widget_aspectframe_envvar_construct(widget);

    case WIDGET_FONTBUTTON:
        return widget_fontbutton_envvar_construct(widget);

    case WIDGET_MENUBAR:
        return widget_menubar_envvar_construct(widget);

    case WIDGET_MENUITEM:
        return widget_menuitem_envvar_construct(widget);

    case WIDGET_MENUITEMSEPARATOR:
        return g_strdup("");

    case WIDGET_MENU:
        return widget_menu_envvar_construct(widget);

    case WIDGET_TABLE:
        return widget_table_envvar_construct(widget);

    case WIDGET_TERMINAL:
        return widget_terminal_envvar_construct(widget);

    case WIDGET_LEVELBAR:
        return widget_levelbar_envvar_construct(widget);

    case WIDGET_DRAWINGAREA:
        return widget_drawingarea_envvar_construct(widget);

    case WIDGET_TREE:
        return widget_tree_envvar_construct(widget);

    case WIDGET_TREETABLE:
        return widget_treetable_envvar_construct(widget);

    case WIDGET_SCROLLEDW:
    case WIDGET_CHOOSER:
        return g_strdup("");

    default:
        return NULL;
    }
}

/* ------------------------------------------------------------------ */
/* Fonction 4 — nom lisible d'un type de widget.                        */
/* ------------------------------------------------------------------ */

char *widgets_to_str(int itype)
{
    switch (itype) {
    case WIDGET_CANCELBUTTON:       return (char *)"CANCELBUTTON";
    case WIDGET_HELPBUTTON:         return (char *)"HELPBUTTON";
    case WIDGET_NOBUTTON:           return (char *)"NOBUTTON";
    case WIDGET_OKBUTTON:           return (char *)"OKBUTTON";
    case WIDGET_YESBUTTON:          return (char *)"YESBUTTON";
    case WIDGET_BUTTON:             return (char *)"BUTTON";
    case WIDGET_CHECKBOX:           return (char *)"CHECKBOX";
    case WIDGET_COLORBUTTON:        return (char *)"COLORBUTTON";
    case WIDGET_COMBOBOX:           return (char *)"COMBOBOX";
    case WIDGET_COMBOBOXENTRY:      return (char *)"COMBOBOXENTRY";
    case WIDGET_COMBOBOXTEXT:       return (char *)"COMBOBOXTEXT";
    case WIDGET_EDIT:               return (char *)"EDIT";
    case WIDGET_ENTRY:              return (char *)"ENTRY";
    case WIDGET_EVENTBOX:           return (char *)"EVENTBOX";
    case WIDGET_EXPANDER:           return (char *)"EXPANDER";
    case WIDGET_SWITCH:             return (char *)"SWITCH";
    case WIDGET_FILECHOOSER:        return (char *)"FILECHOOSER";
    case WIDGET_CALENDAR:           return (char *)"CALENDAR";
    case WIDGET_GRID:               return (char *)"GRID";
    case WIDGET_PANED:              return (char *)"PANED";
    case WIDGET_TOOLBAR:            return (char *)"TOOLBAR";
    case WIDGET_STACK_PAGES:        return (char *)"STACK";
    case WIDGET_WIZARD:             return (char *)"WIZARD";
    case WIDGET_MENUBUTTON:         return (char *)"MENUBUTTON";
    case WIDGET_FLOWBOX:            return (char *)"FLOWBOX";
    case WIDGET_OVERLAY:            return (char *)"OVERLAY";
    case WIDGET_REVEALER:           return (char *)"REVEALER";
    case WIDGET_LINKBUTTON:         return (char *)"LINKBUTTON";
    case WIDGET_SEARCHENTRY:        return (char *)"SEARCHENTRY";
    case WIDGET_INFOBAR:            return (char *)"INFOBAR";
    case WIDGET_SPINNER:            return (char *)"SPINNER";
    case WIDGET_IMAGE:              return (char *)"IMAGE";
    case WIDGET_PULSE:              return (char *)"PULSE";
    case WIDGET_PASSWORD:           return (char *)"PASSWORD";
    case WIDGET_ASPECTFRAME:        return (char *)"ASPECTFRAME";
    case WIDGET_FONTBUTTON:         return (char *)"FONTBUTTON";
    case WIDGET_FRAME:              return (char *)"FRAME";
    case WIDGET_HBOX:               return (char *)"HBOX";
    case WIDGET_HSCALE:             return (char *)"HSCALE";
    case WIDGET_HSEPARATOR:         return (char *)"HSEPARATOR";
    case WIDGET_LIST:               return (char *)"LIST";
    case WIDGET_MENU:               return (char *)"MENU";
    case WIDGET_MENUBAR:            return (char *)"MENUBAR";
    case WIDGET_MENUITEM:           return (char *)"MENUITEM";
    case WIDGET_MENUITEMSEPARATOR:  return (char *)"MENUITEMSEPARATOR";
    case WIDGET_NOTEBOOK:           return (char *)"NOTEBOOK";
    case WIDGET_PIXMAP:             return (char *)"PIXMAP";
    case WIDGET_PROGRESSBAR:        return (char *)"PROGRESSBAR";
    case WIDGET_RADIOBUTTON:        return (char *)"RADIOBUTTON";
    case WIDGET_SPINBUTTON:         return (char *)"SPINBUTTON";
    case WIDGET_STATUSBAR:          return (char *)"STATUSBAR";
    case WIDGET_TABLE:              return (char *)"TABLE";
    case WIDGET_TERMINAL:           return (char *)"TERMINAL";
    case WIDGET_TEXT:               return (char *)"TEXT";
    case WIDGET_TIMER:              return (char *)"TIMER";
    case WIDGET_TOGGLEBUTTON:       return (char *)"TOGGLEBUTTON";
    case WIDGET_TREE:               return (char *)"TREE";
    case WIDGET_TREETABLE:          return (char *)"TREETABLE";
    case WIDGET_VBOX:               return (char *)"VBOX";
    case WIDGET_VSCALE:             return (char *)"VSCALE";
    case WIDGET_VSEPARATOR:         return (char *)"VSEPARATOR";
    case WIDGET_WINDOW:             return (char *)"WINDOW";
    case WIDGET_SCROLLEDW:          return (char *)"SCROLLEDW";
    default:                        return (char *)"THINGY";
    }
}

/* ------------------------------------------------------------------ */
/* Fonction 5 — connexion des signaux (no-op dans ce backend).          */
/* ------------------------------------------------------------------ */

gboolean widget_connect_signals(GtkWidget *widget, AttributeSet *Attr)
{
    (void)widget;
    (void)Attr;
    return TRUE;
}

/* ------------------------------------------------------------------ */
/* Fonctions 6 et 7 — visibilité différée + facteur d'étirement.        */
/* ------------------------------------------------------------------ */

static std::set<Fl_Widget *> sermo_expand_set;
static std::set<Fl_Widget *> sermo_noexpand_set;
static std::set<Fl_Widget *> sermo_fill_set;
static std::set<Fl_Widget *> sermo_nofill_set;

int sermo_widget_expand_rule(Fl_Widget *w)
{
    if (sermo_expand_set.count(w)) return 1;
    if (sermo_noexpand_set.count(w)) return 0;
    return -1;
}

int sermo_widget_fill_rule(Fl_Widget *w)
{
    if (sermo_fill_set.count(w)) return 1;
    if (sermo_nofill_set.count(w)) return 0;
    return -1;
}

/* ------------------------------------------------------------------ */
/* SermoFlex — répartition à la gtk_box (voir widgets.h).               */
/* ------------------------------------------------------------------ */

void SermoFlex::child_rule(Fl_Widget *c, bool expand, bool fill, int nat,
                           int cross, bool cross_fill)
{
    if (!c) return;
    if (nat < 1) nat = 1;
    Fl_Flex::fixed(c, nat);   /* marque « fixé » (lu par l'expander) */
    rules_[c] = Rule{expand, fill, cross_fill, nat, cross};
}

void SermoFlex::natural(Fl_Widget *c, int nat)
{
    auto it = rules_.find(c);
    if (it != rules_.end()) it->second.nat = nat > 0 ? nat : 1;
    need_layout(1);
}

int SermoFlex::natural_h() const
{
    int ml = 0, mt = 0, mr = 0, mb = 0;
    margin(&ml, &mt, &mr, &mb);
    int tot = 0, nv = 0, mx = 0;
    for (int i = 0; i < children(); i++) {
        Fl_Widget *c = child(i);
        if (!c->visible()) continue;
        auto it = rules_.find(c);
        int v = horizontal() ? (it != rules_.end() && it->second.cross > 0 ? it->second.cross : c->h())
                             : (it != rules_.end() ? it->second.nat : c->h());
        tot += v; if (v > mx) mx = v; nv++;
    }
    if (nv == 0) return 0;
    if (horizontal()) return mx + mt + mb;
    if (homog_) tot = mx * nv;            /* cases égales : la plus haute */
    return tot + gap() * (nv - 1) + mt + mb;
}

void SermoFlex::on_remove(int index)
{
    if (index >= 0 && index < children()) rules_.erase(child(index));
    Fl_Flex::on_remove(index);
}

void SermoFlex::resize(int X, int Y, int W, int H)
{
    Fl_Widget::resize(X, Y, W, H);
    place();
}

void SermoFlex::draw()
{
    /* Fl_Flex::draw() relancerait SA répartition (layout() non virtuel) */
    if (need_layout()) place();
    Fl_Group::draw();
}

void SermoFlex::place()
{
    int ml = 0, mt = 0, mr = 0, mb = 0;
    margin(&ml, &mt, &mr, &mb);
    const bool row = horizontal();
    const int bx = x() + Fl::box_dx(box()), by = y() + Fl::box_dy(box());
    const int bw = w() - Fl::box_dw(box()), bh = h() - Fl::box_dh(box());

    int nv = 0;
    for (int i = 0; i < children(); i++) if (child(i)->visible()) nv++;
    need_layout(0);
    if (nv == 0) return;

    const int g = gap();
    int avail = (row ? bw - ml - mr : bh - mt - mb) - g * (nv - 1);
    const int cpos = row ? by + mt : bx + ml;
    const int clen = row ? bh - mt - mb : bw - ml - mr;

    /* tailles naturelles et nombre d'enfants étirés */
    int sum = 0, nexp = 0;
    for (int i = 0; i < children(); i++) {
        Fl_Widget *c = child(i);
        if (!c->visible()) continue;
        auto it = rules_.find(c);
        int nat = it != rules_.end() ? it->second.nat : (row ? c->w() : c->h());
        sum += nat;
        if (it != rules_.end() && it->second.expand) nexp++;
    }
    int extra = avail - sum;

    int cur = row ? bx + ml : by + mt;
    /* rangée rangée par la fin, rien d'étiré : le groupe se cale à droite */
    if (!homog_ && nexp == 0 && pack_end_ && extra > 0) cur += extra;

    int k = 0, ke = 0;
    for (int i = 0; i < children(); i++) {
        Fl_Widget *c = child(i);
        if (!c->visible()) continue;
        auto it = rules_.find(c);
        bool has = it != rules_.end();
        bool fill = has ? it->second.fill : true;
        int nat = has ? it->second.nat : (row ? c->w() : c->h());

        int slot;
        if (homog_) {
            slot = avail / nv + (k < avail % nv ? 1 : 0);
        } else {
            slot = nat;
            if (has && it->second.expand && nexp > 0) {
                slot += extra / nexp;
                if (ke == nexp - 1) slot += extra % nexp;   /* reste au dernier */
                ke++;
            }
        }
        if (slot < 0) slot = 0;
        int len = fill ? slot : (nat < slot ? nat : slot);
        int pos = cur + (slot - len) / 2;

        /* axe croisé : toute l'épaisseur, sauf une feuille qui garde la sienne */
        int cl = clen, cp = cpos;
        if (has && !it->second.cross_fill && it->second.cross > 0 && it->second.cross < clen) {
            cl = it->second.cross;
            cp = cpos + (clen - cl) / 2;
        }
        if (row) c->resize(pos, cp, len, cl);
        else     c->resize(cp, pos, cl, len);
        cur += slot + g;
        k++;
    }
    redraw();
}

/* largeur naturelle d'un widget feuille (image = son image, texte = mesure) */
int sermo_natural_w(Fl_Widget *g)
{
    if (!g) return 0;
    SermoFlex *sf = dynamic_cast<SermoFlex *>(g);
    if (sf) return sf->natural_w() > 0 ? sf->natural_w() : g->w();
    const char *txt = NULL;
    Fl_Output *fo = dynamic_cast<Fl_Output *>(g);
    if (fo) txt = fo->value();
    else if (g->label() && *g->label() && !g->as_group()) txt = g->label();
    int gw = 0, gh = 0;
    if (g->image() && !g->as_group()) {
        gw = g->image()->w() + 4;
        if (txt && *txt) {            /* bouton icone + texte */
            int tw = 0; fl_font(g->labelfont(), g->labelsize()); fl_measure(txt, tw, gh, 0);
            gw += tw + 16;
        }
    } else if (txt && *txt) {
        fl_font(fo ? fo->textfont() : g->labelfont(), fo ? fo->textsize() : g->labelsize());
        fl_measure(txt, gw, gh, 0);
        gw += (dynamic_cast<Fl_Button *>(g) ? 24 : 12);
    } else if (g->as_group()) gw = g->w();               /* cadre : sa largeur */
    else gw = g->w() > 200 ? 200 : g->w();
    return gw;
}

int sermo_attr_rule(tag_attr *attr, const char *name, int dflt)
{
    const char *v = attr ? get_tag_attribute(attr, name) : NULL;
    if (!v) return dflt;
    return (strcasecmp(v, "true") == 0 || strcasecmp(v, "yes") == 0 ||
            atoi(v) == 1) ? 1 : 0;
}

bool sermo_can_grow_v(Fl_Widget *c)
{
    if (!c) return false;
    if (dynamic_cast<Fl_Spinner *>(c) || dynamic_cast<Fl_Input_Choice *>(c))
        return false;                                     /* champs composés */
    if (c->as_group()) return true;   /* cadres, boîtes, onglets, zones, listes */
    if (dynamic_cast<Fl_Button *>(c) && !dynamic_cast<Fl_Light_Button *>(c))
        return true;                                      /* bouton simple */
    return false;
}

bool sermo_widget_expands(Fl_Widget *w)
{
    if (w == NULL)
        return false;

    return sermo_expand_set.find(w) != sermo_expand_set.end();
}

bool sermo_widget_noexpand(Fl_Widget *w)
{
    if (w == NULL)
        return false;

    return sermo_noexpand_set.find(w) != sermo_noexpand_set.end();
}

static bool sermo_attr_is_true(const char *value)
{
    return strcasecmp(value, "true") == 0 ||
           strcasecmp(value, "yes") == 0 ||
           strcasecmp(value, "1") == 0;
}

static bool sermo_attr_is_false(const char *value)
{
    return strcasecmp(value, "false") == 0 ||
           strcasecmp(value, "no") == 0 ||
           strcasecmp(value, "0") == 0;
}

void widget_visibility_list_add(GtkWidget *widget, tag_attr *attr)
{
    if (widget == NULL)
        return;

    Fl_Widget *flw = (Fl_Widget *)widget;
    bool visible = true;

    if (attr != NULL) {
        const char *expand_value = get_tag_attribute(attr, "space-expand");

        if (expand_value != NULL) {
            if (sermo_attr_is_true(expand_value))
                sermo_expand_set.insert(flw);
            else
                sermo_noexpand_set.insert(flw);
        }

        const char *fill_value = get_tag_attribute(attr, "space-fill");

        if (fill_value != NULL) {
            if (sermo_attr_is_true(fill_value))
                sermo_fill_set.insert(flw);
            else
                sermo_nofill_set.insert(flw);
        }

        const char *visible_value = get_tag_attribute(attr, "visible");

        if (visible_value != NULL && sermo_attr_is_false(visible_value))
            visible = false;
    }

    if (visible)
        widget_show_list = g_list_append(widget_show_list, widget);
    else
        widget_hide_list = g_list_append(widget_hide_list, widget);
}

void widget_show_all(void)
{
    for (GList *l = widget_show_list; l != NULL; l = l->next) {
        Fl_Widget *w = (Fl_Widget *)l->data;
        w->show();
    }
    g_list_free(widget_show_list);
    widget_show_list = NULL;

    for (GList *l = widget_hide_list; l != NULL; l = l->next) {
        Fl_Widget *w = (Fl_Widget *)l->data;
        /* Comportement exigé : show() PUIS hide(), dans cet ordre. */
        w->show();
        w->hide();
    }
    g_list_free(widget_hide_list);
    widget_hide_list = NULL;
}
