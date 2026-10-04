/* SPDX-License-Identifier: GPL-2.0-or-later */
#ifndef SERMO_WIDGET_TYPES_H
#define SERMO_WIDGET_TYPES_H

/*
 * widget_types.h — SOURCE UNIQUE des codes de type de widget (WIDGET_*),
 * des opcodes imperatifs et des masques de l'automate.
 *
 * Historiquement ce bloc etait RECOPIE dans chaque automaton.h (coeur,
 * variante GTK4, et les sept backends). Les copies ont DIVERGE : la variante
 * GTK4 numerotait 14 types autrement (PASSWORD, SPINNER, CALENDAR...), un
 * contournement CMake masquait l'ecart. Desormais toutes les copies incluent
 * CE fichier : une seule verite, plus aucune desynchronisation possible.
 *
 * Les valeurs sont internes au binaire (jamais serialisees) : le code aiguille
 * par les NOMS symboliques, donc ces nombres peuvent changer sans danger tant
 * qu'ils restent coherents dans un meme build.
 */

/*                                   -- Type of widget      */
/*                                 xxxxxxxx                 */
#define WIDGET_TYPE              0x00FF0000
#define WIDGET_TEXT              0x00010000
#define WIDGET_ENTRY             0x00020000
#define WIDGET_EDIT              0x00030000
#define WIDGET_BUTTON            0x00040000
#define WIDGET_CHECKBOX          0x00050000
#define WIDGET_RADIOBUTTON       0x00060000
#define WIDGET_LIST              0x00070000
#define WIDGET_TABLE             0x00080000
#define WIDGET_COMBOBOX          0x00090000
#define WIDGET_OKBUTTON          0x000A0000
#define WIDGET_CANCELBUTTON      0x000B0000
#define WIDGET_HELPBUTTON        0x000C0000
#define WIDGET_NOBUTTON          0x000D0000
#define WIDGET_YESBUTTON         0x000E0000
#define WIDGET_SCROLLEDW         0x000F0000
#define WIDGET_VBOX              0x00100000
#define WIDGET_HBOX              0x00200000
#define WIDGET_FRAME             0x00300000
#define WIDGET_NOTEBOOK          0x00310000
#define WIDGET_WINDOW            0x00400000
#define WIDGET_PIXMAP            0x00500000
#define WIDGET_MENUBAR           0x00600000
#define WIDGET_MENU              0x00700000
#define WIDGET_MENUITEM          0x00800000
#define WIDGET_MENUITEMSEPARATOR 0x00900000
#define WIDGET_GVIM              0x00A00000
#define WIDGET_TREE              0x00A10000
#define WIDGET_CHOOSER           0x00A20000
#define WIDGET_PROGRESSBAR       0x00A30000
#define WIDGET_HSEPARATOR        0x00A40000
#define WIDGET_VSEPARATOR        0x00A50000
#define WIDGET_COMBOBOXTEXT      0x00A60000
#define WIDGET_COMBOBOXENTRY     0x00A70000
#define WIDGET_HSCALE            0x00A80000
#define WIDGET_VSCALE            0x00A90000
#define WIDGET_SPINBUTTON        0x00AA0000
#define WIDGET_TIMER             0x00AB0000
#define WIDGET_TOGGLEBUTTON      0x00AC0000
#define WIDGET_STATUSBAR         0x00AD0000
#define WIDGET_COLORBUTTON       0x00AE0000
#define WIDGET_FONTBUTTON        0x00AF0000
#define WIDGET_TERMINAL          0x00B00000
#define WIDGET_EVENTBOX          0x00B10000
#define WIDGET_EXPANDER          0x00B20000
#define WIDGET_SWITCH            0x00B30000
#define WIDGET_FILECHOOSER       0x00B40000
#define WIDGET_CALENDAR          0x00B50000
#define WIDGET_LINKBUTTON        0x00B60000
#define WIDGET_SEARCHENTRY       0x00B70000
#define WIDGET_INFOBAR           0x00B80000
#define WIDGET_SPINNER           0x00B90000
#define WIDGET_IMAGE             0x00BA0000
#define WIDGET_PULSE             0x00BB0000
#define WIDGET_PASSWORD          0x00BC0000
#define WIDGET_ASPECTFRAME       0x00BD0000
#define WIDGET_LEVELBAR          0x00BE0000
#define WIDGET_DRAWINGAREA       0x00BF0000
/* <grid> : mise en page en tableau. ⚠️ La valeur doit être
 * IDENTIQUE dans include/automaton.h et src-gtk4/automaton.h : le lexer et la
 * grammaire sont partagés, mais le parser GÉNÉRÉ est compilé tantôt contre
 * l'un, tantôt contre l'autre (CMake pose GTK4SRC en tête du chemin pour la
 * variante GTK4). 0xC4 est le premier rang libre dans les DEUX — l'énum gtk4
 * monte jusqu'à 0xC3 (REVEALER/STACK/FLOWBOX/OVERLAY qui n'existent pas dans
 * l'étalon). Un décalage ici fabriquerait un widget d'un autre type. */
