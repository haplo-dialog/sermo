// SPDX-License-Identifier: GPL-2.0-or-later
/*
 * widgets.cpp — dispatch des widgets Qt6 (implémentation indépendante)
 *
 * Copyright (C) 2026 S. Cage
 *
 * Façade haplo-dialog <devel@haplo-dialog.fr>
 */

#include "qt6-compat.h"
#include "gtk3d.h"
#include "safe_exec.h"
#include "widgets.h"
#include "stringman.h"
#include "tag_attributes.h"
#include "attributes.h"
#include "macros.h"
#include "signals.h"

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
#include "widget_linkbutton.h"
#include "widget_pulse.h"
#include "widget_spinner.h"
#include "widget_aspectframe.h"
#include "widget_filechooser.h"
#include "sermo_input.h"

#include <QtWidgets/QWidget>

#include <cstdio>
#include <cstdlib>
#include <cstring>
#include <map>
#include <string>

/* Chemin a sourcer avant toute commande shell, ou NULL. Declaree ailleurs
   dans le backend. */
extern gchar *option_include_file;

/* Listes globales consultees par le reste du backend : widgets en attente
   d'un show()/hide() differe par widget_show_all(). */
GList *widget_show_list = NULL;
GList *widget_hide_list = NULL;

namespace {

/* Table associative (facteur d'etirement Qt6) : mémorise, par widget, s'il
   doit s'étirer dans son QHBoxLayout/QVBoxLayout parent. Statique à ce
   fichier : rien d'autre dans le backend n'a besoin d'y accéder
   directement, seulement via qt6_layout_register()/qt6_layout_get_expand(). */
std::map<QWidget *, bool> qt6_layout_expand_map;

} // namespace

/* ---------------------------------------------------------------------- */
/* Fonction 1 — ouverture d'une commande shell pour un widget "a valeur"   */
/* ---------------------------------------------------------------------- */

extern "C" FILE *widget_opencommand(const char *command)
{
    FILE *stream = NULL;

    if (option_include_file != NULL) {
        gchar *full_command = g_strdup_printf(". %s; %s", option_include_file, command);

        PIP_DEBUG("widget_opencommand: opening \"%s\"\n", full_command);
        stream = safe_popen(full_command);
        g_free(full_command);
    } else {
        PIP_DEBUG("widget_opencommand: opening \"%s\"\n", command);
        stream = safe_popen(command);
    }

    if (stream == NULL) {
        g_warning("widget_opencommand: could not open command \"%s\"", command);
        return NULL;
    }

    return stream;
}

/* ---------------------------------------------------------------------- */
/* Fonction 1 bis — contenu des directives <input> d'une balise            */
/* ---------------------------------------------------------------------- */
/* Même règle que fltk1, efl1, sdl3 et ncurses : la première directive
 * reconnue, « Command: » (sortie de la commande) ou « file: » (contenu du
 * fichier), rendue EN ENTIER. Au widget d'en garder ce que l'étalon gtk3sermo
 * garde : <entry> la première ligne, <text> tout. NULL si rien n'est lisible.
 * Jusqu'à la 2.7.0, qt6 ne connaissait que la commande, et seulement pour
 * <entry> : <input file> ne lisait rien. */
extern "C" gchar *widget_input_text(AttributeSet *Attr)
{
    if (Attr == NULL)
        return NULL;

    GList *element = NULL;
    for (gchar *value = attributeset_get_first(&element, Attr, ATTR_INPUT);
         value != NULL;
         value = attributeset_get_next(&element, Attr, ATTR_INPUT)) {

        FILE *stream = NULL;
        bool commande = input_is_shell_command(value);
        if (commande)
            stream = widget_opencommand(input_get_shell_command(value));
        else if (strncasecmp(value, "file:", 5) == 0 && value[5] != '\0') {
            stream = sermo_fopen_input(value + 5);
            if (stream == NULL)
                g_warning("widget_input_text: impossible de lire le fichier '%s'", value + 5);
        } else
            continue;   /* directive non reconnue : la suivante */

        if (stream == NULL)
            return NULL;
        std::string contenu;
        char buf[1024];
        while (fgets(buf, sizeof(buf), stream) != NULL)
            contenu += buf;
        fclose(stream);   /* safe_popen() rend un fdopen() : fclose, jamais pclose */
        return g_strdup(contenu.c_str());
    }
    return NULL;
}

