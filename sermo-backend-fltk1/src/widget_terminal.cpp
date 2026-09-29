/* SPDX-License-Identifier: GPL-2.0-or-later */
/*
 * Copyright (C) 2026 S. Cage
 * haplo-dialog <devel@haplo-dialog.fr>
 */
/* widget_terminal.cpp — Terminal embarqué FLTK
 * sermo — haplo-dialog — GPL-2.0-or-later
 *
 * Si HAVE_FL_TERMINAL=1 : Fl_Terminal (FLTK 1.4+)
 * Sinon : Fl_Multiline_Output en lecture seule (sortie de commande figée).
 * Sécurité : fclose() sur FILE* issu de widget_opencommand() — jamais pclose(). */
#ifndef _GNU_SOURCE
#define _GNU_SOURCE
#endif
#include "fltk-compat.h"
#include "gtkdialog.h"
#include "attributes.h"
#include "automaton.h"
#include "widgets.h"
#include "tag_attributes.h"
#include "safe_exec.h"
#include "widget_terminal.h"
#include <string.h>
#include <stdlib.h>
#include <stdio.h>

static const int TERMINAL_BUF_MAX = 1024 * 1024;  /* 1 Mo */

GtkWidget *widget_terminal_create(AttributeSet *Attr, tag_attr *attr, gint Type)
{
    int ww = 600, hh = 300;
    if (attr) {
        const char *v;
        if ((v = get_tag_attribute(attr, "width-request")))  ww = atoi(v);
        if ((v = get_tag_attribute(attr, "height-request"))) hh = atoi(v);
    }

#if HAVE_FL_TERMINAL
    Fl_Terminal *term = new Fl_Terminal(0, 0, ww, hh);
    term->ansi(true);
    if (Attr) {
        GList *el = NULL;
        gchar *cmd = attributeset_get_first(&el, Attr, ATTR_INPUT);
        if (cmd && *cmd) {
            FILE *fp = widget_opencommand(cmd);
            if (fp) {
                char line[4096];
                while (fgets(line, sizeof(line), fp))
                    term->append(line);
                fclose(fp);  /* safe_popen() fdopen() — jamais pclose() */
            }
        }
    }
    return (GtkWidget *)term;

#else
    /* Stub : Fl_Multiline_Output read-only */
    Fl_Multiline_Output *out = new Fl_Multiline_Output(0, 0, ww, hh);
    out->value("[terminal — Fl_Terminal non disponible (FLTK < 1.4)]");

    if (Attr) {
        GList *el = NULL;
        gchar *cmd = attributeset_get_first(&el, Attr, ATTR_INPUT);
        if (cmd && *cmd) {
            FILE *fp = widget_opencommand(cmd);
            if (fp) {
                char *buf = (char *)malloc(TERMINAL_BUF_MAX);
                if (buf) {
                    size_t total = 0; char line[4096];
                    while (fgets(line, sizeof(line), fp)) {
                        size_t len = strlen(line);
                        if (total + len >= (size_t)(TERMINAL_BUF_MAX - 1)) break;
                        memcpy(buf + total, line, len);
                        total += len;
                    }
                    buf[total] = '\0';
                    out->value(buf);
                    free(buf);
                }
                fclose(fp);  /* safe_popen() fdopen() — jamais pclose() */
            }
        }
    }
    return (GtkWidget *)out;
#endif
}

gchar *widget_terminal_envvar_construct(GtkWidget *w) { return g_strdup(""); }
gchar *widget_terminal_envvar_all_construct(variable *v) { return NULL; }
void widget_terminal_clear(variable *v) {
#if HAVE_FL_TERMINAL
    if (v && v->Widget) ((Fl_Terminal *)v->Widget)->clear();
#endif
}
void widget_terminal_refresh(variable *v)
{ if (v && v->Widget) ((Fl_Widget *)v->Widget)->redraw(); }
void widget_terminal_fileselect(variable *v, const char*, const char*) {}
void widget_terminal_removeselected(variable *v) {}
void widget_terminal_save(variable *v) {}
