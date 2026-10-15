/* SPDX-License-Identifier: GPL-2.0-or-later */
/*
 * Copyright (C) 2026 S. Cage
 * haplo-dialog <devel@haplo-dialog.fr>
 */
/*
 * widget_button.c — Bouton EFL/Elementary
 * sermo — haplo-dialog <devel@haplo-dialog.fr>
 * Licence : GPL-2.0-or-later
 *
 * <button> → elm_button_add
 * Export : label du bouton
 */
#ifndef _GNU_SOURCE
#define _GNU_SOURCE
#endif
#include "efl-compat.h"
#include "efl-globals.h"
#include "gtkdialog.h"
#include "attributes.h"
#include "automaton.h"
#include "widgets.h"
#include "tag_attributes.h"
#include "widget_button.h"
#include "widget_checkbox.h"   /* efl_bascule_lire() */
#include "jouer_actions.h"
#include "widget_togglebutton.h"
#include "safe_exec.h"
#include "actions.h"
Evas_Object *efl_theme_icon_new(Evas_Object *parent, const char *icon, int px);
#include <string.h>
#include <stdlib.h>

/* La machinerie de sortie vit dans actions.c (C), jamais declaree en
 * en-tete — declaration locale. */
extern void action_exitprogram(GtkWidget *widget, char *string);

/* TOUTES les actions du bouton, via le repartiteur du coeur. L'ancienne
 * version passait la PREMIERE action brute a safe_system. */
static void _button_clicked(void *data, Evas_Object *obj, void *event_info)
{
    (void)event_info;
    /* toutes les actions, chacune avec sa fonction : voir jouer_actions.h */
    jouer_actions((GtkWidget *)obj, (AttributeSet *)data, "clicked", "toggled");
}

/* Sortie par defaut : variables + EXIT="<valeur>", puis exit. */
static void _button_exit_clicked(void *data, Evas_Object *obj, void *event_info)
{
    (void)event_info;
    action_exitprogram((GtkWidget *)obj, (char *)data);
}