#define WIDGET_GRID              0x00C40000

/*
 * Imperative stuff.
 */
#define VARIABLE_NAME         0x00010000
#define CONST_NUMBER          0x00020000
#define OP_ADD                0x000a0000
#define OP_SUBST              0x000b0000
#define OP_MULT               0x000c0000
#define OP_DIV                0x000d0000
#define REL_EQ                0x00100000
#define REL_NE                0x00200000



/*                                   - Widget subtype    */
/*                              xxxxxxxx                 */
#define WIDGET_SUBTYPE        0x00000F00

/* Conteneurs GTK4 remontes au coeur sermo : la
 * grammaire etalon est l'UNION des ports ; un port qui ne rend pas
 * ces widgets passe par le default "Unknown widget type". */
#define WIDGET_STACK             0x00110000
#define WIDGET_REVEALER          0x00120000
#define WIDGET_OVERLAY           0x00130000
#define WIDGET_FLOWBOX           0x00140000
/* <paned> : deux zones séparées par une poignée déplaçable.
 * ⚠️ Même règle que WIDGET_GRID : valeur IDENTIQUE dans tous les automaton.h
 * du dépôt (cœur, variante GTK4, copies locales des backends). Le cœur crée
 * les widgets avec SES valeurs ; une copie qui renumérote fabrique un widget
 * d'un autre type (arrivé à la variante GTK4, puis à fltk1). */
#define WIDGET_PANED             0x00C50000
/* <toolbar> : barre d'actions. ⚠️ Valeur IDENTIQUE dans TOUS les
 * automaton.h du dépôt — le cœur crée les widgets avec SES valeurs. */
#define WIDGET_TOOLBAR           0x00C60000
/* <stack> : N pages, une seule visible. ⚠️ Valeur IDENTIQUE dans
 * TOUS les automaton.h du dépôt. */
#define WIDGET_STACK_PAGES       0x00C70000
/* <wizard> : suite d'étapes avec Précédent/Suivant/Terminer.
 * ⚠️ Valeur IDENTIQUE dans TOUS les automaton.h du dépôt. */
#define WIDGET_WIZARD            0x00C80000
/* <menubutton> : bouton qui déroule ses <menuitem>.
 * ⚠️ Valeur IDENTIQUE dans TOUS les automaton.h du dépôt. */
#define WIDGET_MENUBUTTON        0x00C90000
/* <treetable> : arbre multi-colonnes, porté UNIQUEMENT par le backend fltk1
 * (widget_treetable.cpp). Fait partie de l'union des types de la grammaire ;
 * 0xC0 est libre dans l'énumération canonique (0xC0-0xC3 laissés vacants, voir
 * WIDGET_GRID). Les autres ports l'ignorent (« Unknown widget type »). */
#define WIDGET_TREETABLE         0x00C00000

#endif /* SERMO_WIDGET_TYPES_H */
