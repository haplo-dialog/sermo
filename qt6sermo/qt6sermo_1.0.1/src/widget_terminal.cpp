/*
 * widget_terminal.cpp — Terminal embarqué Qt6
 * qt6sermo 1.0.0 — haplo-dialog <devel@haplo-dialog.fr>
 * Licence : GPL-2.0-or-later
 *
 * Si QTermWidget6 est disponible (HAVE_QTERMWIDGET=1) :
 *   - Terminal VTE-équivalent via qtermwidget6
 *   - Thème Catppuccin Mocha inline (pas de fichier externe)
 *   - Commande <input> injectée via sendText() après démarrage du shell
 *   - refresh() : force redraw
 *
 * Sinon : QPlainTextEdit en lecture seule, sortie de commande via fclose().
 *
 * Sécurité :
 *   - Aucun system()/popen() direct — uniquement widget_opencommand()
 *   - fclose() sur tout FILE* issu de widget_opencommand()
 *   - sendText() : commande depuis AttributeSet, pas d'interpolation
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
#include "widget_terminal.h"
#include "safe_exec.h"

#include <string.h>
#include <stdlib.h>
#include <stdio.h>

#if HAVE_QTERMWIDGET
#  include <qtermwidget6/qtermwidget.h>
#  include <QtGui/QFont>
#  include <QtGui/QColor>
#  include <QtCore/QFile>
#  include <QtCore/QTextStream>
#  include <QtCore/QTemporaryFile>
#  include <QtCore/QDir>
#  include <QtCore/QStandardPaths>
#else
#  include <QtWidgets/QPlainTextEdit>
#  include <QtWidgets/QWidget>
#  include <QtGui/QFont>
#  include <QtGui/QPalette>
#  include <QtGui/QColor>
#endif

/* ─── Catppuccin Mocha — couleurs terminales ─────────────────────────────── */
#if HAVE_QTERMWIDGET
static void _apply_catppuccin_mocha(QTermWidget *term)
{
    /*
     * QTermWidget supporte les color schemes via un fichier .colorscheme
     * chargé depuis ~/.local/share/qtermwidget6/color-schemes/ ou
     * /usr/share/qtermwidget6/color-schemes/.
     * On écrit le fichier à la volée dans le répertoire utilisateur.
     */
    static const char SCHEME_NAME[] = "CatppuccinMocha";
    static const char SCHEME_CONTENT[] =
        "[General]\n"
        "Description=Catppuccin Mocha (haplo-dialog)\n"
        "Opacity=1\n"
        "[Background]\n"  "Color=30,30,46\n"
        "[BackgroundIntense]\n" "Color=30,30,46\n"
        "[Foreground]\n"  "Color=205,214,244\n"
        "[ForegroundIntense]\n" "Color=205,214,244\n"
        "[Color0]\n"  "Color=69,71,90\n"   /* surface1 */
        "[Color0Intense]\n" "Color=88,91,112\n"
        "[Color1]\n"  "Color=243,139,168\n" /* red */
        "[Color1Intense]\n" "Color=243,139,168\n"
        "[Color2]\n"  "Color=166,227,161\n" /* green */
        "[Color2Intense]\n" "Color=166,227,161\n"
        "[Color3]\n"  "Color=249,226,175\n" /* yellow */
        "[Color3Intense]\n" "Color=249,226,175\n"
        "[Color4]\n"  "Color=137,180,250\n" /* blue */
        "[Color4Intense]\n" "Color=137,180,250\n"
        "[Color5]\n"  "Color=203,166,247\n" /* mauve */
        "[Color5Intense]\n" "Color=203,166,247\n"
        "[Color6]\n"  "Color=137,220,235\n" /* sky */
        "[Color6Intense]\n" "Color=137,220,235\n"
        "[Color7]\n"  "Color=186,194,222\n" /* subtext1 */
        "[Color7Intense]\n" "Color=205,214,244\n";

    /* Écrire le colorscheme si absent */
    QString dir = QStandardPaths::writableLocation(QStandardPaths::GenericDataLocation)
                  + "/qtermwidget6/color-schemes";
    QDir().mkpath(dir);
    QString path = dir + "/" + SCHEME_NAME + ".colorscheme";
    if (!QFile::exists(path)) {
        QFile f(path);
        if (f.open(QIODevice::WriteOnly | QIODevice::Text)) {
            QTextStream s(&f);
            s << SCHEME_CONTENT;
        }
    }

    QStringList available = term->availableColorSchemes();
    if (available.contains(SCHEME_NAME))
        term->setColorScheme(SCHEME_NAME);
    else
        term->setColorScheme("Linux");  /* fallback sûr */
}
#endif /* HAVE_QTERMWIDGET */

