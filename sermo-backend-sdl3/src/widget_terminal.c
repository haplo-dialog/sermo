/* SPDX-License-Identifier: GPL-2.0-or-later */
/*
 * Copyright (C) 2026 S. Cage
 * haplo-dialog <devel@haplo-dialog.fr>
 */
/*
 * widget_terminal.c — Console ImGui avec scrollback v2
 * sermo — haplo-dialog <devel@haplo-dialog.fr>
 * Licence : GPL-2.0-or-later
 *
 * v2 : ring-buffer de TERMINAL_SCROLLBACK lignes (4096).
 * render.cpp affiche ImGui::InputText readonly multi-line + scrollbar.
 * La commande est lue via safe_popen() en arrière-plan (non-bloquant).
 * Les nouvelles lignes sont ajoutées via terminal_append_line().
 *
 * Commandes d'action :
 *   :clear:   vide le buffer
 *   :refresh: ré-exécute la commande
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
#include "widget_terminal.h"
#include "safe_exec.h"
#include <string.h>
#include <stdlib.h>
#include <stdio.h>

/* -----------------------------------------------------------------------
 * terminal_append_line — ajoute une ligne dans le ring buffer
 * ----------------------------------------------------------------------- */
void terminal_append_line(TerminalState *t, const char *line)
{
    if (!t || !line) return;
    int slot = t->head % TERMINAL_SCROLLBACK;
    snprintf(t->lines[slot], TERMINAL_LINE_MAX, "%s", line);
    /* Supprimer le \n final si présent */
    int l = (int)strlen(t->lines[slot]);
    if (l > 0 && t->lines[slot][l-1] == '\n')
        t->lines[slot][l-1] = '\0';
    t->head = (t->head + 1) % TERMINAL_SCROLLBACK;
    if (t->count < TERMINAL_SCROLLBACK) t->count++;
}

const char *terminal_get_line(const TerminalState *t, int idx)
{
    if (!t || idx < 0 || idx >= t->count) return "";
    /* idx=0 → ligne la plus ancienne, idx=count-1 → la plus récente */
    int base = (t->count < TERMINAL_SCROLLBACK)
        ? 0
        : (t->head) % TERMINAL_SCROLLBACK;
    int slot = (base + idx) % TERMINAL_SCROLLBACK;
    return t->lines[slot];
}

/* -----------------------------------------------------------------------
 * Lecture commande et remplissage du buffer
 * ----------------------------------------------------------------------- */
static void _load_command_output(WidgetNode *node, const char *cmd)
{
    if (!cmd || !*cmd) return;
    FILE *fp = safe_popen(cmd);
    if (!fp) {
        terminal_append_line(&node->state.terminal, "[sdl3sermo] command failed");
        return;
    }
    char line[TERMINAL_LINE_MAX];
    while (fgets(line, sizeof(line), fp))
        terminal_append_line(&node->state.terminal, line);
    fclose(fp);
}

/* -----------------------------------------------------------------------
 * widget_terminal_create
 * ----------------------------------------------------------------------- */
GtkWidget *widget_terminal_create(AttributeSet *Attr, tag_attr *attr, gint Type)
{
    WidgetNode *node = widget_node_new(WT_TERMINAL_W, NULL, "Terminal");
    memset(&node->state.terminal, 0, sizeof(node->state.terminal));
    node->state.terminal.autoscroll = TRUE;

    if (Attr) {
        GList *el = NULL;
        gchar *lbl = attributeset_get_first(&el, Attr, ATTR_LABEL);
        if (lbl && *lbl) { free(node->label); node->label = strdup(lbl); }
        el = NULL;
        gchar *cmd = attributeset_get_first(&el, Attr, ATTR_INPUT);
        if (cmd && *cmd) {
            snprintf(node->state.terminal.cmd,
                     sizeof(node->state.terminal.cmd), "%s", cmd);
            node->action = strdup(cmd);
            _load_command_output(node, cmd);
        }
    }

    node->width  = node->width  ? node->width  : 600;
    node->height = node->height ? node->height : 300;

    return (GtkWidget *)node;
}

gchar *widget_terminal_envvar_construct(GtkWidget *widget)
{
    WidgetNode *n = (WidgetNode *)widget;
    if (!n || n->state.terminal.count == 0) return g_strdup("");

    /* Concaténer toutes les lignes */
    size_t total = 0;
    for (int i = 0; i < n->state.terminal.count; i++)
        total += strlen(terminal_get_line(&n->state.terminal, i)) + 1;

    char *buf = (char *)malloc(total + 1);
    if (!buf) return g_strdup("");
    buf[0] = '\0';
    for (int i = 0; i < n->state.terminal.count; i++) {
        g_strlcat(buf, terminal_get_line(&n->state.terminal, i), total + 1);
        g_strlcat(buf, "\n", total + 1);
    }
    return buf;
}
gchar *widget_terminal_envvar_all_construct(variable *var)
{
    if (!var || !var->Widget) return NULL;
    return widget_terminal_envvar_construct(var->Widget);
}
void widget_terminal_clear(variable *var)
{
    if (!var || !var->Widget) return;
    WidgetNode *n = (WidgetNode *)var->Widget;
    memset(&n->state.terminal, 0, sizeof(n->state.terminal));
    n->state.terminal.autoscroll = TRUE;
}
void widget_terminal_refresh(variable *var)
{
    if (!var || !var->Widget) return;
    WidgetNode *n = (WidgetNode *)var->Widget;
    widget_terminal_clear(var);
    if (n->state.terminal.cmd[0])
        _load_command_output(n, n->state.terminal.cmd);
}
void widget_terminal_fileselect(variable *var, const char *nm, const char *v) {}
void widget_terminal_removeselected(variable *var) {}
void widget_terminal_save(variable *var) {}