/* ---------------------------------------------------------------------- */
/* Fonction 1 ter — les mêmes directives <input>, ligne par ligne          */
/* ---------------------------------------------------------------------- */
/* Pour les widgets qui font une rangée ou un élément par ligne : <list>,
 * <tree>, <table>, <comboboxtext>. Chacun avait sa boucle fgets() sur la
 * directive BRUTE ; un seul découpage, celui de l'étalon gtk3sermo, qui lit
 * par fgets() : le saut de ligne final ferme la dernière ligne sans en ouvrir
 * une vide, et une ligne s'arrête à son premier CR. Au widget de décider du
 * sort des lignes vides. Vecteur terminé par NULL, à libérer par g_strfreev ;
 * NULL si aucune directive n'est lisible (vecteur vide si elle ne rend rien). */
extern "C" gchar **widget_input_lines(AttributeSet *Attr)
{
    gchar *contenu = widget_input_text(Attr);
    if (contenu == NULL)
        return NULL;

    gchar **lignes = g_strsplit(contenu, "\n", -1);
    g_free(contenu);
    if (lignes == NULL)
        return NULL;

    gsize n = 0;
    while (lignes[n] != NULL)
        n++;
    /* « un\ndeux\n » se découpe en « un », « deux » et « » : ce dernier
     * morceau n'est pas une ligne. Testé AVANT de couper aux CR, pour qu'un
     * contenu fini par un CR seul garde sa dernière ligne (vide). */
    if (n > 0 && lignes[n - 1][0] == '\0') {
        g_free(lignes[n - 1]);
        lignes[n - 1] = NULL;
    }
    for (gchar **ligne = lignes; *ligne != NULL; ligne++)
        (*ligne)[strcspn(*ligne, "\r")] = '\0';
    return lignes;
}

/* ---------------------------------------------------------------------- */
/* Fonction 2 — valeur numérique lue depuis <input> (commande ou fichier)  */
/* ---------------------------------------------------------------------- */
/* Le début du contenu, lu comme un nombre (g_ascii_strtod : insensible à la
 * locale). Jusqu'à la 2.7.0 seule la commande était décodée : « File:… »
 * partait vers le shell, et <input file> laissait spinbutton, hscale, vscale
 * et levelbar à leur valeur par défaut (mesuré : 0 au lieu de 7). */

extern "C" double widget_command_value(AttributeSet *Attr, double fallback)
{
    gchar *text = widget_input_text(Attr);

    if (text == NULL)
        return fallback;

    double result = fallback;
    char *endptr = NULL;
    double parsed = g_ascii_strtod(text, &endptr);

    if (endptr != text)
        result = parsed;
    g_free(text);

    return result;
}

/* ---------------------------------------------------------------------- */
/* Fonction 3 — représentation texte courante d'un widget                 */
/* ---------------------------------------------------------------------- */

extern "C" char *widget_get_text_value(GtkWidget *widget, int type)
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
    case WIDGET_TOGGLEBUTTON: /* regroupe volontairement avec les boutons */
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
        return widget_spinner_envvar_construct(widget);

    case WIDGET_PULSE:
        return widget_pulse_envvar_construct(widget);

    case WIDGET_ASPECTFRAME:
        return widget_aspectframe_envvar_construct(widget);

    case WIDGET_FILECHOOSER:
        return widget_filechooser_envvar_construct(widget);

    case WIDGET_MENUITEM:
        return widget_menuitem_envvar_construct(widget);

    case WIDGET_LINKBUTTON:
        return widget_linkbutton_envvar_construct(widget);

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

    case WIDGET_LEVELBAR:
        return widget_levelbar_envvar_construct(widget);

    case WIDGET_DRAWINGAREA:
        return widget_drawingarea_envvar_construct(widget);

    /* ⚠️ Groupe « rien à exporter » : ces cas CHUTENT jusqu'au return
     * commun. N'insérer aucun case AVANT le return sans le vouloir — un
     * WIDGET_LEVELBAR glissé ici a fait passer un <eventbox> par
     * l'exportateur de la barre, qui l'a lu comme une QProgressBar :
     * EB="722734" au lieu de "" (attrapé par le banc, cas 25). */
    case WIDGET_EVENTBOX:
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

/* ---------------------------------------------------------------------- */
/* Fonction 4 — nom lisible d'un type de widget                           */
/* ---------------------------------------------------------------------- */

