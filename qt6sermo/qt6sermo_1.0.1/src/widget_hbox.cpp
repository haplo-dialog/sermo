/*
 * widget_hbox.cpp — Conteneur horizontal Qt6 (QWidget + QHBoxLayout)
 * qt6sermo 1.0.0 — haplo-dialog <devel@haplo-dialog.fr>
 * Licence : GPL-2.0-or-later
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
#include "widget_hbox.h"

#include <QtWidgets/QWidget>
#include <QtWidgets/QHBoxLayout>

#include <stdlib.h>

GtkWidget *widget_hbox_create(AttributeSet *Attr, tag_attr *attr, gint Type)
{
    int spacing = 2;
    if (attr) {
        const char *v = get_tag_attribute(attr, "spacing");
        if (v) spacing = atoi(v);
    }

    QWidget     *box    = new QWidget();
    QHBoxLayout *layout = new QHBoxLayout(box);
    layout->setContentsMargins(0, 0, 0, 0);
    layout->setSpacing(spacing);

    /* Dépiler les enfants groupés par SUM et les ré-parenter (cf. vbox) —
     * sinon ils restent sans parent → fenêtres top-level séparées.
     * Facteur d'étirement : 1 pour les enfants space-expand, 0 sinon. Si AUCUN
     * n'est extensible, on ajoute un ressort final pour empiler à gauche (sinon
     * Qt étale l'espace à parts égales : icône à gauche, texte loin à droite). */
    stackelement s = pop();
    bool anyExpand = false;
    for (int n = 0; n < s.nwidgets; ++n) {
        if (!s.widgets[n]) continue;
        int e = qt6_layout_get_expand((GtkWidget *)s.widgets[n]);
        if (e) anyExpand = true;
        layout->addWidget(static_cast<QWidget *>(s.widgets[n]), e ? 1 : 0);
    }
    /* Aucun enfant extensible → ressort EN TÊTE : le contenu est poussé à
     * DROITE, comme gtk3 (gtk_box_pack_end). Les lignes d'infos de l'en-tête
     * (icône+texte) s'alignent ainsi au bord droit. */
    if (!anyExpand) layout->insertStretch(0, 1);

    return (GtkWidget *)box;
}

gchar *widget_hbox_envvar_construct(GtkWidget *widget) { return NULL; }
gchar *widget_hbox_envvar_all_construct(variable *var) { return NULL; }
void   widget_hbox_clear(variable *var) {}
void   widget_hbox_refresh(variable *var) { if (var && var->Widget) static_cast<QWidget *>(var->Widget)->update(); }
void   widget_hbox_fileselect(variable *var, const char *n, const char *v) {}
void   widget_hbox_removeselected(variable *var) {}
void   widget_hbox_save(variable *var) {}
