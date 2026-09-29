/* SPDX-License-Identifier: GPL-2.0-or-later */
/*
 * Copyright (C) 2026 S. Cage
 * haplo-dialog <devel@haplo-dialog.fr>
 */
/* qt6-compat.cpp — Wrappers C + implémentations GList/GSList
 * haplo-dialog / qt6sermo 1.0.0
 * Contact : devel@haplo-dialog.fr
 */

#include "qt6-compat.h"
#include <QtWidgets/QWidget>
#include <QtWidgets/QScrollArea>
#include <QtWidgets/QBoxLayout>
#include <stdlib.h>

/* ─── GSList minimal ────────────────────────────────────────────────────── */

extern "C" {

GSList* g_slist_append(GSList *list, void *data) {
    GSList *node = (GSList*)malloc(sizeof(GSList));
    node->data = data; node->next = NULL;
    if (!list) return node;
    GSList *last = list;
    while (last->next) last = last->next;
    last->next = node;
    return list;
}

GSList* g_slist_prepend(GSList *list, void *data) {
    GSList *node = (GSList*)malloc(sizeof(GSList));
    node->data = data; node->next = list;
    return node;
}

void g_slist_free(GSList *list) {
    while (list) { GSList *n = list->next; free(list); list = n; }
}

guint g_slist_length(GSList *list) {
    guint n = 0;
    while (list) { n++; list = list->next; }
    return n;
}

/* ─── GList minimal ─────────────────────────────────────────────────────── */

GList* g_list_append(GList *list, void *data) {
    GList *node = (GList*)malloc(sizeof(GList));
    node->data = data; node->next = NULL; node->prev = NULL;
    if (!list) return node;
    GList *last = list;
    while (last->next) last = last->next;
    last->next = node; node->prev = last;
    return list;
}

void g_list_free(GList *list) {
    while (list) { GList *n = list->next; free(list); list = n; }
}

guint g_list_length(GList *list) {
    guint n = 0;
    while (list) { n++; list = list->next; }
    return n;
}

} /* extern "C" — GSList/GList */

/* ─── Wrappers C appelables depuis les fichiers core (.c) ──────────────── */

extern "C" {

void sermo_be_widget_show(Qt6Widget_C *w) {
    if (w) static_cast<QWidget*>(w)->show();
}

void sermo_be_widget_hide(Qt6Widget_C *w) {
    if (w) static_cast<QWidget*>(w)->hide();
}

void sermo_be_widget_set_sensitive(Qt6Widget_C *w, int sensitive) {
    if (w) static_cast<QWidget*>(w)->setEnabled(sensitive != 0);
}

void sermo_be_widget_redraw(Qt6Widget_C *w) {
    if (w) static_cast<QWidget*>(w)->update();
}

void sermo_be_container_add(Qt6Widget_C *container, Qt6Widget_C *child) {
    if (!container || !child) return;
    QWidget *c  = static_cast<QWidget*>(container);
    QWidget *ch = static_cast<QWidget*>(child);
    /* Conteneur de défilement : l'enfant doit être confié au VIEWPORT via
     * setWidget(), pas simplement reparenté. Avec un setParent() nu, l'enfant
     * n'est pas géré par la zone de défilement : setWidgetResizable(true) reste
     * sans effet et le widget garde sa taille propre — un <edit> restait à ses
     * 300 px de large au milieu d'un cadre trois fois plus large, avec deux
     * jeux de barres de défilement imbriquées. */
    if (auto *sa = qobject_cast<QScrollArea*>(c)) {
        sa->setWidget(ch);
        return;
    }
    if (auto *lay = qobject_cast<QBoxLayout*>(c->layout()))
        lay->addWidget(ch);
    else
        ch->setParent(c);
}

void *sermo_be_container_child0(Qt6Widget_C *container) {
    if (!container) return nullptr;
    QWidget *c = static_cast<QWidget*>(container);
    if (auto *lay = c->layout()) {
        if (lay->count() > 0) {
            auto *item = lay->itemAt(0);
            return item ? (void*)item->widget() : nullptr;
        }
    }
    for (auto *obj : c->children())
        if (auto *w = qobject_cast<QWidget*>(obj)) return (void*)w;
    return nullptr;
}

void sermo_be_window_move(Qt6Widget_C *w, int x, int y) {
    if (w) static_cast<QWidget*>(w)->move(x, y);
}

void *sermo_be_scroll_new(int w, int h) {
    QScrollArea *sa = new QScrollArea();
    sa->resize(w, h);
    sa->setWidgetResizable(true);
    return (void*)sa;
}

} /* extern "C" */

