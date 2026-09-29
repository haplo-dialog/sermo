/* SPDX-License-Identifier: GPL-2.0-or-later */
/*
 * Copyright (C) 2026 S. Cage
 * haplo-dialog <devel@haplo-dialog.fr>
 */
/*
 * widget_filechooser.c — Sélecteur de fichier EFL/Elementary
 * sermo — haplo-dialog <devel@haplo-dialog.fr>
 * Licence : GPL-2.0-or-later
 *
 * <filechooser> : l'étalon gtk3 ouvre un GtkFileChooser. Elementary a le sien,
 * elm_fileselector_button : un bouton qui déplie un vrai sélecteur (fenêtre
 * interne), avec mode dossier natif (elm_fileselector_folder_only_set). Le
 * chemin retenu est rangé dans les données de l'objet et c'est lui qui sort en
 * variable. L'ancienne implémentation « mini » reprenait juste <default> : le
 * banc passait, mais l'utilisateur n'avait aucun moyen de CHOISIR.
 *
 * Attribut de balise « action » : open (défaut) | save | select-folder.
 *
 * Export : le chemin retenu, comme l'étalon gtk3.
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
#include "widget_filechooser.h"
#include <stdlib.h>

#define FC_PATH_KEY "sermo_path"

static void _fc_pose(Evas_Object *obj, const char *chemin)
{
    char *ancien = evas_object_data_del(obj, FC_PATH_KEY);
    free(ancien);
    evas_object_data_set(obj, FC_PATH_KEY, g_strdup(chemin ? chemin : ""));
}

/* Rappel « file,chosen » : NULL si l'utilisateur a renoncé — on garde alors
 * la valeur courante plutôt que de la vider. */
static void _fc_choisi(void *data, Evas_Object *obj, void *event_info)
{
    const char *chemin = (const char *) event_info;
    (void) data;
    if (!chemin || !*chemin) return;
    _fc_pose(obj, chemin);
    elm_object_text_set(obj, chemin);
}

GtkWidget *widget_filechooser_create(AttributeSet *Attr, tag_attr *attr, gint Type)
{
    (void) Type;
    Evas_Object *parent = efl_main_win_get();
    Evas_Object *fs = elm_fileselector_button_add(parent ? parent
                                                         : elm_win_add(NULL, "tmp", ELM_WIN_BASIC));
    const char *defaut = NULL, *label = NULL, *mode = NULL;

    if (Attr) {
        GList *el = NULL;
        gchar *d = attributeset_get_first(&el, Attr, ATTR_DEFAULT);
        if (d && *d) defaut = d;
        el = NULL;
        gchar *l = attributeset_get_first(&el, Attr, ATTR_LABEL);
        if (l && *l) label = l;
    }
    if (attr) {
        const char *a = get_tag_attribute(attr, "action");
        if (a && *a) mode = a;
    }

    elm_fileselector_button_inwin_mode_set(fs, EINA_TRUE);
    if (mode && !strcasecmp(mode, "select-folder"))
        elm_fileselector_folder_only_set(fs, EINA_TRUE);
    if (mode && !strcasecmp(mode, "save"))
        elm_fileselector_is_save_set(fs, EINA_TRUE);
    if (defaut) elm_fileselector_path_set(fs, defaut);

    _fc_pose(fs, defaut);
    elm_object_text_set(fs, label ? label : (defaut ? defaut : "Parcourir…"));
    evas_object_smart_callback_add(fs, "file,chosen", _fc_choisi, NULL);
    evas_object_show(fs);
    return (GtkWidget *) fs;
}

gchar *widget_filechooser_envvar_construct(GtkWidget *widget)
{
    const char *p = widget ? evas_object_data_get((Evas_Object *) widget, FC_PATH_KEY)
                           : NULL;
    return g_strdup(p ? p : "");
}
gchar *widget_filechooser_envvar_all_construct(variable *var)
{
    if (!var || !var->Widget) return NULL;
    return widget_filechooser_envvar_construct(var->Widget);
}
void widget_filechooser_clear(variable *var)
{
    if (!var || !var->Widget) return;
    _fc_pose((Evas_Object *) var->Widget, "");
}
void widget_filechooser_refresh(variable *var) { (void) var; }
void widget_filechooser_fileselect(variable *var, const char *name, const char *value)
{
    /* Le cœur pousse un chemin choisi ailleurs (action fileselect). */
    (void) name;
    if (!var || !var->Widget || !value) return;
    _fc_pose((Evas_Object *) var->Widget, value);
}
void widget_filechooser_removeselected(variable *var) { (void) var; }
void widget_filechooser_save(variable *var)           { (void) var; }
