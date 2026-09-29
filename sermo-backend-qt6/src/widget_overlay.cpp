/* SPDX-License-Identifier: GPL-2.0-or-later */
/*
 * Copyright (C) 2026 S. Cage
 * haplo-dialog <devel@haplo-dialog.fr>
 */
/*
 * widget_overlay.cpp — Enfants empilés (Qt 6)
 * sermo — haplo-dialog <devel@haplo-dialog.fr>
 * Licence : GPL-2.0-or-later
 *
 * QStackedLayout en mode StackAll : tous les enfants sont montrés, empilés
 * dans le même rectangle — c'est exactement GtkOverlay. Le premier ajouté est
 * dessous.
 */
#include "qt6-compat.h"
#include "gtk3d.h"
#include "attributes.h"
#include "automaton.h"
#include "widgets.h"
#include "tag_attributes.h"
#include "stack.h"
#include "widget_overlay.h"
#include <QtWidgets/QWidget>
#include <QtWidgets/QStackedLayout>
#include <stdio.h>
#include <stdlib.h>

GtkWidget *widget_overlay_create(AttributeSet *Attr, tag_attr *attr, gint Type)
{
    (void) Attr; (void) attr; (void) Type;

    QWidget        *hote = new QWidget();
    QStackedLayout *pile = new QStackedLayout(hote);
    pile->setStackingMode(QStackedLayout::StackAll);
    pile->setContentsMargins(0, 0, 0, 0);

    stackelement s = pop();
    int poses = 0;
    for (int n = 0; n < s.nwidgets; ++n) {
        if (!s.widgets[n]) continue;
        pile->addWidget(static_cast<QWidget *>(s.widgets[n]));
        poses++;
    }
    if (poses < 2)
        fprintf(stderr, "qt6sermo: <overlay> n'a reçu que %d enfant(s) : il n'y "
                        "a rien à superposer.\n", poses);
    /* ⚠️ StackAll montre tout, mais RELÈVE l'enfant COURANT — et currentIndex
     * vaut 0 par défaut, donc le FOND passait au-dessus de tout le reste.
     * Mesuré à l'écran contre l'étalon : gtk3 affiche « DESSUS (second) »,
     * qt6 affichait « FOND (premier enfant) ». Le banc ne le voyait pas :
     * un conteneur exporte une chaîne vide, quel que soit l'ordre de tirage.
     * On relève donc le DERNIER enfant, comme GtkOverlay. */
    if (poses > 0)
        pile->setCurrentIndex(poses - 1);
    return (GtkWidget *) hote;
}

/* Un conteneur n'a pas de valeur propre : chaîne vide (étalon gtk3). */
gchar *widget_overlay_envvar_construct(GtkWidget *widget)
{
    (void) widget;
    return g_strdup("");
}
gchar *widget_overlay_envvar_all_construct(variable *var)
{
    if (!var || !var->Widget) return nullptr;
    return widget_overlay_envvar_construct(var->Widget);
}
void widget_overlay_clear(variable *var)          { (void) var; }
void widget_overlay_refresh(variable *var)        { (void) var; }
void widget_overlay_fileselect(variable *var, const char *n, const char *v)
{   (void) var; (void) n; (void) v; }
void widget_overlay_removeselected(variable *var) { (void) var; }
void widget_overlay_save(variable *var)           { (void) var; }
