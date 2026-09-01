/*
 * widget_progressbar.cpp — Barre de progression Qt6 (QProgressBar)
 * qt6sermo 1.0.0 — haplo-dialog <devel@haplo-dialog.fr>
 * Licence : GPL-2.0-or-later
 *
 * <progressbar> → QProgressBar (0-100)
 * Export : valeur courante en chaîne
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
#include "widget_progressbar.h"

#include <QtWidgets/QProgressBar>

#include <string.h>
#include <stdlib.h>
#include <stdio.h>

/* Lit la valeur produite par la commande <input> (ex. « echo 65 »), comme le
 * port de référence gtk. L'attribut ATTR_INPUT est stocké préfixé
 * « Command:… » : input_get_shell_command() retire le préfixe (sinon
 * widget_opencommand échoue et la barre reste à 0). widget_opencommand ==
 * safe_popen (fdopen → fclose, jamais pclose). */
static int progressbar_value_from_command(const char *input, int fallback)
{
    const char *cmd = input_get_shell_command(input);
    if (!cmd || !*cmd) cmd = input;
    if (!cmd || !*cmd) return fallback;
    FILE *fp = widget_opencommand(cmd);
    if (!fp) return fallback;
    char line[64];
    int  v = fallback;
    if (fgets(line, sizeof line, fp)) v = (int)atof(line);
    fclose(fp);
    return v;
}

GtkWidget *widget_progressbar_create(AttributeSet *Attr, tag_attr *attr, gint Type)
{
    QProgressBar *pb = new QProgressBar();
    pb->setMinimum(0);
    pb->setMaximum(100);
    pb->setValue(0);

    if (Attr) {
        GList *element = NULL;
        /* Priorité à la commande <input> (ex. « echo 65 ») comme gtk ; sinon
         * attribut value= ou <default>. Sans cela la barre restait à 0 %. */
        gchar *cmd = attributeset_get_first(&element, Attr, ATTR_INPUT);
        const char *val = NULL;
        if (attr) val = get_tag_attribute(attr, "value");
        if (!val) {
            GList *e2 = NULL;
            val = attributeset_get_first(&e2, Attr, ATTR_DEFAULT);
        }
        if (cmd && *cmd)      pb->setValue(progressbar_value_from_command(cmd, 0));
        else if (val)         pb->setValue((int)atof(val));

        element = NULL;
        gchar *txt = attributeset_get_first(&element, Attr, ATTR_LABEL);
        if (txt && *txt) pb->setFormat(QString::fromUtf8(txt));
    }

    if (attr) {
        const char *v;
        if ((v = get_tag_attribute(attr, "width-request")))  pb->setMinimumWidth(atoi(v));
        if ((v = get_tag_attribute(attr, "height-request"))) pb->setMinimumHeight(atoi(v));
    }

    return (GtkWidget *)pb;
}

gchar *widget_progressbar_envvar_construct(GtkWidget *widget)
{
    QProgressBar *pb = static_cast<QProgressBar *>(widget);
    if (!pb) return g_strdup("0");
    char buf[32];
    snprintf(buf, sizeof(buf), "%d", pb->value());
    return g_strdup(buf);
}

gchar *widget_progressbar_envvar_all_construct(variable *var)
{
    if (!var || !var->Widget) return NULL;
    return widget_progressbar_envvar_construct(var->Widget);
}

void widget_progressbar_clear(variable *var)
{
    if (!var || !var->Widget) return;
    static_cast<QProgressBar *>(var->Widget)->setValue(0);
}

void widget_progressbar_refresh(variable *var)
{
    if (!var || !var->Widget) return;
    static_cast<QWidget *>(var->Widget)->update();
}

void widget_progressbar_fileselect(variable *var, const char *n, const char *v) {}
void widget_progressbar_removeselected(variable *var) {}
void widget_progressbar_save(variable *var) {}
