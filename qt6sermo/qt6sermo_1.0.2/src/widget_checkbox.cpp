/* widget_checkbox.cpp — Case à cocher Qt6 */
#include "qt6-compat.h"
#include "gtk3d.h"
#include "attributes.h"
#include "automaton.h"
#include "widgets.h"
#include "tag_attributes.h"
#include "widget_checkbox.h"
#include <QtWidgets/QCheckBox>
#include <string.h>
#include <stdlib.h>

GtkWidget *widget_checkbox_create(AttributeSet *Attr, tag_attr *attr, gint Type)
{
    GList *element = nullptr;
    int w = 120, h = 28;
    if (attr) {
        const char *v;
        if ((v = get_tag_attribute(attr, "width-request")))  w = atoi(v);
        if ((v = get_tag_attribute(attr, "height-request"))) h = atoi(v);
    }
    gchar *label = nullptr;
    if (Attr) label = attributeset_get_first(&element, Attr, ATTR_LABEL);
    QCheckBox *cb = new QCheckBox(label ? label : "");
    cb->resize(w, h);
    if (Attr) {
        element = nullptr;
        gchar *def = attributeset_get_first(&element, Attr, ATTR_DEFAULT);
        if (def && (g_ascii_strcasecmp(def, "true") == 0 || strcmp(def, "1") == 0))
            cb->setChecked(true);
    }
    return (GtkWidget *)cb;
}

gchar *widget_checkbox_envvar_construct(GtkWidget *widget)
{
    QCheckBox *cb = static_cast<QCheckBox*>(widget);
    if (!cb) return g_strdup("false");
    return g_strdup(cb->isChecked() ? "true" : "false");
}
gchar *widget_checkbox_envvar_all_construct(variable *var) { if (!var || !var->Widget) return nullptr; return widget_checkbox_envvar_construct(var->Widget); }
void   widget_checkbox_clear(variable *var) { if (var && var->Widget) static_cast<QCheckBox*>(var->Widget)->setChecked(false); }
void   widget_checkbox_refresh(variable *var) { if (var && var->Widget) static_cast<QWidget*>(var->Widget)->update(); }
void   widget_checkbox_fileselect(variable *var, const char *n, const char *v) {}
void   widget_checkbox_removeselected(variable *var) {}
void   widget_checkbox_save(variable *var) {}
