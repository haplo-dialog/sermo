/* SPDX-License-Identifier: GPL-2.0-or-later */
/*
 * Copyright (C) 2026 S. Cage
 * haplo-dialog <devel@haplo-dialog.fr>
 */
/*
 * widget_colorbutton.c — Sélecteur de couleur SDL3/ImGui v2
 * sermo — haplo-dialog <devel@haplo-dialog.fr>
 * Licence : GPL-2.0-or-later
 *
 * v2 : utilise WT_COLORBUTTON + state.color (ColorState).
 * render.cpp appelle ImGui::ColorEdit3 ou ImGui::ColorButton + popup.
 * L'export est "#rrggbb" (hex 6 digits).
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
#include "widget_colorbutton.h"
#include <string.h>
#include <stdlib.h>
#include <stdio.h>

/* Conversion #rrggbb → float RGB */
static void parse_hex_color(const char *hex, ColorState *c)
{
    c->r = c->g = c->b = 1.0f; c->a = 1.0f;
    if (!hex || hex[0] != '#' || strlen(hex) < 7) return;
    unsigned int ri = 0, gi = 0, bi = 0;
    sscanf(hex + 1, "%02x%02x%02x", &ri, &gi, &bi);
    c->r = (float)ri / 255.0f;
    c->g = (float)gi / 255.0f;
    c->b = (float)bi / 255.0f;
    snprintf(c->hex, sizeof(c->hex), "#%02x%02x%02x", ri, gi, bi);
}

GtkWidget *widget_colorbutton_create(AttributeSet *Attr, tag_attr *attr, gint Type)
{
    WidgetNode *node = widget_node_new(WT_COLORBUTTON, NULL, "Color");

    parse_hex_color("#ffffff", &node->state.color);

    if (Attr) {
        GList *el = NULL;
        gchar *lbl = attributeset_get_first(&el, Attr, ATTR_LABEL);
        if (lbl && *lbl) { free(node->label); node->label = strdup(lbl); }
        el = NULL;
        gchar *def = attributeset_get_first(&el, Attr, ATTR_DEFAULT);
        if (def && *def) parse_hex_color(def, &node->state.color);
    }
    return (GtkWidget *)node;
}

gchar *widget_colorbutton_envvar_construct(GtkWidget *widget)
{
    WidgetNode *n = (WidgetNode *)widget;
    if (!n) return g_strdup("#ffffff");
    /* Reconstruire hex depuis floats (render.cpp peut les modifier). Arrondi :
     * tronqué, 3/255 relu pouvait rendre 2 (« #000002 » au lieu de « #000003 »). */
    snprintf(n->state.color.hex, sizeof(n->state.color.hex), "#%02x%02x%02x",
             (int)(n->state.color.r * 255 + 0.5f),
             (int)(n->state.color.g * 255 + 0.5f),
             (int)(n->state.color.b * 255 + 0.5f));
    return g_strdup(n->state.color.hex);
}
gchar *widget_colorbutton_envvar_all_construct(variable *var)
{
    if (!var || !var->Widget) return NULL;
    return widget_colorbutton_envvar_construct(var->Widget);
}
void widget_colorbutton_clear(variable *var)
{
    if (!var || !var->Widget) return;
    parse_hex_color("#ffffff", &((WidgetNode *)var->Widget)->state.color);
}
/* « couleur » ou « couleur|alpha » → couleur, comme l'étalon (linecutter sur
 * '|', gdk_rgba_parse, puis gtk_color_button_set_alpha d'un entier 0..65535).
 * Ce port ne lit que « #rgb » et « #rrggbb » (pas les noms X11 ni « rgb() »
 * que GDK accepte aussi) ; une couleur illisible ne change rien, l'alpha
 * seul s'applique quand même, comme chez l'étalon. Coupure à la main : le
 * g_strsplit de sdl3-compat.h passe par strtok et prendrait l'alpha de
 * « |65535 » pour la couleur (2026-10-13). */
static void colorbutton_poser(ColorState *c, const char *texte)
{
    gchar       *copie, *h;
    char        *barre;
    const char  *suite = NULL;
    unsigned int ri = 0, gi = 0, bi = 0, alpha = 0;
    size_t       lg;

    if (!texte) return;
    copie = g_strdup(texte);
    barre = strchr(copie, '|');
    if (barre) { *barre = '\0'; suite = barre + 1; }
    {
        h = g_strstrip(copie);
        lg = strlen(h);
        if (h[0] == '#' && strspn(h + 1, "0123456789abcdefABCDEF") == lg - 1 &&
            (lg == 4 || lg == 7)) {
            if (lg == 7) {
                sscanf(h + 1, "%02x%02x%02x", &ri, &gi, &bi);
            } else {
                sscanf(h + 1, "%1x%1x%1x", &ri, &gi, &bi);
                ri *= 17; gi *= 17; bi *= 17;
            }
            c->r = (float)ri / 255.0f;
            c->g = (float)gi / 255.0f;
            c->b = (float)bi / 255.0f;
            snprintf(c->hex, sizeof(c->hex), "#%02x%02x%02x", ri, gi, bi);
        }
        if (suite && sscanf(suite, "%u", &alpha) == 1)
            c->a = (float)(alpha > 65535u ? 65535u : alpha) / 65535.0f;
    }
    g_free(copie);
}

void widget_colorbutton_refresh(variable *var)
{
    WidgetNode *n;
    gchar      *ligne;
    GList      *el = NULL;

    if (!var || !var->Widget) return;
    /* <input> : rejoué à chaque « refresh », comme l'étalon gtk3sermo (sa
     * première ligne), sans jouer d'action. Au premier passage, un <default>
     * l'emporte : l'étalon l'applique après (mesuré). Jusqu'au 2026-10-13 il
     * n'était jamais lu. */
    n = (WidgetNode *)var->Widget;
    ligne = widget_input_ligne(var->Attributes);
    colorbutton_poser(&n->state.color, ligne);
    g_free(ligne);
    if (!n->initialised) {
        n->initialised = TRUE;
        colorbutton_poser(&n->state.color,
                          attributeset_get_first(&el, var->Attributes, ATTR_DEFAULT));
    }
}
void widget_colorbutton_fileselect(variable *var, const char *n, const char *v) {}
void widget_colorbutton_removeselected(variable *var) {}
void widget_colorbutton_save(variable *var) {}
