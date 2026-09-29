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
#include "widget_vscale.h"
#include <stdlib.h>
#include <stdio.h>

GtkWidget *widget_vscale_create(AttributeSet *Attr, tag_attr *attr, gint Type)
{
    WidgetNode *node = widget_node_new(WT_HSCALE, NULL, "");
    node->state.scale.value = 0.0;
    node->state.scale.min   = 0.0;
    node->state.scale.max   = 100.0;
    node->state.scale.step  = 1.0;

    if (attr) {
        const char *v;
        if ((v = get_tag_attribute(attr, "min")))  node->state.scale.min  = g_ascii_strtod(v, NULL);
        if ((v = get_tag_attribute(attr, "max")))  node->state.scale.max  = g_ascii_strtod(v, NULL);
        if ((v = get_tag_attribute(attr, "step"))) node->state.scale.step = g_ascii_strtod(v, NULL);
        /* forme publique : range-min / range-max / range-step */
        if ((v = get_tag_attribute(attr, "range-min")))  node->state.scale.min  = g_ascii_strtod(v, NULL);
        if ((v = get_tag_attribute(attr, "range-max")))  node->state.scale.max  = g_ascii_strtod(v, NULL);
        if ((v = get_tag_attribute(attr, "range-step"))) node->state.scale.step = g_ascii_strtod(v, NULL);
        /* digits= : nombre de décimales de la valeur EXPORTÉE. L'étalon
         * l'honore (gtk_scale_get_digits) ; ce port l'ignorait, et rendait
         * l'entier tronqué quoi qu'on écrive. */
        if ((v = get_tag_attribute(attr, "digits"))) node->state.scale.digits = atoi(v);
    }
    if (Attr) {
        GList *el = NULL;
        gchar *def = attributeset_get_first(&el, Attr, ATTR_DEFAULT);
        if (def && *def) node->state.scale.value = g_ascii_strtod(def, NULL);
    }
    {   /* <input> : valeur initiale par commande */
        gchar *itext = widget_input_text(Attr);
        if (itext) { node->state.scale.value = g_ascii_strtod(g_strchomp(itext), NULL); g_free(itext); }
    }
    return (GtkWidget *)node;
}

gchar *widget_vscale_envvar_construct(GtkWidget *w)
{
    WidgetNode *n = (WidgetNode *)w;
    if (!n) return g_strdup("0");
    /* Règle de l'étalon : « %.Nf » avec N = digits (0 par défaut, donc un
     * entier). Et g_ascii_formatd, pas snprintf : sous fr_FR « %.2f » de 2.5
     * écrit « 2,50 » et la valeur exportée dépendrait de la machine. */
    char buf[64], format[8];
    int d = n->state.scale.digits;
    if (d < 0 || d > 15) snprintf(format, sizeof(format), "%%f");
    else                 snprintf(format, sizeof(format), "%%.%df", d);
    return g_strdup(g_ascii_formatd(buf, sizeof buf, format, n->state.scale.value));
}
gchar *widget_vscale_envvar_all_construct(variable *var)
{
    if (!var || !var->Widget) return NULL;
    return widget_vscale_envvar_construct(var->Widget);
}
void widget_vscale_clear(variable *var)
{
    if (!var || !var->Widget) return;
    ((WidgetNode *)var->Widget)->state.scale.value = 0.0;
}
void widget_vscale_refresh(variable *var) {}
void widget_vscale_fileselect(variable *var, const char *n, const char *v) {}
void widget_vscale_removeselected(variable *var) {}
void widget_vscale_save(variable *var) {}
