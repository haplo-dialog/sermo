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
#include <QtWidgets/QBoxLayout>
#include <QtCore/QVector>
#include <algorithm>
#include <QtWidgets/QLayout>
#include <QtWidgets/QPushButton>
#include <QtWidgets/QToolButton>
#include <QtCore/QList>

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

/* Table associative (étirement Qt6) : mémorise, par widget, ce que SA
   balise dit de space-expand et space-fill — 1 oui, 0 non, -1 « non dit ».
   Le « non dit » compte : une boîte gtk3 ne remplace sa valeur courante que
   si l'enfant en a une (voir qt6_box_construire). Statique à ce fichier :
   on n'y accède que par qt6_layout_register()/qt6_layout_get_expand() et
   qt6_box_construire(). */
struct Qt6Etirement {
    int expand;
    int fill;
};
std::map<QWidget *, Qt6Etirement> qt6_layout_expand_map;

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

extern "C" gint widget_input_bool(AttributeSet *Attr)
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
/* Fonction 1 quater — la PREMIÈRE ligne des directives <input>            */
/* ---------------------------------------------------------------------- */
/* Règle de l'étalon gtk3sermo pour les widgets à une valeur (calendar,
 * colorbutton, fontbutton, searchentry, statusbar, infobar, linkbutton,
 * notebook, entry) : la première ligne, coupée au premier CR ou LF. Une ligne
 * vide est une ligne (rendue ""). NULL si rien n'est lisible : pas de
 * directive, ou une sortie VIDE (aucune ligne) — le widget ne change pas.
 * Chaîne à libérer par g_free. (2026-10-13 : jusque-là, aucun de ces widgets
 * sauf <entry> ne lisait son <input> dans ce port.) */
