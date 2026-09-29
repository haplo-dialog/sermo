/* SPDX-License-Identifier: GPL-2.0-or-later */
/*
 * Copyright (C) 2026 S. Cage
 * haplo-dialog <devel@haplo-dialog.fr>
 */
/*
 * widget_terminal.c — Terminal EFL via ecore_exe + elm_entry scrollable
 * sermo — haplo-dialog <devel@haplo-dialog.fr>
 * Licence : GPL-2.0-or-later
 *
 * Implémentation :
 *   - Si HAVE_ECORE_EXE : lance la commande via ecore_exe_pipe_run(),
 *     reçoit stdout/stderr en temps réel via ECORE_EXE_EVENT_DATA,
 *     affiche dans elm_entry read-only scrollable.
 *   - Sinon : charge la sortie en une fois via widget_opencommand()
 *     (popen safe), avec fclose() obligatoire.
 *
 * Sécurité :
 *   - La commande passe par safe_popen() (pas popen() direct).
 *   - fclose() utilisé sur FILE* issu de widget_opencommand() — JAMAIS pclose().
 *   - Pas de realloc() illimité : buffer limité à TERMINAL_BUF_MAX octets.
 *   - Pas d'eval, pas de $() shell non validé.
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
#include "widget_terminal.h"
#include "safe_exec.h"
#include <string.h>
#include <stdlib.h>
#include <stdio.h>

#define TERMINAL_BUF_MAX  (1024 * 1024)  /* 1 Mo max */
#define TERMINAL_LINE_MAX 4096

/* ─── Données associées au widget terminal ───────────────────────────────── */
typedef struct {
    Evas_Object *entry;     /* elm_entry affichant la sortie */
    char        *buf;       /* Buffer accumulé */
    size_t       buf_len;   /* Longueur courante */
    size_t       buf_cap;   /* Capacité allouée */
#if HAVE_ECORE_EXE
    Ecore_Exe   *exe;       /* Processus lancé */
    Ecore_Event_Handler *data_handler; /* Handler stdout */
    Ecore_Event_Handler *del_handler;  /* Handler fin processus */
#endif
} TerminalData;

/* ─── Append sécurisé dans le buffer ─────────────────────────────────────── */
static void _term_buf_append(TerminalData *td, const char *text, size_t len)
{
    if (!td || !text || len == 0) return;
    if (td->buf_len + len + 1 > TERMINAL_BUF_MAX) return; /* limite dure */
    if (td->buf_len + len + 1 > td->buf_cap) {
        size_t new_cap = td->buf_cap ? td->buf_cap * 2 : 4096;
        while (new_cap < td->buf_len + len + 1) new_cap *= 2;
        if (new_cap > TERMINAL_BUF_MAX) new_cap = TERMINAL_BUF_MAX;
        char *nb = realloc(td->buf, new_cap);
        if (!nb) return;
        td->buf = nb;
        td->buf_cap = new_cap;
    }
    memcpy(td->buf + td->buf_len, text, len);
    td->buf_len += len;
    td->buf[td->buf_len] = '\0';
}

#if HAVE_ECORE_EXE
/* ─── Callback : données reçues de ecore_exe ─────────────────────────────── */
static Eina_Bool _exe_data_cb(void *data, int type, void *event)
{
    TerminalData *td = (TerminalData *)data;
    Ecore_Exe_Event_Data *ev = (Ecore_Exe_Event_Data *)event;
    if (!ev || !ev->data || ev->size <= 0) return ECORE_CALLBACK_PASS_ON;
    if (ev->exe != td->exe)               return ECORE_CALLBACK_PASS_ON;

    _term_buf_append(td, (const char *)ev->data, (size_t)ev->size);

    /* Mettre à jour elm_entry avec le buffer courant */
    if (td->entry && td->buf)
        elm_entry_entry_set(td->entry, td->buf);

    return ECORE_CALLBACK_PASS_ON;
}

/* ─── Callback : fin du processus ─────────────────────────────────────────── */
static Eina_Bool _exe_del_cb(void *data, int type, void *event)
{
    TerminalData *td = (TerminalData *)data;
    Ecore_Exe_Event_Del *ev = (Ecore_Exe_Event_Del *)event;
    if (!ev || ev->exe != td->exe) return ECORE_CALLBACK_PASS_ON;

    ecore_event_handler_del(td->data_handler); td->data_handler = NULL;
    ecore_event_handler_del(td->del_handler);  td->del_handler  = NULL;
    td->exe = NULL;
    return ECORE_CALLBACK_PASS_ON;
}
#endif /* HAVE_ECORE_EXE */

