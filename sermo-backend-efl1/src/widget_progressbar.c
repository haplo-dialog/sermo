/* SPDX-License-Identifier: GPL-2.0-or-later */
/*
 * Copyright (C) 2026 S. Cage
 * haplo-dialog <devel@haplo-dialog.fr>
 */
/*
 * widget_progressbar.c — Barre de progression EFL
 * sermo — haplo-dialog <devel@haplo-dialog.fr>
 * Licence : GPL-2.0-or-later
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
#include "widget_progressbar.h"
#include "sermo_progress.h"
#include <string.h>
#include <stdlib.h>

/* La commande <input> est lue AU FIL DE L'EAU par le cœur
 * (sermo_progress.h). Jusqu'à la 2.6.8, ce port la lisait EN ENTIER avant
 * d'ouvrir la fenêtre (3,5 s d'écran vide sur examples/progressbar), puis
 * posait un seul nombre : la barre ne progressait jamais et l'action prévue
 * à 100 ne partait pas. */
typedef struct {
    sermo_progress *lecture;
    Ecore_Timer    *releve;
    int             en_releve;   /* une action peut détruire la fenêtre pendant le relevé */
    int             detruite;
} BarreLecture;

static Eina_Bool efl_barre_releve(void *data)
{
    BarreLecture *bl = (BarreLecture *)data;
    int encore;

    bl->en_releve = 1;
    encore = bl->lecture && sermo_progress_poll(bl->lecture);
    bl->en_releve = 0;
    if (!encore) {
        bl->lecture = NULL;      /* rendue par le cœur */
        bl->releve = NULL;       /* ECORE_CALLBACK_CANCEL supprime la minuterie */
    }
    if (bl->detruite) {
        free(bl);
        return ECORE_CALLBACK_CANCEL;
    }
    return encore ? ECORE_CALLBACK_RENEW : ECORE_CALLBACK_CANCEL;
}

static void efl_barre_detruite(void *data, Evas *e, Evas_Object *obj, void *event_info)
{
    BarreLecture *bl = (BarreLecture *)data;
    (void)e; (void)obj; (void)event_info;

    sermo_progress_free(bl->lecture);   /* différée si le cœur est en train de lire */
    bl->lecture = NULL;
    if (bl->en_releve) {                /* le relevé en cours libère et s'arrête */
        bl->detruite = 1;
        return;
    }
    if (bl->releve) ecore_timer_del(bl->releve);
    free(bl);
}

static void efl_barre_fraction(void *widget, double fraction)
{
    elm_progressbar_value_set((Evas_Object *)widget, fraction);
}

static void efl_barre_texte(void *widget, const char *texte)
{
    elm_object_text_set((Evas_Object *)widget, texte);
}

GtkWidget *widget_progressbar_create(AttributeSet *Attr, tag_attr *attr, gint Type)
{
    Evas_Object *parent = efl_main_win_get();
    Evas_Object *pb = elm_progressbar_add(parent ? parent : elm_win_add(NULL,"tmp",ELM_WIN_BASIC));

    double val = 0.0;
    if (Attr) {
        GList *el = NULL;
        gchar *def = attributeset_get_first(&el, Attr, ATTR_DEFAULT);
        if (def && *def) val = g_ascii_strtod(def, NULL) / 100.0;
        el = NULL;
        gchar *lbl = attributeset_get_first(&el, Attr, ATTR_LABEL);
        if (lbl && *lbl) elm_object_text_set(pb, lbl);
    }
    elm_progressbar_value_set(pb, val);
    evas_object_size_hint_weight_set(pb, EVAS_HINT_EXPAND, 0);
    evas_object_size_hint_align_set(pb, EVAS_HINT_FILL, 0.5);

    sermo_progress *lecture = sermo_progress_start(pb, Attr, efl_barre_fraction, efl_barre_texte);
    if (lecture) {
        BarreLecture *bl = calloc(1, sizeof *bl);
        if (bl) {
            bl->lecture = lecture;
            bl->releve = ecore_timer_add(0.04, efl_barre_releve, bl);
            evas_object_event_callback_add(pb, EVAS_CALLBACK_DEL, efl_barre_detruite, bl);
        } else {
            sermo_progress_free(lecture);
        }
    }
    evas_object_show(pb);
    return (GtkWidget *)pb;
}

gchar *widget_progressbar_envvar_construct(GtkWidget *w)
{
    /* L'étalon gtk3 n'exporte RIEN pour une barre de progression. */
    (void) w;
    return g_strdup("");
}
gchar *widget_progressbar_envvar_all_construct(variable *var)
{
    if (!var || !var->Widget) return NULL;
    return widget_progressbar_envvar_construct(var->Widget);
}
void widget_progressbar_clear(variable *var)
{
    if (!var || !var->Widget) return;
    elm_progressbar_value_set((Evas_Object *)var->Widget, 0.0);
}
void widget_progressbar_refresh(variable *var) {}
void widget_progressbar_fileselect(variable *var, const char *n, const char *v) {}
void widget_progressbar_removeselected(variable *var) {}
void widget_progressbar_save(variable *var) {}
