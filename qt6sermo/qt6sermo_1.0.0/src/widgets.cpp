/*
 * widgets.cpp — Dispatch des widgets Qt6
 * qt6sermo 1.0.0 — haplo-dialog <devel@haplo-dialog.fr>
 * Licence : GPL-2.0-or-later
 *
 * Adapté de gtk3d/widgets.c (László Pere, Thunor)
 * Toutes les dépendances GTK remplacées par Qt6.
 */

#ifndef _GNU_SOURCE
#define _GNU_SOURCE
#endif
#include "qt6-compat.h"
#include "gtk3d.h"
#include "safe_exec.h"
#include "widgets.h"
#include "stringman.h"
#include "tag_attributes.h"
#include "macros.h"
#include "signals.h"

/* --- Widgets implémentés (C++) --- */
#include "widget_button.h"
#include "widget_checkbox.h"
#include "widget_colorbutton.h"
#include "widget_combobox.h"
#include "widget_comboboxtext.h"
#include "widget_edit.h"
#include "widget_entry.h"
#include "widget_expander.h"
#include "widget_fontbutton.h"
#include "widget_frame.h"
#include "widget_hbox.h"
#include "widget_hscale.h"
#include "widget_hseparator.h"
#include "widget_list.h"
#include "widget_menubar.h"
#include "widget_menuitem.h"
#include "widget_notebook.h"
#include "widget_pixmap.h"
#include "widget_progressbar.h"
#include "widget_radiobutton.h"
#include "widget_spinbutton.h"
#include "widget_statusbar.h"
#include "widget_table.h"
#include "widget_terminal.h"
#include "widget_text.h"
#include "widget_timer.h"
#include "widget_togglebutton.h"
#include "widget_tree.h"
#include "widget_vbox.h"
#include "widget_vscale.h"
#include "widget_vseparator.h"
#include "widget_window.h"
#include "widget_switch.h"
#include "widget_password.h"
#include "widget_searchentry.h"
#include "widget_calendar.h"
#include "widget_infobar.h"
#include "widget_levelbar.h"
#include "widget_spinner.h"
#include "widget_aspectframe.h"
#include "widget_drawingarea.h"
#include "widget_filechooser.h"

#include <QtWidgets/QWidget>

#include <math.h>
#include <string.h>
#include <stdio.h>
#include <map>

/* Listes de visibilité différée */
GList *widget_show_list = NULL;
GList *widget_hide_list = NULL;

extern gchar *option_include_file;

/* ───────────────── Indices de disposition (space-expand) ─────────────────
 * Renseignés une fois par widget à la création (qt6_layout_register, appelé
 * depuis l'automate). Les QHBoxLayout/QVBoxLayout (widget_hbox/vbox) s'en
 * servent pour donner un facteur d'étirement aux seuls enfants extensibles —
 * sinon Qt répartit l'espace excédentaire à parts égales entre toutes les
 * étiquettes (icône à gauche, texte rejeté à droite ; boutons de pied trop
 * larges). */
static std::map<QWidget *, bool> g_qt6_expand;

static bool qt6_attr_true(const char *v)
{
    return v && (!strcasecmp(v, "true") || !strcasecmp(v, "yes") || atoi(v) == 1);
}

extern "C" void qt6_layout_register(GtkWidget *widget, tag_attr *attr)
{
    if (!widget) return;
    bool expand = false;
    if (attr) {
        const char *v = get_tag_attribute(attr, "space-expand");
        if (v) expand = qt6_attr_true(v);
    }
    g_qt6_expand[static_cast<QWidget *>(widget)] = expand;
}

extern "C" int qt6_layout_get_expand(GtkWidget *widget)
{
    if (!widget) return 0;
    std::map<QWidget *, bool>::iterator it =
        g_qt6_expand.find(static_cast<QWidget *>(widget));
    return (it != g_qt6_expand.end() && it->second) ? 1 : 0;
}

/* -----------------------------------------------------------------------
 * widget_opencommand
 * ----------------------------------------------------------------------- */
