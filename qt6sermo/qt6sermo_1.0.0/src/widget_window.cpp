/*
 * widget_window.cpp — Fenêtre principale Qt6
 * qt6sermo 1.0.0 — haplo-dialog <devel@haplo-dialog.fr>
 * Licence : GPL-2.0-or-later
 *
 * <window> → QMainWindow avec un QWidget central contenant un QVBoxLayout.
 * Les enfants (vbox/hbox) sont déjà sur la pile du parser ; on les récupère
 * via pop() et on les place dans le layout central.
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
#include "stack.h"
#include "widget_window.h"

#include <QtWidgets/QMainWindow>
#include <QtWidgets/QWidget>
#include <QtWidgets/QVBoxLayout>
#include <QtWidgets/QApplication>
#include <QtGui/QScreen>
#include <QtGui/QIcon>

#include <string.h>
#include <stdlib.h>

GtkWidget *widget_window_create(AttributeSet *Attr, tag_attr *attr, gint Type)
{
    GList      *element = NULL;
    const char *title   = "qt6sermo";
    int         dw      = 640, dh = 480;

    if (Attr) {
        gchar *lbl = attributeset_get_first(&element, Attr, ATTR_LABEL);
        if (lbl && *lbl) title = lbl;
    }
    if (attr) {
        const char *t = get_tag_attribute(attr, "title");   /* <window title="…"> */
        if (t && *t) title = t;
    }

    if (attr) {
        const char *v;
        if ((v = get_tag_attribute(attr, "default-width")))  dw = atoi(v);
        if ((v = get_tag_attribute(attr, "default-height"))) dh = atoi(v);
        if ((v = get_tag_attribute(attr, "resizable"))) {
            /* resizable=false : taille fixée après show() */
            (void)v; /* géré ci-dessous */
        }
    }

    QMainWindow *win = new QMainWindow();
    win->setWindowTitle(QString::fromUtf8(title));
    /* Icône de fenêtre (_NET_WM_ICON) : sinon le gestionnaire de fenêtres
     * affiche une icône générique dans la barre de titre. */
    win->setWindowIcon(QIcon::fromTheme(QStringLiteral("qt6sermo"),
        QIcon(QStringLiteral("/usr/share/icons/hicolor/32x32/apps/qt6sermo.png"))));
    win->resize(dw, dh);

    /* Widget central avec layout vertical */
    QWidget     *central = new QWidget(win);
    QVBoxLayout *layout  = new QVBoxLayout(central);
    layout->setContentsMargins(4, 4, 4, 4);
    layout->setSpacing(2);
    win->setCentralWidget(central);

    /* Récupérer l'enfant du parser (vbox / hbox) */
    stackelement s = pop();
    if (s.widgets[0]) {
        QWidget *child = static_cast<QWidget *>(s.widgets[0]);
        layout->addWidget(child);
    }

    /* Centrage */
    if (attr) {
        const char *pos = get_tag_attribute(attr, "window-position");
        if (pos && atoi(pos) == 1) { /* GTK_WIN_POS_CENTER */
            QRect sr = QGuiApplication::primaryScreen()->availableGeometry();
            win->move((sr.width() - dw) / 2, (sr.height() - dh) / 2);
        }
    }

    return (GtkWidget *)win;
}

gchar *widget_window_envvar_construct(GtkWidget *widget)
{
    return NULL;
}

gchar *widget_window_envvar_all_construct(variable *var)
{
    return NULL;
}

void widget_window_clear(variable *var)
{
    if (!var || !var->Widget) return;
    QMainWindow *win = static_cast<QMainWindow *>(var->Widget);
    win->setWindowTitle("qt6sermo");
}

void widget_window_refresh(variable *var)
{
    if (!var || !var->Widget) return;
    static_cast<QWidget *>(var->Widget)->update();
}

void widget_window_fileselect(variable *var, const char *name, const char *value)
{
    g_warning("%s(): not implemented", __func__);
}

void widget_window_removeselected(variable *var)
{
    g_warning("%s(): not implemented", __func__);
}

void widget_window_save(variable *var)
{
    g_warning("%s(): not implemented", __func__);
}