/* ─── Cycle de vie QApplication ─────────────────────────────────────────────
 * La QApplication doit exister AVANT tout QWidget (sinon « Must construct a
 * QApplication before a QWidget »). On la crée dans gtk_init (mappé ici), sauf
 * en --print-ir : ce mode parse seul, ne construit aucun widget et doit rester
 * headless (pas de connexion au serveur d'affichage). gtk_main lance la boucle
 * d'événements Qt ; gtk_main_quit l'arrête.
 */
#include <QtWidgets/QApplication>
#include <QtWidgets/QWidget>
#include <QtGui/QPixmap>
#include <QtGui/QIcon>
#include <QtGui/QPalette>
#include <QtGui/QColor>
#include <QtCore/QDir>
#include <QtCore/QFile>
#include <QtCore/QSettings>
#include <QtCore/QStandardPaths>
#include "sermo_icon_theme.h"   /* sermo_desktop_is_dark() — suivre le theme systeme */

static QApplication *qt6_app = nullptr;

/* Suivre le thème du bureau : Qt, hors session (Xvfb/CI) ou sans greffon de
 * plateforme (qt6ct/qgnomeplatform), garde sa palette CLAIRE d'usine même quand
 * le bureau est sombre. On applique donc une palette sombre nous-mêmes lorsque
 * sermo_desktop_is_dark() le dit, via le style Fusion (le seul qui honore une
 * QPalette custom). En clair : on laisse la palette système par défaut.
 * Teintes alignées sur Catppuccin, comme sdl3, pour une famille cohérente. */
static void qt6_palette_systeme(void)
{
    if (!sermo_desktop_is_dark()) return;
    QApplication::setStyle(QStringLiteral("Fusion"));
    QPalette p;
    const QColor base(30, 30, 46);      /* #1e1e2e */
    const QColor mantle(24, 24, 37);    /* #181825 */
    const QColor surface(49, 50, 68);   /* #313244 */
    const QColor surface1(60, 62, 80);  /* #3c3e50 */
    const QColor text(205, 214, 244);   /* #cdd6f4 */
    const QColor blue(137, 180, 250);   /* #89b4fa */
    p.setColor(QPalette::Window,          base);
    p.setColor(QPalette::WindowText,      text);
    p.setColor(QPalette::Base,            mantle);
    p.setColor(QPalette::AlternateBase,   surface);
    p.setColor(QPalette::ToolTipBase,     surface);
    p.setColor(QPalette::ToolTipText,     text);
    p.setColor(QPalette::Text,            text);
    p.setColor(QPalette::Button,          surface);
    p.setColor(QPalette::ButtonText,      text);
    p.setColor(QPalette::BrightText,      QColor(243, 139, 168)); /* red */
    p.setColor(QPalette::Link,            blue);
    p.setColor(QPalette::Highlight,       blue);
    p.setColor(QPalette::HighlightedText, base);
    p.setColor(QPalette::PlaceholderText, QColor(147, 153, 178)); /* overlay */
    p.setColor(QPalette::Disabled, QPalette::Text,       QColor(127, 132, 156));
    p.setColor(QPalette::Disabled, QPalette::ButtonText, QColor(127, 132, 156));
    p.setColor(QPalette::Disabled, QPalette::WindowText, QColor(127, 132, 156));
    (void) surface1;
    QApplication::setPalette(p);
}

/* ⚠️ Sans thème d'icônes déclaré, QIcon::fromTheme() rend un pixmap VIDE : le
 * dialecte <pixmap><input file icon="folder"> n'affichait rien, et la fenêtre
 * gardait la même taille quel que soit theme-icon-size. Le port de référence
 * n'a pas ce problème — GTK lit le réglage du bureau tout seul. Qt, lancé hors
 * d'une session de bureau (Xvfb, CI), n'a personne pour le lui dire.
 * On reproduit donc la chaîne de GTK : réglage gtk-3.0, puis Adwaita, puis
 * hicolor. Mesuré par tests/comportement/geometrie.sh le 2026-09-03. */