FILE *widget_opencommand(const char *command)
{
    gchar *the_line = NULL;
    FILE  *infile;

    PIP_DEBUG("Opening command: '%s'", command);

    if (option_include_file != NULL) {
        the_line = g_strdup_printf(". %s; %s", option_include_file, command);
        infile = safe_popen(the_line);
        g_free(the_line);
    } else {
        infile = safe_popen(command);
    }

    if (infile == NULL)
        g_warning("%s(): failed to open command '%s'", __func__, command);

    return infile;
}

/* -----------------------------------------------------------------------------
 * widget_command_value — valeur numérique produite par la commande <input>
 * d'un widget « à valeur » (progressbar, hscale/vscale, levelbar, spinbutton).
 * L'attribut ATTR_INPUT est stocké préfixé « Command:… » : input_get_shell_command()
 * retire le préfixe (sinon widget_opencommand échoue). Renvoie `fallback` s'il
 * n'y a pas d'<input> — le widget conserve alors sa valeur par défaut.
 * --------------------------------------------------------------------------- */
double widget_command_value(AttributeSet *Attr, double fallback)
{
    GList      *element = NULL;
    gchar      *inp;
    const char *cmd;
    FILE       *fp;
    char        line[64];
    double      v = fallback;

    if (!Attr) return fallback;
    inp = attributeset_get_first(&element, Attr, ATTR_INPUT);
    if (!inp || !*inp) return fallback;
    cmd = input_get_shell_command(inp);
    if (!cmd || !*cmd) cmd = inp;
    fp = widget_opencommand(cmd);
    if (!fp) return fallback;
    if (fgets(line, sizeof line, fp)) v = atof(line);
    fclose(fp);   /* widget_opencommand = safe_popen (fdopen) → fclose */
    return v;
}

/* -----------------------------------------------------------------------
 * widget_get_text_value
 * ----------------------------------------------------------------------- */
char *widget_get_text_value(GtkWidget *widget, int type)
{
    if (widget == NULL) {
        g_warning("%s(): NULL Widget", __func__);
        return (char *)"NULL WIDGET";
    }

    switch (type) {
        case WIDGET_CANCELBUTTON:
        case WIDGET_HELPBUTTON:
        case WIDGET_NOBUTTON:
        case WIDGET_OKBUTTON:
        case WIDGET_YESBUTTON:
        case WIDGET_TOGGLEBUTTON:
        case WIDGET_BUTTON:
            return widget_button_envvar_construct(widget);

        case WIDGET_CHECKBOX:
            return widget_checkbox_envvar_construct(widget);

        case WIDGET_COLORBUTTON:
            return widget_colorbutton_envvar_construct(widget);

        case WIDGET_COMBOBOX:
        case WIDGET_COMBOBOXENTRY:
            return widget_combobox_envvar_construct(widget);

        case WIDGET_COMBOBOXTEXT:
            return widget_comboboxtext_envvar_construct(widget);

        case WIDGET_EDIT:
            return widget_edit_envvar_construct(widget);

        case WIDGET_ENTRY:
            return widget_entry_envvar_construct(widget);

        case WIDGET_EXPANDER:
            return widget_expander_envvar_construct(widget);

        case WIDGET_FONTBUTTON:
            return widget_fontbutton_envvar_construct(widget);

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

        case WIDGET_TABLE:
            return widget_table_envvar_construct(widget);

        case WIDGET_TERMINAL:
            return widget_terminal_envvar_construct(widget);

        case WIDGET_TEXT:
            return widget_text_envvar_construct(widget);

        case WIDGET_TIMER:
            return widget_timer_envvar_construct(widget);

        case WIDGET_TREE:
            return widget_tree_envvar_construct(widget);

        case WIDGET_VBOX:
            return widget_vbox_envvar_construct(widget);

        case WIDGET_WINDOW:
            return widget_window_envvar_construct(widget);

        case WIDGET_SWITCH:
            return widget_switch_envvar_construct(widget);

        case WIDGET_PASSWORD:
            return widget_password_envvar_construct(widget);

        case WIDGET_SEARCHENTRY:
            return widget_searchentry_envvar_construct(widget);

        case WIDGET_CALENDAR:
            return widget_calendar_envvar_construct(widget);

        case WIDGET_INFOBAR:
            return widget_infobar_envvar_construct(widget);

        case WIDGET_SPINNER:
        case WIDGET_PULSE:
            return widget_spinner_envvar_construct(widget);

        case WIDGET_ASPECTFRAME:
            return widget_aspectframe_envvar_construct(widget);

        /* Widgets sans valeur exportable */
        case WIDGET_FILECHOOSER:
            return widget_filechooser_envvar_construct(widget);
        case WIDGET_MENUITEM:
            return widget_menuitem_envvar_construct(widget);
        case WIDGET_EVENTBOX:
        case WIDGET_LINKBUTTON:
        case WIDGET_IMAGE:
        case WIDGET_MENUBAR:
        case WIDGET_MENUITEMSEPARATOR:
        case WIDGET_MENU:
        case WIDGET_SCROLLEDW:
        case WIDGET_CHOOSER:
            return g_strdup("");

        default:
            return NULL;
    }
}

