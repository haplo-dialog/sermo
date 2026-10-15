/* SPDX-License-Identifier: GPL-2.0-or-later */
/*
 * Copyright (C) 2026 S. Cage
 * haplo-dialog <devel@haplo-dialog.fr>
 */
/*
 * widget_button.cpp — Bouton FLTK
 * sermo — haplo-dialog <devel@haplo-dialog.fr>
 * Licence : GPL-2.0-or-later
 *
 * Couvre : WIDGET_BUTTON, WIDGET_OKBUTTON, WIDGET_CANCELBUTTON,
 *          WIDGET_YESBUTTON, WIDGET_NOBUTTON, WIDGET_HELPBUTTON
 *
 * Pattern action :
 *   Le label du bouton peut contenir une <action>cmd</action>.
 *   On stocke la commande dans user_data() du widget ; le callback
 *   l'exécute via safe_exec() (hérité du core).
 */

#ifndef _GNU_SOURCE
#define _GNU_SOURCE
#endif
#include "fltk-compat.h"
#include "gtkdialog.h"
#include "attributes.h"
#include "automaton.h"
#include "widgets.h"
#include "signals.h"
#include "tag_attributes.h"
#include "widget_button.h"
#include "widget_checkbox.h"   /* fltk1_bascule_brancher() */
#include "jouer_actions.h"
#include "sermo_icon_theme.h"
#include <FL/Fl_Shared_Image.H>
#include "safe_exec.h"
#include "actions.h"

#include <string.h>
#include <stdlib.h>

/* Callback : exécuter TOUTES les actions du bouton, via le répartiteur du
 * cœur (exit:, refresh:, closewindow:, commande shell…). L'ancienne version
 * passait la PREMIÈRE action brute à safe_system : « exit:fin » partait au
 * shell et les types d'action n'existaient pas. */
static void button_cb(Fl_Widget *w, void *data)
{
    AttributeSet *Attr = (AttributeSet *)data;
    /* chacune avec sa fonction (attribut function=) : voir jouer_actions.h */
    jouer_actions((GtkWidget *)w, Attr, "clicked", "toggled");
}

/* Sémantique gtkdialog des boutons de sortie (<button ok> etc.) : imprimer
 * les variables puis sortir avec EXIT="<valeur>". La machinerie est dans
 * actions.c (C), jamais déclarée en en-tête — d'où la déclaration locale.
 * Sans ce câblage, un clic sur OK ne faisait RIEN (défaut d'origine du port). */
extern "C" void action_exitprogram(GtkWidget *widget, char *string);

static char BUTTON_EXIT_OK[]     = "OK";
static char BUTTON_EXIT_CANCEL[] = "Cancel";
static char BUTTON_EXIT_YES[]    = "Yes";
static char BUTTON_EXIT_NO[]     = "No";
static char BUTTON_EXIT_HELP[]   = "Help";

static void button_exit_cb(Fl_Widget *w, void *data)
{
    action_exitprogram((GtkWidget *)w, (char *)data);
}

