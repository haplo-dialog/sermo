/* widget_text.cpp — Étiquette texte Qt6 (QLabel)
 *
 * <text> est une étiquette d'affichage (comme GtkLabel), PAS une zone éditable.
 * Le texte vient de <label> (ATTR_LABEL, cas courant), sinon <default>, sinon
 * la sortie d'une commande <input>. (Les zones éditables multilignes sont des
 * <edit>, gérées par widget_edit.)
 */
#include "qt6-compat.h"
#include "gtk3d.h"
#include "attributes.h"
#include "automaton.h"
#include "widgets.h"
#include "tag_attributes.h"
#include "widget_text.h"
#include <QtWidgets/QLabel>
#include <string.h>
#include <stdlib.h>

GtkWidget *widget_text_create(AttributeSet *Attr, tag_attr *attr, gint Type)
{
    GList  *element = nullptr;
    QLabel *lbl     = new QLabel();
    /* Pas de retour à la ligne automatique (comme GtkLabel par défaut) : avec
     * un ressort d'empilement à gauche dans la boîte, le wordwrap réduisait la
     * cellule à sa largeur minimale et coupait « Debian GNU/Linux » en deux. */
    lbl->setWordWrap(false);
    lbl->setTextInteractionFlags(Qt::TextSelectableByMouse);
    /* GtkLabel est CENTRÉ par défaut (xalign 0.5), QLabel est aligné à gauche.
     * Sans cette ligne le même XML ne rendait pas pareil : gtk3sermo et
     * gtk4sermo centraient « Votre nom : », qt6sermo le collait à gauche.
     * Mesuré sur les captures des trois ports le 2026-09-03. */
    lbl->setAlignment(Qt::AlignCenter);

    if (Attr) {
        /* <text><label>…</label></text> : cas le plus courant (était ignoré). */
        gchar *l = attributeset_get_first(&element, Attr, ATTR_LABEL);
        if (l && *l) lbl->setText(QString::fromUtf8(l));
        element = nullptr;
        gchar *def = attributeset_get_first(&element, Attr, ATTR_DEFAULT);
        if (def && *def) lbl->setText(QString::fromUtf8(def));
        element = nullptr;
        gchar *input = attributeset_get_first(&element, Attr, ATTR_INPUT);
        if (input && *input) {
            FILE *f = widget_opencommand(input);  /* wrapper sécurisé (g_warning si NULL) */
            if (f) {
                char buf[4096]; QString txt;
                while (fgets(buf, sizeof(buf), f)) txt += buf;
                fclose(f);  /* widget_opencommand → safe_popen → fdopen → fclose */
                if (txt.endsWith('\n')) txt.chop(1);
                lbl->setText(txt);
            }
        }
    }
    if (attr) {
        const char *v;
        if ((v = get_tag_attribute(attr, "width-request")))
            lbl->setMinimumWidth(atoi(v));
        if ((v = get_tag_attribute(attr, "sensitive")) && strcasecmp(v, "false") == 0)
            lbl->setEnabled(false);
    }
    return (GtkWidget *)lbl;
}

gchar *widget_text_envvar_construct(GtkWidget *widget)
{
    QLabel *lbl = static_cast<QLabel*>(widget);
    if (!lbl) return g_strdup("");
    return g_strdup(lbl->text().toUtf8().constData());
}
gchar *widget_text_envvar_all_construct(variable *var) { if (!var || !var->Widget) return nullptr; return widget_text_envvar_construct(var->Widget); }
void   widget_text_clear(variable *var) { if (var && var->Widget) static_cast<QLabel*>(var->Widget)->clear(); }
void   widget_text_refresh(variable *var) {}
void   widget_text_fileselect(variable *var, const char *n, const char *v) { if (var && var->Widget && v) static_cast<QLabel*>(var->Widget)->setText(QString::fromUtf8(v)); }
void   widget_text_removeselected(variable *var) {}
void   widget_text_save(variable *var) {}
