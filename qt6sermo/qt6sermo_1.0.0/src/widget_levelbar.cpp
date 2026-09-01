/* widget_levelbar.cpp — Barre de niveau Qt6 (QProgressBar stylisée)
 * qt6sermo 1.0.0 — haplo-dialog — GPL-2.0-or-later
 * Export : valeur 0.0–1.0 sous forme "0.75" */
#ifndef _GNU_SOURCE
#define _GNU_SOURCE
#endif
#include "qt6-compat.h"
#include "gtk3d.h"
#include "attributes.h"
#include "automaton.h"
#include "widgets.h"
#include "tag_attributes.h"
#include "widget_levelbar.h"
#include <QtWidgets/QProgressBar>
#include <string.h>
#include <cstdio>

GtkWidget *widget_levelbar_create(AttributeSet *Attr, tag_attr *attr, gint Type)
{
    QProgressBar *pb = new QProgressBar();
    pb->setRange(0, 1000);
    pb->setValue(0);
    pb->setTextVisible(false);
    pb->setStyleSheet(
        "QProgressBar{border:1px solid #585b70;border-radius:4px;background:#1e1e2e;}"
        "QProgressBar::chunk{background:#89b4fa;border-radius:3px;}");
    if (Attr) {
        /* Valeur dans [range-min, range-max] (defaut 0..100), comme les scales
         * et le port de reference, normalisee en fraction 0..1000. */
        double rmin = 0.0, rmax = 100.0, val = 0.0;
        if (attr) {
            const char *v;
            if ((v = get_tag_attribute(attr, "range-min")) || (v = get_tag_attribute(attr, "min"))) rmin = atof(v);
            if ((v = get_tag_attribute(attr, "range-max")) || (v = get_tag_attribute(attr, "max"))) rmax = atof(v);
            if ((v = get_tag_attribute(attr, "value"))) val = atof(v);
            if ((v = get_tag_attribute(attr, "width-request"))) pb->setMinimumWidth(atoi(v));
        }
        GList *el = NULL;
        gchar *def = attributeset_get_first(&el, Attr, ATTR_DEFAULT);
        if (def && *def) val = atof(def);
        val = widget_command_value(Attr, val);          /* <input> prioritaire */
        double frac = (rmax > rmin) ? (val - rmin) / (rmax - rmin) : 0.0;
        if (frac < 0.0) frac = 0.0;
        if (frac > 1.0) frac = 1.0;
        pb->setValue((int)(frac * 1000));
    }
    return (GtkWidget *)pb;
}
gchar *widget_levelbar_envvar_construct(GtkWidget *w)
{
    char buf[32];
    snprintf(buf, sizeof(buf), "%.3f", static_cast<QProgressBar*>(w)->value() / 1000.0);
    return g_strdup(buf);
}
gchar *widget_levelbar_envvar_all_construct(variable *v)
{ return v && v->Widget ? widget_levelbar_envvar_construct(v->Widget) : NULL; }
void widget_levelbar_clear(variable *v)
{ if (v && v->Widget) static_cast<QProgressBar*>(v->Widget)->setValue(0); }
void widget_levelbar_refresh(variable *v)
{ if (v && v->Widget) static_cast<QProgressBar*>(v->Widget)->update(); }
void widget_levelbar_fileselect(variable *v, const char*, const char*) {}
void widget_levelbar_removeselected(variable *v) {}
void widget_levelbar_save(variable *v) {}
