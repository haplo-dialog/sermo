/*
 * widget_frame.cpp — Cadre avec label Qt6 (QGroupBox)
 * qt6sermo 1.0.0 — haplo-dialog <devel@haplo-dialog.fr>
 * Licence : GPL-2.0-or-later
 *
 * <frame> → QGroupBox non checkable avec titre
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
#include "widget_frame.h"

#include <QtWidgets/QGroupBox>
#include <QtWidgets/QVBoxLayout>

#include <string.h>
#include <stdlib.h>

GtkWidget *widget_frame_create(AttributeSet *Attr, tag_attr *attr, gint Type)
{
    /* Libellé du cadre. Il peut venir de DEUX endroits :
     *   - un <label>…</label> enfant, qui pose ATTR_LABEL ;
     *   - l'attribut de balise <frame label="…">, qui n'était PAS lu ici — d'où
     *     des cadres sans titre alors que l'étalon GTK 3 les affiche.
     * ORDRE DE PRIORITÉ : ATTR_LABEL d'abord, attribut de balise en repli.
     * C'est le comportement RÉEL de l'étalon : widget_frame.c utilise
     * attributeset_set_if_unset(), qui ne fait rien si ATTR_LABEL est déjà
     * posé — un <label> enfant l'emporte donc, malgré le commentaire amont
     * qui annonce l'inverse. On copie le comportement, pas le commentaire. */
    const char *label = "";
    if (Attr) {
        GList *element = NULL;
        gchar *lbl = attributeset_get_first(&element, Attr, ATTR_LABEL);
        if (lbl && *lbl) label = lbl;
    }
    if (!*label && attr) {
        const char *tv = get_tag_attribute(attr, "label");
        if (tv && *tv) label = tv;
    }

    QGroupBox   *gb     = new QGroupBox(QString::fromUtf8(label));
    QVBoxLayout *layout = new QVBoxLayout(gb);
    layout->setContentsMargins(6, 6, 6, 6);
    layout->setSpacing(2);
    gb->setCheckable(false);

    /* Dépiler les enfants groupés par SUM et les ré-parenter (cf. vbox).
     * ⚠️ Le facteur d'étirement doit être posé ICI aussi : sans lui, un enfant
     * space-expand="true" gardait sa taille naturelle et ne remplissait pas le
     * cadre — un <edit> restait à sa largeur minimale (300 px) au milieu d'un
     * cadre trois fois plus large, alors que le port GTK 3 le fait remplir.
     * Même logique que widget_vbox/widget_hbox. */
    stackelement s = pop();
    bool anyExpand = false;
    for (int n = 0; n < s.nwidgets; ++n) {
        if (!s.widgets[n]) continue;
        int e = qt6_layout_get_expand((GtkWidget *)s.widgets[n]);
        if (e) anyExpand = true;
        layout->addWidget(static_cast<QWidget *>(s.widgets[n]), e ? 1 : 0);
    }
    if (!anyExpand) layout->addStretch(1);

    return (GtkWidget *)gb;
}

gchar *widget_frame_envvar_construct(GtkWidget *widget) { return NULL; }
gchar *widget_frame_envvar_all_construct(variable *var) { return NULL; }
void   widget_frame_clear(variable *var) {}
void   widget_frame_refresh(variable *var) { if (var && var->Widget) static_cast<QWidget *>(var->Widget)->update(); }
void   widget_frame_fileselect(variable *var, const char *n, const char *v) {}
void   widget_frame_removeselected(variable *var) {}
void   widget_frame_save(variable *var) {}
