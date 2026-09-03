/*
 * widget_pixmap.cpp — Image / icône Qt6 (QLabel + QPixmap)
 * qt6sermo 1.0.0 — haplo-dialog <devel@haplo-dialog.fr>
 * Licence : GPL-2.0-or-later
 *
 * <pixmap> → QLabel affichant un QPixmap.
 * Sources : <input file>/chemin</input>, <input file icon="nom">,
 *           <input file stock="nom">, ou l'attribut file=.
 * Attributs : file, width-request, height-request, theme-icon-size,
 *             stock-icon-size, <width>/<height>
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
#include <QtGui/QIcon>
#include <QtCore/QString>

#include <string.h>
#include <stdlib.h>

GtkWidget *widget_pixmap_create(AttributeSet *Attr, tag_attr *attr, gint Type)
{
    QLabel *lbl = new QLabel();
    lbl->setAlignment(Qt::AlignCenter);

    const char *filepath = NULL;
    int w = 0, h = 0;
    int larg = -1, haut = -1;           /* <width>/<height> du dialecte */
    int taille_theme = 32;              /* même défaut que le port de référence */
    int taille_stock = 32;              /* GTK_ICON_SIZE_DND ≈ 32 px */

    if (attr) {
        const char *v;
        if ((v = get_tag_attribute(attr, "file")))           filepath = v;
        if ((v = get_tag_attribute(attr, "width-request")))  w = atoi(v);
        if ((v = get_tag_attribute(attr, "height-request"))) h = atoi(v);
    }

    GList *element = NULL;
    if (Attr) {
        if (attributeset_is_avail(Attr, ATTR_HEIGHT)) {
            element = NULL;
            haut = atoi(attributeset_get_first(&element, Attr, ATTR_HEIGHT));
        }
        if (attributeset_is_avail(Attr, ATTR_WIDTH)) {
            element = NULL;
            larg = atoi(attributeset_get_first(&element, Attr, ATTR_WIDTH));
        }
    }

    /* ⚠️ La forme normale du dialecte n'est PAS l'attribut file= mais la balise
     * <input file …> — avec trois variantes : un chemin (« file:/chemin »), une
     * icône de thème (icon="nom") et une icône stock (stock="nom"). Le port ne
     * lisait que file= : toute icône de thème restait vide, et la fenêtre gardait
     * la même taille quelle que soit theme-icon-size. Le banc geometrie.sh le
     * voyait (« l'icone de theme n'est PAS chargee ») là où le banc de valeurs,
     * qui ne compare que des variables, ne pouvait rien voir. Mesuré 2026-09-03. */
    QPixmap px;
    if (Attr) {
        element = NULL;
        gchar *act = attributeset_get_first(&element, Attr, ATTR_INPUT);
        while (act) {
            if (g_ascii_strncasecmp(act, "file:", 5) == 0) {
                gchar *nom_stock = attributeset_get_this_tagattr(&element, Attr,
                                                     ATTR_INPUT, (gchar *)"stock");
                if (nom_stock) {
                    const char *v;
                    if (attr && (v = get_tag_attribute(attr, "stock-icon-size")))
                        taille_stock = atoi(v);
                    px = QIcon::fromTheme(QString::fromUtf8(nom_stock))
                             .pixmap(taille_stock, taille_stock);
                    filepath = nom_stock;
                    break;
                }
                gchar *nom_icone = attributeset_get_this_tagattr(&element, Attr,
                                                     ATTR_INPUT, (gchar *)"icon");
                if (nom_icone) {
                    const char *v;
                    if (haut > -1)      taille_theme = haut;
                    else if (larg > -1) taille_theme = larg;
                    if (attr && (v = get_tag_attribute(attr, "theme-icon-size")))
                        taille_theme = atoi(v);
                    px = QIcon::fromTheme(QString::fromUtf8(nom_icone))
                             .pixmap(taille_theme, taille_theme);
                    filepath = nom_icone;
                    break;
                }
                if (strlen(act) > 5) { filepath = act + 5; break; }
            }
            act = attributeset_get_next(&element, Attr, ATTR_INPUT);
        }
    }

    if (!filepath && Attr) {
        element = NULL;
        filepath = attributeset_get_first(&element, Attr, ATTR_DEFAULT);
    }

    if (px.isNull() && filepath && *filepath)
        px = QPixmap(QString::fromUtf8(filepath));

    if (!px.isNull()) {
        int cw = (w > 0) ? w : larg;
        int ch = (h > 0) ? h : haut;
        if (cw > 0 && ch > 0)
            px = px.scaled(cw, ch, Qt::KeepAspectRatio, Qt::SmoothTransformation);
        lbl->setPixmap(px);
    }
    if (filepath && *filepath)
        lbl->setProperty("pixmapFile", QString::fromUtf8(filepath));

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