extern "C" char *widgets_to_str(int itype)
{
    switch (itype) {
    case WIDGET_CANCELBUTTON:       return (char *) "CANCELBUTTON";
    case WIDGET_HELPBUTTON:         return (char *) "HELPBUTTON";
    case WIDGET_NOBUTTON:           return (char *) "NOBUTTON";
    case WIDGET_OKBUTTON:           return (char *) "OKBUTTON";
    case WIDGET_YESBUTTON:          return (char *) "YESBUTTON";
    case WIDGET_BUTTON:             return (char *) "BUTTON";
    case WIDGET_CHECKBOX:           return (char *) "CHECKBOX";
    case WIDGET_COLORBUTTON:        return (char *) "COLORBUTTON";
    case WIDGET_COMBOBOX:           return (char *) "COMBOBOX";
    case WIDGET_COMBOBOXENTRY:      return (char *) "COMBOBOXENTRY";
    case WIDGET_COMBOBOXTEXT:       return (char *) "COMBOBOXTEXT";
    case WIDGET_EDIT:               return (char *) "EDIT";
    case WIDGET_ENTRY:              return (char *) "ENTRY";
    case WIDGET_EVENTBOX:           return (char *) "EVENTBOX";
    case WIDGET_EXPANDER:           return (char *) "EXPANDER";
    case WIDGET_SWITCH:             return (char *) "SWITCH";
    case WIDGET_FILECHOOSER:        return (char *) "FILECHOOSER";
    case WIDGET_CALENDAR:           return (char *) "CALENDAR";
    case WIDGET_GRID:               return (char *) "GRID";
    case WIDGET_PANED:              return (char *) "PANED";
    case WIDGET_TOOLBAR:            return (char *) "TOOLBAR";
    case WIDGET_STACK_PAGES:        return (char *) "STACK";
    case WIDGET_WIZARD:             return (char *) "WIZARD";
    case WIDGET_MENUBUTTON:         return (char *) "MENUBUTTON";
    case WIDGET_FLOWBOX:            return (char *) "FLOWBOX";
    case WIDGET_OVERLAY:            return (char *) "OVERLAY";
    case WIDGET_REVEALER:           return (char *) "REVEALER";
    case WIDGET_LINKBUTTON:         return (char *) "LINKBUTTON";
    case WIDGET_SEARCHENTRY:        return (char *) "SEARCHENTRY";
    case WIDGET_INFOBAR:            return (char *) "INFOBAR";
    case WIDGET_SPINNER:            return (char *) "SPINNER";
    case WIDGET_IMAGE:              return (char *) "IMAGE";
    case WIDGET_PULSE:              return (char *) "PULSE";
    case WIDGET_PASSWORD:           return (char *) "PASSWORD";
    case WIDGET_ASPECTFRAME:        return (char *) "ASPECTFRAME";
    case WIDGET_FONTBUTTON:         return (char *) "FONTBUTTON";
    case WIDGET_FRAME:              return (char *) "FRAME";
    case WIDGET_HBOX:               return (char *) "HBOX";
    case WIDGET_HSCALE:             return (char *) "HSCALE";
    case WIDGET_HSEPARATOR:         return (char *) "HSEPARATOR";
    case WIDGET_LIST:               return (char *) "LIST";
    case WIDGET_MENU:               return (char *) "MENU";
    case WIDGET_MENUBAR:            return (char *) "MENUBAR";
    case WIDGET_MENUITEM:           return (char *) "MENUITEM";
    case WIDGET_MENUITEMSEPARATOR:  return (char *) "MENUITEMSEPARATOR";
    case WIDGET_NOTEBOOK:           return (char *) "NOTEBOOK";
    case WIDGET_PIXMAP:             return (char *) "PIXMAP";
    case WIDGET_PROGRESSBAR:        return (char *) "PROGRESSBAR";
    case WIDGET_RADIOBUTTON:        return (char *) "RADIOBUTTON";
    case WIDGET_SPINBUTTON:         return (char *) "SPINBUTTON";
    case WIDGET_STATUSBAR:          return (char *) "STATUSBAR";
    case WIDGET_TABLE:              return (char *) "TABLE";
    case WIDGET_TERMINAL:           return (char *) "TERMINAL";
    case WIDGET_TEXT:               return (char *) "TEXT";
    case WIDGET_TIMER:              return (char *) "TIMER";
    case WIDGET_TOGGLEBUTTON:       return (char *) "TOGGLEBUTTON";
    case WIDGET_TREE:               return (char *) "TREE";
    case WIDGET_VBOX:               return (char *) "VBOX";
    case WIDGET_VSCALE:             return (char *) "VSCALE";
    case WIDGET_VSEPARATOR:         return (char *) "VSEPARATOR";
    case WIDGET_WINDOW:             return (char *) "WINDOW";
    case WIDGET_LEVELBAR:           return (char *) "LEVELBAR";
    case WIDGET_DRAWINGAREA:        return (char *) "DRAWINGAREA";
    case WIDGET_SCROLLEDW:          return (char *) "SCROLLEDW";
    default:                        return (char *) "THINGY";
    }
}

