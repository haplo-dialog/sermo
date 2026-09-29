/* SPDX-License-Identifier: GPL-2.0-or-later */
/*
 * widgets.c — dispatch des widgets (implementation independante)
 *
 * Copyright (C) 2026 S. Cage
 * Facade haplo-dialog <devel@haplo-dialog.fr>
 */

/* pclose()/strcasecmp()/strncasecmp() sont POSIX : en -std=c11 strict, il
 * faut le demander explicitement avant tout en-tete systeme. */
#define _POSIX_C_SOURCE 200809L

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <strings.h>

#include "gtkdialog.h"
#include "dialog_state.h"
#include "safe_exec.h"
#include "widgets.h"
#include "stringman.h"
#include "tag_attributes.h"
#include "macros.h"

#include "widget_button.h"
#include "widget_togglebutton.h"
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
#include "widget_vscale.h"
#include "widget_hseparator.h"
#include "widget_vseparator.h"
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
#include "widget_tree.h"
#include "widget_vbox.h"
#include "widget_window.h"
#include "widget_password.h"
#include "widget_switch.h"
#include "widget_searchentry.h"
#include "widget_infobar.h"
#include "widget_calendar.h"
#include "widget_aspectframe.h"
#include "widget_spinner.h"
#include "widget_eventbox.h"
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
#include "widget_image.h"
#include "widget_filechooser.h"
#include "widget_linkbutton.h"
#include "widget_pulse.h"
#include "sermo_input.h"

/* Chemin a sourcer avant toute commande shell, ou NULL. Definie dans le
 * coeur ; ce module a besoin de sa propre declaration externe pour la
 * voir (correction d'integration : gtkdialog.h ne la declare pas). */
extern gchar *option_include_file;

/* Files d'attente de visibilite differee. Deja declarees ailleurs, dans
 * le fichier qui heberge le point d'entree du backend. */
extern GList *widget_show_list;
extern GList *widget_hide_list;

/* -------------------------------------------------------------------- */
/* Fonction 1 — ouverture encadree d'une commande shell en lecture       */
/* -------------------------------------------------------------------- */

FILE *widget_opencommand(const char *command)
{
    FILE *stream;

    if (option_include_file != NULL) {
        gchar *full_command = g_strdup_printf(". %s; %s",
                                               option_include_file, command);

        PIP_DEBUG("widget_opencommand: %s\n", full_command);
        stream = safe_popen(full_command);
        g_free(full_command);
    } else {
        PIP_DEBUG("widget_opencommand: %s\n", command);
        stream = safe_popen(command);
    }

    if (stream == NULL) {
        g_warning("widget_opencommand: impossible d'ouvrir la commande '%s'",
                  command);
    }

    return stream;
}

/* -------------------------------------------------------------------- */
/* Fonction 2 — resolution des directives <input> d'une balise          */
/* -------------------------------------------------------------------- */

gchar *widget_input_text(AttributeSet *Attr)
{
    GList *element = NULL;
    char *value;

    if (Attr == NULL) {
        return NULL;
    }

    for (value = attributeset_get_first(&element, Attr, ATTR_INPUT);
         value != NULL;
         value = attributeset_get_next(&element, Attr, ATTR_INPUT)) {

        if (strncasecmp(value, "Command:", 8) == 0 && value[8] != '\0') {
            const char *command = value + 8;
            FILE *stream = widget_opencommand(command);
            GString *buffer;
            char line[1024];

            if (stream == NULL) {
                return NULL;
            }

            buffer = g_string_new(NULL);
            while (fgets(line, sizeof(line), stream) != NULL) {
                g_string_append(buffer, line);
            }
            fclose(stream);   /* safe_popen() rend un fdopen() : fclose, jamais pclose */

            return g_string_free(buffer, FALSE);
        }

        if (strncasecmp(value, "file:", 5) == 0 && value[5] != '\0') {
            const char *path = value + 5;
            FILE *fp = sermo_fopen_input(path);
            GString *buffer;
            char line[1024];

            if (fp == NULL) {
                g_warning("widget_input_text: impossible de lire le fichier '%s'",
                          path);
                return NULL;
            }

            buffer = g_string_new(NULL);
            while (fgets(line, sizeof(line), fp) != NULL) {
                g_string_append(buffer, line);
            }
            fclose(fp);

            return g_string_free(buffer, FALSE);
        }

        /* Valeur non reconnue : on l'ignore et on passe a la suivante. */
    }

    return NULL;
}

/* -------------------------------------------------------------------- */
/* Fonction 2 bis — la même directive, découpée en lignes               */
/* -------------------------------------------------------------------- */

/* Pour les widgets à rangées (comboboxtext, list, tree, table) : une ligne
 * par élément, coupée au premier CR/LF, lignes vides GARDÉES — l'étalon
 * gtk3sermo lit par fgets() et ajoute chaque ligne, vide comprise (mesuré :
 * une sortie qui commence par une ligne vide exporte ""). Pas de g_strsplit :
 * celui de la couche de compatibilité passe par strtok et sauterait les
 * lignes vides. Tableau terminé par NULL, à libérer par g_strfreev ; NULL si
 * aucune directive n'a pu être lue. */
