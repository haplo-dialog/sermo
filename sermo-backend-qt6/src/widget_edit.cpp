/* SPDX-License-Identifier: GPL-2.0-or-later */
/*
 * Copyright (C) 2026 S. Cage
 * haplo-dialog <devel@haplo-dialog.fr>
 */
/*
 * widget_edit.cpp — Zone de texte multi-lignes éditable Qt6 (QTextEdit)
 * sermo — haplo-dialog <devel@haplo-dialog.fr>
 * Licence : GPL-2.0-or-later
 *
 * <edit> → QTextEdit éditable
 * <input> (commande ou fichier) alimente le contenu, lu par widget_edit_refresh().
 * Export : texte en clair (toPlainText)
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
#include "widget_edit.h"
#include "safe_exec.h"
#include "stringman.h"

#include <QtWidgets/QTextEdit>

#include <string.h>
#include <stdlib.h>
#include <stdio.h>

GtkWidget *widget_edit_create(AttributeSet *Attr, tag_attr *attr, gint Type)
{
    int w = 300, h = 150;
    if (attr) {
        const char *v;
        if ((v = get_tag_attribute(attr, "width-request")))  w = atoi(v);
        if ((v = get_tag_attribute(attr, "height-request"))) h = atoi(v);
    }

    QTextEdit *te = new QTextEdit();
    /* background / foreground (extension sermo) */
    if (attr) {
        const char *bg = get_tag_attribute(attr, "background");
        const char *fg = get_tag_attribute(attr, "foreground");
        if (bg || fg)
            te->setStyleSheet(QString("QTextEdit { %1 %2 }")
                .arg(bg ? QString("background: %1;").arg(bg) : QString())
                .arg(fg ? QString("color: %1;").arg(fg) : QString()));
    }
    te->setReadOnly(false);
    te->setMinimumSize(w, h);
    /* Pas d'enroulement des lignes : le port GTK 3 de référence ne pose aucun
     * mode d'enroulement sur son GtkTextView, dont le défaut est
     * GTK_WRAP_NONE — les lignes longues débordent et sont atteignables par la
     * barre de défilement horizontale. Le défaut de Qt (WidgetWidth) coupait
     * au contraire les lignes, tassant la sortie des commandes (dmesg, lsblk,
     * journalctl) sur une colonne étroite. */
    te->setLineWrapMode(QTextEdit::NoWrap);

    if (Attr) {
        GList *element = NULL;
        gchar *def = attributeset_get_first(&element, Attr, ATTR_DEFAULT);
        if (def && *def)
            te->setPlainText(QString::fromUtf8(def));
        /* <input> : lu par widget_edit_refresh(), que le cœur appelle juste
         * après la création — le lire aussi ici exécuterait la commande deux
         * fois. Dès qu'il est lisible, il remplace <default> ; l'étalon
         * gtk3sermo fait l'inverse (son <default> s'applique après <input>,
         * mesuré le 2026-09-17). */
    }

    return (GtkWidget *)te;
}

gchar *widget_edit_envvar_construct(GtkWidget *widget)
{
    QTextEdit *te = static_cast<QTextEdit *>(widget);
    if (!te) return g_strdup("");
    return g_strdup(te->toPlainText().toUtf8().constData());
}

gchar *widget_edit_envvar_all_construct(variable *var)
{
    if (!var || !var->Widget) return NULL;
    return widget_edit_envvar_construct(var->Widget);
}

void widget_edit_clear(variable *var)
{
    if (!var || !var->Widget) return;
    static_cast<QTextEdit *>(var->Widget)->clear();
}

void widget_edit_refresh(variable *var)
{
    if (!var || !var->Widget) return;
    QTextEdit *te = static_cast<QTextEdit *>(var->Widget);
    /* <input> (commande ou fichier, préfixes « Command: »/« File: » décodés par
     * widget_input_text) : le contenu ENTIER, saut de ligne final compris —
     * règle de l'étalon gtk3sermo. */
    gchar *text = widget_input_text(var->Attributes);
    if (text) {
        te->setPlainText(QString::fromUtf8(text));
        g_free(text);
    }
    te->update();
}

void widget_edit_fileselect(variable *var, const char *n, const char *v) {}
void widget_edit_removeselected(variable *var) {}
void widget_edit_save(variable *var) {}
