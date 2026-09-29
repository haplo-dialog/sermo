/* SPDX-License-Identifier: GPL-2.0-or-later */
/*
 * widgets.c — dispatch des widgets EFL (implémentation indépendante)
 *
 * Copyright (C) 2026 S. Cage
 *
 * Façade haplo-dialog <devel@haplo-dialog.fr>
 */

#include "efl-compat.h"
#include "gtkdialog.h"
#include "safe_exec.h"
#include "widgets.h"
#include "stringman.h"
#include "tag_attributes.h"
#include "macros.h"
#include "signals.h"

#include "widget_button.h"
#include "widget_levelbar.h"
#include "widget_drawingarea.h"
#include "widget_grid.h"
#include "widget_paned.h"
#include "widget_toolbar.h"
#include "widget_stackpages.h"
#include "widget_wizard.h"
#include "widget_menubutton.h"
#include "widget_flowbox.h"
#include "widget_overlay.h"
#include "widget_revealer.h"
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
#include "widget_switch.h"
#include "widget_password.h"
#include "widget_searchentry.h"
#include "widget_calendar.h"
#include "widget_infobar.h"
#include "widget_spinner.h"
#include "widget_aspectframe.h"
#include "widget_vbox.h"
#include "widget_vscale.h"
#include "widget_vseparator.h"
#include "widget_window.h"
#include "sermo_input.h"

#include <string.h>
#include <strings.h>

/*
 * Widgets sans portage visuel (implémentation minimale ailleurs dans ce
 * backend) : pas d'en-tête dédié pour ces cinq-là, on les déclare nous-mêmes.
 */
gchar *widget_eventbox_envvar_construct(GtkWidget *widget);
gchar *widget_filechooser_envvar_construct(GtkWidget *widget);
gchar *widget_linkbutton_envvar_construct(GtkWidget *widget);
gchar *widget_image_envvar_construct(GtkWidget *widget);
gchar *widget_pulse_envvar_construct(GtkWidget *widget);

extern gchar *option_include_file;

/* ------------------------------------------------------------------- */
/* Fonction 1 — ouverture d'une commande externe                       */
/* ------------------------------------------------------------------- */

FILE *
widget_opencommand(const char *command)
{
    FILE *stream;
    gchar *full_command;

    if (option_include_file != NULL) {
        full_command = g_strdup_printf(". %s; %s", option_include_file, command);
        stream = safe_popen(full_command);
        g_free(full_command);
    } else {
        stream = safe_popen(command);
    }

    if (stream == NULL) {
        g_warning("widget_opencommand: impossible d'ouvrir la commande '%s'", command);
        return NULL;
    }

    PIP_DEBUG("widget_opencommand: commande ouverte: %s\n", command);

    return stream;
}

/* ------------------------------------------------------------------- */
/* Fonction 2 — résolution de <input>                                  */
/* ------------------------------------------------------------------- */

gchar *
widget_input_text(AttributeSet *Attr)
{
    GList *element = NULL;
    gchar *value;

    if (Attr == NULL)
        return NULL;

    value = attributeset_get_first(&element, Attr, ATTR_INPUT);
    while (value != NULL) {
        if (strncasecmp(value, "Command:", 8) == 0 && value[8] != '\0') {
            FILE *stream = widget_opencommand(value + 8);

            if (stream != NULL) {
                GString *s = g_string_new("");
                char buffer[1024];

                while (fgets(buffer, sizeof(buffer), stream) != NULL)
                    g_string_append(s, buffer);

                /* widget_opencommand repose sur safe_popen : le flux se
                 * referme par fclose dans ce backend, jamais pclose. */
                fclose(stream);

                return g_string_free(s, FALSE);
            }
        } else if (strncasecmp(value, "file:", 5) == 0 && value[5] != '\0') {
            /* 2.7.3 : lu par sermo_fopen_input(), plafonné comme les autres
             * ports. g_file_get_contents() lisait /dev/zero sans fin. */
            FILE *fp = sermo_fopen_input(value + 5);

            if (fp != NULL) {
                GString *s = g_string_new("");
                char buffer[1024];

                while (fgets(buffer, sizeof(buffer), fp) != NULL)
                    g_string_append(s, buffer);

                fclose(fp);

                return g_string_free(s, FALSE);
            }
        }

        value = attributeset_get_next(&element, Attr, ATTR_INPUT);
    }

    return NULL;
}

/* ------------------------------------------------------------------- */
/* Fonction 2 bis — <input> découpé en lignes                          */
/* ------------------------------------------------------------------- */

/*
 * Les widgets à éléments (comboboxtext, list, tree, table) lisent <input>
 * ligne par ligne, comme l'étalon gtk3sermo avec fgets() : une ligne vide
 * reste une ligne (elle fait un élément vide), la ligne s'arrête au premier
 * CR ou LF, et le saut de ligne final n'ouvre pas de ligne de plus. NULL si
 * aucune directive n'est lisible ; sinon un tableau terminé par NULL, à
 * libérer par g_strfreev.
 */
