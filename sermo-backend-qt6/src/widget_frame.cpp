/* SPDX-License-Identifier: GPL-2.0-or-later */
/*
 * Copyright (C) 2026 S. Cage
 * haplo-dialog <devel@haplo-dialog.fr>
 */
/*
 * widget_frame.cpp — Cadre avec label Qt6 (QGroupBox)
 * sermo — haplo-dialog <devel@haplo-dialog.fr>
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
    gb->setCheckable(false);

    /* Enfants rangés en COLONNE avec les règles de gtk3 (widget_frame.c) :
     * valeur courante partie du PROJET seulement — l'attribut du cadre ne
     * compte pas, la vbox interne de gtk3 lui est inaccessible —, reportée
     * d'enfant en enfant, défaut edit/frame/fenêtre défilante/paned étirés.
     * Avant : étiré seulement si space-expand="true" était écrit, donc un
     * <edit> ou un <frame> enfant restait à sa taille naturelle. */
    QLayout *layout = qt6_box_ranger(gb, NULL, false, 2);
    layout->setContentsMargins(6, 6, 6, 6);

    return (GtkWidget *)gb;
}

/* L'étalon exporte le TITRE du cadre (gtk_frame_get_label) ; un cadre sans
 * titre rend une chaîne vide. Ce port rendait TOUJOURS vide — le cas 38 du banc
 * de comportement l'a mesuré. Un conteneur qui n'exporte rien, ça se décrète ;
 * ici l'étalon exporte, donc on exporte. */
gchar *widget_frame_envvar_construct(GtkWidget *widget)
{
    QGroupBox *gb = qobject_cast<QGroupBox *>(static_cast<QWidget *>(widget));
    if (!gb) return g_strdup("");
    return g_strdup(gb->title().toUtf8().constData());
}
gchar *widget_frame_envvar_all_construct(variable *var) { return NULL; }
void   widget_frame_clear(variable *var) {}
void   widget_frame_refresh(variable *var) { if (var && var->Widget) static_cast<QWidget *>(var->Widget)->update(); }
void   widget_frame_fileselect(variable *var, const char *n, const char *v) {}
void   widget_frame_removeselected(variable *var) {}
void   widget_frame_save(variable *var) {}
