/* SPDX-License-Identifier: GPL-2.0-or-later */
/*
 * Copyright (C) 2026 S. Cage
 * haplo-dialog <devel@haplo-dialog.fr>
 */
/*
 * widget_colorbutton.c — Sélecteur de couleur ncurses (terminal) v2
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
#include "ncurses-compat.h"
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
#include <ctype.h>

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

/* Une couleur comme gdk_rgba_parse() de l'étalon la lit, sans GDK : blancs
 * autour ôtés, puis « #rgb », « #rrggbb », « #rrrgggbbb », « #rrrrggggbbbb »,
 * ou « rgb(r,g,b) » / « rgba(r,g,b,a) » (entiers 0..255 ou pourcentages).
 * Les NOMS de couleur (« red »…) que GDK prend dans la table de Pango ne sont
 * pas lus ici. Chaîne illisible : FALSE, la couleur ne change pas (mesuré sur
 * l'étalon : « bogus » laisse la couleur d'avant). */
static float composante_rgb(const char *s)
{
    char *fin = NULL;
    double v = g_ascii_strtod(s, &fin);
    while (fin && (*fin == ' ' || *fin == '\t')) fin++;
    if (fin && *fin == '%') v = v * 255.0 / 100.0;
    if (v < 0.0) v = 0.0;
    if (v > 255.0) v = 255.0;
    return (float)(v / 255.0);
}

static gboolean couleur_lire(const char *s, ColorState *c)
{
    gchar *t;
    gboolean ok = FALSE;
    float r = 0, g = 0, b = 0;

    if (!s) return FALSE;
    t = g_strstrip(g_strdup(s));
    if (t[0] == '#') {
        size_t L = strlen(t + 1), k;
        gboolean hexa = TRUE;
        for (k = 1; k <= L; k++) if (!isxdigit((unsigned char)t[k])) hexa = FALSE;
        if (hexa && (L == 3 || L == 6 || L == 9 || L == 12)) {
            int nd = (int)(L / 3), i;
            float v[3];
            for (i = 0; i < 3; i++) {
                char part[5];
                memcpy(part, t + 1 + i * nd, nd); part[nd] = '\0';
                unsigned long x = strtoul(part, NULL, 16);
                v[i] = (float)((double)x / (double)((1ul << (4 * nd)) - 1));
            }
            r = v[0]; g = v[1]; b = v[2]; ok = TRUE;
        }
    } else if (g_ascii_strncasecmp(t, "rgb(", 4) == 0 ||
               g_ascii_strncasecmp(t, "rgba(", 5) == 0) {
        /* Pas de g_strsplit : celui de la couche de compatibilité passe par
         * strtok et sauterait un champ vide. On compte les virgules. */
        gchar *dedans = strchr(t, '(') + 1, *ferme = strchr(dedans, ')');
        int attendu = (t[3] == '(') ? 3 : 4, nb = 1;
        const char *c1, *c2 = NULL;
        if (ferme && ferme[1] == '\0') {
            *ferme = '\0';
            for (const char *q = dedans; *q; q++) if (*q == ',') nb++;
            c1 = strchr(dedans, ',');
            if (c1) c2 = strchr(c1 + 1, ',');
            if (nb == attendu && c1 && c2) {
                r = composante_rgb(dedans); g = composante_rgb(c1 + 1);
                b = composante_rgb(c2 + 1); ok = TRUE;
            }
        }
    }
    g_free(t);
    if (!ok) return FALSE;
    c->r = r; c->g = g; c->b = b;
    snprintf(c->hex, sizeof(c->hex), "#%02x%02x%02x",
             (unsigned)(r * 255.0f + 0.5f), (unsigned)(g * 255.0f + 0.5f),
             (unsigned)(b * 255.0f + 0.5f));
    return TRUE;
}

/* « couleur|alpha », comme l'étalon (linecutter sur « | ») : la couleur, puis
 * un alpha entier 0..65535 lu par sscanf %u. L'alpha est gardé dans c->a ;
 * ce port n'exporte pas « |alpha » (use-alpha="true" n'y est pas lu). */
static void couleur_poser(const char *s, ColorState *c)
{
    gchar *couleur;
    const char *barre;
    unsigned int alpha;

    if (!s) return;
    barre = strchr(s, '|');
    couleur = barre ? g_strndup(s, (gsize)(barre - s)) : g_strdup(s);
    couleur_lire(couleur, c);
    g_free(couleur);
    if (barre && sscanf(barre + 1, "%u", &alpha) == 1)
        c->a = (float)(alpha > 65535u ? 65535u : alpha) / 65535.0f;
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
    /* Reconstruire hex depuis floats (render.cpp peut les modifier), ARRONDI
     * comme l'étalon (×255 + 0,5) : tronquer rendait #000002 pour #000003. */
    snprintf(n->state.color.hex, sizeof(n->state.color.hex), "#%02x%02x%02x",
             (unsigned)(n->state.color.r * 255.0f + 0.5f),
             (unsigned)(n->state.color.g * 255.0f + 0.5f),
             (unsigned)(n->state.color.b * 255.0f + 0.5f));
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
    /* <input> (commande ou fichier) : rejoué à chaque « refresh », comme
     * l'étalon gtk3sermo — sa PREMIÈRE ligne, CR/LF ôtés ; rien de lisible,
     * rien ne change ; aucune action ne joue. Jusqu'au 2026-10-13 il n'était
     * jamais lu. */
    if (!var || !var->Widget) return;
    WidgetNode *n = (WidgetNode *)var->Widget;
    gchar *ligne = widget_input_ligne(var->Attributes);
    if (ligne) { couleur_poser(ligne, &n->state.color); g_free(ligne); }
    /* Premier passage : <default> l'emporte s'il est lisible, l'étalon
     * l'applique APRÈS l'<input>. */
    if (!n->initialised) {
        GList *el = NULL;
        gchar *def = attributeset_get_first(&el, var->Attributes, ATTR_DEFAULT);
        n->initialised = TRUE;
        if (def) couleur_poser(def, &n->state.color);
    }
}
void widget_colorbutton_fileselect(variable *var, const char *n, const char *v) {}
void widget_colorbutton_removeselected(variable *var) {}
void widget_colorbutton_save(variable *var) {}
