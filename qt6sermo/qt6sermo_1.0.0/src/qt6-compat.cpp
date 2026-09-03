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

void qt6_widget_show(Qt6Widget_C *w) {
    if (w) static_cast<QWidget*>(w)->show();
}

void qt6_widget_hide(Qt6Widget_C *w) {
    if (w) static_cast<QWidget*>(w)->hide();
}

void qt6_widget_set_sensitive(Qt6Widget_C *w, int sensitive) {
    if (w) static_cast<QWidget*>(w)->setEnabled(sensitive != 0);
}

void qt6_widget_redraw(Qt6Widget_C *w) {
    if (w) static_cast<QWidget*>(w)->update();
}

void qt6_container_add(Qt6Widget_C *container, Qt6Widget_C *child) {
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

void *qt6_container_child0(Qt6Widget_C *container) {
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

void qt6_window_move(Qt6Widget_C *w, int x, int y) {
    if (w) static_cast<QWidget*>(w)->move(x, y);
}

void *qt6_scroll_new(int w, int h) {
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
#include <QtGui/QIcon>
#include <QtCore/QDir>
#include <QtCore/QFile>
#include <QtCore/QSettings>
#include <QtCore/QStandardPaths>

static QApplication *qt6_app = nullptr;

/* ⚠️ Sans thème d'icônes déclaré, QIcon::fromTheme() rend un pixmap VIDE : le
 * dialecte <pixmap><input file icon="folder"> n'affichait rien, et la fenêtre
 * gardait la même taille quel que soit theme-icon-size. Le port de référence
 * n'a pas ce problème — GTK lit le réglage du bureau tout seul. Qt, lancé hors
 * d'une session de bureau (Xvfb, CI), n'a personne pour le lui dire.
 * On reproduit donc la chaîne de GTK : réglage gtk-3.0, puis Adwaita, puis
 * hicolor. Mesuré par tests/comportement/geometrie.sh le 2026-09-03. */
static void qt6_theme_icones_par_defaut(void)
{
    if (!QIcon::themeName().isEmpty()) return;

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

extern int option_print_ir;   /* gboolean (gint) défini dans gtkdialog.c */

void qt6_app_init(int *argc, char ***argv) {
    if (option_print_ir) return;               /* parse seul : pas de QApplication */
    if (!qt6_app) qt6_app = new QApplication(*argc, *argv);
    qt6_theme_icones_par_defaut();
}

int qt6_app_run(void) {
    return qt6_app ? qt6_app->exec() : 0;      /* boucle d'événements */
}

void qt6_app_quit(void) {
    if (qt6_app) qt6_app->quit();
}

} /* extern "C" */
