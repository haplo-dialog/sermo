/* widget_pulse.cpp — Barre de progression « pulse » Qt6 (indéterminée). */
#include "qt6-compat.h"
#include "gtk3d.h"
#include "attributes.h"
#include "automaton.h"
#include "widgets.h"
#include "tag_attributes.h"
#include "widget_pulse.h"
#include <QtWidgets/QProgressBar>

GtkWidget *widget_pulse_create(AttributeSet *Attr, tag_attr *attr, gint Type)
{
    (void)Attr; (void)attr; (void)Type;
    QProgressBar *pb = new QProgressBar();
    pb->setRange(0, 0);            /* mode indéterminé (busy) */
    pb->setTextVisible(false);
    return (GtkWidget *)pb;
}
gchar *widget_pulse_envvar_construct(GtkWidget *w) { (void)w; return g_strdup(""); }
gchar *widget_pulse_envvar_all_construct(variable *var) { (void)var; return nullptr; }
void   widget_pulse_clear(variable *var) { (void)var; }
void   widget_pulse_refresh(variable *var) { (void)var; }
void   widget_pulse_fileselect(variable *var, const char *n, const char *v) { (void)var;(void)n;(void)v; }
void   widget_pulse_removeselected(variable *var) { (void)var; }
void   widget_pulse_save(variable *var) { (void)var; }
