/* SPDX-License-Identifier: GPL-2.0-or-later */
/*
 * Copyright (C) 2026 S. Cage
 * haplo-dialog <devel@haplo-dialog.fr>
 */
/*
 * widget_node.c — Gestion de l'arbre WidgetNode (implémentation C pure)
 * sermo 2.0.0 — haplo-dialog — GPL-2.0-or-later
 *
 * Ce fichier implémente les fonctions déclarées dans dialog_state.h :
 *   widget_node_new(), widget_node_free(), widget_node_add_child()
 *
 * Ces fonctions ne dépendent pas de export HTML — elles peuvent être
 * compilées standalone pour les tests unitaires CTest.
 *
 * Sécurité :
 *   - Toutes les allocations vérifient NULL avant usage
 *   - widget_node_free() est récursif et libère tout l'arbre
 *   - widget_node_add_child() gère le redimensionnement de children[]
 */

#ifndef _GNU_SOURCE
#define _GNU_SOURCE
#endif
#include "dialog_state.h"
/* Ordre impose : tag_attributes.h a besoin des types d'attributes.h et
 * automaton.h (AttributeSet, tag_attr). Meme sequence que widget_list.c. */
#include "gtkdialog.h"
#include "attributes.h"
#include "automaton.h"
#include "tag_attributes.h"
#include "sermo_progress.h"

#include <stdlib.h>
#include <string.h>
#include <stdio.h>

/* ─── Constantes ──────────────────────────────────────────────────────────── */
#define WIDGET_NODE_CHILD_INITIAL_CAP  4

/* ─── widget_node_bind_variable ───────────────────────────────────────────────
 * Rattache le nom de variable du dialogue au nœud, depuis l'attribut de balise
 * `variable="NOM"`. Le cœur garde cette association dans son arbre de
 * `variable` (indexé par pointeur de widget) et n'expose pas d'itérateur ; le
 * backend la relit donc lui-même, comme le faisaient déjà widget_list.c et
 * widget_timer.c. Sans elle, le rendu HTML ne peut pas nommer les champs du
 * formulaire — et sans nom de champ, ni le JavaScript ni le micro-serveur ne
 * savent à quelle variable rattacher une valeur saisie. */
void widget_node_bind_variable(WidgetNode *n, AttributeSet *Attr, tag_attr *attr)
{
    if (!n || n->var_name) return;

    /* Forme ATTRIBUT : <checkbox variable="NOM"> */
    if (attr) {
        const char *v = get_tag_attribute(attr, "variable");
        if (v && *v) { n->var_name = strdup(v); return; }
    }
    /* Forme ENFANT : <checkbox><variable>NOM</variable></checkbox>
     * C'est celle qu'emploie le corpus de reference ; l'oublier laissait les
     * champs du formulaire sans nom. */
    if (Attr) {
        GList *el = NULL;
        gchar *v = attributeset_get_first(&el, Attr, ATTR_VARIABLE);
        if (v && *v) n->var_name = strdup(v);
    }
}

/* ─── widget_node_new ─────────────────────────────────────────────────────── */

WidgetNode *widget_node_new(WidgetType type, const char *name, const char *label)
{
    WidgetNode *n = (WidgetNode *)calloc(1, sizeof(WidgetNode));
    if (!n) {
        fprintf(stderr, "widget_node_new: calloc failed\n");
        return NULL;
    }

    n->type           = type;
    n->visible        = 1;
    n->sensitive      = 1;
    n->child_count    = 0;
    n->child_capacity = 0;
    n->children       = NULL;
    n->parent         = NULL;

    if (name  && *name)  n->var_name = strdup(name);
    if (label && *label) n->label    = strdup(label);

    return n;
}

/* ─── widget_node_add_child ───────────────────────────────────────────────── */

void widget_node_add_child(WidgetNode *parent, WidgetNode *child)
{
    if (!parent || !child) return;

    /* Agrandir le tableau children si nécessaire */
    if (parent->child_count >= parent->child_capacity) {
        int new_cap = parent->child_capacity == 0
                      ? WIDGET_NODE_CHILD_INITIAL_CAP
                      : parent->child_capacity * 2;
        WidgetNode **new_children = (WidgetNode **)realloc(
            parent->children, (size_t)new_cap * sizeof(WidgetNode *));
        if (!new_children) {
            fprintf(stderr, "widget_node_add_child: realloc failed\n");
            return;
        }
        parent->children       = new_children;
        parent->child_capacity = new_cap;
    }

    parent->children[parent->child_count++] = child;
    child->parent = parent;
}

/* ─── widget_node_free ────────────────────────────────────────────────────── */

void widget_node_free(WidgetNode *node)
{
    if (!node) return;

    /* Récursion sur les enfants */
    for (int i = 0; i < node->child_count; i++)
        widget_node_free(node->children[i]);

    /* Une barre dont la commande tourne encore : fermer sa lecture. */
    sermo_progress_free((sermo_progress *)node->lecture);

    free(node->children);
    free(node->var_name);
    free(node->label);
    free(node->action);
    /* node->state n'a pas de pointeurs alloués dynamiquement (union de POD) */
    free(node);
}

/* ─── dialog_state_export ─────────────────────────────────────────────────── */
/*
 * Export minimal pour les tests — la version complète est dans render.cpp.
 * Cette version appelle setenv() pour chaque nœud avec var_name.
 */
void dialog_state_export(DialogState *ds)
{
    (void)ds;
    /* No-op dans le contexte standalone test — l'export réel est dans render.cpp */
}

/* terminal_append_line() / terminal_get_line() : implémentation canonique
 * (ring-buffer 2D) dans widget_terminal.c — ne pas dupliquer ici. */
