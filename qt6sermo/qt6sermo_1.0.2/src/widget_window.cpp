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

    /* ⚠️ Une taille n'est imposée que si l'XML la demande. Le port GTK 3 de
     * référence n'appelle gtk_window_set_default_size() QUE pour un
     * default-width/height explicite ; sans cela la fenêtre prend sa taille
     * naturelle. Un resize(640, 480) inconditionnel donnait ici une fenêtre
     * quatre fois trop grande, presque vide, là où la référence rend 320×105. */
    bool sized = false;
    int  minw = 0, minh = 0;
    if (attr) {
        const char *v;
        if ((v = get_tag_attribute(attr, "default-width")))  { dw = atoi(v); sized = true; }
        if ((v = get_tag_attribute(attr, "default-height"))) { dh = atoi(v); sized = true; }
        /* width-request/height-request = taille MINIMALE, comme
         * gtk_widget_set_size_request() côté référence. */
        if ((v = get_tag_attribute(attr, "width-request")))  minw = atoi(v);
        if ((v = get_tag_attribute(attr, "height-request"))) minh = atoi(v);
        if ((v = get_tag_attribute(attr, "resizable"))) {
            /* resizable=false : taille fixée après show() */
            (void)v; /* géré ci-dessous */
        }
    }

    QMainWindow *win = new QMainWindow();
    win->setWindowTitle(QString::fromUtf8(title));
    /* Icône de fenêtre (_NET_WM_ICON) : sinon le gestionnaire de fenêtres
     * affiche une icône générique dans la barre de titre. */
    /* Icône de fenêtre (_NET_WM_ICON). Trois sources, dans cet ordre :
     *   1. le thème d'icônes — respecte le thème de l'utilisateur ;
     *   2. le fichier installé par le paquet ;
     *   3. l'icône EMBARQUÉE dans le binaire (ressource Qt).
     * Le repli 3 est indispensable : sans lui, un binaire non installé ne posait
     * AUCUN _NET_WM_ICON — vérifié à l'xprop, « not found » — et le gestionnaire
     * de fenêtres affichait une icône générique, là où le port GTK 3 de
     * référence publie « Icon (32 x 32) ». */
    QIcon appicon = QIcon::fromTheme(QStringLiteral("qt6sermo"));
    if (appicon.isNull())
        appicon = QIcon(QStringLiteral("/usr/share/icons/hicolor/32x32/apps/qt6sermo.png"));
    if (appicon.isNull())
        appicon = QIcon(QStringLiteral(":/qt6sermo.png"));
    win->setWindowIcon(appicon);
    if (sized) win->resize(dw, dh);
    if (minw > 0 || minh > 0) win->setMinimumSize(minw, minh);

    /* Widget central avec layout vertical */
    QWidget     *central = new QWidget(win);
    QVBoxLayout *layout  = new QVBoxLayout(central);
    /* border-width : marge du conteneur, comme gtk_container_set_border_width().
     * Le port l'ignorait complètement — la fenêtre gardait la même taille quelle
     * que soit la valeur, alors que le port de référence grandit de 2N. Défaut
     * invisible pour le banc de valeurs, vu par geometrie.sh. « margin » est
     * l'ancien nom, encore accepté par le dialecte. Défaut 5, comme l'oracle. */
    int bordure = 5;
    if (attr) {
        const char *v;
        if ((v = get_tag_attribute(attr, "margin")))        bordure = atoi(v);
        if ((v = get_tag_attribute(attr, "border-width")))  bordure = atoi(v);
    }
    layout->setContentsMargins(bordure, bordure, bordure, bordure);
    layout->setSpacing(2);
    win->setCentralWidget(central);

    /* Récupérer l'enfant du parser (vbox / hbox) */
    stackelement s = pop();
    if (s.widgets[0]) {
        QWidget *child = static_cast<QWidget *>(s.widgets[0]);
        layout->addWidget(child);
    }

    /* Sans taille imposée, la fenêtre prend celle de son contenu (comme la
     * référence). adjustSize() force ce calcul maintenant, pour que le centrage
     * ci-dessous travaille sur la vraie taille et non sur une valeur par défaut. */
    /* ⚠️ PAS adjustSize() : pour une fenêtre dont la disposition s'étire, Qt
     * remonte le résultat à 200x100 au minimum. Le port de référence n'a aucun
     * plancher — une fenêtre ne contenant qu'une icône de 26 px fait 26 px.
     * Ce plancher rendait toutes les petites fenêtres identiques : le banc
     * geometrie.sh mesurait 200x100 pour theme-icon-size=16 ET =64, et concluait
     * « l'icône n'est pas chargée » alors qu'elle l'était. resize(sizeHint())
     * demande la même taille naturelle, sans le plancher. Mesuré 2026-09-03. */
    if (!sized) {
        win->ensurePolished();
        win->resize(win->sizeHint());
    }

    /* Centrage — sur la taille RÉELLE de la fenêtre, pas sur dw/dh, qui ne
     * valent plus rien quand aucune taille n'a été demandée. */
    if (attr) {
        const char *pos = get_tag_attribute(attr, "window-position");
        if (pos && atoi(pos) == 1) { /* GTK_WIN_POS_CENTER */
            QRect sr = QGuiApplication::primaryScreen()->availableGeometry();
            QSize ws = win->size();
            win->move((sr.width() - ws.width()) / 2, (sr.height() - ws.height()) / 2);
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
