/* SPDX-License-Identifier: GPL-2.0-or-later */
/*
 * Copyright (C) 2026 S. Cage
 * haplo-dialog <devel@haplo-dialog.fr>
 */
/*
 * widget_toolbar.cpp — Barre d'actions (Qt 6)
 * sermo — haplo-dialog <devel@haplo-dialog.fr>
 * Licence : GPL-2.0-or-later
 *
 * QToolBar accepte des widgets quelconques par addWidget() : c'est bien un
 * conteneur, pas une fenêtre. Étalon = gtk3 (boîte + classe CSS « toolbar »).
 */
#include "qt6-compat.h"
#include "gtk3d.h"
#include "attributes.h"
#include "automaton.h"
#include "widgets.h"
#include "tag_attributes.h"
#include "stack.h"
#include "widget_toolbar.h"
#include <QtWidgets/QToolBar>
#include <QtWidgets/QWidget>
#include <string.h>
#include <stdlib.h>

GtkWidget *widget_toolbar_create(AttributeSet *Attr, tag_attr *attr, gint Type)
{
    (void) Attr; (void) Type;

    QToolBar *tb = new QToolBar();
    tb->setMovable(false);
    if (attr) {
        const char *v = get_tag_attribute(attr, "orientation");
        if (v && !strcasecmp(v, "vertical")) tb->setOrientation(Qt::Vertical);
    }

    stackelement s = pop();
    for (int n = 0; n < s.nwidgets; ++n)
        if (s.widgets[n]) tb->addWidget(static_cast<QWidget *>(s.widgets[n]));

    return (GtkWidget *) tb;
}

/* Un conteneur n'a pas de valeur propre : chaîne vide (étalon gtk3). */
gchar *widget_toolbar_envvar_construct(GtkWidget *widget)
{
    (void) widget;
    return g_strdup("");
}
gchar *widget_toolbar_envvar_all_construct(variable *var)
{
    if (!var || !var->Widget) return nullptr;
    return widget_toolbar_envvar_construct(var->Widget);
}
void widget_toolbar_clear(variable *var)          { (void) var; }
void widget_toolbar_refresh(variable *var)        { (void) var; }
void widget_toolbar_fileselect(variable *var, const char *n, const char *v)
{   (void) var; (void) n; (void) v; }
void widget_toolbar_removeselected(variable *var) { (void) var; }
void widget_toolbar_save(variable *var)           { (void) var; }