extern "C" gchar *widget_input_line(AttributeSet *Attr)
{
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

/* Lit un booléen de balise comme gtk3 : true/yes/1 → 1, toute autre valeur
 * présente → 0, absent → -1. */
static int qt6_lire_tristate(tag_attr *attr, const char *nom)
{
    if (attr == NULL)
        return -1;
    const gchar *value = get_tag_attribute(attr, nom);
    if (value == NULL)
        return -1;
    if (g_ascii_strcasecmp(value, "true") == 0
        || g_ascii_strcasecmp(value, "yes") == 0
        || atoi(value) == 1)
        return 1;
    return 0;
}

extern "C" void qt6_layout_register(GtkWidget *widget, tag_attr *attr)
{
    if (widget == NULL)
        return;

    Qt6Etirement e;
    e.expand = qt6_lire_tristate(attr, "space-expand");
    e.fill   = qt6_lire_tristate(attr, "space-fill");
    qt6_layout_expand_map[reinterpret_cast<QWidget *>(widget)] = e;
}

extern "C" int qt6_layout_get_expand(GtkWidget *widget)
{
    if (widget == NULL)
        return 0;

    std::map<QWidget *, Qt6Etirement>::const_iterator it =
        qt6_layout_expand_map.find(reinterpret_cast<QWidget *>(widget));

    if (it != qt6_layout_expand_map.end() && it->second.expand == 1)
        return 1;

    return 0;
}

/* ---------------------------------------------------------------------- */
/* Fonction 10 — construction d'une boîte <hbox>/<vbox> comme gtk3         */
/* ---------------------------------------------------------------------- */

namespace {

/* homogeneous="true" : chaque enfant VISIBLE reçoit une case de même taille
 * sur l'axe de la boîte, et les cases couvrent TOUTE la boîte (gtk3 :
 * GtkBox:homogeneous). Un QBoxLayout ne sait pas le faire : à facteurs égaux,
 * Qt garde au moins la taille propre de chacun, et la taille naturelle de la
 * boîte est la SOMME des tailles propres, pas n fois la plus grande. Un
 * enfant qui ne remplit pas sa case y est centré par son alignement
 * (QWidgetItem::setGeometry s'en charge). */
class Qt6CasesEgales : public QLayout
{
public:
    Qt6CasesEgales(QWidget *parent, Qt::Orientation o) : QLayout(parent), m_o(o) {}
    ~Qt6CasesEgales() override
    {
        QLayoutItem *item;
        while ((item = takeAt(0)) != nullptr)
            delete item;
    }

    void addItem(QLayoutItem *item) override { m_items.append(item); }
    int count() const override { return m_items.size(); }
    QLayoutItem *itemAt(int i) const override
    {
        return (i >= 0 && i < m_items.size()) ? m_items.at(i) : nullptr;
    }
    QLayoutItem *takeAt(int i) override
    {
        return (i >= 0 && i < m_items.size()) ? m_items.takeAt(i) : nullptr;
    }
    Qt::Orientations expandingDirections() const override { return Qt::Orientations(); }

    QSize sizeHint() const override { return calculer(false); }
    QSize minimumSize() const override { return calculer(true); }

    void setGeometry(const QRect &rect) override
    {
        QLayout::setGeometry(rect);
        QRect r = rect.marginsRemoved(contentsMargins());
        int n = visibles();
        if (n == 0)
            return;
        int esp   = spacing() > 0 ? spacing() : 0;
        int total = (m_o == Qt::Horizontal ? r.width() : r.height()) - esp * (n - 1);
        if (total < 0)
            total = 0;
        int base  = total / n;
        int reste = total % n;
        int pos   = (m_o == Qt::Horizontal ? r.x() : r.y());
        int k = 0;
        for (QLayoutItem *item : m_items) {
            if (item->isEmpty())
                continue;
            /* Le reste de la division va aux premières cases, un pixel
             * chacune, comme gtk_box_size_allocate. */
            int taille = base + (k < reste ? 1 : 0);
            if (m_o == Qt::Horizontal)
                item->setGeometry(QRect(pos, r.y(), taille, r.height()));
            else
                item->setGeometry(QRect(r.x(), pos, r.width(), taille));
            pos += taille + esp;
            ++k;
        }
    }

private:
    int visibles() const
    {
        int n = 0;
        for (QLayoutItem *item : m_items)
            if (!item->isEmpty())
                ++n;
        return n;
    }

    QSize calculer(bool minimum) const
    {
        int n = 0, plusGrand = 0, travers = 0;
        for (QLayoutItem *item : m_items) {
            if (item->isEmpty())
                continue;
            QSize t = minimum ? item->minimumSize() : item->sizeHint();
            int axe = (m_o == Qt::Horizontal ? t.width() : t.height());
            int crx = (m_o == Qt::Horizontal ? t.height() : t.width());
            if (axe > plusGrand) plusGrand = axe;
            if (crx > travers)   travers   = crx;
            ++n;
        }
        int esp = spacing() > 0 ? spacing() : 0;
        int axe = n > 0 ? n * plusGrand + esp * (n - 1) : 0;
        QMargins m = contentsMargins();
        if (m_o == Qt::Horizontal)
            return QSize(axe + m.left() + m.right(), travers + m.top() + m.bottom());
        return QSize(travers + m.left() + m.right(), axe + m.top() + m.bottom());
    }

    Qt::Orientation      m_o;
    QList<QLayoutItem *> m_items;
};

/* Boîte NON homogène, partage EXACT de gtk3 (gtk_box_size_allocate) :
 *  1. chaque enfant visible reçoit d'abord sa taille minimale ;
 *  2. le reste monte les enfants vers leur taille naturelle, à parts égales
 *     en commençant par ceux à qui il manque le moins
 *     (gtk_distribute_natural_allocation) ;
 *  3. le surplus restant est divisé à parts ÉGALES entre les enfants étirés
 *     (le reste de la division, un pixel chacun, aux premiers) ;
 *  4. un enfant étiré sans fill garde sa taille et est centré dans sa place ;
 *  5. sans enfant étiré, le surplus reste libre : en TÊTE pour une rangée
 *     (gtk_box_pack_end → calée à droite), en queue pour une colonne.
 * Un QBoxLayout, lui, donne des tailles PROPORTIONNELLES au facteur : dans
 * 03-onglets, notebook et bouton OK étirés coupaient la colonne moitié-moitié
 * (190/190 px) là où gtk3 donne ~260/100 (naturel + même part du surplus).
 * Travers : l'enfant reçoit toute la largeur (ou hauteur) ; un widget à taille
 * bornée y est centré par QWidgetItem::setGeometry. */
class Qt6BoiteGtk : public QLayout
{
public:
    Qt6BoiteGtk(QWidget *parent, Qt::Orientation o, bool calerFin)
        : QLayout(parent), m_o(o), m_calerFin(calerFin) {}
    ~Qt6BoiteGtk() override
    {
        QLayoutItem *item;
        while ((item = takeAt(0)) != nullptr)
            delete item;
    }

    void ajouter(QWidget *w, bool etire, bool remplit)
    {
        m_etireSuivant   = etire;
        m_remplitSuivant = remplit;
        addWidget(w);
    }

    void addItem(QLayoutItem *item) override
    {
        Enfant e;
        e.item    = item;
        e.etire   = m_etireSuivant;
        e.remplit = m_remplitSuivant;
        m_enfants.append(e);
        m_etireSuivant = m_remplitSuivant = false;
    }
    int count() const override { return m_enfants.size(); }
    QLayoutItem *itemAt(int i) const override
    {
        return (i >= 0 && i < m_enfants.size()) ? m_enfants.at(i).item : nullptr;
    }
    QLayoutItem *takeAt(int i) override
    {
        if (i < 0 || i >= m_enfants.size())
            return nullptr;
        return m_enfants.takeAt(i).item;
    }

    /* Comme gtk_widget_compute_expand : une boîte qui contient un enfant étiré
     * demande elle-même à s'étirer sur cet axe ; plus ce que demandent les
     * enfants (politiques Expanding), comme le faisait le QBoxLayout. */
    Qt::Orientations expandingDirections() const override
    {
        Qt::Orientations d;
        for (const Enfant &e : m_enfants) {
            if (e.item->isEmpty())
                continue;
            d |= e.item->expandingDirections();
            if (e.etire)
                d |= m_o;
        }
        return d;
    }

    QSize sizeHint() const override { return calculer(false); }
    QSize minimumSize() const override { return calculer(true); }

    void setGeometry(const QRect &rect) override
    {
        QLayout::setGeometry(rect);
        QRect r = rect.marginsRemoved(contentsMargins());
        bool horiz = (m_o == Qt::Horizontal);

        QList<int> vis;
        for (int i = 0; i < m_enfants.size(); ++i)
            if (!m_enfants.at(i).item->isEmpty())
                vis.append(i);
        int n = vis.size();
        if (n == 0)
            return;

        int esp   = spacing() > 0 ? spacing() : 0;
        int reste = (horiz ? r.width() : r.height()) - esp * (n - 1);

        QVector<int> taille(n), naturel(n);
        int nEtires = 0;
        for (int k = 0; k < n; ++k) {
            QLayoutItem *it = m_enfants.at(vis[k]).item;
            int mn = horiz ? it->minimumSize().width() : it->minimumSize().height();
            int nt = horiz ? it->sizeHint().width()    : it->sizeHint().height();
            if (nt < mn) nt = mn;
            taille[k]  = mn;
            naturel[k] = nt;
            reste     -= mn;
            if (m_enfants.at(vis[k]).etire)
                ++nEtires;
        }

        /* 2. vers la taille naturelle — gtk_distribute_natural_allocation :
         * tri par écart croissant, puis part = (reste + i) / (i + 1) en
         * remontant, plafonnée à l'écart. */
        if (reste > 0) {
            QVector<int> ordre(n);
            for (int k = 0; k < n; ++k) ordre[k] = k;
            std::stable_sort(ordre.begin(), ordre.end(), [&](int a, int b) {
                return (naturel[a] - taille[a]) < (naturel[b] - taille[b]);
            });
            for (int i = n - 1; reste > 0 && i >= 0; --i) {
                int k     = ordre[i];
                int ecart = naturel[k] - taille[k];
                int part  = (reste + i) / (i + 1);
                int donne = part < ecart ? part : ecart;
                taille[k] += donne;
                reste     -= donne;
            }
        }
        if (reste < 0)
            reste = 0;

        /* 3. surplus à parts égales entre les étirés. */
        int base = 0, surplusPixels = 0, libre = reste;
        if (nEtires > 0) {
            base          = reste / nEtires;
            surplusPixels = reste % nEtires;
            libre         = 0;
        }

        /* 5. sans étiré, la place libre va en tête (rangée) ou en queue. */
        int pos = horiz ? r.x() : r.y();
        if (m_calerFin)
            pos += libre;

        for (int k = 0; k < n; ++k) {
            const Enfant &e = m_enfants.at(vis[k]);
            int place = taille[k];
            if (e.etire) {
                place += base;
                if (surplusPixels > 0) { ++place; --surplusPixels; }
            }
            int dim = place, decal = 0;
            if (e.etire && !e.remplit) {
                /* 4. garde sa taille, centré dans la place reçue. */
                dim   = taille[k];
                decal = (place - dim) / 2;
            }
            if (horiz)
                e.item->setGeometry(QRect(pos + decal, r.y(), dim, r.height()));
            else
                e.item->setGeometry(QRect(r.x(), pos + decal, r.width(), dim));
            pos += place + esp;
        }
    }

private:
    struct Enfant {
        QLayoutItem *item;
        bool         etire;
        bool         remplit;
    };

    QSize calculer(bool minimum) const
    {
        int n = 0, axe = 0, travers = 0;
        for (const Enfant &e : m_enfants) {
            if (e.item->isEmpty())
                continue;
            QSize t = minimum ? e.item->minimumSize() : e.item->sizeHint();
            axe += (m_o == Qt::Horizontal ? t.width() : t.height());
            int c = (m_o == Qt::Horizontal ? t.height() : t.width());
            if (c > travers) travers = c;
            ++n;
        }
        int esp = spacing() > 0 ? spacing() : 0;
        if (n > 1) axe += esp * (n - 1);
        QMargins m = contentsMargins();
        if (m_o == Qt::Horizontal)
            return QSize(axe + m.left() + m.right(), travers + m.top() + m.bottom());
        return QSize(travers + m.left() + m.right(), axe + m.top() + m.bottom());
    }

    Qt::Orientation m_o;
    bool            m_calerFin;
    bool            m_etireSuivant   = false;
    bool            m_remplitSuivant = false;
    QList<Enfant>   m_enfants;
};

/* Un bouton Qt a une hauteur FIXE ; un bouton gtk3 prend toute la place que
 * sa boîte lui donne (une rangée étirée → des boutons hauts, étalon t2). On
 * lui permet donc de grandir dans le sens demandé. Seuls les boutons-poussoirs
 * sont concernés : une barre, un curseur, un champ, une case à cocher gardent
 * leur hauteur propre, centrés dans leur place. Une taille imposée
 * (width-request/height-request → taille min/max) reste prioritaire. */
void qt6_bouton_peut_grandir(QWidget *w, Qt::Orientation o)
{
    if (w == nullptr)
        return;
    if (qobject_cast<QPushButton *>(w) == nullptr
        && qobject_cast<QToolButton *>(w) == nullptr)
        return;
    QSizePolicy p = w->sizePolicy();
    if (o == Qt::Vertical && p.verticalPolicy() == QSizePolicy::Fixed)
        p.setVerticalPolicy(QSizePolicy::Preferred);
    else if (o == Qt::Horizontal && p.horizontalPolicy() == QSizePolicy::Fixed)
        p.setHorizontalPolicy(QSizePolicy::Preferred);
    else
        return;
    w->setSizePolicy(p);
}

} // namespace

/* Construit une <hbox> (horizontal) ou une <vbox> et y range les enfants
 * dépilés, avec les règles EXACTES de gtk3 (widget_hbox.c / widget_vbox.c) :
 *
 *  - valeur courante (expand, fill) = celle de la boîte, sinon celle du
 *    projet (--space-expand/--space-fill), sinon « non dit » ;
 *  - parcours des enfants : colonne dans l'ordre, RANGÉE à l'envers ; un
 *    enfant qui a son propre space-expand/space-fill remplace la valeur
 *    courante pour lui ET pour les suivants du parcours (jamais remise à zéro
 *    — l'étalon le fait, c'est voulu) ;
 *  - valeur « non dit » → défaut du type : colonne edit/frame/fenêtre
 *    défilante/paned, rangée edit/frame/fenêtre défilante/entry s'étirent ;
 *  - rangée : gtk_box_pack_end — ce qui ne s'étire pas se cale à DROITE ;
 *    colonne : gtk_box_pack_start — en haut ;
 *  - étiré sans fill → centré dans la place reçue ;
 *  - homogeneous="true" → cases égales sur toute la longueur.
 *
 * Traduction Qt : disposition maison Qt6BoiteGtk (partage exact de gtk3 :
 * naturel + surplus à parts égales entre étirés) ou Qt6CasesEgales
 * (homogène). */
QWidget *qt6_box_construire(tag_attr *attr, bool horizontal, int spacing)
{
    QWidget *box = new QWidget();
    qt6_box_ranger(box, attr, horizontal, spacing);
    return box;
}

/* Pose sur « box » la disposition gtk3 et y range les enfants dépilés.
 * attr = balise de la boîte (valeur courante de départ, homogeneous) ; NULL
 * pour un contenant qui n'a pas de réglage propre — le <frame> : gtk3 part
 * alors de la valeur du PROJET seule (widget_frame.c). Rend la disposition
 * (marges 0) pour que l'appelant ajuste marges et espacement. */
QLayout *qt6_box_ranger(QWidget *box, tag_attr *attr, bool horizontal, int spacing)
{
    bool homogene = (qt6_lire_tristate(attr, "homogeneous") == 1);
    Qt::Orientation axe = horizontal ? Qt::Horizontal : Qt::Vertical;

    Qt6BoiteGtk    *boite = nullptr;
    Qt6CasesEgales *cases = nullptr;
    QLayout        *layout;
    if (homogene) {
        cases  = new Qt6CasesEgales(box, axe);
        layout = cases;
    } else {
        /* Rangée : pack_end, la place libre va en tête (calage à droite). */
        boite  = new Qt6BoiteGtk(box, axe, horizontal);
        layout = boite;
    }
    layout->setContentsMargins(0, 0, 0, 0);
    layout->setSpacing(spacing);

    int cur_expand = project_space_expand;
    int cur_fill   = project_space_fill;
    int v;
    if ((v = qt6_lire_tristate(attr, "space-expand")) != -1) cur_expand = v;
    if ((v = qt6_lire_tristate(attr, "space-fill"))   != -1) cur_fill   = v;

    /* Le parser émet push(new box(pop())) : l'instruction SUM a regroupé TOUS
     * les enfants frères dans UN seul stackelement. On dépile une fois et on
     * les ré-parente (sinon chacun devient une fenêtre top-level). */
    stackelement s = pop();
    int n_enf = s.nwidgets;
    if (n_enf > MAXWIDGETS) n_enf = MAXWIDGETS;
    if (n_enf < 0) n_enf = 0;

    bool etire[MAXWIDGETS];
    bool remplit[MAXWIDGETS];
    for (int k = 0; k < n_enf; ++k) {
        int n = horizontal ? (n_enf - 1 - k) : k;
        etire[n] = remplit[n] = false;
        if (!s.widgets[n]) continue;

        std::map<QWidget *, Qt6Etirement>::const_iterator it =
            qt6_layout_expand_map.find(reinterpret_cast<QWidget *>(s.widgets[n]));
        if (it != qt6_layout_expand_map.end()) {
            if (it->second.expand != -1) cur_expand = it->second.expand;
            if (it->second.fill   != -1) cur_fill   = it->second.fill;
        }

        int t = s.widgettypes[n];
        bool defaut = (t == WIDGET_EDIT || t == WIDGET_FRAME || t == WIDGET_SCROLLEDW
                       || (horizontal ? t == WIDGET_ENTRY : t == WIDGET_PANED));
        etire[n]   = (cur_expand != -1) ? (cur_expand != 0) : defaut;
        remplit[n] = (cur_fill   != -1) ? (cur_fill   != 0) : defaut;
    }

    Qt::Alignment centre = horizontal ? Qt::AlignHCenter : Qt::AlignVCenter;
    for (int n = 0; n < n_enf; ++n) {
        if (!s.widgets[n]) continue;
        QWidget *w = static_cast<QWidget *>(s.widgets[n]);

        /* Travers : gtk3 remplit toujours (une rangée de 120 px donne des
         * boutons de 120 px). Axe : seulement si l'enfant remplit sa place. */
        if (horizontal)
            qt6_bouton_peut_grandir(w, Qt::Vertical);
        if (remplit[n] && (homogene || etire[n]))
            qt6_bouton_peut_grandir(w, axe);

        if (homogene) {
            /* En homogène, gtk3 donne la case à TOUS, étirés ou non : seul
             * fill décide si l'enfant la remplit ou y est centré. */
            cases->addWidget(w);
            if (!remplit[n])
                cases->setAlignment(w, centre);
            continue;
        }
        boite->ajouter(w, etire[n], remplit[n]);
    }

    return layout;
}