/* ---------------------------------------------------------------------- */
/* Fonction 5 — câblage des signaux (rien à faire dans ce backend)        */
/* ---------------------------------------------------------------------- */

extern "C" gboolean widget_connect_signals(GtkWidget *widget, AttributeSet *Attr)
{
    (void) widget;
    (void) Attr;

    return TRUE;
}

/* ---------------------------------------------------------------------- */
/* Fonctions 6 et 7 — visibilité différée (show()/hide() Qt réels)        */
/* ---------------------------------------------------------------------- */

extern "C" void widget_visibility_list_add(GtkWidget *widget, tag_attr *attr)
{
    if (widget == NULL)
        return;

    /* Le facteur d'etirement se note ICI, et pas ailleurs.
     *
     * qt6_layout_register() existait depuis la modularisation mais n'etait
     * APPELEE NULLE PART : la table restait vide, qt6_layout_get_expand()
     * rendait 0 pour tout, et les hbox/vbox en concluaient qu'aucun enfant
     * n'etait extensible. Ils inseraient donc leur ressort de calage a droite
     * -- prevu pour les rangees d'en-tete -- et n'etiraient personne. Mesure du
     * 2026-09-20 : deux <frame space-expand> cote a cote dans une fenetre de
     * 500 px sortaient minuscules, colles au bord droit, la ou gtk3 leur donne
     * une moitie chacun. C'est ce qui decalait tout system-tools vers la
     * droite dans la galerie du site.
     *
     * Pourquoi cette fonction : le coeur l'appelle a la CREATION de chaque
     * widget (automaton.c), avec ses tag_attr -- juste ce qu'il faut, et assez
     * tot. widget_set_tag_attributes(), lui, ne passe qu'au « realize », quand
     * le parent a deja lu la table : trop tard. L'appel de l'automate est
     * d'ailleurs commente la-bas depuis longtemps. */
    qt6_layout_register(widget, attr);

    gboolean visible = TRUE;

    if (attr != NULL) {
        const gchar *value = get_tag_attribute(attr, "visible");

        if (value != NULL
            && (g_ascii_strcasecmp(value, "false") == 0
                || g_ascii_strcasecmp(value, "no") == 0
                || strcmp(value, "0") == 0)) {
            visible = FALSE;
        }
    }

    if (visible)
        widget_show_list = g_list_append(widget_show_list, widget);
    else
        widget_hide_list = g_list_append(widget_hide_list, widget);
}

extern "C" void widget_show_all(void)
{
    for (GList *item = widget_show_list; item != NULL; item = item->next) {
        QWidget *qwidget = reinterpret_cast<QWidget *>(item->data);

        /* Un enfant est deja affiche en cascade par son parent ; un show()
           explicite ici re-afficherait a tort une page non courante d'un
           QTabWidget qui doit rester masquee. */
        if (qwidget != NULL && qwidget->parentWidget() == NULL)
            qwidget->show();
    }

    g_list_free(widget_show_list);
    widget_show_list = NULL;

    for (GList *item = widget_hide_list; item != NULL; item = item->next) {
        QWidget *qwidget = reinterpret_cast<QWidget *>(item->data);

        /* Cacher un widget explicitement demande cache s'applique toujours,
           y compris une page d'onglet : pas de verification de parent. */
        if (qwidget != NULL)
            qwidget->hide();
    }

    g_list_free(widget_hide_list);
    widget_hide_list = NULL;
}

/* ---------------------------------------------------------------------- */
/* Fonctions 8 et 9 — facteur d'étirement de disposition (Qt6)            */
/* ---------------------------------------------------------------------- */

extern "C" void qt6_layout_register(GtkWidget *widget, tag_attr *attr)
{
    if (widget == NULL)
        return;

    gboolean expand = FALSE;

    if (attr != NULL) {
        const gchar *value = get_tag_attribute(attr, "space-expand");

        if (value != NULL
            && (g_ascii_strcasecmp(value, "true") == 0
                || g_ascii_strcasecmp(value, "yes") == 0
                || atoi(value) == 1)) {
            expand = TRUE;
        }
    }

    qt6_layout_expand_map[reinterpret_cast<QWidget *>(widget)] = (expand != FALSE);
}

extern "C" int qt6_layout_get_expand(GtkWidget *widget)
{
    if (widget == NULL)
        return 0;

    std::map<QWidget *, bool>::const_iterator it =
        qt6_layout_expand_map.find(reinterpret_cast<QWidget *>(widget));

    if (it != qt6_layout_expand_map.end() && it->second)
        return 1;

    return 0;
}