gchar **
widget_input_lines(AttributeSet *Attr)
{
    gchar *text = widget_input_text(Attr);
    gchar **lines;
    gsize n = 0;

    if (text == NULL)
        return NULL;

    lines = g_strsplit(text, "\n", -1);
    g_free(text);

    while (lines[n] != NULL)
        n++;
    /* « un\ndeux\n » se découpe en « un », « deux », « » : ce dernier
     * morceau suit le saut de ligne final, ce n'est pas une ligne. */
    if (n > 0 && lines[n - 1][0] == '\0') {
        g_free(lines[n - 1]);
        lines[n - 1] = NULL;
    }
    for (n = 0; lines[n] != NULL; n++)
        lines[n][strcspn(lines[n], "\r")] = '\0';

    return lines;
}

/* ------------------------------------------------------------------- */
/* Fonction 3 — table de dispatch : valeur texte exportable            */
/* ------------------------------------------------------------------- */

char *
widget_get_text_value(GtkWidget *widget, int type)
{
    if (widget == NULL) {
        g_warning("widget_get_text_value: widget NULL (type %d)", type);
        return "NULL WIDGET";
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
        /* Fonction dédiée : exporte l'état true/false, pas le label. */
        return widget_togglebutton_envvar_construct(widget);

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

    case WIDGET_EVENTBOX:
        return widget_eventbox_envvar_construct(widget);

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

    case WIDGET_SWITCH:
        return widget_switch_envvar_construct(widget);

    case WIDGET_FILECHOOSER:
        return widget_filechooser_envvar_construct(widget);

    case WIDGET_CALENDAR:
        return widget_calendar_envvar_construct(widget);

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

    case WIDGET_MENUBAR:
        return widget_menubar_envvar_construct(widget);

    case WIDGET_MENUITEM:
        return widget_menuitem_envvar_construct(widget);

    case WIDGET_MENUITEMSEPARATOR:
    case WIDGET_MENU:
    case WIDGET_SCROLLEDW:
    case WIDGET_CHOOSER:
        return g_strdup("");

    case WIDGET_LEVELBAR:
        return widget_levelbar_envvar_construct(widget);

    case WIDGET_DRAWINGAREA:
        return widget_drawingarea_envvar_construct(widget);

    default:
        /* ⚠️ Ici vivait une note disant que LEVELBAR et DRAWINGAREA tombaient
         * dans ce défaut « volontairement — état actuel réel de ce backend ».
         * C'était honnête et c'était faux comme politique : les deux widgets
         * étaient bel et bien portés, seul le répartiteur ne les appelait pas,
         * si bien que <levelbar> rendait une chaîne vide là où l'étalon rendait
         * sa valeur. Corrigé le 2026-09-14. */
        return NULL;
    }
}

/* ------------------------------------------------------------------- */
/* Fonction 4 — nom lisible d'un type de widget                        */
/* ------------------------------------------------------------------- */

