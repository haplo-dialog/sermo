/*
 * widget_expander.cpp — Zone dépliable Qt6
 * qt6sermo 1.0.0 — haplo-dialog <devel@haplo-dialog.fr>
 * Licence : GPL-2.0-or-later
 *
 * <expander> → QGroupBox avec setCheckable(true) qui agit comme expander.
 * Qt6 n'a pas de QExpander natif ; QGroupBox checkable est le plus proche.
 * Export : "true" si déplié (checked), "false" sinon.
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
#include "widget_expander.h"

#include <QtWidgets/QGroupBox>
#include <QtWidgets/QVBoxLayout>

#include <string.h>
#include <stdlib.h>

GtkWidget *widget_expander_create(AttributeSet *Attr, tag_attr *attr, gint Type)
{
    const char *label = "Expander";
    if (Attr) {
        GList *element = NULL;
        gchar *lbl = attributeset_get_first(&element, Attr, ATTR_LABEL);
        if (lbl && *lbl) label = lbl;
    }

    QGroupBox   *gb     = new QGroupBox(QString::fromUtf8(label));
    QVBoxLayout *layout = new QVBoxLayout(gb);
    layout->setContentsMargins(4, 4, 4, 4);
    gb->setCheckable(true);
    gb->setChecked(false); /* replié par défaut */

    return (GtkWidget *)gb;
}

gchar *widget_expander_envvar_construct(GtkWidget *widget)
{
    QGroupBox *gb = static_cast<QGroupBox *>(widget);
    if (!gb) return g_strdup("false");
    return g_strdup(gb->isChecked() ? "true" : "false");
}

gchar *widget_expander_envvar_all_construct(variable *var)
{
    if (!var || !var->Widget) return NULL;
    return widget_expander_envvar_construct(var->Widget);
}

void widget_expander_clear(variable *var)
{
    if (!var || !var->Widget) return;
    static_cast<QGroupBox *>(var->Widget)->setChecked(false);
}

void widget_expander_refresh(variable *var)
{
    if (!var || !var->Widget) return;
    static_cast<QWidget *>(var->Widget)->update();
}

void widget_expander_fileselect(variable *var, const char *n, const char *v) {}
void widget_expander_removeselected(variable *var) {}
void widget_expander_save(variable *var) {}
