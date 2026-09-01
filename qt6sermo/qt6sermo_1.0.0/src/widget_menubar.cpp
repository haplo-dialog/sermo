/*
 * widget_menubar.cpp — Barre de menus Qt6 (QMenuBar)
 * qt6sermo 1.0.0 — haplo-dialog <devel@haplo-dialog.fr>
 * Licence : GPL-2.0-or-later
 *
 * <menubar> → QMenuBar
 * Les <menuitem> enfants sont pushés sur la pile ; on les récupère
 * et on les ajoute comme QMenu/QAction.
 * Export : NULL (la menubar n'exporte pas de variable)
 */

#ifndef _GNU_SOURCE
#define _GNU_SOURCE
#endif
#include "qt6-compat.h"
#include "gtk3d.h"
#include "attributes.h"
#include "automaton.h"
#include "widgets.h"
#include "tag_attributes.h"
#include "stack.h"
#include "widget_menubar.h"

#include <QtWidgets/QMenuBar>
#include <QtWidgets/QMenu>
#include <QtWidgets/QWidget>

#include <string.h>
#include <stdlib.h>

GtkWidget *widget_menubar_create(AttributeSet *Attr, tag_attr *attr, gint Type)
{
    QMenuBar *mb = new QMenuBar();

    /* Les menus enfants ont été poussés sur la pile puis regroupés (SUM) en
     * UN seul stackelement : on dépile UNE fois et on parcourt s.nwidgets,
     * déjà dans l'ordre d'apparition XML. (L'ancienne boucle ne lisait que
     * widgets[0] et perdait tous les menus au-delà du premier.) */
    stackelement s = pop();
    for (int n = 0; n < s.nwidgets; ++n) {
        if (!s.widgets[n]) continue;
        QMenu *menu = qobject_cast<QMenu *>(static_cast<QObject *>(s.widgets[n]));
        if (menu)
            mb->addMenu(menu);
    }

    return (GtkWidget *)mb;
}

gchar *widget_menubar_envvar_construct(GtkWidget *widget) { return NULL; }
gchar *widget_menubar_envvar_all_construct(variable *var) { return NULL; }
void   widget_menubar_clear(variable *var) {}
void   widget_menubar_refresh(variable *var) { if (var && var->Widget) static_cast<QWidget *>(var->Widget)->update(); }
void   widget_menubar_fileselect(variable *var, const char *n, const char *v) {}
void   widget_menubar_removeselected(variable *var) {}
void   widget_menubar_save(variable *var) {}
