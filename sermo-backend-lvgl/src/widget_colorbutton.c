/* SPDX-License-Identifier: GPL-2.0-or-later */
/*
 * Copyright (C) 2026 S. Cage
 * haplo-dialog <devel@haplo-dialog.fr>
 */
/*
 * widget_colorbutton.c — Sélecteur de couleur LVGL v2
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
#include "lvgl-compat.h"
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

/* « couleur|alpha » → couleur, comme l'<input> et le <default> de l'étalon :
 * la couleur avant le « | », blancs ôtés, en « #rrggbb » ou « #rgb » ; l'alpha
 * après (entier 0..65535). Une couleur illisible (ou une ligne vide) laisse la
 * valeur en place, comme gdk_rgba_parse() qui échoue. Les noms (« blue ») ne
 * sont pas lus : écart connu. Rend TRUE si la couleur a été posée. */
static gboolean couleur_poser(ColorState *c, const char *s)
{
    char        *copie, *barre, *h;
    unsigned int ri, gi, bi, alpha;
    gboolean     lu = FALSE;

    if (!s) return FALSE;
    copie = strdup(s);
    if (!copie) return FALSE;
    barre = strchr(copie, '|');          /* pas g_strsplit : celui de la couche
                                          * de compatibilité saute les champs vides */
    if (barre) *barre = '\0';
    h = g_strstrip(copie);
    if (h[0] == '#' && strlen(h) == 7 && strspn(h + 1, "0123456789abcdefABCDEF") == 6 &&
        sscanf(h + 1, "%02x%02x%02x", &ri, &gi, &bi) == 3) {
        lu = TRUE;
    } else if (h[0] == '#' && strlen(h) == 4 && strspn(h + 1, "0123456789abcdefABCDEF") == 3 &&
               sscanf(h + 1, "%1x%1x%1x", &ri, &gi, &bi) == 3) {
        ri *= 17; gi *= 17; bi *= 17;
        lu = TRUE;
    }
    if (lu) {
        c->r = (float)ri / 255.0f;
        c->g = (float)gi / 255.0f;
        c->b = (float)bi / 255.0f;
        snprintf(c->hex, sizeof(c->hex), "#%02x%02x%02x", ri, gi, bi);
        if (barre && sscanf(barre + 1, "%u", &alpha) == 1)
            c->a = (float)(alpha > 65535u ? 65535u : alpha) / 65535.0f;
    }
    free(copie);
    return lu;
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
    /* Reconstruire hex depuis floats (render.cpp peut les modifier), arrondi
     * comme l'étalon (× 255 + 0,5) : la troncature rendait « #000002 » pour
     * une composante posée à 3/255 (corrigé le 2026-10-13). */
    snprintf(n->state.color.hex, sizeof(n->state.color.hex), "#%02x%02x%02x",
             (int)(n->state.color.r * 255.0f + 0.5f),
             (int)(n->state.color.g * 255.0f + 0.5f),
             (int)(n->state.color.b * 255.0f + 0.5f));
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
void widget_colorbutton_refresh(variable *var)
{
    /* <input> : rejoué à chaque « refresh », comme l'étalon gtk3sermo, sans
     * jouer d'action ; render_lvgl.c (synchroniser) repeint le bouton.
     * Jusqu'au 2026-10-13 il n'était jamais lu. Au premier passage, un
     * <default> lisible l'emporte : l'étalon l'applique après l'<input>. */
    WidgetNode *n;
    gchar      *ligne;
    GList      *el = NULL;

    if (!var || !var->Widget) return;
    n = (WidgetNode *)var->Widget;
    ligne = widget_input_line(var->Attributes);
    if (ligne) { couleur_poser(&n->state.color, ligne); g_free(ligne); }
    if (!n->initialised) {
        n->initialised = TRUE;
        couleur_poser(&n->state.color, attributeset_get_first(&el, var->Attributes, ATTR_DEFAULT));
    }
}
void widget_colorbutton_fileselect(variable *var, const char *n, const char *v) {}
void widget_colorbutton_removeselected(variable *var) {}
void widget_colorbutton_save(variable *var) {}
