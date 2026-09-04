/* widget_radiobutton.cpp — Bouton radio Qt6 */
#include "qt6-compat.h"
#include "gtk3d.h"
#include "attributes.h"
#include "automaton.h"
#include "widgets.h"
#include "tag_attributes.h"
#include "widget_radiobutton.h"
#include <QtWidgets/QRadioButton>
#include <string.h>
#include <stdlib.h>

GtkWidget *widget_radiobutton_create(AttributeSet *Attr, tag_attr *attr, gint Type)
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
    QRadioButton *rb = new QRadioButton(label ? label : "");
    rb->resize(w, h);
    if (Attr) {
        element = nullptr;
        gchar *def = attributeset_get_first(&element, Attr, ATTR_DEFAULT);
        if (def && (g_ascii_strcasecmp(def, "true") == 0 || strcmp(def, "1") == 0))
            rb->setChecked(true);
    }
    return (GtkWidget *)rb;
}

gchar *widget_radiobutton_envvar_construct(GtkWidget *widget)
{
    QRadioButton *rb = static_cast<QRadioButton*>(widget);
    if (!rb) return g_strdup("false");
    return g_strdup(rb->isChecked() ? "true" : "false");
}
gchar *widget_radiobutton_envvar_all_construct(variable *var) { if (!var || !var->Widget) return nullptr; return widget_radiobutton_envvar_construct(var->Widget); }
void   widget_radiobutton_clear(variable *var) { if (var && var->Widget) static_cast<QRadioButton*>(var->Widget)->setChecked(false); }
void   widget_radiobutton_refresh(variable *var) { if (var && var->Widget) static_cast<QWidget*>(var->Widget)->update(); }
void   widget_radiobutton_fileselect(variable *var, const char *n, const char *v) {}
void   widget_radiobutton_removeselected(variable *var) {}
void   widget_radiobutton_save(variable *var) {}
