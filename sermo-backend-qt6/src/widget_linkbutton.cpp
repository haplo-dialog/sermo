/* SPDX-License-Identifier: GPL-2.0-or-later */
/*
 * Copyright (C) 2026 S. Cage
 * haplo-dialog <devel@haplo-dialog.fr>
 */
/* widget_linkbutton.cpp — Bouton lien Qt6 (QPushButton → ouvre l'URL).
 *
 * L'URI vient de <default>, comme l'étalon gtk3 (gtk_link_button_new_with_label
 * lit ATTR_DEFAULT), puis de l'<input>, lu par widget_linkbutton_refresh() à
 * chaque « refresh ». Jusqu'au 2026-10-13, la création prenait en repli le
 * TEXTE BRUT de la directive <input> (« Command:… ») pour URI, sans l'exécuter.
 * L'export rend l'URI — pas le LIBELLÉ affiché, que rendait l'ancienne version
 * (écart mesuré au banc de comportement, cas 26 : LB="" au lieu de l'adresse).
 * L'URI est portée par la propriété Qt « sermoUri » : elle survit au changement
 * de libellé et n'oblige à aucune structure parallèle. */
#include "qt6-compat.h"
#include "gtk3d.h"
#include "attributes.h"
#include "automaton.h"
#include "widgets.h"
#include "tag_attributes.h"
#include "widget_linkbutton.h"
#include <QtWidgets/QPushButton>
#include <QtGui/QDesktopServices>
#include <QtCore/QUrl>

GtkWidget *widget_linkbutton_create(AttributeSet *Attr, tag_attr *attr, gint Type)
{
    (void)attr; (void)Type;
    GList *el = nullptr;
    const char *label = nullptr, *uri = nullptr;
    if (Attr) {
        gchar *l = attributeset_get_first(&el, Attr, ATTR_LABEL);
        if (l && *l) label = l;
        el = nullptr;
        el = nullptr;
        gchar *d = attributeset_get_first(&el, Attr, ATTR_DEFAULT);
        if (d && *d) uri = d;
        /* <input> : lu par widget_linkbutton_refresh(), que le cœur appelle
         * juste après la création — le lire aussi ici exécuterait la commande
         * deux fois. */
    }
    QString url = (uri && *uri) ? QString::fromUtf8(uri)
                                : QString::fromUtf8(label ? label : "");
    QPushButton *btn = new QPushButton(QString::fromUtf8(label && *label ? label : (uri ? uri : "")));
    btn->setProperty("sermoUri", url);
    btn->setProperty("sermoLibelle", label != nullptr);
    /* L'URI COURANTE, lue au clic : un <input> rafraîchi la remplace. */
    QObject::connect(btn, &QPushButton::clicked, btn, [btn]() {
        QString u = btn->property("sermoUri").toString();
        if (!u.isEmpty()) QDesktopServices::openUrl(QUrl(u));
    });
    return (GtkWidget *)btn;
}
gchar *widget_linkbutton_envvar_construct(GtkWidget *w)
{
    QPushButton *b = static_cast<QPushButton*>(w);
    if (!b) return g_strdup("");
    return g_strdup(b->property("sermoUri").toString().toUtf8().constData());
}
gchar *widget_linkbutton_envvar_all_construct(variable *var) { if(!var||!var->Widget) return nullptr; return widget_linkbutton_envvar_construct(var->Widget); }
void   widget_linkbutton_clear(variable *var) { (void)var; }
/* <input> à chaque « refresh », premier passage compris (il l'emporte sur
 * <default>, comme chez l'étalon) : la première ligne est l'URI. Sans <label>,
 * le bouton affiche l'URI : l'affichage suit. Aucune <action> n'est branchée
 * sur ce widget dans ce port (le clic ouvre l'URI). 2026-10-13. */
void   widget_linkbutton_refresh(variable *var)
{
    if (!var || !var->Widget) return;
    QPushButton *b = static_cast<QPushButton *>(var->Widget);
    gchar *ligne = widget_input_line(var->Attributes);
    if (!ligne) return;
    QString u = QString::fromUtf8(ligne);
    b->setProperty("sermoUri", u);
    if (!b->property("sermoLibelle").toBool()) b->setText(u);
    g_free(ligne);
}
void   widget_linkbutton_fileselect(variable *var, const char *n, const char *v) { (void)var;(void)n;(void)v; }
void   widget_linkbutton_removeselected(variable *var) { (void)var; }
void   widget_linkbutton_save(variable *var) { (void)var; }