GtkWidget *widget_terminal_create(AttributeSet *Attr, tag_attr *attr, gint Type)
{
    Evas_Object *win = efl_main_win_get();
    Evas_Object *parent = win ? win : elm_win_add(NULL, "efl1dialog-tmp", ELM_WIN_BASIC);

    /* ── elm_entry read-only scrollable ────────────────────────────────── */
    Evas_Object *en = elm_entry_add(parent);
    elm_entry_single_line_set(en, EINA_FALSE);
    elm_entry_editable_set(en, EINA_FALSE);
    elm_entry_scrollable_set(en, EINA_TRUE);
    elm_entry_line_wrap_set(en, ELM_WRAP_CHAR);
    evas_object_size_hint_weight_set(en, EVAS_HINT_EXPAND, EVAS_HINT_EXPAND);
    evas_object_size_hint_align_set(en, EVAS_HINT_FILL, EVAS_HINT_FILL);

    TerminalData *td = calloc(1, sizeof(TerminalData));
    if (!td) { evas_object_show(en); return (GtkWidget *)en; }
    td->entry   = en;
    td->buf     = NULL;
    td->buf_len = 0;
    td->buf_cap = 0;

    evas_object_data_set(en, "term_data", td);

    if (!Attr) { evas_object_show(en); return (GtkWidget *)en; }

    GList *el = NULL;
    gchar *cmd = attributeset_get_first(&el, Attr, ATTR_INPUT);
    if (!cmd || !*cmd) {
        evas_object_show(en);
        return (GtkWidget *)en;
    }

#if HAVE_ECORE_EXE
    /* ── Mode temps réel : ecore_exe_pipe_run ──────────────────────────── */
    td->exe = ecore_exe_pipe_run(cmd,
                                  ECORE_EXE_PIPE_READ |
                                  ECORE_EXE_PIPE_READ_LINE_BUFFERED |
                                  ECORE_EXE_PIPE_ERROR,
                                  td);
    if (td->exe) {
        td->data_handler = ecore_event_handler_add(
            ECORE_EXE_EVENT_DATA, _exe_data_cb, td);
        td->del_handler  = ecore_event_handler_add(
            ECORE_EXE_EVENT_DEL,  _exe_del_cb,  td);
    } else {
        elm_entry_entry_set(en, "[terminal : échec lancement commande]");
    }
#else
    /* ── Fallback : lecture complète via safe_popen ─────────────────────── */
    FILE *fp = widget_opencommand(cmd);
    if (fp) {
        char line[TERMINAL_LINE_MAX];
        while (fgets(line, sizeof(line), fp))
            _term_buf_append(td, line, strlen(line));
        /* CRITIQUE : fclose(), pas pclose() — FILE* issu de fdopen() */
        fclose(fp);
        if (td->buf)
            elm_entry_entry_set(en, td->buf);
    } else {
        elm_entry_entry_set(en, "[terminal : commande introuvable]");
    }
#endif

    evas_object_show(en);
    return (GtkWidget *)en;
}

gchar *widget_terminal_envvar_construct(GtkWidget *w)
{
    if (!w) return g_strdup("");
    TerminalData *td = (TerminalData *)evas_object_data_get((Evas_Object *)w, "term_data");
    if (!td || !td->buf) return g_strdup("");
    return g_strdup(td->buf);
}
gchar *widget_terminal_envvar_all_construct(variable *var)
{
    if (!var || !var->Widget) return NULL;
    return widget_terminal_envvar_construct(var->Widget);
}
void widget_terminal_clear(variable *var)
{
    if (!var || !var->Widget) return;
    TerminalData *td = (TerminalData *)evas_object_data_get(
        (Evas_Object *)var->Widget, "term_data");
    if (td) { free(td->buf); td->buf = NULL; td->buf_len = 0; }
    elm_entry_entry_set((Evas_Object *)var->Widget, "");
}
void widget_terminal_refresh(variable *var) {}
void widget_terminal_fileselect(variable *var, const char *n, const char *v) {}
void widget_terminal_removeselected(variable *var) {}
void widget_terminal_save(variable *var) {}