/* ─── Création ───────────────────────────────────────────────────────────── */

GtkWidget *widget_terminal_create(AttributeSet *Attr, tag_attr *attr, gint Type)
{
    int w = 600, h = 300;
    const char *font_name  = "Monospace";
    int         font_size  = 11;

    if (attr) {
        const char *v;
        if ((v = get_tag_attribute(attr, "width-request")))  w = atoi(v);
        if ((v = get_tag_attribute(attr, "height-request"))) h = atoi(v);
        if ((v = get_tag_attribute(attr, "font-name")))      font_name = v;
        if ((v = get_tag_attribute(attr, "font-size")))      font_size = atoi(v);
    }

#if HAVE_QTERMWIDGET
    /*
     * Créer le terminal avec startNow=0 pour configurer avant le démarrage
     * du shell, puis appeler startShellProgram() manuellement.
     */
    QTermWidget *term = new QTermWidget(0 /* don't start shell yet */);
    term->setMinimumSize(w, h);

    /* Police */
    QFont mono(QString::fromUtf8(font_name), font_size);
    mono.setStyleHint(QFont::Monospace);
    mono.setFixedPitch(true);
    term->setTerminalFont(mono);

    /* Thème */
    _apply_catppuccin_mocha(term);

    /* Défilement */
    term->setScrollBarPosition(QTermWidget::ScrollBarRight);
    term->setHistorySize(5000);

    /* Démarrer le shell */
    term->startShellProgram();

    /* Injecter la commande si présente */
    if (Attr) {
        GList *element = NULL;
        gchar *cmd = attributeset_get_first(&element, Attr, ATTR_INPUT);
        if (cmd && *cmd) {
            /* Utiliser sendText pour envoyer la commande au shell actif */
            term->sendText(QString::fromUtf8(cmd) + "\n");
        }
    }

    return (GtkWidget *)term;

#else
    /* ── Stub : QPlainTextEdit read-only ────────────────────────────────── */
    QPlainTextEdit *te = new QPlainTextEdit();
    te->setReadOnly(true);
    te->setMinimumSize(w, h);

    /* Thème Catppuccin Mocha via palette */
    QPalette pal = te->palette();
    pal.setColor(QPalette::Base,   QColor(0x1e, 0x1e, 0x2e));
    pal.setColor(QPalette::Text,   QColor(0xcd, 0xd6, 0xf4));
    te->setPalette(pal);

    QFont mono(QString::fromUtf8(font_name), font_size);
    mono.setStyleHint(QFont::Monospace);
    mono.setFixedPitch(true);
    te->setFont(mono);

    te->setPlaceholderText(
        QString::fromUtf8("[terminal — QTermWidget6 non disponible]\n"
                          "Installer libqtermwidget6-dev pour le terminal live."));

    if (Attr) {
        GList *element = NULL;
        gchar *cmd = attributeset_get_first(&element, Attr, ATTR_INPUT);
        if (cmd && *cmd) {
            FILE *fp = widget_opencommand(cmd);
            if (fp) {
                char line[1024];
                QString out;
                while (fgets(line, sizeof(line), fp))
                    out += QString::fromUtf8(line);
                fclose(fp);  /* safe_popen() → fdopen() : fclose() obligatoire */
                te->setPlainText(out);
            }
        }
    }
    return (GtkWidget *)te;
#endif
}

/* ─── API widget standard ────────────────────────────────────────────────── */

gchar *widget_terminal_envvar_construct(GtkWidget *widget)
{
    (void)widget;
    return g_strdup("");  /* terminal n'exporte pas de variable */
}

gchar *widget_terminal_envvar_all_construct(variable *var)
{
    (void)var;
    return NULL;
}

void widget_terminal_clear(variable *var)
{
    if (!var || !var->Widget) return;
#if HAVE_QTERMWIDGET
    QTermWidget *t = qobject_cast<QTermWidget *>(static_cast<QWidget *>(var->Widget));
    if (t) t->clear();
#else
    QPlainTextEdit *te = qobject_cast<QPlainTextEdit *>(static_cast<QWidget *>(var->Widget));
    if (te) te->clear();
#endif
}

void widget_terminal_refresh(variable *var)
{
    if (!var || !var->Widget) return;
    static_cast<QWidget *>(var->Widget)->update();
}

void widget_terminal_fileselect(variable *var, const char *n, const char *v)
{
    (void)var; (void)n; (void)v;
}

void widget_terminal_removeselected(variable *var) { (void)var; }
void widget_terminal_save(variable *var)           { (void)var; }
