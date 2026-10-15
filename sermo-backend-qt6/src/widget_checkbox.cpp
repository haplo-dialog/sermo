/* SPDX-License-Identifier: GPL-2.0-or-later */
/*
 * Copyright (C) 2026 S. Cage
 * haplo-dialog <devel@haplo-dialog.fr>
 */
/* widget_checkbox.cpp — Case à cocher Qt6 */
#include "qt6-compat.h"
#include "gtk3d.h"
#include "attributes.h"
#include "automaton.h"
#include "widgets.h"
#include "tag_attributes.h"
#include "widget_checkbox.h"
#include "jouer_actions.h"
#include <QtWidgets/QCheckBox>
#include <QtWidgets/QRadioButton>
#include <QtCore/QSet>
#include <string.h>
#include <stdlib.h>

/* ─── Widgets à bascule : case à cocher, bouton à bascule, bouton radio ──────
 * L'étalon joue leurs actions à chaque changement d'état (« toggled »), avec
 * « if true »/« if false ». Ce port ne les branchait pas : aucune ne jouait.
 * Le registre sert au crochet « activate: » : il n'agit que sur un bouton qu'il
 * connaît, jamais sur un pointeur d'un autre type. */
static QSet<QAbstractButton *> g_bascules;
/* Le widget à bascule dont le premier « refresh » pose l'état lu dans son
 * <input> sans jouer SES actions (voir qt6_bascule_lire). Un frère radio qu'il
 * décoche joue les siennes, comme chez l'étalon. */
static QAbstractButton *g_silence;

static void brancher(GtkWidget *bouton, AttributeSet *Attr, bool interrupteur)
{
    QAbstractButton *b = static_cast<QAbstractButton *>((void *)bouton);
    GList *el = nullptr;

    if (!b) return;
    g_bascules.insert(b);
    QObject::connect(b, &QObject::destroyed, [b]() { g_bascules.remove(b); });
    if (!Attr || !attributeset_get_first(&el, Attr, ATTR_ACTION)) return;
    QObject::connect(b, &QAbstractButton::toggled, b, [b, Attr, interrupteur](bool actif) {
        if (b == g_silence) return;
        /* exit:… peut sortir ici */
        if (interrupteur) jouer_actions_interrupteur((GtkWidget *)(void *)b, Attr, actif ? 1 : 0);
        else              jouer_actions_bascule((GtkWidget *)(void *)b, Attr, actif ? 1 : 0);
    });
}

void qt6_bascule_brancher(GtkWidget *bouton, AttributeSet *Attr)
{
    brancher(bouton, Attr, false);
}

/* L'interrupteur (<switch>) : même chose, au signal « changed » de l'étalon. */
void qt6_interrupteur_brancher(GtkWidget *bouton, AttributeSet *Attr)
{
    brancher(bouton, Attr, true);
}

/* <input> d'un widget à bascule (case, interrupteur, bouton radio, bouton à
 * bascule), à chaque « refresh », comme l'étalon gtk3sermo. Au premier passage
 * — le cœur appelle refresh juste après la création — l'état lu se pose sans
 * jouer ses actions (un frère radio décoché joue les siennes), et un <default>
 * l'emporte (l'étalon l'applique après). Aux
 * suivants, un changement d'état bascule comme un clic : les actions jouent.
 * Un bouton radio ne lit que « vrai » : il ne se décoche pas seul, et un
 * <default>false ne l'empêche pas d'être choisi. Jusqu'au 2026-10-13, l'<input>
 * de ces widgets n'était jamais lu. */
void qt6_bascule_lire(variable *var)
{
    if (!var || !var->Widget) return;
    QAbstractButton *b = static_cast<QAbstractButton *>(var->Widget);
    bool radio   = qobject_cast<QRadioButton *>(b) != nullptr;
    bool premier = !b->property("sermoLu").toBool();
    gint v = widget_input_bool(var->Attributes);

    b->setProperty("sermoLu", true);
    if (v < 0 || (radio && v == 0)) return;
    if (premier) {
        GList *el = nullptr;
        gchar *def = attributeset_get_first(&el, var->Attributes, ATTR_DEFAULT);
        if (radio ? (def && (g_ascii_strcasecmp(def, "true") == 0 || strcmp(def, "1") == 0))
                  : (def != nullptr))
            return;
        g_silence = b;
        b->setChecked(v == 1);
        g_silence = nullptr;
        return;
    }
    if ((v == 1) != b->isChecked())
        b->setChecked(v == 1);
}

/* Crochet du contrat : « activate:NOM » bascule comme un clic (le shim fait de
 * gtk_widget_activate() un FALSE). Un radio choisi le reste (groupe exclusif). */
extern "C" int sermo_be_widget_activate(void *w)
{
    QAbstractButton *b = static_cast<QAbstractButton *>(w);
    if (!b || !g_bascules.contains(b)) return 0;
    b->click();
    return 1;
}

/* Crochet du contrat : l'état d'un widget pour condition="active_is_…/
 * sensitive_is_…/visible_is_…(NOM)". Tout widget de ce port est un QWidget ;
 * « actif » seulement pour un bouton du registre. Visible = le drapeau du
 * widget lui-même (isHidden), comme gtk_widget_get_visible(). */
extern "C" int sermo_be_widget_etat(void *w, int quoi)
{
    QWidget *q = static_cast<QWidget *>(w);
    if (!q) return -1;
    switch (quoi) {
    case 0: {
        QAbstractButton *b = static_cast<QAbstractButton *>(w);
        return g_bascules.contains(b) ? (b->isChecked() ? 1 : 0) : -1;
    }
    case 1: return q->isEnabled() ? 1 : 0;
    case 2: return q->isHidden() ? 0 : 1;
    default: return -1;
    }
}

GtkWidget *widget_checkbox_create(AttributeSet *Attr, tag_attr *attr, gint Type)
{
    GList *element = nullptr;
    int w = 120, h = 28;
    if (attr) {
        const char *v;
        if ((v = get_tag_attribute(attr, "width-request")))  w = atoi(v);
        if ((v = get_tag_attribute(attr, "height-request"))) h = atoi(v);
    }
    gchar *label = nullptr;
    if (Attr) label = attributeset_get_first(&element, Attr, ATTR_LABEL);
    QCheckBox *cb = new QCheckBox(label ? label : "");
    cb->resize(w, h);
    if (Attr) {
        element = nullptr;
        gchar *def = attributeset_get_first(&element, Attr, ATTR_DEFAULT);
        if (def && (g_ascii_strcasecmp(def, "true") == 0 || strcmp(def, "1") == 0))
            cb->setChecked(true);
    }
    qt6_bascule_brancher((GtkWidget *)cb, Attr);
    return (GtkWidget *)cb;
}

gchar *widget_checkbox_envvar_construct(GtkWidget *widget)
{
    QCheckBox *cb = static_cast<QCheckBox*>(widget);
    if (!cb) return g_strdup("false");
    return g_strdup(cb->isChecked() ? "true" : "false");
}
gchar *widget_checkbox_envvar_all_construct(variable *var) { if (!var || !var->Widget) return nullptr; return widget_checkbox_envvar_construct(var->Widget); }
void   widget_checkbox_clear(variable *var) { if (var && var->Widget) static_cast<QCheckBox*>(var->Widget)->setChecked(false); }
void   widget_checkbox_refresh(variable *var) { qt6_bascule_lire(var); }
void   widget_checkbox_fileselect(variable *var, const char *n, const char *v) {}
void   widget_checkbox_removeselected(variable *var) {}
void   widget_checkbox_save(variable *var) {}