GtkWidget *widget_button_create(AttributeSet *Attr, tag_attr *attr, gint Type)
{
    /* Le cœur (étalon GTK, automaton.c) route <togglebutton> vers le groupe
     * bouton : en GTK un togglebutton EST un bouton. Sous EFL le bouton bascule
     * est un widget distinct (elm_check style=toggle, qui porte l'état + le
     * défaut) — on y redispatche pour préserver la sémantique du port. */
    if (Type == WIDGET_TOGGLEBUTTON)
        return widget_togglebutton_create(Attr, attr, Type);

    Evas_Object *win = efl_main_win_get();
    /* Fallback : créer sans parent — sera reparenté par le conteneur */
    Evas_Object *btn = elm_button_add(win ? win : elm_win_add(NULL, "tmp", ELM_WIN_BASIC));

    const char *label = NULL;
    if (Attr) {
        GList *el = NULL;
        gchar *lbl = attributeset_get_first(&el, Attr, ATTR_LABEL);
        if (lbl && *lbl) label = lbl;
    }
    if (!label) {
        switch (Type) {
            case WIDGET_OKBUTTON:     label = "OK";      break;
            case WIDGET_CANCELBUTTON: label = "Annuler"; break;
            case WIDGET_YESBUTTON:    label = "Oui";     break;
            case WIDGET_NOBUTTON:     label = "Non";     break;
            case WIDGET_HELPBUTTON:   label = "Aide";    break;
            default:                  label = "button";  break;
        }
    }
    elm_object_text_set(btn, label);

    /* <input file icon="nom"> / stock="nom" : icone du theme dans le bouton */
    if (Attr) {
        GList *el = NULL;
        gchar *inp = attributeset_get_first(&el, Attr, ATTR_INPUT);
        while (inp) {
            if (strncasecmp(inp, "file:", 5) == 0) {
                gchar *icon = attributeset_get_this_tagattr(&el, Attr, ATTR_INPUT, "icon");
                if (!icon) icon = attributeset_get_this_tagattr(&el, Attr, ATTR_INPUT, "stock");
                if (icon && *icon) {
                    int px = 20;
                    const char *v = attr ? get_tag_attribute(attr, "theme-icon-size") : NULL;
                    if (v && atoi(v) > 0) px = atoi(v);
                    Evas_Object *ic = efl_theme_icon_new(btn, icon, px);
                    if (ic) {
                        /* Icône + libellé = UN groupe centré dans le bouton
                         * (le thème d'EFL collait l'icône au bord gauche et
                         * centrait le texte seul). Le groupe va dans la part
                         * « icon », que le thème centre quand le texte est
                         * vide ; image-position règle l'ordre (gauche par
                         * défaut, right / top / bottom ou 1 / 2 / 3). */
                        const char *pos = attr ? get_tag_attribute(attr, "image-position") : NULL;
                        int droite = 0, haut = 0, bas = 0;
                        if (pos) {
                            droite = !strcasecmp(pos, "right")  || !strcmp(pos, "1");
                            haut   = !strcasecmp(pos, "top")    || !strcmp(pos, "2");
                            bas    = !strcasecmp(pos, "bottom") || !strcmp(pos, "3");
                        }
                        Evas_Object *grp = elm_box_add(btn);
                        elm_box_horizontal_set(grp, (haut || bas) ? EINA_FALSE : EINA_TRUE);
                        if (haut || bas) elm_box_padding_set(grp, 0, 2);
                        else             elm_box_padding_set(grp, 4, 0);
                        Evas_Object *lb = elm_label_add(grp);
                        elm_object_text_set(lb, label);
                        evas_object_show(lb);
                        if (droite || bas) { elm_box_pack_end(grp, lb); elm_box_pack_end(grp, ic); }
                        else               { elm_box_pack_end(grp, ic); elm_box_pack_end(grp, lb); }
                        evas_object_show(ic);
                        evas_object_show(grp);
                        elm_object_text_set(btn, NULL);
                        elm_object_part_content_set(btn, "icon", grp);
                    }
                }
                break;
            }
            inp = attributeset_get_next(&el, Attr, ATTR_INPUT);
        }
    }

    /* Actions : le repartiteur du coeur ; sans <action>, semantique
     * gtkdialog de sortie (variables + EXIT), y compris bouton nu. */
    {
        GList *el = NULL;
        gchar *cmd = Attr ? attributeset_get_first(&el, Attr, ATTR_ACTION) : NULL;
        if (cmd && *cmd) {
            evas_object_smart_callback_add(btn, "clicked", _button_clicked, Attr);
        } else {
            const char *ev;
            switch (Type) {
                case WIDGET_OKBUTTON:     ev = "OK";     break;
                case WIDGET_CANCELBUTTON: ev = "Cancel"; break;
                case WIDGET_YESBUTTON:    ev = "Yes";    break;
                case WIDGET_NOBUTTON:     ev = "No";     break;
                case WIDGET_HELPBUTTON:   ev = "Help";   break;
                default:                  ev = label;    break;
            }
            evas_object_smart_callback_add(btn, "clicked", _button_exit_clicked,
                                           strdup(ev));
        }
    }

    evas_object_data_set(btn, "label", strdup(label));
    evas_object_show(btn);
    return (GtkWidget *)btn;
}

gchar *widget_button_envvar_construct(GtkWidget *widget)
{
    /* Comme l'étalon (gtk3 widget_button.c) : un bouton ordinaire exporte "",
     * un bouton à bascule son état (widget_togglebutton.c ici). Ce port exportait le libellé (cas 71). */
    (void)widget;
    return g_strdup("");
}

gchar *widget_button_envvar_all_construct(variable *var)
{
    if (!var || !var->Widget) return NULL;
    return widget_button_envvar_construct(var->Widget);
}

void widget_button_clear(variable *var) {}
void widget_button_refresh(variable *var)
{
    if (!var || !var->Widget) return;
    /* Le cœur route <togglebutton> ici : son <input> se lit comme celui d'une
     * case à cocher (widget_checkbox.c). Un bouton simple n'en fait rien,
     * comme l'étalon. */
    if (var->Type == WIDGET_TOGGLEBUTTON) { efl_bascule_lire(var); return; }
    evas_object_show((Evas_Object *)var->Widget);
}
void widget_button_fileselect(variable *var, const char *n, const char *v) {}
void widget_button_removeselected(variable *var) {}
void widget_button_save(variable *var) {}