/* ── widget_button_create ─────────────────────────────────────────────────── */
GtkWidget *widget_button_create(AttributeSet *Attr, tag_attr *attr, gint Type)
{
    GList      *element = NULL;
    gchar      *label   = NULL;
    gchar      *action  = NULL;
    int         w       = 120, h = 30;

    /* Label */
    if (Attr)
        label = attributeset_get_first(&element, Attr, ATTR_LABEL);
    if (!label || !*label) {
        switch (Type) {
            case WIDGET_OKBUTTON:     label = (gchar *)"OK";      break;
            case WIDGET_CANCELBUTTON: label = (gchar *)"Annuler"; break;
            case WIDGET_YESBUTTON:    label = (gchar *)"Oui";     break;
            case WIDGET_NOBUTTON:     label = (gchar *)"Non";     break;
            case WIDGET_HELPBUTTON:   label = (gchar *)"Aide";    break;
            default:                  label = (gchar *)"Bouton";  break;
        }
    }

    /* Taille demandée */
    if (attr) {
        const char *v;
        if ((v = get_tag_attribute(attr, "width-request")))  w = atoi(v);
        if ((v = get_tag_attribute(attr, "height-request"))) h = atoi(v);
    }

    /* Première action (il peut y en avoir plusieurs — on prend la première) */
    if (Attr) {
        element = NULL;
        action = attributeset_get_first(&element, Attr, ATTR_ACTION);
    }

    /* WIDGET_TOGGLEBUTTON : le cœur partagé (libsermocore) route <togglebutton>
     * dans la famille bouton → widget_button_create (widget_togglebutton_create
     * du port autonome n'est plus appelé). On le traite donc ici, comme le
     * faisait ce dernier : bouton BASCULE + état <default>. Impératif : l'export
     * passe par widget_togglebutton_envvar_construct (widgets.cpp), qui caste en
     * Fl_Toggle_Button et lit value() — il FAUT donc en créer un (un Fl_Button
     * nu rendrait toujours "false" et ignorerait le défaut). */
    if (Type == WIDGET_TOGGLEBUTTON) {
        Fl_Toggle_Button *tb = new Fl_Toggle_Button(0, 0, w, h, nullptr);
        tb->copy_label(label);
        if (Attr) {
            GList *de = NULL;
            gchar *def = attributeset_get_first(&de, Attr, ATTR_DEFAULT);
            if (def && (strcasecmp(def, "true") == 0 || strcmp(def, "1") == 0))
                tb->value(1);
        }
        /* Un togglebutton ne SORT pas au clic : il bascule. Ses <action> jouent
         * au basculement, « if true »/« if false » compris (widget_checkbox.cpp). */
        fltk1_bascule_brancher((GtkWidget *)tb, Attr, NULL);
        return (GtkWidget *)tb;
    }

    Fl_Button *btn = new Fl_Button(0, 0, w, h, nullptr);
    btn->copy_label(label);

    /* <input file icon="nom"> (ou stock="nom") : icone du theme a gauche
     * du libelle, 20 px comme l'etalon (theme-icon-size pour changer) */
    if (Attr) {
        GList *el = NULL;
        gchar *inp = attributeset_get_first(&el, Attr, ATTR_INPUT);
        while (inp) {
            if (strncasecmp(inp, "file:", 5) == 0) {
                gchar *icon = attributeset_get_this_tagattr(&el, Attr, ATTR_INPUT, (gchar *)"icon");
                if (!icon) icon = attributeset_get_this_tagattr(&el, Attr, ATTR_INPUT, (gchar *)"stock");
                if (icon && *icon) {
                    int px = 20;
                    const char *v = attr ? get_tag_attribute(attr, "theme-icon-size") : NULL;
                    if (v && atoi(v) > 0) px = atoi(v);
                    char *path = sermo_icon_lookup(icon, px);
                    if (path) {
                        Fl_Shared_Image *img = Fl_Shared_Image::get(path, px, px);
                        if (img) {
                            btn->image(img);
                            const char *pos = attr ? get_tag_attribute(attr, "image-position") : NULL;
                            Fl_Align a = FL_ALIGN_INSIDE | FL_ALIGN_CENTER;
                            if (pos && strcasecmp(pos, "right") == 0) a |= FL_ALIGN_TEXT_NEXT_TO_IMAGE;
                            else if (pos && strcasecmp(pos, "top") == 0) a |= FL_ALIGN_TEXT_OVER_IMAGE;
                            else if (pos && strcasecmp(pos, "bottom") == 0) a |= FL_ALIGN_IMAGE_OVER_TEXT;
                            else a |= FL_ALIGN_IMAGE_NEXT_TO_TEXT;
                            btn->align(a);
                        }
                        free(path);
                    }
                }
                break;
            }
            inp = attributeset_get_next(&el, Attr, ATTR_INPUT);
        }
    }

    if (action && *action) {
        /* L'AttributeSet vit toute la durée du programme : le callback
         * itère les actions dedans au moment du clic. */
        btn->callback(button_cb, (void *)Attr);
    } else {
        /* Pas d'<action> : les boutons de sortie reçoivent leur sémantique
         * gtkdialog par défaut (variables + EXIT="...", puis exit). */
        switch (Type) {
            case WIDGET_OKBUTTON:     btn->callback(button_exit_cb, BUTTON_EXIT_OK);     break;
            case WIDGET_CANCELBUTTON: btn->callback(button_exit_cb, BUTTON_EXIT_CANCEL); break;
            case WIDGET_YESBUTTON:    btn->callback(button_exit_cb, BUTTON_EXIT_YES);    break;
            case WIDGET_NOBUTTON:     btn->callback(button_exit_cb, BUTTON_EXIT_NO);     break;
            case WIDGET_HELPBUTTON:   btn->callback(button_exit_cb, BUTTON_EXIT_HELP);   break;
            default:
                /* gtkdialog : un bouton nu (sans <action>) ferme aussi le
                 * dialogue — variables imprimées, EXIT = son label. */
                btn->callback(button_exit_cb, strdup(label ? label : "OK"));
                break;
        }
    }

    return (GtkWidget *)btn;
}

/* ── widget_button_envvar_construct ──────────────────────────────────────── */
gchar *widget_button_envvar_construct(GtkWidget *widget)
{
    /* Comme l'étalon (gtk3 widget_button.c) : un bouton ordinaire exporte "",
     * un bouton à bascule son état. Ce port exportait le libellé (cas 71). */
    Fl_Button *btn = (Fl_Button *)widget;
    if (btn && btn->type() == FL_TOGGLE_BUTTON)
        return g_strdup(btn->value() ? "true" : "false");
    return g_strdup("");
}

gchar *widget_button_envvar_all_construct(variable *var)
{
    if (!var || !var->Widget) return NULL;
    return widget_button_envvar_construct(var->Widget);
}

/* ── Stubs ───────────────────────────────────────────────────────────────── */
void widget_button_clear(variable *var) {}
void widget_button_refresh(variable *var)
{
    if (!var || !var->Widget) return;
    /* Le cœur route <togglebutton> ici : son <input> se lit comme celui d'une
     * case à cocher (widget_checkbox.cpp). Un bouton simple n'en fait rien,
     * comme l'étalon. */
    if (var->Type == WIDGET_TOGGLEBUTTON) fltk1_bascule_lire(var);
    else ((Fl_Widget *)var->Widget)->redraw();
}
void widget_button_fileselect(variable *var, const char *n, const char *v) {}
void widget_button_removeselected(variable *var) {}
void widget_button_save(variable *var) {}