/* -----------------------------------------------------------------------
 * widgets_to_str
 * ----------------------------------------------------------------------- */
char *widgets_to_str(int itype)
{
    switch (itype) {
        case WIDGET_CANCELBUTTON:      return (char *)"CANCELBUTTON";
        case WIDGET_HELPBUTTON:        return (char *)"HELPBUTTON";
        case WIDGET_NOBUTTON:          return (char *)"NOBUTTON";
        case WIDGET_OKBUTTON:          return (char *)"OKBUTTON";
        case WIDGET_YESBUTTON:         return (char *)"YESBUTTON";
        case WIDGET_BUTTON:            return (char *)"BUTTON";
        case WIDGET_CHECKBOX:          return (char *)"CHECKBOX";
        case WIDGET_COLORBUTTON:       return (char *)"COLORBUTTON";
        case WIDGET_COMBOBOX:          return (char *)"COMBOBOX";
        case WIDGET_COMBOBOXENTRY:     return (char *)"COMBOBOXENTRY";
        case WIDGET_COMBOBOXTEXT:      return (char *)"COMBOBOXTEXT";
        case WIDGET_EDIT:              return (char *)"EDIT";
        case WIDGET_ENTRY:             return (char *)"ENTRY";
        case WIDGET_EVENTBOX:          return (char *)"EVENTBOX";
        case WIDGET_EXPANDER:          return (char *)"EXPANDER";
        case WIDGET_SWITCH:            return (char *)"SWITCH";
        case WIDGET_FILECHOOSER:       return (char *)"FILECHOOSER";
        case WIDGET_CALENDAR:          return (char *)"CALENDAR";
        case WIDGET_LINKBUTTON:        return (char *)"LINKBUTTON";
        case WIDGET_SEARCHENTRY:       return (char *)"SEARCHENTRY";
        case WIDGET_INFOBAR:           return (char *)"INFOBAR";
        case WIDGET_SPINNER:           return (char *)"SPINNER";
        case WIDGET_IMAGE:             return (char *)"IMAGE";
        case WIDGET_PULSE:             return (char *)"PULSE";
        case WIDGET_PASSWORD:          return (char *)"PASSWORD";
        case WIDGET_ASPECTFRAME:       return (char *)"ASPECTFRAME";
        case WIDGET_FONTBUTTON:        return (char *)"FONTBUTTON";
        case WIDGET_FRAME:             return (char *)"FRAME";
        case WIDGET_HBOX:              return (char *)"HBOX";
        case WIDGET_HSCALE:            return (char *)"HSCALE";
        case WIDGET_HSEPARATOR:        return (char *)"HSEPARATOR";
        case WIDGET_LIST:              return (char *)"LIST";
        case WIDGET_MENU:              return (char *)"MENU";
        case WIDGET_MENUBAR:           return (char *)"MENUBAR";
        case WIDGET_MENUITEM:          return (char *)"MENUITEM";
        case WIDGET_MENUITEMSEPARATOR: return (char *)"MENUITEMSEPARATOR";
        case WIDGET_NOTEBOOK:          return (char *)"NOTEBOOK";
        case WIDGET_PIXMAP:            return (char *)"PIXMAP";
        case WIDGET_PROGRESSBAR:       return (char *)"PROGRESSBAR";
        case WIDGET_RADIOBUTTON:       return (char *)"RADIOBUTTON";
        case WIDGET_SPINBUTTON:        return (char *)"SPINBUTTON";
        case WIDGET_STATUSBAR:         return (char *)"STATUSBAR";
        case WIDGET_TABLE:             return (char *)"TABLE";
        case WIDGET_TERMINAL:          return (char *)"TERMINAL";
        case WIDGET_TEXT:              return (char *)"TEXT";
        case WIDGET_TIMER:             return (char *)"TIMER";
        case WIDGET_TOGGLEBUTTON:      return (char *)"TOGGLEBUTTON";
        case WIDGET_TREE:              return (char *)"TREE";
        case WIDGET_VBOX:              return (char *)"VBOX";
        case WIDGET_VSCALE:            return (char *)"VSCALE";
        case WIDGET_VSEPARATOR:        return (char *)"VSEPARATOR";
        case WIDGET_WINDOW:            return (char *)"WINDOW";
        case WIDGET_SCROLLEDW:         return (char *)"SCROLLEDW";
        default:                       return (char *)"THINGY";
    }
}

