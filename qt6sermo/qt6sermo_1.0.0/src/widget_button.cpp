/* widget_button.cpp — Bouton Qt6
 * fltk1d 1.0.0 — haplo-dialog <devel@haplo-dialog.fr>
 */
#include "qt6-compat.h"
#include "gtk3d.h"
#include "attributes.h"
#include "automaton.h"
#include "widgets.h"
#include "tag_attributes.h"
#include "widget_button.h"
#include "signals.h"
#include "actions.h"
#include "safe_exec.h"
#include <QtWidgets/QPushButton>
#include <QtGui/QIcon>
#include <string.h>
#include <stdlib.h>

GtkWidget *widget_button_create(AttributeSet *Attr, tag_attr *attr, gint Type)
{
    GList *element = nullptr;
    gchar *label = nullptr;
    if (Attr) label = attributeset_get_first(&element, Attr, ATTR_LABEL);
    QPushButton *btn = new QPushButton(label ? QString::fromUtf8(label) : QString());

    /* Icône : <input file icon="nom"> / stock="nom" → thème d'icônes Qt
     * (équivalent du chargement gtk_icon_theme du port de référence). */
    if (Attr) {
        GList *ie = nullptr;
        gchar *inp = attributeset_get_first(&ie, Attr, ATTR_INPUT);
        if (inp) {
            gchar *icon  = attributeset_get_this_tagattr(&ie, Attr, ATTR_INPUT, "icon");
            gchar *stock = attributeset_get_this_tagattr(&ie, Attr, ATTR_INPUT, "stock");
            const char *name = (icon && *icon) ? icon : ((stock && *stock) ? stock : nullptr);
            if (name) {
                QIcon ic = QIcon::fromTheme(QString::fromUtf8(name));
                if (!ic.isNull()) btn->setIcon(ic);
            } else if (strncmp(inp, "file:", 5) == 0 && inp[5]) {
                QIcon ic(QString::fromUtf8(inp + 5));     /* chemin d'image */
                if (!ic.isNull()) btn->setIcon(ic);
            }
        }
    }

    /* Taille : ne forcer un minimum que si width/height-request est explicite ;
     * sinon laisser Qt/le layout dimensionner (évite les boutons trop gros). */
    if (attr) {
        const char *v;
        if ((v = get_tag_attribute(attr, "width-request"))  && atoi(v) > 0)
            btn->setMinimumWidth(atoi(v));
        if ((v = get_tag_attribute(attr, "height-request")) && atoi(v) > 0)
            btn->setMinimumHeight(atoi(v));
    }

    /* Actions exécutées au clic. On ne peut PAS passer par
     * widget_signal_executor() : il décide du signal par défaut via
     * GTK_IS_BUTTON(), qui vaut 0 sur ce port → l'action ne serait jamais
     * exécutée. On itère donc ATTR_ACTION et on appelle execute_action()
     * directement (gère les préfixes exit:/command:/…). Le signal par défaut
     * d'un bouton est « clicked ». */
    AttributeSet *acap = Attr;
    QObject::connect(btn, &QPushButton::clicked, btn, [acap]() {
        if (!acap) return;
        GList *ae = nullptr;
        gchar *cmd = attributeset_get_first(&ae, acap, ATTR_ACTION);
        while (cmd) {
            gchar *function = attributeset_get_this_tagattr(&ae, acap, ATTR_ACTION, "function");
            if (!function)
                function = attributeset_get_this_tagattr(&ae, acap, ATTR_ACTION, "type");
            gchar *signal = attributeset_get_this_tagattr(&ae, acap, ATTR_ACTION, "signal");
            if (!signal || g_ascii_strcasecmp(signal, "clicked") == 0)
                execute_action(nullptr, cmd, function);
            cmd = attributeset_get_next(&ae, acap, ATTR_ACTION);
        }
    });

    return (GtkWidget *)btn;
}

gchar *widget_button_envvar_construct(GtkWidget *widget) { return g_strdup("true"); }
gchar *widget_button_envvar_all_construct(variable *var) { if (!var || !var->Widget) return nullptr; return widget_button_envvar_construct(var->Widget); }
void   widget_button_clear(variable *var) {}
void   widget_button_refresh(variable *var) { if (var && var->Widget) static_cast<QWidget*>(var->Widget)->update(); }
void   widget_button_fileselect(variable *var, const char *n, const char *v) {}
void   widget_button_removeselected(variable *var) {}
void   widget_button_save(variable *var) {}
