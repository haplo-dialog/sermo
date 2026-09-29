/* SPDX-License-Identifier: GPL-2.0-or-later */
/*
 * Copyright (C) 2026 S. Cage
 * haplo-dialog <devel@haplo-dialog.fr>
 */
/*
 * widget_pulse.c — Barre d'activité indéterminée EFL/Elementary
 * sermo — haplo-dialog <devel@haplo-dialog.fr>
 * Licence : GPL-2.0-or-later
 *
 * <pulse> : barre SANS valeur — elle dit « ça travaille », pas « 42 % ».
 * Elementary a exactement ce mode en natif : elm_progressbar_pulse_set() puis
 * elm_progressbar_pulse() lance l'aller-retour. Rien à animer à la main.
 * Attributs de balise text= / show-text= et <default> donnent le texte, comme
 * l'étalon gtk3.
 *
 * Export : la chaîne fixe « pulse », comme l'étalon gtk3. Le stub exportait
 * une chaîne VIDE.
 */
#ifndef _GNU_SOURCE
#define _GNU_SOURCE
#endif
#include "efl-compat.h"
#include "efl-globals.h"
#include "gtkdialog.h"
#include "attributes.h"
#include "automaton.h"
#include "widgets.h"
#include "tag_attributes.h"
#include "widget_pulse.h"
#include <stdlib.h>

GtkWidget *widget_pulse_create(AttributeSet *Attr, tag_attr *attr, gint Type)
{
    (void) Type;
    Evas_Object *parent = efl_main_win_get();
    Evas_Object *pb = elm_progressbar_add(parent ? parent
                                                 : elm_win_add(NULL, "tmp", ELM_WIN_BASIC));
    const char *text = NULL;

    elm_progressbar_pulse_set(pb, EINA_TRUE);   /* mode indéterminé natif */
    elm_progressbar_pulse(pb, EINA_TRUE);       /* et il démarre */

    if (attr) {
        const char *t = get_tag_attribute(attr, "text");
        if (t && *t) text = t;
    }
    if (!text && Attr) {
        GList *el = NULL;
        gchar *d = attributeset_get_first(&el, Attr, ATTR_DEFAULT);
        if (d && *d) text = d;
    }
    if (text) elm_object_text_set(pb, text);

    evas_object_size_hint_align_set(pb, EVAS_HINT_FILL, 0.5);
    evas_object_show(pb);
    return (GtkWidget *) pb;
}

gchar *widget_pulse_envvar_construct(GtkWidget *widget)
{
    (void) widget;
    return g_strdup("pulse");              /* étalon gtk3 : valeur fixe */
}
gchar *widget_pulse_envvar_all_construct(variable *var)
{
    if (!var || !var->Widget) return NULL;
    return widget_pulse_envvar_construct(var->Widget);
}
void widget_pulse_clear(variable *var)          { (void) var; }
void widget_pulse_refresh(variable *var)        { (void) var; }
void widget_pulse_fileselect(variable *var, const char *name, const char *value)
{   (void) var; (void) name; (void) value; }
void widget_pulse_removeselected(variable *var) { (void) var; }
void widget_pulse_save(variable *var)           { (void) var; }
