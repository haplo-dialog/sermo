/* SPDX-License-Identifier: GPL-2.0-or-later */
/*
 * Copyright (C) 2026 S. Cage
 * haplo-dialog <devel@haplo-dialog.fr>
 */
/*
 * widget_grid.cpp — Conteneur de mise en page en tableau (Qt 6)
 * sermo — haplo-dialog <devel@haplo-dialog.fr>
 * Licence : GPL-2.0-or-later
 *
 * <grid columns="N"> : QGridLayout, remplissage EN FLOT (ordre du document,
 * retour à la ligne tous les N enfants). Étalon = gtk3 (GtkGrid).
 *
 * ⚠️ <grid> n'est pas <table> : <table> est la liste à colonnes des données.
 */
#include "qt6-compat.h"
#include "gtk3d.h"
#include "attributes.h"
#include "automaton.h"
#include "widgets.h"
#include "tag_attributes.h"
#include "stack.h"
#include "widget_grid.h"
#include <QtWidgets/QWidget>
#include <QtWidgets/QGridLayout>
#include <stdlib.h>
#include <stdio.h>

static int grid_attr_int(tag_attr *attr, const char *nom, int repli)
{
    if (!attr) return repli;
    const char *v = get_tag_attribute(attr, nom);
    if (!v || !*v) return repli;
    int n = atoi(v);
    return (n >= 0) ? n : repli;
}

GtkWidget *widget_grid_create(AttributeSet *Attr, tag_attr *attr, gint Type)
{
    (void) Attr; (void) Type;

    QWidget     *hote   = new QWidget();
    QGridLayout *layout = new QGridLayout(hote);

    int columns = grid_attr_int(attr, "columns", 0);
    if (columns <= 0) {
        fprintf(stderr, "qt6sermo: <grid> sans attribut columns= utilisable : "
                        "une seule colonne.\n");
        columns = 1;
    }

    layout->setContentsMargins(0, 0, 0, 0);
    layout->setVerticalSpacing(grid_attr_int(attr, "row-spacing", 4));
    layout->setHorizontalSpacing(grid_attr_int(attr, "column-spacing", 8));

    stackelement s = pop();
    for (int n = 0; n < s.nwidgets; ++n) {
        if (!s.widgets[n]) continue;
        layout->addWidget(static_cast<QWidget *>(s.widgets[n]),
                          n / columns, n % columns);
    }

    if (attr) {
        const char *h = get_tag_attribute(attr, "homogeneous");
        if (h && (!strcasecmp(h, "true") || !strcasecmp(h, "yes") || atoi(h) == 1)) {
            /* Colonnes de largeur égale : chaque colonne reçoit le même
             * facteur d'étirement. */
            for (int c = 0; c < columns; ++c) layout->setColumnStretch(c, 1);
        }
    }

    return (GtkWidget *) hote;
}

/* Un conteneur n'a pas de valeur propre : chaîne vide (étalon gtk3). */
gchar *widget_grid_envvar_construct(GtkWidget *widget)
{
    (void) widget;
    return g_strdup("");
}
gchar *widget_grid_envvar_all_construct(variable *var)
{
    if (!var || !var->Widget) return nullptr;
    return widget_grid_envvar_construct(var->Widget);
}
void widget_grid_clear(variable *var)          { (void) var; }
void widget_grid_refresh(variable *var)        { (void) var; }
void widget_grid_fileselect(variable *var, const char *n, const char *v)
{   (void) var; (void) n; (void) v; }
void widget_grid_removeselected(variable *var) { (void) var; }
void widget_grid_save(variable *var)           { (void) var; }