gchar **widget_input_lines(AttributeSet *Attr)
{
    gchar *text = widget_input_text(Attr);
    gchar **lines;
    gsize count = 0, n = 0;
    char *p;

    if (text == NULL) {
        return NULL;
    }

    for (p = text; *p != '\0'; p++) {
        if (*p == '\n') count++;
    }
    if (p != text && p[-1] != '\n') count++;   /* dernière ligne sans LF */

    lines = g_new(gchar *, count + 1);
    for (p = text; *p != '\0'; ) {
        char *lf = strchr(p, '\n');
        size_t len = lf ? (size_t)(lf - p) : strlen(p);

        lines[n] = g_strndup(p, len);
        lines[n][strcspn(lines[n], "\r")] = '\0';
        n++;
        p += lf ? len + 1 : len;
    }
    lines[n] = NULL;

    g_free(text);
    return lines;
}

/* -------------------------------------------------------------------- */
/* Fonction 3 — texte courant d'un widget, pour l'export de variable    */
/* -------------------------------------------------------------------- */

char *widget_get_text_value(GtkWidget *widget, int type)
{
    if (widget == NULL) {
        g_warning("widget_get_text_value: widget NULL");
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

    case WIDGET_MENUBAR:
        return widget_menubar_envvar_construct(widget);

    case WIDGET_MENUITEMSEPARATOR:
    case WIDGET_MENUITEM:
    case WIDGET_MENU:
        return widget_menuitem_envvar_construct(widget);

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

    case WIDGET_PASSWORD:
        return widget_password_envvar_construct(widget);

    case WIDGET_SWITCH:
        return widget_switch_envvar_construct(widget);

    case WIDGET_SEARCHENTRY:
        return widget_searchentry_envvar_construct(widget);

    case WIDGET_INFOBAR:
        return widget_infobar_envvar_construct(widget);

    case WIDGET_CALENDAR:
        return widget_calendar_envvar_construct(widget);

    case WIDGET_ASPECTFRAME:
        return widget_aspectframe_envvar_construct(widget);

    case WIDGET_SPINNER:
        return widget_spinner_envvar_construct(widget);

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

    case WIDGET_FILECHOOSER:
        return widget_filechooser_envvar_construct(widget);

    case WIDGET_LINKBUTTON:
        return widget_linkbutton_envvar_construct(widget);

    case WIDGET_PULSE:
        return widget_pulse_envvar_construct(widget);

    case WIDGET_IMAGE:
        return widget_image_envvar_construct(widget);

    case WIDGET_LEVELBAR:
        return widget_levelbar_envvar_construct(widget);

    case WIDGET_DRAWINGAREA:
        return widget_drawingarea_envvar_construct(widget);

    case WIDGET_SCROLLEDW:
    case WIDGET_CHOOSER:
        /* Widgets reconnus par la grammaire mais sans porteur dans ce
         * backend : chaine vide, pas d'erreur. */
        return g_strdup("");

    default:
        return NULL;
    }
}

/* -------------------------------------------------------------------- */
/* Fonction 4 — nom lisible d'un type de widget                         */
/* -------------------------------------------------------------------- */

