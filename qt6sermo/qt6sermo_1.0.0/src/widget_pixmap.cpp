/*
 * widget_pixmap.cpp — Image / icône Qt6 (QLabel + QPixmap)
 * qt6sermo 1.0.0 — haplo-dialog <devel@haplo-dialog.fr>
 * Licence : GPL-2.0-or-later
 *
 * <pixmap> → QLabel affichant un QPixmap chargé depuis un fichier.
 * Attributs : file (chemin image), width-request, height-request
 * Export : chemin du fichier courant (ou "")
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
#include "widget_pixmap.h"

#include <QtWidgets/QLabel>
#include <QtGui/QPixmap>
#include <QtCore/QString>

#include <string.h>
#include <stdlib.h>

GtkWidget *widget_pixmap_create(AttributeSet *Attr, tag_attr *attr, gint Type)
{
    QLabel *lbl = new QLabel();
    lbl->setAlignment(Qt::AlignCenter);

    const char *filepath = NULL;
    int w = 0, h = 0;

    if (attr) {
        const char *v;
        if ((v = get_tag_attribute(attr, "file")))           filepath = v;
        if ((v = get_tag_attribute(attr, "width-request")))  w = atoi(v);
        if ((v = get_tag_attribute(attr, "height-request"))) h = atoi(v);
    }
    if (!filepath && Attr) {
        GList *element = NULL;
        filepath = attributeset_get_first(&element, Attr, ATTR_DEFAULT);
    }

    if (filepath && *filepath) {
        QPixmap px(QString::fromUtf8(filepath));
        if (!px.isNull()) {
            if (w > 0 && h > 0)
                px = px.scaled(w, h, Qt::KeepAspectRatio, Qt::SmoothTransformation);
            lbl->setPixmap(px);
        }
        lbl->setProperty("pixmapFile", QString::fromUtf8(filepath));
    }

    if (w > 0) lbl->setMinimumWidth(w);
    if (h > 0) lbl->setMinimumHeight(h);

    return (GtkWidget *)lbl;
}

gchar *widget_pixmap_envvar_construct(GtkWidget *widget)
{
    QLabel *lbl = static_cast<QLabel *>(widget);
    if (!lbl) return g_strdup("");
    QVariant v = lbl->property("pixmapFile");
    if (!v.isValid()) return g_strdup("");
    return g_strdup(v.toString().toUtf8().constData());
}

gchar *widget_pixmap_envvar_all_construct(variable *var)
{
    if (!var || !var->Widget) return NULL;
    return widget_pixmap_envvar_construct(var->Widget);
}

void widget_pixmap_clear(variable *var)
{
    if (!var || !var->Widget) return;
    QLabel *lbl = static_cast<QLabel *>(var->Widget);
    lbl->clear();
    lbl->setProperty("pixmapFile", QVariant());
}

void widget_pixmap_refresh(variable *var)
{
    if (!var || !var->Widget) return;
    static_cast<QWidget *>(var->Widget)->update();
}

void widget_pixmap_fileselect(variable *var, const char *name, const char *filepath)
{
    if (!var || !var->Widget || !filepath) return;
    QLabel  *lbl = static_cast<QLabel *>(var->Widget);
    QPixmap  px(QString::fromUtf8(filepath));
    if (!px.isNull()) {
        lbl->setPixmap(px);
        lbl->setProperty("pixmapFile", QString::fromUtf8(filepath));
    }
}

void widget_pixmap_removeselected(variable *var) {}
void widget_pixmap_save(variable *var) {}