char *
widgets_to_str(int itype)
{
    switch (itype) {
    case WIDGET_CANCELBUTTON:      return "CANCELBUTTON";
    case WIDGET_HELPBUTTON:        return "HELPBUTTON";
    case WIDGET_NOBUTTON:          return "NOBUTTON";
    case WIDGET_OKBUTTON:          return "OKBUTTON";
    case WIDGET_YESBUTTON:         return "YESBUTTON";
    case WIDGET_BUTTON:            return "BUTTON";
    case WIDGET_CHECKBOX:          return "CHECKBOX";
    case WIDGET_COLORBUTTON:       return "COLORBUTTON";
    case WIDGET_COMBOBOX:          return "COMBOBOX";
    case WIDGET_COMBOBOXENTRY:     return "COMBOBOXENTRY";
    case WIDGET_COMBOBOXTEXT:      return "COMBOBOXTEXT";
    case WIDGET_EDIT:              return "EDIT";
    case WIDGET_ENTRY:             return "ENTRY";
    case WIDGET_EVENTBOX:          return "EVENTBOX";
    case WIDGET_EXPANDER:          return "EXPANDER";
    case WIDGET_SWITCH:            return "SWITCH";
    case WIDGET_FILECHOOSER:       return "FILECHOOSER";
    case WIDGET_CALENDAR:          return "CALENDAR";
    case WIDGET_LINKBUTTON:        return "LINKBUTTON";
    case WIDGET_SEARCHENTRY:       return "SEARCHENTRY";
    case WIDGET_INFOBAR:           return "INFOBAR";
    case WIDGET_SPINNER:           return "SPINNER";
    case WIDGET_IMAGE:             return "IMAGE";
    case WIDGET_PULSE:             return "PULSE";
    case WIDGET_PASSWORD:          return "PASSWORD";
    case WIDGET_ASPECTFRAME:       return "ASPECTFRAME";
    case WIDGET_FONTBUTTON:        return "FONTBUTTON";
    case WIDGET_FRAME:             return "FRAME";
    case WIDGET_GRID:               return "GRID";
    case WIDGET_PANED:              return "PANED";
    case WIDGET_TOOLBAR:            return "TOOLBAR";
    case WIDGET_STACK_PAGES:        return "STACK";
    case WIDGET_WIZARD:             return "WIZARD";
    case WIDGET_MENUBUTTON:         return "MENUBUTTON";
    case WIDGET_FLOWBOX:            return "FLOWBOX";
    case WIDGET_OVERLAY:            return "OVERLAY";
    case WIDGET_REVEALER:           return "REVEALER";
    case WIDGET_HBOX:              return "HBOX";
    case WIDGET_HSCALE:            return "HSCALE";
    case WIDGET_HSEPARATOR:        return "HSEPARATOR";
    case WIDGET_LIST:              return "LIST";
    case WIDGET_MENU:              return "MENU";
    case WIDGET_MENUBAR:           return "MENUBAR";
    case WIDGET_MENUITEM:          return "MENUITEM";
    case WIDGET_MENUITEMSEPARATOR: return "MENUITEMSEPARATOR";
    case WIDGET_NOTEBOOK:          return "NOTEBOOK";
    case WIDGET_PIXMAP:            return "PIXMAP";
    case WIDGET_PROGRESSBAR:       return "PROGRESSBAR";
    case WIDGET_RADIOBUTTON:       return "RADIOBUTTON";
    case WIDGET_SPINBUTTON:        return "SPINBUTTON";
    case WIDGET_STATUSBAR:         return "STATUSBAR";
    case WIDGET_TABLE:             return "TABLE";
    case WIDGET_TERMINAL:          return "TERMINAL";
    case WIDGET_TEXT:              return "TEXT";
    case WIDGET_TIMER:             return "TIMER";
    case WIDGET_TOGGLEBUTTON:      return "TOGGLEBUTTON";
    case WIDGET_TREE:              return "TREE";
    case WIDGET_VBOX:              return "VBOX";
    case WIDGET_VSCALE:            return "VSCALE";
    case WIDGET_VSEPARATOR:        return "VSEPARATOR";
    case WIDGET_WINDOW:            return "WINDOW";
    case WIDGET_SCROLLEDW:         return "SCROLLEDW";
    default:                       return "THINGY";
    }
}

/* ------------------------------------------------------------------- */
/* Fonction 5 — connexion des signaux                                  */
/* ------------------------------------------------------------------- */

gboolean
widget_connect_signals(GtkWidget *widget, AttributeSet *Attr)
{
    /* Chaque widget_*_create() connecte déjà ses propres callbacks EFL ;
     * rien à faire ici. */
    (void)widget;
    (void)Attr;

    return TRUE;
}

/* ------------------------------------------------------------------- */
/* Fonctions 6/7 — visibilité et facteur d'étirement (evas_object_*)   */
/* ------------------------------------------------------------------- */

void
widget_visibility_list_add(GtkWidget *widget, tag_attr *attr)
{
    gboolean visible = TRUE;

    if (widget == NULL)
        return;

    if (attr != NULL) {
        char *expand = get_tag_attribute(attr, "space-expand");
        char *vis;

        if (expand != NULL) {
            if (strcasecmp(expand, "true") == 0 ||
                strcasecmp(expand, "yes") == 0 ||
                strcmp(expand, "1") == 0)
                evas_object_data_set(widget, "sermo_expand", (void *)1);
            else
                evas_object_data_set(widget, "sermo_noexpand", (void *)1);
        }

        vis = get_tag_attribute(attr, "visible");
        if (vis != NULL &&
            (strcasecmp(vis, "false") == 0 ||
             strcasecmp(vis, "no") == 0 ||
             strcmp(vis, "0") == 0))
            visible = FALSE;
    }

    if (visible)
        widget_show_list = g_list_append(widget_show_list, widget);
    else
        widget_hide_list = g_list_append(widget_hide_list, widget);
}

void
widget_show_all(void)
{
    GList *element;

    element = widget_show_list;
    while (element != NULL) {
        GtkWidget *widget = (GtkWidget *)element->data;

        /* Une page d'onglet non courante porte "nb_hidden" et doit rester
         * cachée : sinon elle se dessinerait en superposition à l'origine
         * de la fenêtre. */
        if (evas_object_data_get(widget, "nb_hidden") == NULL)
            evas_object_show(widget);

        element = g_list_next(element);
    }
    g_list_free(widget_show_list);
    widget_show_list = NULL;

    element = widget_hide_list;
    while (element != NULL) {
        GtkWidget *widget = (GtkWidget *)element->data;

        /* show puis hide, dans cet ordre, sans condition : comportement
         * exigé de ce backend, ne pas le simplifier en un simple hide(). */
        evas_object_show(widget);
        evas_object_hide(widget);

        element = g_list_next(element);
    }
    g_list_free(widget_hide_list);
    widget_hide_list = NULL;
}
