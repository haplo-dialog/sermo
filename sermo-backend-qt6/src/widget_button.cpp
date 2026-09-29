/* SPDX-License-Identifier: GPL-2.0-or-later */
/*
 * Copyright (C) 2026 S. Cage
 * haplo-dialog <devel@haplo-dialog.fr>
 */
/* widget_button.cpp — Bouton Qt6
 * sermo — haplo-dialog <devel@haplo-dialog.fr>
 * Licence : GPL-2.0-or-later
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
#include "sermo_icon_theme.h"   /* repli freedesktop quand le theme Qt est absent */
#include <string.h>
#include <stdlib.h>

/* Machinerie de sortie (actions.c, C) — jamais declaree en en-tete. */
extern "C" void action_exitprogram(GtkWidget *widget, char *string);

GtkWidget *widget_button_create(AttributeSet *Attr, tag_attr *attr, gint Type)
{
    GList *element = nullptr;
    gchar *label = nullptr;
    if (Attr) label = attributeset_get_first(&element, Attr, ATTR_LABEL);

    /* Boutons standard : <button ok>, <button cancel>, <button yes>… n'ont pas
     * de <label>, leur libellé vient de leur TYPE. Le port de référence le pose
     * explicitement (widget_button.c : « OK », « Cancel », « Help », « No »,
     * « Yes »). Sans cela le bouton sortait VIDE — visible dès l'exemple
     * d'accueil, dont le <button ok> n'affichait rien. */
    if (!label || !*label) {
        switch (Type) {
        case WIDGET_OKBUTTON:     label = (gchar *)"OK";     break;
        case WIDGET_CANCELBUTTON: label = (gchar *)"Cancel"; break;
        case WIDGET_HELPBUTTON:   label = (gchar *)"Help";   break;
        case WIDGET_NOBUTTON:     label = (gchar *)"No";     break;
        case WIDGET_YESBUTTON:    label = (gchar *)"Yes";    break;
        default: break;
        }
    }

    QPushButton *btn = new QPushButton(label ? QString::fromUtf8(label) : QString());

    /* Togglebutton : bouton CHECKABLE qui bascule. Il n'est PAS de la famille
     * OK/Cancel/… et n'a donc PAS d'action de sortie par défaut (sémantique
     * étalon gtk3 widget_button.c:347). setCheckable sert aussi à le distinguer
     * à l'export de valeur (isCheckable(), voir envvar_construct). Sans cela un
     * clic sur un togglebutton nu appelait action_exitprogram → le processus
     * sortait au lieu de basculer (garde_clic_widgets, mesuré 2026-09-09). */
    if (Type == WIDGET_TOGGLEBUTTON) {
        btn->setCheckable(true);
        if (Attr) {
            GList *de = nullptr;
            gchar *def = attributeset_get_first(&de, Attr, ATTR_DEFAULT);
            if (def && (g_ascii_strcasecmp(def, "true") == 0 || strcmp(def, "1") == 0))
                btn->setChecked(true);
        }
    }

    /* Icône : <input file icon="nom"> / stock="nom" → thème d'icônes Qt
     * (équivalent du chargement gtk_icon_theme du port de référence). */
    if (Attr) {
        GList *ie = nullptr;
        gchar *inp = attributeset_get_first(&ie, Attr, ATTR_INPUT);
        if (inp) {
            gchar *icon  = attributeset_get_this_tagattr(&ie, Attr, ATTR_INPUT, (gchar *)"icon");
            gchar *stock = attributeset_get_this_tagattr(&ie, Attr, ATTR_INPUT, (gchar *)"stock");
            const char *name = (icon && *icon) ? icon : ((stock && *stock) ? stock : nullptr);
            if (name) {
                QIcon ic = QIcon::fromTheme(QString::fromUtf8(name));
                if (ic.isNull()) {
                    /* Session nue (Xvfb/sway sans thème de plateforme) : Qt ne
                     * connaît aucun thème d'icônes → fromTheme rend un QIcon
                     * vide. On résout par le cœur freedesktop partagé, comme
                     * les ports sans bibliothèque d'icônes (fltk1/efl1/sdl3). */
                    char *p = sermo_icon_lookup(name, 16);
                    if (p) { ic = QIcon(QString::fromUtf8(p)); free(p); }
                }
                if (!ic.isNull()) btn->setIcon(ic);
            } else if (strncmp(inp, "file:", 5) == 0 && inp[5]) {
                QIcon ic(QString::fromUtf8(inp + 5));     /* chemin d'image */
                if (!ic.isNull()) btn->setIcon(ic);
            }
        }
    }

    /* image-position : le port de référence GTK 3 place l'icône à DROITE du
     * libellé quand image-position vaut "right" (ou 1 = GTK_POS_RIGHT).
     * Un QPushButton dessine toujours son icône du côté « début » de sa
     * direction de mise en page ; on inverse donc cette direction POUR CE
     * BOUTON SEUL, ce qui suffit à faire passer l'icône à droite du texte.
     * Le libellé, lui, n'est pas retourné : le style le dessine via
     * QStyle::drawItemText(), dont la direction d'écriture est déduite du
     * contenu (Qt::LayoutDirectionAuto), pas de celle du widget. */
    if (attr && label && *label && !btn->icon().isNull()) {
        const char *ipos = get_tag_attribute(attr, "image-position");
        if (ipos && (g_ascii_strcasecmp(ipos, "right") == 0 || atoi(ipos) == 1))
            btn->setLayoutDirection(Qt::RightToLeft);
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
    /* Bouton SANS <action> : semantique gtkdialog de sortie — variables
     * imprimees + EXIT=<valeur> (OK/Cancel/... pour la famille, sinon le
     * label). Manquait : un clic sur un bouton nu ne faisait RIEN (meme
     * dette que les ports fltk/efl/sdl3 avant leur greffe, mesuree par la
     * garde d'echappement). */
    {
        GList *ae0 = nullptr;
        gchar *cmd0 = Attr ? attributeset_get_first(&ae0, Attr, ATTR_ACTION) : nullptr;
        if (!cmd0 || !*cmd0) {
            if (Type == WIDGET_TOGGLEBUTTON)
                return (GtkWidget *)btn;   /* togglebutton nu : bascule, ne sort pas */
            const char *ev;
            switch (Type) {
                case WIDGET_OKBUTTON:     ev = "OK";     break;
                case WIDGET_CANCELBUTTON: ev = "Cancel"; break;
                case WIDGET_YESBUTTON:    ev = "Yes";    break;
                case WIDGET_NOBUTTON:     ev = "No";     break;
                case WIDGET_HELPBUTTON:   ev = "Help";   break;
                default: ev = (label && *label) ? label : "OK"; break;
            }
            char *evd = g_strdup(ev);
            QObject::connect(btn, &QPushButton::clicked, btn, [evd]() {
                action_exitprogram(nullptr, evd);
            });
            return (GtkWidget *)btn;
        }
    }

    AttributeSet *acap = Attr;
    QObject::connect(btn, &QPushButton::clicked, btn, [acap, Type]() {
        if (!acap) return;
        GList *ae = nullptr;
        gchar *cmd = attributeset_get_first(&ae, acap, ATTR_ACTION);
        while (cmd) {
            gchar *function = attributeset_get_this_tagattr(&ae, acap, ATTR_ACTION, (gchar *)"function");
            if (!function)
                function = attributeset_get_this_tagattr(&ae, acap, ATTR_ACTION, (gchar *)"type");
            gchar *signal = attributeset_get_this_tagattr(&ae, acap, ATTR_ACTION, (gchar *)"signal");
            if (!signal || g_ascii_strcasecmp(signal, "clicked") == 0
                || (Type == WIDGET_TOGGLEBUTTON && g_ascii_strcasecmp(signal, "toggled") == 0))
                execute_action(nullptr, cmd, function);
            cmd = attributeset_get_next(&ae, acap, ATTR_ACTION);
        }
    });

    return (GtkWidget *)btn;
}

gchar *widget_button_envvar_construct(GtkWidget *widget)
{
    /* Togglebutton = seul bouton CHECKABLE : exporte son état actif true/false,
     * comme l'étalon gtk3 (widget_button.c:413). Bouton ordinaire : "true". */
    QPushButton *b = static_cast<QPushButton *>(widget);
    if (b && b->isCheckable())
        return g_strdup(b->isChecked() ? "true" : "false");
    return g_strdup("true");
}
gchar *widget_button_envvar_all_construct(variable *var) { if (!var || !var->Widget) return nullptr; return widget_button_envvar_construct(var->Widget); }
void   widget_button_clear(variable *var) {}
void   widget_button_refresh(variable *var) { if (var && var->Widget) static_cast<QWidget*>(var->Widget)->update(); }
void   widget_button_fileselect(variable *var, const char *n, const char *v) {}
void   widget_button_removeselected(variable *var) {}
void   widget_button_save(variable *var) {}
