/*
 * widget_edit.cpp — Zone de texte multi-lignes éditable Qt6 (QTextEdit)
 * qt6sermo 1.0.0 — haplo-dialog <devel@haplo-dialog.fr>
 * Licence : GPL-2.0-or-later
 *
 * <edit> → QTextEdit éditable
 * L'attribut <input>cmd</input> alimente le contenu via popen().
 * Export : texte en clair (toPlainText)
 */

#ifndef _GNU_SOURCE
#define _GNU_SOURCE
#endif
#include "qt6-compat.h"
#include "gtk3d.h"
#include "attributes.h"
#include "automaton.h"
#include "widgets.h"
#include "tag_attributes.h"
#include "widget_edit.h"
#include "safe_exec.h"

#include <QtWidgets/QTextEdit>

#include <string.h>
#include <stdlib.h>
#include <stdio.h>

static char *read_command_output(const char *cmd)
{
    FILE *fp = widget_opencommand(cmd);
    if (!fp) return strdup("");
    size_t size = 0, cap = 4096;
    char  *buf  = (char *)malloc(cap);
    if (!buf) { fclose(fp);  /* safe_popen() uses fdopen() — pclose() is UB */ return strdup(""); }
    int c;
    while ((c = fgetc(fp)) != EOF) {
        if (size + 1 >= cap) {
            cap *= 2;
            char *nb = (char *)realloc(buf, cap);
            if (!nb) break;
            buf = nb;
        }
        buf[size++] = (char)c;
    }
    buf[size] = '\0';
    fclose(fp);  /* safe_popen() uses fdopen() — pclose() is UB */
    return buf;
}

GtkWidget *widget_edit_create(AttributeSet *Attr, tag_attr *attr, gint Type)
{
    int w = 300, h = 150;
    if (attr) {
        const char *v;
        if ((v = get_tag_attribute(attr, "width-request")))  w = atoi(v);
        if ((v = get_tag_attribute(attr, "height-request"))) h = atoi(v);
    }

    QTextEdit *te = new QTextEdit();
    te->setReadOnly(false);
    te->setMinimumSize(w, h);

    if (Attr) {
        GList *element = NULL;
        gchar *cmd = attributeset_get_first(&element, Attr, ATTR_INPUT);
        if (cmd && *cmd) {
            char *content = read_command_output(cmd);
            te->setPlainText(QString::fromUtf8(content));
            free(content);
        }
        element = NULL;
        gchar *def = attributeset_get_first(&element, Attr, ATTR_DEFAULT);
        if (def && *def && te->toPlainText().isEmpty())
            te->setPlainText(QString::fromUtf8(def));
    }

    return (GtkWidget *)te;
}

gchar *widget_edit_envvar_construct(GtkWidget *widget)
{
    QTextEdit *te = static_cast<QTextEdit *>(widget);
    if (!te) return g_strdup("");
    return g_strdup(te->toPlainText().toUtf8().constData());
}

gchar *widget_edit_envvar_all_construct(variable *var)
{
    if (!var || !var->Widget) return NULL;
    return widget_edit_envvar_construct(var->Widget);
}

void widget_edit_clear(variable *var)
{
    if (!var || !var->Widget) return;
    static_cast<QTextEdit *>(var->Widget)->clear();
}

void widget_edit_refresh(variable *var)
{
    if (!var || !var->Widget) return;
    QTextEdit *te = static_cast<QTextEdit *>(var->Widget);
    if (var->Attributes) {
        GList *element = NULL;
        gchar *cmd = attributeset_get_first(&element, var->Attributes, ATTR_INPUT);
        if (cmd && *cmd) {
            char *content = read_command_output(cmd);
            te->setPlainText(QString::fromUtf8(content));
            free(content);
        }
    }
    te->update();
}

void widget_edit_fileselect(variable *var, const char *n, const char *v) {}
void widget_edit_removeselected(variable *var) {}
void widget_edit_save(variable *var) {}
