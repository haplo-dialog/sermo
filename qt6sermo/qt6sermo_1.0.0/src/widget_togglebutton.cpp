/* widget_togglebutton.cpp — Bouton à bascule Qt6 */
#include "qt6-compat.h"
#include "gtk3d.h"
#include "attributes.h"
#include "automaton.h"
#include "widgets.h"
#include "tag_attributes.h"
#include "widget_togglebutton.h"
#include <QtWidgets/QPushButton>
#include <string.h>
#include <stdlib.h>

GtkWidget *widget_togglebutton_create(AttributeSet *Attr, tag_attr *attr, gint Type)
{
    GList *element = nullptr;
    int w = 120, h = 30;
    if (attr) {
        const char *v;
        if ((v = get_tag_attribute(attr, "width-request")))  w = atoi(v);
        if ((v = get_tag_attribute(attr, "height-request"))) h = atoi(v);
    }
    gchar *label = nullptr;
    if (Attr) label = attributeset_get_first(&element, Attr, ATTR_LABEL);
    QPushButton *btn = new QPushButton(label ? label : "");
    btn->setCheckable(true);
    btn->resize(w, h);
    if (Attr) {
        element = nullptr;
        gchar *def = attributeset_get_first(&element, Attr, ATTR_DEFAULT);
        if (def && (strcasecmp(def, "true") == 0 || strcmp(def, "1") == 0))
            btn->setChecked(true);
    }
    return (GtkWidget *)btn;
}

gchar *widget_togglebutton_envvar_construct(GtkWidget *widget)
{
    QPushButton *btn = static_cast<QPushButton*>(widget);
    if (!btn) return g_strdup("false");
    return g_strdup(btn->isChecked() ? "true" : "false");
}
gchar *widget_togglebutton_envvar_all_construct(variable *var) { if (!var || !var->Widget) return nullptr; return widget_togglebutton_envvar_construct(var->Widget); }
void   widget_togglebutton_clear(variable *var) { if (var && var->Widget) static_cast<QPushButton*>(var->Widget)->setChecked(false); }
void   widget_togglebutton_refresh(variable *var) { if (var && var->Widget) static_cast<QWidget*>(var->Widget)->update(); }
void   widget_togglebutton_fileselect(variable *var, const char *n, const char *v) {}
void   widget_togglebutton_removeselected(variable *var) {}
void   widget_togglebutton_save(variable *var) {}