/* -----------------------------------------------------------------------
 * widget_connect_signals
 * Qt6 : les signaux sont connectés dans chaque widget_*_create().
 * ----------------------------------------------------------------------- */
gboolean widget_connect_signals(GtkWidget *widget, AttributeSet *Attr)
{
    (void)widget;
    (void)Attr;
    return TRUE;
}

/* -----------------------------------------------------------------------
 * widget_visibility_list_add
 * ----------------------------------------------------------------------- */
void widget_visibility_list_add(GtkWidget *widget, tag_attr *attr)
{
    gint visible = TRUE;

    if (widget) {
        if (attr) {
            gchar *value = get_tag_attribute(attr, "visible");
            if (value &&
                (strcasecmp(value, "false") == 0 ||
                 strcasecmp(value, "no")    == 0 ||
                 strcmp(value, "0")         == 0)) {
                visible = FALSE;
            }
        }
        if (visible)
            widget_show_list = g_list_append(widget_show_list, widget);
        else
            widget_hide_list = g_list_append(widget_hide_list, widget);
    }
}

/* -----------------------------------------------------------------------
 * widget_show_all
 * Qt6 : show() / hide() sur QWidget.
 * ----------------------------------------------------------------------- */
void widget_show_all(void)
{
    GList *element;

    if (widget_show_list) {
        element = widget_show_list;
        while (element) {
            QWidget *w = static_cast<QWidget *>(element->data);
            /* Qt : afficher uniquement les widgets top-level (la fenêtre).
             * Les enfants sont affichés en cascade par leur parent ; un show()
             * explicite sur chaque enfant ré-affiche les pages non-courantes
             * d'un QTabWidget (onglets superposés). */
            if (!w->parentWidget()) w->show();
            element = element->next;
        }
        g_list_free(widget_show_list);
        widget_show_list = NULL;
    }

    if (widget_hide_list) {
        element = widget_hide_list;
        while (element) {
            QWidget *w = static_cast<QWidget *>(element->data);
            w->hide();   /* visible="false" : masquer (pas de show() préalable
                          * qui ré-afficherait des pages d'onglet). */
            element = element->next;
        }
        g_list_free(widget_hide_list);
        widget_hide_list = NULL;
    }
}
