/* SPDX-License-Identifier: GPL-2.0-or-later */
/*
 * Copyright (C) 2026 S. Cage
 * haplo-dialog <devel@haplo-dialog.fr>
 */
#ifndef _GNU_SOURCE
#define _GNU_SOURCE
#endif
#include "sdl3-compat.h"
#include "dialog_state.h"
#include "gtkdialog.h"
#include "attributes.h"
#include "automaton.h"
#include "widgets.h"
#include "tag_attributes.h"
#include "widget_spinbutton.h"
#include <stdlib.h>
#include <stdio.h>

GtkWidget *widget_spinbutton_create(AttributeSet *Attr, tag_attr *attr, gint Type)
{
    WidgetNode *node = widget_node_new(WT_SPINBUTTON, NULL, "");
    node->state.spin.value  = 0.0;
    node->state.spin.min    = 0.0;
    node->state.spin.max    = 100.0;
    node->state.spin.digits = 0;   /* comme l'étalon : entier par défaut */

    if (attr) {
        const char *v;
        if ((v = get_tag_attribute(attr, "min"))) node->state.spin.min = g_ascii_strtod(v, NULL);
        if ((v = get_tag_attribute(attr, "max"))) node->state.spin.max = g_ascii_strtod(v, NULL);
        /* digits= : décimales de la valeur EXPORTÉE. Le champ existait dans
         * l'état du nœud, mais personne ne le remplissait — d'où un export
         * qui ignorait l'attribut. */
        if ((v = get_tag_attribute(attr, "digits"))) node->state.spin.digits = atoi(v);
        if ((v = get_tag_attribute(attr, "range-min"))) node->state.spin.min = g_ascii_strtod(v, NULL);
        if ((v = get_tag_attribute(attr, "range-max"))) node->state.spin.max = g_ascii_strtod(v, NULL);
    }
    if (Attr) {
        GList *el = NULL;
        gchar *def = attributeset_get_first(&el, Attr, ATTR_DEFAULT);
        if (def && *def) node->state.spin.value = g_ascii_strtod(def, NULL);
    }
    /* <input> (ex. echo 30) : valeur initiale par commande */
    {
        gchar *itext = widget_input_text(Attr);
        if (itext) {
            node->state.spin.value = g_ascii_strtod(g_strchomp(itext), NULL);
            g_free(itext);
        }
    }
    return (GtkWidget *)node;
}

gchar *widget_spinbutton_envvar_construct(GtkWidget *w)
{
    WidgetNode *n = (WidgetNode *)w;
    if (!n) return g_strdup("0");
    /* Règle de l'étalon : « %.Nf » avec N = digits. Ce port rendait « %g »,
     * qui ignore digits (4.25 restait « 4.25 » même avec digits="0", là où
     * l'étalon rend « 4 »). g_ascii_formatd : la locale ne décide pas. */
    char buf[64], format[8];
    int d = n->state.spin.digits;
    if (d < 0 || d > 15) snprintf(format, sizeof(format), "%%f");
    else                 snprintf(format, sizeof(format), "%%.%df", d);
    return g_strdup(g_ascii_formatd(buf, sizeof buf, format, n->state.spin.value));
}
gchar *widget_spinbutton_envvar_all_construct(variable *var)
{
    if (!var || !var->Widget) return NULL;
    return widget_spinbutton_envvar_construct(var->Widget);
}
void widget_spinbutton_clear(variable *var)
{
    if (!var || !var->Widget) return;
    ((WidgetNode *)var->Widget)->state.spin.value = 0.0;
}
void widget_spinbutton_refresh(variable *var) {}
void widget_spinbutton_fileselect(variable *var, const char *n, const char *v) {}
void widget_spinbutton_removeselected(variable *var) {}
void widget_spinbutton_save(variable *var) {}