char *widgets_to_str(int itype)
{
    switch (itype) {
    case WIDGET_CANCELBUTTON:       return "CANCELBUTTON";
    case WIDGET_HELPBUTTON:         return "HELPBUTTON";
    case WIDGET_NOBUTTON:           return "NOBUTTON";
    case WIDGET_OKBUTTON:           return "OKBUTTON";
    case WIDGET_YESBUTTON:          return "YESBUTTON";
    case WIDGET_BUTTON:             return "BUTTON";
    case WIDGET_CHECKBOX:           return "CHECKBOX";
    case WIDGET_COLORBUTTON:        return "COLORBUTTON";
    case WIDGET_COMBOBOX:           return "COMBOBOX";
    case WIDGET_COMBOBOXENTRY:      return "COMBOBOXENTRY";
    case WIDGET_COMBOBOXTEXT:       return "COMBOBOXTEXT";
    case WIDGET_EDIT:               return "EDIT";
    case WIDGET_ENTRY:              return "ENTRY";
    case WIDGET_EVENTBOX:           return "EVENTBOX";
    case WIDGET_GRID:               return "GRID";
    case WIDGET_PANED:              return "PANED";
    case WIDGET_TOOLBAR:            return "TOOLBAR";
    case WIDGET_STACK_PAGES:        return "STACK";
    case WIDGET_WIZARD:             return "WIZARD";
    case WIDGET_MENUBUTTON:         return "MENUBUTTON";
    case WIDGET_FLOWBOX:            return "FLOWBOX";
    case WIDGET_OVERLAY:            return "OVERLAY";
    case WIDGET_REVEALER:           return "REVEALER";
    case WIDGET_EXPANDER:           return "EXPANDER";
    case WIDGET_SWITCH:             return "SWITCH";
    case WIDGET_FILECHOOSER:        return "FILECHOOSER";
    case WIDGET_CALENDAR:           return "CALENDAR";
    case WIDGET_LINKBUTTON:         return "LINKBUTTON";
    case WIDGET_SEARCHENTRY:        return "SEARCHENTRY";
    case WIDGET_INFOBAR:            return "INFOBAR";
    case WIDGET_SPINNER:            return "SPINNER";
    case WIDGET_IMAGE:              return "IMAGE";
    case WIDGET_PULSE:              return "PULSE";
    case WIDGET_PASSWORD:           return "PASSWORD";
    case WIDGET_ASPECTFRAME:        return "ASPECTFRAME";
    case WIDGET_FONTBUTTON:         return "FONTBUTTON";
    case WIDGET_FRAME:              return "FRAME";
    case WIDGET_HBOX:               return "HBOX";
    case WIDGET_HSCALE:             return "HSCALE";
    case WIDGET_HSEPARATOR:         return "HSEPARATOR";
    case WIDGET_LIST:               return "LIST";
    case WIDGET_MENU:               return "MENU";
    case WIDGET_MENUBAR:            return "MENUBAR";
    case WIDGET_MENUITEM:           return "MENUITEM";
    case WIDGET_MENUITEMSEPARATOR:  return "MENUITEMSEPARATOR";
    case WIDGET_NOTEBOOK:           return "NOTEBOOK";
    case WIDGET_PIXMAP:             return "PIXMAP";
    case WIDGET_PROGRESSBAR:        return "PROGRESSBAR";
    case WIDGET_RADIOBUTTON:        return "RADIOBUTTON";
    case WIDGET_SPINBUTTON:         return "SPINBUTTON";
    case WIDGET_STATUSBAR:          return "STATUSBAR";
    case WIDGET_TABLE:              return "TABLE";
    case WIDGET_TERMINAL:           return "TERMINAL";
    case WIDGET_TEXT:               return "TEXT";
    case WIDGET_TIMER:              return "TIMER";
    case WIDGET_TOGGLEBUTTON:       return "TOGGLEBUTTON";
    case WIDGET_TREE:               return "TREE";
    case WIDGET_VBOX:               return "VBOX";
    case WIDGET_VSCALE:             return "VSCALE";
    case WIDGET_VSEPARATOR:         return "VSEPARATOR";
    case WIDGET_WINDOW:             return "WINDOW";
    case WIDGET_LEVELBAR:           return "LEVELBAR";
    case WIDGET_DRAWINGAREA:        return "DRAWINGAREA";
    case WIDGET_SCROLLEDW:          return "SCROLLEDW";
    default:                        return "THINGY";
    }
}

/* -------------------------------------------------------------------- */
/* Fonction 5 — connexion des signaux (sans objet dans ce backend)      */
/* -------------------------------------------------------------------- */

gboolean widget_connect_signals(GtkWidget *widget, AttributeSet *Attr)
{
    (void)widget;
    (void)Attr;

    /* Ce backend recalcule tout a chaque rendu : pas de systeme de
     * signaux/callbacks differe a brancher ici. */
    return TRUE;
}

/* -------------------------------------------------------------------- */
/* Fonctions 6 et 7 — visibilite initiale d'un widget                   */
/* -------------------------------------------------------------------- */

void widget_visibility_list_add(GtkWidget *widget, tag_attr *attr)
{
    gboolean hide = FALSE;

    if (widget == NULL) {
        return;
    }

    if (attr != NULL) {
        char *space_expand = get_tag_attribute(attr, "space-expand");
        char *visible = get_tag_attribute(attr, "visible");

        if (space_expand != NULL) {
            if (strcasecmp(space_expand, "true") == 0 ||
                strcasecmp(space_expand, "yes") == 0 ||
                strcmp(space_expand, "1") == 0) {
                ((WidgetNode *)widget)->expand = 1;
            } else {
                ((WidgetNode *)widget)->expand = -1;
            }
        }

        if (visible != NULL &&
            (strcasecmp(visible, "false") == 0 ||
             strcasecmp(visible, "no") == 0 ||
             strcmp(visible, "0") == 0)) {
            hide = TRUE;
        }
    }

    if (hide) {
        widget_hide_list = g_list_append(widget_hide_list, widget);
    } else {
        widget_show_list = g_list_append(widget_show_list, widget);
    }
}

void widget_show_all(void)
{
    GList *element;

    for (element = widget_show_list; element != NULL; element = element->next) {
        /* Deja vrai par defaut a la construction ; reaffirme par
         * robustesse, sans que ce soit necessaire. */
        ((WidgetNode *)element->data)->visible = TRUE;
    }
    g_list_free(widget_show_list);
    widget_show_list = NULL;

    for (element = widget_hide_list; element != NULL; element = element->next) {
        /* Correctif : c'est bien le champ "visible" qui doit changer,
         * jamais "tooltip". */
        ((WidgetNode *)element->data)->visible = FALSE;
    }
    g_list_free(widget_hide_list);
    widget_hide_list = NULL;
}
