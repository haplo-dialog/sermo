/* SPDX-License-Identifier: GPL-2.0-or-later */
/*
 * Copyright (C) 2026 S. Cage
 * haplo-dialog <devel@haplo-dialog.fr>
 */
/* widget_pulse.cpp — Barre de progression « pulse » Qt6 (indéterminée). */
#include "qt6-compat.h"
#include "gtk3d.h"
#include "attributes.h"
#include "automaton.h"
#include "widgets.h"
#include "tag_attributes.h"
#include "widget_pulse.h"
#include <QtWidgets/QProgressBar>
#include <strings.h>   /* strcasecmp */
#include <string.h>

GtkWidget *widget_pulse_create(AttributeSet *Attr, tag_attr *attr, gint Type)
{
    (void)Type;
    QProgressBar *pb = new QProgressBar();
    pb->setRange(0, 0);            /* mode indéterminé (busy) */
    pb->setTextVisible(false);

    /* text= / show-text= (attributs de balise) puis <default> : le texte
     * affiché dans la barre, comme l'étalon gtk3. */
    const char *text = nullptr;
    if (attr) {
        const char *t = get_tag_attribute(attr, "text");
        if (t && *t) text = t;
    }
    if (!text && Attr) {
        GList *el = nullptr;
        gchar *d = attributeset_get_first(&el, Attr, ATTR_DEFAULT);
        if (d && *d) text = d;
    }
    if (text) { pb->setFormat(QString::fromUtf8(text)); pb->setTextVisible(true); }
    if (attr) {
        const char *st = get_tag_attribute(attr, "show-text");
        if (st && *st)
            pb->setTextVisible(!strcasecmp(st, "true") || !strcasecmp(st, "yes") || !strcmp(st, "1"));
    }
    return (GtkWidget *)pb;
}
/* Étalon gtk3 : la valeur exportée est la chaîne fixe « pulse » (l'ancienne
 * version rendait une chaîne vide — écart mesuré au banc, cas 27). */
gchar *widget_pulse_envvar_construct(GtkWidget *w) { (void)w; return g_strdup("pulse"); }
gchar *widget_pulse_envvar_all_construct(variable *var) { (void)var; return nullptr; }
void   widget_pulse_clear(variable *var) { (void)var; }
void   widget_pulse_refresh(variable *var) { (void)var; }
void   widget_pulse_fileselect(variable *var, const char *n, const char *v) { (void)var;(void)n;(void)v; }
void   widget_pulse_removeselected(variable *var) { (void)var; }
void   widget_pulse_save(variable *var) { (void)var; }