static void qt6_theme_icones_par_defaut(void)
{
    /* « hicolor » n'est pas un choix de thème : c'est le SOCLE de repli de la
     * spécification — Qt le rapporte quand rien n'est configuré (mesuré en
     * conteneur CI : themeName()=="hicolor", toutes les icônes nulles). Ne
     * s'arrêter que devant un VRAI thème. */
    const QString deja = QIcon::themeName();
    if (!deja.isEmpty() && deja != QLatin1String("hicolor")) return;

    QStringList candidats;
    const QString ini = QStandardPaths::writableLocation(QStandardPaths::GenericConfigLocation)
                        + QStringLiteral("/gtk-3.0/settings.ini");
    if (QFile::exists(ini)) {
        const QString v = QSettings(ini, QSettings::IniFormat)
                              .value(QStringLiteral("Settings/gtk-icon-theme-name")).toString();
        if (!v.isEmpty()) candidats << v;
    }
    candidats << QStringLiteral("Adwaita") << QStringLiteral("hicolor");

    for (const QString &nom : candidats) {
        for (const QString &racine : QIcon::themeSearchPaths()) {
            if (QDir(racine + QLatin1Char('/') + nom).exists()) {
                QIcon::setThemeName(nom);
                return;
            }
        }
    }
}

extern "C" {

extern int option_print_ir;    /* gboolean (gint) défini dans gtkdialog.c */
extern char *option_render_png; /* --render-png FILE : rendu offscreen (gtkdialog.c) */

void sermo_be_app_init(int *argc, char ***argv) {
    if (option_print_ir) return;               /* parse seul : pas de QApplication */
    if (!qt6_app) qt6_app = new QApplication(*argc, *argv);
    qt6_theme_icones_par_defaut();
    qt6_palette_systeme();
}

int sermo_be_run_loop(void) {
    /* Rendu offscreen : dessine la fenêtre dans un PNG et sort, sans boucle ni
     * affichage. Sous QT_QPA_PLATFORM=offscreen c'est totalement headless (pas
     * de X/Wayland). Sert l'outil sermo_render du MCP (rendu propre, déterministe
     * — remplace la capture xvfb). */
    if (option_render_png && *option_render_png && qt6_app) {
        QWidget *win = nullptr;
        const QList<QWidget *> tops = QApplication::topLevelWidgets();
        /* Celle que <window> a marquee, PAS la premiere venue : une liste
         * deroulante ouverte est elle aussi une fenetre de premier niveau, et
         * le PNG sortait en 2x2 pixels — le popup seul — 4 essais sur 6.
         * Repli sur l'ancien choix si rien n'est marque (backend tiers). */
        for (QWidget *w : tops) {
            if (w->isWindow() && w->property("sermoFenetrePrincipale").toBool()) { win = w; break; }
        }
        if (!win)
            for (QWidget *w : tops) {
                /* a defaut, tout sauf un popup ou une infobulle */
                if (w->isWindow() && !(w->windowFlags() & (Qt::Popup | Qt::ToolTip))) { win = w; break; }
            }
        if (win) {
            /* adjustSize() remplace la taille de la fenetre par celle,
             * naturelle, de son contenu. C'est ce qu'il faut quand le script
             * n'a rien demande — mais pas quand <window> porte default-width
             * ou default-height : le PNG doit montrer la fenetre telle qu'elle
             * s'ouvre a l'ecran. */
            const QVariant demandee = win->property("sermoTailleDemandee");
            if (demandee.isValid())
                win->resize(demandee.toSize());
            else
                win->adjustSize();
            win->show();
            QApplication::processEvents();
            QApplication::processEvents();
            QPixmap pm = win->grab();
            pm.save(QString::fromUtf8(option_render_png), "PNG");
        }
        return 0;
    }
    return qt6_app ? qt6_app->exec() : 0;      /* boucle d'événements */
}

void sermo_be_app_quit(void) {
    if (qt6_app) qt6_app->quit();
}

} /* extern "C" */
