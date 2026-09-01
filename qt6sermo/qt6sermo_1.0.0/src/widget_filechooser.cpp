/* widget_filechooser.cpp — Sélecteur de fichier Qt6 (QLineEdit + bouton « … »
 * qui ouvre QFileDialog). La variable exporte le chemin choisi. */
#include "qt6-compat.h"
#include "gtk3d.h"
#include "attributes.h"
#include "automaton.h"
#include "widgets.h"
#include "tag_attributes.h"
#include "widget_filechooser.h"
#include <QtWidgets/QWidget>
#include <QtWidgets/QHBoxLayout>
#include <QtWidgets/QLineEdit>
#include <QtWidgets/QPushButton>
#include <QtWidgets/QFileDialog>

GtkWidget *widget_filechooser_create(AttributeSet *Attr, tag_attr *attr, gint Type)
{
    (void)attr; (void)Type;
    QWidget     *w      = new QWidget();
    QHBoxLayout *layout = new QHBoxLayout(w);
    layout->setContentsMargins(0, 0, 0, 0);
    QLineEdit   *edit   = new QLineEdit();
    edit->setObjectName("fc_path");
    QPushButton *btn    = new QPushButton(QString::fromUtf8("…"));
    btn->setMaximumWidth(36);
    layout->addWidget(edit);
    layout->addWidget(btn);

    if (Attr) {
        GList *el = nullptr;
        gchar *def = attributeset_get_first(&el, Attr, ATTR_DEFAULT);
        if (def && *def) edit->setText(QString::fromUtf8(def));
    }
    QObject::connect(btn, &QPushButton::clicked, w, [edit]() {
        QString f = QFileDialog::getOpenFileName(nullptr, QString::fromUtf8("Choisir un fichier"));
        if (!f.isEmpty()) edit->setText(f);
    });
    return (GtkWidget *)w;
}
gchar *widget_filechooser_envvar_construct(GtkWidget *widget)
{
    QWidget *w = static_cast<QWidget *>(widget);
    if (!w) return g_strdup("");
    QLineEdit *e = w->findChild<QLineEdit *>("fc_path");
    return g_strdup(e ? e->text().toUtf8().constData() : "");
}
gchar *widget_filechooser_envvar_all_construct(variable *var) { if(!var||!var->Widget) return nullptr; return widget_filechooser_envvar_construct(var->Widget); }
void   widget_filechooser_clear(variable *var) { if (var && var->Widget) { QLineEdit *e = static_cast<QWidget*>(var->Widget)->findChild<QLineEdit*>("fc_path"); if (e) e->clear(); } }
void   widget_filechooser_refresh(variable *var) { (void)var; }
void   widget_filechooser_fileselect(variable *var, const char *n, const char *v) { if (var && var->Widget && v) { QLineEdit *e = static_cast<QWidget*>(var->Widget)->findChild<QLineEdit*>("fc_path"); if (e) e->setText(QString::fromUtf8(v)); } (void)n; }
void   widget_filechooser_removeselected(variable *var) { (void)var; }
void   widget_filechooser_save(variable *var) { (void)var; }
