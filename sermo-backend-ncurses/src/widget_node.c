/* SPDX-License-Identifier: GPL-2.0-or-later */
/*
 * Copyright (C) 2026 S. Cage
 * haplo-dialog <devel@haplo-dialog.fr>
 */
/*
 * widget_node.c — Gestion de l'arbre WidgetNode (implémentation C pure)
 * sermo — haplo-dialog — GPL-2.0-or-later
 *
 * Ce fichier implémente les fonctions déclarées dans dialog_state.h :
 *   widget_node_new(), widget_node_free(), widget_node_add_child()
 *
 * Ces fonctions ne dépendent pas de ncurses (terminal) — elles peuvent être
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

#include <stdlib.h>
#include <string.h>
#include <stdio.h>
#include "sermo_progress.h"

/* ─── Constantes ──────────────────────────────────────────────────────────── */
#define WIDGET_NODE_CHILD_INITIAL_CAP  4

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
