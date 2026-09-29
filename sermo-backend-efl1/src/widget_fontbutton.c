/* SPDX-License-Identifier: GPL-2.0-or-later */
/*
 * Copyright (C) 2026 S. Cage
 * haplo-dialog <devel@haplo-dialog.fr>
 */
/*
 * widget_fontbutton.c — Sélecteur de police EFL
 * sermo — haplo-dialog <devel@haplo-dialog.fr>
 * Licence : GPL-2.0-or-later
 *
 * EFL n'a pas d'équivalent GtkFontChooserDialog natif.
 * Solution : bouton qui ouvre un elm_popup avec elm_genlist
 * listant les polices disponibles via fc-list.
 *
 * Sécurité :
 *   - fc-list passe par safe_popen → fclose (jamais pclose)
 *   - Nombre de polices borné à FONT_MAX
 *   - Les noms de polices sont sanitisés (pas d'injection EFL markup)
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
#include "widget_fontbutton.h"
#include "safe_exec.h"
#include <string.h>
#include <stdlib.h>
#include <stdio.h>

#define FONT_MAX       512
#define FONT_NAME_MAX  128

typedef struct {
    Evas_Object *btn;               /* Bouton principal */
    char         font_desc[FONT_NAME_MAX]; /* Police sélectionnée */
    char        *font_list[FONT_MAX];      /* Liste des polices */
    int          font_count;
} FontData;

/* ─── Item class genlist pour la liste de polices ─────────────────────────── */
static char *_font_text_get(void *data, Evas_Object *obj, const char *part)
{
    return strdup((const char *)data);
}

static Elm_Genlist_Item_Class *_font_itc(void)
{
    static Elm_Genlist_Item_Class *itc = NULL;
    if (!itc) {
        itc = elm_genlist_item_class_new();
        itc->item_style    = "default";
        itc->func.text_get = _font_text_get;
        itc->func.content_get = NULL;
        itc->func.state_get   = NULL;
        itc->func.del         = NULL;
    }
    return itc;
}

/* ─── Callback sélection police ───────────────────────────────────────────── */
static void _font_selected(void *data, Evas_Object *obj, void *event_info)
{
    FontData *fd = (FontData *)data;
    Elm_Object_Item *item = (Elm_Object_Item *)event_info;
    if (!item || !fd) return;
    const char *name = elm_object_item_text_get(item);
    if (!name) return;
    snprintf(fd->font_desc, sizeof(fd->font_desc), "%s", name);
    elm_object_text_set(fd->btn, fd->font_desc);
    /* Fermer le popup parent */
    Evas_Object *popup = evas_object_data_get(obj, "font_popup");
    if (popup) evas_object_hide(popup);
}

/* ─── Callback clic sur le bouton → ouvre le popup ─────────────────────────── */
static void _btn_clicked(void *data, Evas_Object *obj, void *event_info)
{
    FontData *fd = (FontData *)data;
    Evas_Object *win = efl_main_win_get();
    if (!win || !fd) return;

    Evas_Object *popup = elm_popup_add(win);
    elm_object_part_text_set(popup, "title,text", "Choisir une police");
    elm_popup_orient_set(popup, ELM_POPUP_ORIENT_CENTER);

    Evas_Object *box = elm_box_add(popup);
    evas_object_size_hint_min_set(box, 300, 400);

    Evas_Object *gl = elm_genlist_add(box);
    evas_object_data_set(gl, "font_popup", popup);
    evas_object_size_hint_weight_set(gl, EVAS_HINT_EXPAND, EVAS_HINT_EXPAND);
    evas_object_size_hint_align_set(gl, EVAS_HINT_FILL, EVAS_HINT_FILL);
    evas_object_smart_callback_add(gl, "selected", _font_selected, fd);

    Elm_Genlist_Item_Class *itc = _font_itc();
    for (int i = 0; i < fd->font_count; i++) {
        elm_genlist_item_append(gl, itc, fd->font_list[i],
                                NULL, ELM_GENLIST_ITEM_NONE, NULL, NULL);
    }
    evas_object_show(gl);
    elm_box_pack_end(box, gl);
    evas_object_show(box);
    elm_object_content_set(popup, box);
    evas_object_show(popup);
}

/* ─── Charger la liste de polices via fc-list ─────────────────────────────── */
static void _load_fonts(FontData *fd)
{
    fd->font_count = 0;
    FILE *fp = safe_popen("fc-list : family");
    if (!fp) {
        /* Fallback : quelques polices communes */
        fd->font_list[fd->font_count++] = strdup("Sans");
        fd->font_list[fd->font_count++] = strdup("Serif");
        fd->font_list[fd->font_count++] = strdup("Monospace");
        return;
    }
    char line[FONT_NAME_MAX];
    while (fgets(line, sizeof(line), fp) && fd->font_count < FONT_MAX) {
        size_t len = strlen(line);
        while (len > 0 && (line[len-1]=='\n'||line[len-1]=='\r'||line[len-1]==' '))
            line[--len] = '\0';
        if (len == 0) continue;
        /* Prendre seulement la première famille (avant la virgule) */
        char *comma = strchr(line, ',');
        if (comma) *comma = '\0';
        /* Sanitiser : supprimer les balises EFL markup < > */
        char *lt;
        while ((lt = strchr(line, '<'))) *lt = '(';
        while ((lt = strchr(line, '>'))) *lt = ')';
        fd->font_list[fd->font_count++] = strdup(line);
    }
    /* CRITIQUE : fclose, pas pclose */
    fclose(fp);
}

GtkWidget *widget_fontbutton_create(AttributeSet *Attr, tag_attr *attr, gint Type)
{
    Evas_Object *win = efl_main_win_get();
    Evas_Object *parent = win ? win : elm_win_add(NULL, "efl1dialog-tmp", ELM_WIN_BASIC);

    FontData *fd = calloc(1, sizeof(FontData));
    if (!fd) return NULL;
    snprintf(fd->font_desc, sizeof(fd->font_desc), "Sans 12");

    if (Attr) {
        GList *el = NULL;
        gchar *def = attributeset_get_first(&el, Attr, ATTR_DEFAULT);
        if (def && *def)
            snprintf(fd->font_desc, sizeof(fd->font_desc), "%s", def);
    }

    Evas_Object *btn = elm_button_add(parent);
    elm_object_text_set(btn, fd->font_desc);
    fd->btn = btn;

    /* Charger la liste de polices en arrière-plan (synchrone ici) */
    _load_fonts(fd);

    evas_object_smart_callback_add(btn, "clicked", _btn_clicked, fd);
    evas_object_data_set(btn, "font_data", fd);
    evas_object_show(btn);
    return (GtkWidget *)btn;
}

gchar *widget_fontbutton_envvar_construct(GtkWidget *w)
{
    if (!w) return g_strdup("Sans 12");
    FontData *fd = (FontData *)evas_object_data_get((Evas_Object *)w, "font_data");
    return g_strdup(fd ? fd->font_desc : "Sans 12");
}
gchar *widget_fontbutton_envvar_all_construct(variable *var)
{
    if (!var || !var->Widget) return NULL;
    return widget_fontbutton_envvar_construct(var->Widget);
}
void widget_fontbutton_clear(variable *var) {}
void widget_fontbutton_refresh(variable *var) {}
void widget_fontbutton_fileselect(variable *var, const char *n, const char *v) {}
void widget_fontbutton_removeselected(variable *var) {}
void widget_fontbutton_save(variable *var) {}
