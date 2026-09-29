/* SPDX-License-Identifier: GPL-2.0-or-later */
/*
 * Copyright (C) 2026 S. Cage
 * haplo-dialog <devel@haplo-dialog.fr>
 */
/*
 * widget_flowbox.cpp — Rangement en lignes (Qt 6)
 * sermo — haplo-dialog <devel@haplo-dialog.fr>
 * Licence : GPL-2.0-or-later
 *
 * ⚠️ Qt n'a PAS de disposition en flot dans QtWidgets (la classe FlowLayout
 * de la documentation est un exemple à recopier, pas une API). Le port range
 * donc les enfants dans un QGridLayout à « max-children-per-line » colonnes :
 * même rendu tant que la fenêtre ne rétrécit pas, sans reflux dynamique.
 * C'est une limite du port, pas du tag — et elle est écrite ici plutôt que
 * cachée.
 *
 * ⚠️ Pas de sélection : un QGridLayout ne sélectionne rien. L'export rend donc
 * toujours la chaîne vide, ce que l'étalon rend aussi tant que rien n'est
 * sélectionné.
 */
#include "qt6-compat.h"
#include "gtk3d.h"
#include "attributes.h"
#include "automaton.h"
#include "widgets.h"
#include "tag_attributes.h"
#include "stack.h"
#include "widget_flowbox.h"
#include <QtWidgets/QWidget>
#include <QtWidgets/QGridLayout>
#include <string.h>
#include <stdlib.h>

GtkWidget *widget_flowbox_create(AttributeSet *Attr, tag_attr *attr, gint Type)
{
    (void) Attr; (void) Type;

    int par_ligne = 4, esp_col = 6, esp_lig = 6;
    if (attr) {
        const char *v;
        if ((v = get_tag_attribute(attr, "max-children-per-line"))) par_ligne = atoi(v);
        if ((v = get_tag_attribute(attr, "column-spacing")))        esp_col   = atoi(v);
        if ((v = get_tag_attribute(attr, "row-spacing")))           esp_lig   = atoi(v);
    }
    if (par_ligne < 1) par_ligne = 1;

    QWidget     *hote   = new QWidget();
    QGridLayout *grille = new QGridLayout(hote);
    grille->setContentsMargins(0, 0, 0, 0);
    grille->setHorizontalSpacing(esp_col);
    grille->setVerticalSpacing(esp_lig);

    stackelement s = pop();
    int i = 0;
    for (int n = 0; n < s.nwidgets; ++n) {
        if (!s.widgets[n]) continue;
        grille->addWidget(static_cast<QWidget *>(s.widgets[n]), i / par_ligne, i % par_ligne);
        i++;
    }
    return (GtkWidget *) hote;
}

/* Export : l'index de l'enfant sélectionné — ce port n'en sélectionne aucun. */
gchar *widget_flowbox_envvar_construct(GtkWidget *widget)
{
    (void) widget;
    return g_strdup("");
}
gchar *widget_flowbox_envvar_all_construct(variable *var)
{
    if (!var || !var->Widget) return nullptr;
    return widget_flowbox_envvar_construct(var->Widget);
}
void widget_flowbox_clear(variable *var)          { (void) var; }
void widget_flowbox_refresh(variable *var)        { (void) var; }
void widget_flowbox_fileselect(variable *var, const char *n, const char *v)
{   (void) var; (void) n; (void) v; }
void widget_flowbox_removeselected(variable *var) { (void) var; }
void widget_flowbox_save(variable *var)           { (void) var; }
