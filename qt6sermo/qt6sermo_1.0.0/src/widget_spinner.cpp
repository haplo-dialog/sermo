/* widget_spinner.cpp — Indicateur de chargement Qt6 (QLabel animé via timer)
 * qt6sermo 1.0.0 — haplo-dialog — GPL-2.0-or-later */
#ifndef _GNU_SOURCE
#define _GNU_SOURCE
#endif
#include "qt6-compat.h"
#include "gtk3d.h"
#include "attributes.h"
#include "automaton.h"
#include "widgets.h"
#include "tag_attributes.h"
#include "widget_spinner.h"
#include <QtWidgets/QLabel>
#include <QtCore/QTimer>
#include <string.h>

static const char * const SPINNER_FRAMES[] = {"⠋","⠙","⠹","⠸","⠼","⠴","⠦","⠧","⠇","⠏"};
static const int SPINNER_NFRAMES = 10;

GtkWidget *widget_spinner_create(AttributeSet *Attr, tag_attr *attr, gint Type)
{
    QLabel *lbl = new QLabel(SPINNER_FRAMES[0]);
    lbl->setStyleSheet("color:#89b4fa; font-size:18px;");
    lbl->setAlignment(Qt::AlignCenter);
    int *frame = new int(0);
    QTimer *timer = new QTimer(lbl);
    QObject::connect(timer, &QTimer::timeout, [lbl, frame](){
        *frame = (*frame + 1) % SPINNER_NFRAMES;
        lbl->setText(SPINNER_FRAMES[*frame]);
    });
    timer->start(100);
    QObject::connect(lbl, &QObject::destroyed, [frame](){ delete frame; });
    return (GtkWidget *)lbl;
}
gchar *widget_spinner_envvar_construct(GtkWidget *w) { return g_strdup(""); }
gchar *widget_spinner_envvar_all_construct(variable *v) { return NULL; }
void widget_spinner_clear(variable *v) {}
void widget_spinner_refresh(variable *v)
{ if (v && v->Widget) static_cast<QLabel*>(v->Widget)->update(); }
void widget_spinner_fileselect(variable *v, const char*, const char*) {}
void widget_spinner_removeselected(variable *v) {}
void widget_spinner_save(variable *v) {}
