/* SPDX-License-Identifier: GPL-2.0-or-later */
/*
 * Copyright (C) 2026 S. Cage
 * haplo-dialog <devel@haplo-dialog.fr>
 */
/*
 * widget_button.c — Bouton export HTML
 * sermo 2.0.0 — haplo-dialog <devel@haplo-dialog.fr>
 * Licence : GPL-2.0-or-later
 *
 * Construit un WidgetNode WT_BUTTON.
 * La render_loop() appelle ImGui::Button() en lisant le label.
 */
#ifndef _GNU_SOURCE
#define _GNU_SOURCE
#endif
#include "html-compat.h"
#include "dialog_state.h"
#include "gtkdialog.h"
#include "attributes.h"
#include "automaton.h"
#include "widgets.h"
#include "tag_attributes.h"
#include "widget_button.h"
#include "widget_togglebutton.h"
#include "sermo_icon_theme.h"
#include <string.h>
#include <stdlib.h>
#include <strings.h>

GtkWidget *widget_button_create(AttributeSet *Attr, tag_attr *attr, gint Type)
{
    const char *label = NULL;
    const char *action = NULL;

    /* le coeur (etalon gtk3) route <togglebutton> vers
     * widget_button_create (WIDGET_TOGGLEBUTTON), la ou l'ancien automaton du
     * port avait un cas separe. On delegue au createur de togglebutton, qui
     * fabrique un WT_TOGGLEBUTTON et applique <default> (etat rendu + exporte). */
    if (Type == WIDGET_TOGGLEBUTTON)
        return widget_togglebutton_create(Attr, attr, Type);

    if (Attr) {
        GList *el = NULL;
        gchar *lbl = attributeset_get_first(&el, Attr, ATTR_LABEL);
        if (lbl && *lbl) label = lbl;
        el = NULL;
        gchar *act = attributeset_get_first(&el, Attr, ATTR_ACTION);
        if (act && *act) action = act;
    }

    /* Libelles par defaut de la famille de sortie (parite des autres ports) */
    if (!label || !*label) {
        switch (Type) {
            case WIDGET_OKBUTTON:     label = "OK";      break;
            case WIDGET_CANCELBUTTON: label = "Annuler"; break;
            case WIDGET_YESBUTTON:    label = "Oui";     break;
            case WIDGET_NOBUTTON:     label = "Non";     break;
            case WIDGET_HELPBUTTON:   label = "Aide";    break;
            default:                  label = "Bouton";  break;
        }
    }

    WidgetNode *node = widget_node_new(WT_BUTTON, NULL, label);
    if (action) node->action = strdup(action);

    /* Valeur d'EXIT, comme l'étalon gtk3sermo : une action « exit:X » ou
     * <action type="exit">X</action> rend X ; un bouton SANS action sort avec
     * son libellé (la famille de sortie : OK, Cancel, Yes, No, Help si aucun
     * libellé n'est donné) ; des actions sans sortie ne ferment pas. */
    {
        GList *el = NULL;
        gchar *a = Attr ? attributeset_get_first(&el, Attr, ATTR_ACTION) : NULL;
        if (!a) {
            const char *l = (Attr && attributeset_is_avail(Attr, ATTR_LABEL))
                ? attributeset_get_first(&el, Attr, ATTR_LABEL) : NULL;
            if (!l || !*l) {
                switch (Type) {
                    case WIDGET_CANCELBUTTON: l = "Cancel"; break;
                    case WIDGET_YESBUTTON:    l = "Yes";    break;
                    case WIDGET_NOBUTTON:     l = "No";     break;
                    case WIDGET_HELPBUTTON:   l = "Help";   break;
                    default:                  l = "OK";     break;
                }
            }
            node->sortie = strdup(l);
        }
        for (; a && !node->sortie; a = attributeset_get_next(&el, Attr, ATTR_ACTION)) {
            const char *t = attributeset_get_this_tagattr(&el, Attr, ATTR_ACTION, "type");
            if (t && strcasecmp(t, "exit") == 0) node->sortie = strdup(a);
            else if (strncasecmp(a, "exit:", 5) == 0) node->sortie = strdup(a + 5);
        }
    }

    /* <input file icon="nom"> / stock="nom" : icone du theme, 20 px (etalon) */
    if (Attr) {
        GList *el = NULL;
        gchar *inp = attributeset_get_first(&el, Attr, ATTR_INPUT);
        while (inp) {
            if (strncasecmp(inp, "file:", 5) == 0) {
                gchar *icon = attributeset_get_this_tagattr(&el, Attr, ATTR_INPUT, "icon");
                if (!icon) icon = attributeset_get_this_tagattr(&el, Attr, ATTR_INPUT, "stock");
                if (icon && *icon) {
                    int px = 20;
                    const char *v = attr ? get_tag_attribute(attr, "theme-icon-size") : NULL;
                    if (v && atoi(v) > 0) px = atoi(v);
                    node->icon_path = sermo_icon_lookup(icon, px);
                    node->icon_px   = px;
                    /* image-position : l'icône par rapport au libellé, lu comme
                     * l'étalon (gtk3 widget_button.c) — left/right/top/bottom
                     * ou 0-3 ; toute autre valeur = à gauche. */
                    const char *pos = attr ? get_tag_attribute(attr, "image-position") : NULL;
                    if (pos) {
                        if (!strcasecmp(pos, "bottom") || atoi(pos) == 3)     node->image_pos = 3;
                        else if (!strcasecmp(pos, "top") || atoi(pos) == 2)   node->image_pos = 2;
                        else if (!strcasecmp(pos, "right") || atoi(pos) == 1) node->image_pos = 1;
                        else                                                  node->image_pos = 0;
                    }
                }
                break;
            }
            inp = attributeset_get_next(&el, Attr, ATTR_INPUT);
        }
    }
    /* valeur d'EXIT de la famille de sortie : ANGLAISE (parite etalon),
     * independante du libelle affiche */
    switch (Type) {
        case WIDGET_OKBUTTON:     node->tooltip = strdup("OK");     break;
        case WIDGET_CANCELBUTTON: node->tooltip = strdup("Cancel"); break;
        case WIDGET_YESBUTTON:    node->tooltip = strdup("Yes");    break;
        case WIDGET_NOBUTTON:     node->tooltip = strdup("No");     break;
        case WIDGET_HELPBUTTON:   node->tooltip = strdup("Help");   break;
        default: break;
    }
    return (GtkWidget *)node;
}

gchar *widget_button_envvar_construct(GtkWidget *widget)
{
    WidgetNode *n = (WidgetNode *)widget;
    if (!n) return g_strdup("NULL");
    return g_strdup(n->label ? n->label : "button");
}
gchar *widget_button_envvar_all_construct(variable *var)
{
    if (!var || !var->Widget) return NULL;
    return widget_button_envvar_construct(var->Widget);
}
void widget_button_clear(variable *var) {}
void widget_button_refresh(variable *var) {}
void widget_button_fileselect(variable *var, const char *n, const char *v) {}
void widget_button_removeselected(variable *var) {}
void widget_button_save(variable *var) {}
