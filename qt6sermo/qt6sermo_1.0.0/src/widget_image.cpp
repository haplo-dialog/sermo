/* widget_image.cpp — Image Qt6 (QLabel + QPixmap depuis un fichier, ou QIcon
 * depuis le thème via icon-name). Remplace l'ancien stub. */
#include "qt6-compat.h"
#include "gtk3d.h"
#include "attributes.h"
#include "automaton.h"
#include "widgets.h"
#include "tag_attributes.h"
#include "widget_image.h"
#include <QtWidgets/QLabel>
#include <QtGui/QPixmap>
#include <QtGui/QIcon>
#include <string.h>
#include <stdlib.h>

/* Tailles d'icônes nommées GTK → pixels. */
static int icon_size_px(const char *name)
{
    if (!name) return 24;
    if (!g_ascii_strcasecmp(name, "menu") || !g_ascii_strcasecmp(name, "button") ||
        !g_ascii_strcasecmp(name, "small-toolbar")) return 16;
    if (!g_ascii_strcasecmp(name, "large-toolbar")) return 24;
    if (!g_ascii_strcasecmp(name, "dnd"))    return 32;
    if (!g_ascii_strcasecmp(name, "dialog")) return 48;
    int n = atoi(name);
    return n > 0 ? n : 24;
}

GtkWidget *widget_image_create(AttributeSet *Attr, tag_attr *attr, gint Type)
{
    QLabel *lbl = new QLabel();
    int w = 0, h = 0;
    const char *file = nullptr, *icon = nullptr, *sizestr = nullptr;

    if (attr) {
        const char *v;
        if ((v = get_tag_attribute(attr, "width")))  w = atoi(v);
        else if ((v = get_tag_attribute(attr, "width-request")))  w = atoi(v);
        if ((v = get_tag_attribute(attr, "height"))) h = atoi(v);
        else if ((v = get_tag_attribute(attr, "height-request"))) h = atoi(v);
        file    = get_tag_attribute(attr, "file");
        icon    = get_tag_attribute(attr, "icon-name");
        sizestr = get_tag_attribute(attr, "icon-size");
    }
    /* <image><input file="path"></input></image> (forme alternative). */
    if (Attr && (!file || !*file)) {
        GList *el = nullptr;
        gchar *inp = attributeset_get_first(&el, Attr, ATTR_INPUT);
        if (inp && *inp) {
            if (!strncmp(inp, "file:", 5)) file = inp + 5;
            else                            file = inp;
        }
    }

    if (file && *file) {
        QPixmap pm(QString::fromUtf8(file));
        if (!pm.isNull()) {
            /* Taille NATIVE comme la référence gtk3 (dont le parseur ne
             * transmet pas width/height à <image>) : on n'amoindrit pas
             * l'image fichier, pour que le logo ait ~150px comme gtk3.
             * (w/h conservés pour usage futur.) */
            (void)w; (void)h;
            lbl->setPixmap(pm);
        }
    } else if (icon && *icon) {
        int sz = (w > 0) ? w : icon_size_px(sizestr);
        QIcon ic = QIcon::fromTheme(QString::fromUtf8(icon));
        if (!ic.isNull()) lbl->setPixmap(ic.pixmap(sz, sz));
    }
    return (GtkWidget *)lbl;
}

gchar *widget_image_envvar_construct(GtkWidget *widget) { (void)widget; return g_strdup(""); }
gchar *widget_image_envvar_all_construct(variable *var) { (void)var; return nullptr; }
void   widget_image_clear(variable *var)          { (void)var; }
void   widget_image_refresh(variable *var)        { (void)var; }
void   widget_image_fileselect(variable *var, const char *name, const char *value)
{ if (var && var->Widget && value) { QPixmap pm(QString::fromUtf8(value)); if (!pm.isNull()) static_cast<QLabel*>(var->Widget)->setPixmap(pm); } (void)name; }
void   widget_image_removeselected(variable *var) { (void)var; }
void   widget_image_save(variable *var)           { (void)var; }
