/* widget_switch.cpp — Interrupteur booléen Qt6 (QCheckBox stylisé)
 * qt6sermo 1.0.0 — haplo-dialog <devel@haplo-dialog.fr> — GPL-2.0-or-later */
#ifndef _GNU_SOURCE
#define _GNU_SOURCE
#endif
#include "qt6-compat.h"
#include "gtk3d.h"
#include "attributes.h"
#include "automaton.h"
#include "widgets.h"
#include "tag_attributes.h"
#include "widget_switch.h"
#include <QtWidgets/QCheckBox>
#include <string.h>
#include <stdlib.h>

GtkWidget *widget_switch_create(AttributeSet *Attr, tag_attr *attr, gint Type)
{
    QCheckBox *cb = new QCheckBox("OFF");
    cb->setStyleSheet(
        "QCheckBox::indicator { width:40px; height:20px; }"
        "QCheckBox::indicator:checked { background:#89b4fa; border-radius:10px; }"
        "QCheckBox::indicator:unchecked { background:#585b70; border-radius:10px; }");
    QObject::connect(cb, &QCheckBox::toggled, [cb](bool on){
        cb->setText(on ? "ON" : "OFF");
    });

    if (Attr) {
        GList *el = NULL;
        gchar *lbl = attributeset_get_first(&el, Attr, ATTR_LABEL);
        if (lbl && *lbl) cb->setText(lbl);
        el = NULL;
        gchar *def = attributeset_get_first(&el, Attr, ATTR_DEFAULT);
        if (def && (strcmp(def,"true")==0||strcmp(def,"1")==0||strcmp(def,"on")==0))
            cb->setChecked(true);
        if (attr) {
            const char *v = get_tag_attribute(attr, "tooltip");
            if (v && *v) cb->setToolTip(QString::fromUtf8(v));
        }
    }
    return (GtkWidget *)cb;
}
gchar *widget_switch_envvar_construct(GtkWidget *w)
{ return g_strdup(static_cast<QCheckBox*>(w)->isChecked() ? "true" : "false"); }
gchar *widget_switch_envvar_all_construct(variable *v)
{ return v && v->Widget ? widget_switch_envvar_construct(v->Widget) : NULL; }
void widget_switch_clear(variable *v)
{ if (v && v->Widget) static_cast<QCheckBox*>(v->Widget)->setChecked(false); }
void widget_switch_refresh(variable *v)
{ if (v && v->Widget) static_cast<QCheckBox*>(v->Widget)->update(); }
void widget_switch_fileselect(variable *v, const char*, const char*) {}
void widget_switch_removeselected(variable *v) {}
void widget_switch_save(variable *v) {}
