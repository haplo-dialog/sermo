/*
 * widget_entry.cpp — Champ de saisie monoligne Qt6 (QLineEdit)
 * qt6sermo 1.0.0 — haplo-dialog <devel@haplo-dialog.fr>
 * Licence : GPL-2.0-or-later
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
#include "widget_entry.h"
#include "stringman.h"
#include <QtWidgets/QLineEdit>
#include <string.h>
#include <stdlib.h>
#include <stdio.h>

GtkWidget *widget_entry_create(AttributeSet *Attr, tag_attr *attr, gint Type)
{
    GList *element = NULL;
    int w = 200, h = 28;
    if (attr) {
        const char *v;
        if ((v = get_tag_attribute(attr, "width-request")))  w = atoi(v);
        if ((v = get_tag_attribute(attr, "height-request"))) h = atoi(v);
    }
    QLineEdit *le = new QLineEdit();
    le->resize(w, h);
    if (Attr) {
        element = NULL;
        gchar *def = attributeset_get_first(&element, Attr, ATTR_DEFAULT);
        if (def && *def) le->setText(QString::fromUtf8(def));

        /* <input>command</input> : lance la commande, pose sa PREMIERE ligne
         * comme valeur (CRLF final retires), via safe_popen. Parite gtk3sermo
         * widget_entry_input_by_command. Prime sur <default>. */
        element = NULL;
        gchar *incmd = attributeset_get_first(&element, Attr, ATTR_INPUT);
        if (incmd && input_is_shell_command(incmd)) {
            FILE *fp = widget_opencommand(input_get_shell_command(incmd));
            if (fp) {
                char line[512];
                if (fgets(line, sizeof(line), fp)) {
                    line[sizeof(line) - 1] = 0;
                    for (int c = (int)strlen(line) - 1; c >= 0; c--) {
                        if (line[c] == '\n' || line[c] == '\r') line[c] = 0;
                        else break;
                    }
                    le->setText(QString::fromUtf8(line));
                }
                fclose(fp);   /* safe_popen() uses fdopen() -- pclose() is UB */
            }
        }
    }
    if (attr) {
        const char *v;
        /* visibility="false" ou visible="false" → champ mot de passe */
        if ((v = get_tag_attribute(attr, "visibility")))
            if (g_ascii_strcasecmp(v, "false") == 0 || strcmp(v, "0") == 0)
                le->setEchoMode(QLineEdit::Password);
        if ((v = get_tag_attribute(attr, "max-length")))
            le->setMaxLength(atoi(v));
        if ((v = get_tag_attribute(attr, "sensitive")))
            if (g_ascii_strcasecmp(v, "false") == 0) le->setEnabled(false);
    }
    return (GtkWidget *)le;
}

gchar *widget_entry_envvar_construct(GtkWidget *widget)
{
    QLineEdit *le = static_cast<QLineEdit *>(widget);
    if (!le) return g_strdup("");
    return g_strdup(le->text().toUtf8().constData());
}
gchar *widget_entry_envvar_all_construct(variable *var) {
    if (!var || !var->Widget) return NULL;
    return widget_entry_envvar_construct(var->Widget);
}
void widget_entry_clear(variable *var) { if (var && var->Widget) static_cast<QLineEdit*>(var->Widget)->clear(); }
void widget_entry_refresh(variable *var) {}
void widget_entry_fileselect(variable *var, const char *n, const char *v) {
    if (var && var->Widget && v) static_cast<QLineEdit*>(var->Widget)->setText(QString::fromUtf8(v));
}
void widget_entry_removeselected(variable *var) {}
void widget_entry_save(variable *var) {}
