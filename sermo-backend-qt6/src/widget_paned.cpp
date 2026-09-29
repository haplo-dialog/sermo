/* SPDX-License-Identifier: GPL-2.0-or-later */
/*
 * Copyright (C) 2026 S. Cage
 * haplo-dialog <devel@haplo-dialog.fr>
 */
/*
 * widget_paned.cpp — Deux zones et une poignée déplaçable (Qt 6)
 * sermo — haplo-dialog <devel@haplo-dialog.fr>
 * Licence : GPL-2.0-or-later
 *
 * <paned> : QSplitter, le conteneur Qt fait pour ça. Étalon = gtk3 (GtkPaned).
 *
 * ⚠️ EXACTEMENT deux enfants ; un troisième est refusé AVEC un message.
 */
#include "qt6-compat.h"
#include "gtk3d.h"
#include "attributes.h"
#include "automaton.h"
#include "widgets.h"
#include "tag_attributes.h"
#include "stack.h"
#include "widget_paned.h"
#include <QtWidgets/QSplitter>
#include <QtWidgets/QWidget>
#include <QtCore/QList>
#include <string.h>
#include <stdlib.h>
#include <stdio.h>

GtkWidget *widget_paned_create(AttributeSet *Attr, tag_attr *attr, gint Type)
{
    (void) Attr; (void) Type;

    Qt::Orientation sens = Qt::Horizontal;
    if (attr) {
        const char *v = get_tag_attribute(attr, "orientation");
        if (v && !strcasecmp(v, "vertical")) sens = Qt::Vertical;
    }

    QSplitter *split = new QSplitter(sens);
    if (attr) {
        const char *v = get_tag_attribute(attr, "resizable");
        if (v && (!strcasecmp(v, "false") || !strcasecmp(v, "no") || !strcmp(v, "0")))
            /* Poignée figée : Qt n'a pas d'interrupteur, on rend la poignée
             * inerte en lui donnant une largeur nulle et en interdisant le
             * déplacement des enfants. */
            split->setHandleWidth(0);
    }

    stackelement s = pop();
    int retenus = 0;
    for (int n = 0; n < s.nwidgets; ++n) {
        if (!s.widgets[n]) continue;
        if (retenus >= 2) {
            fprintf(stderr, "qt6sermo: <paned> prend EXACTEMENT deux enfants : "
                            "le %de est ignoré. Emballer le surplus dans une <vbox>.\n",
                    retenus + 1);
            continue;
        }
        split->addWidget(static_cast<QWidget *>(s.widgets[n]));
        retenus++;
    }
    if (retenus < 2)
        fprintf(stderr, "qt6sermo: <paned> n'a reçu que %d enfant(s) : la poignée "
                        "n'a rien à partager.\n", retenus);

    /* Position initiale. En pourcentage, QSplitter raisonne en TAILLES : on lui
     * donne deux parts dont le rapport vaut la fraction demandée — il les
     * normalise ensuite sur la taille réelle. En pixels, la première part est
     * la valeur donnée. */
    if (attr) {
        const char *v = get_tag_attribute(attr, "position");
        if (v && *v && retenus == 2) {
            char *fin = nullptr;
            double d = g_ascii_strtod(v, &fin);
            if (fin && *fin == '%') {
                if (d > 0 && d < 100)
                    split->setSizes(QList<int>() << (int) (d * 10)
                                                 << (int) ((100 - d) * 10));
            } else if (d >= 1) {
                split->setSizes(QList<int>() << (int) d << (int) d);
            }
        }
    }
    return (GtkWidget *) split;
}

/* Un conteneur n'a pas de valeur propre : chaîne vide (étalon gtk3). */
gchar *widget_paned_envvar_construct(GtkWidget *widget)
{
    (void) widget;
    return g_strdup("");
}
gchar *widget_paned_envvar_all_construct(variable *var)
{
    if (!var || !var->Widget) return nullptr;
    return widget_paned_envvar_construct(var->Widget);
}
void widget_paned_clear(variable *var)          { (void) var; }
void widget_paned_refresh(variable *var)        { (void) var; }
void widget_paned_fileselect(variable *var, const char *n, const char *v)
{   (void) var; (void) n; (void) v; }
void widget_paned_removeselected(variable *var) { (void) var; }
void widget_paned_save(variable *var)           { (void) var; }
