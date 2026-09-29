/* SPDX-License-Identifier: GPL-2.0-or-later */
/*
 * Copyright (C) 2026 S. Cage
 * haplo-dialog <devel@haplo-dialog.fr>
 */
/*
 * widget_table.c — Tableau EFL via elm_table (grille native)
 * sermo — haplo-dialog <devel@haplo-dialog.fr>
 * Licence : GPL-2.0-or-later
 *
 * elm_table dispose les widgets en grille rows×cols.
 * Chaque cellule est un elm_label.
 * Colonnes séparées par « | » dans <label>, <item> et <input>, comme chez
 * l'étalon gtk3sermo (une tabulation les séparait autrefois : l'en-tête
 * « A|B » tenait alors dans une seule cellule).
 *
 * Sécurité :
 *   - fclose() sur FILE* de widget_opencommand() — jamais pclose()
 *   - Taille de ligne bornée à TABLE_LINE_MAX
 *   - Nombre de lignes borné à TABLE_ROWS_MAX
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
#include "widget_table.h"
#include "safe_exec.h"
#include <string.h>
#include <stdlib.h>
#include <stdio.h>

#define TABLE_LINE_MAX 4096
#define TABLE_ROWS_MAX 1024
#define TABLE_COLS_MAX 32

typedef struct {
    Evas_Object *table;     /* elm_table principal */
    char        *selected;  /* Ligne sélectionnée */
    int          n_rows;    /* rangées posées, en-tête compris */
    int          n_cols;
    int          first_data; /* indice de la 1re rangée de données (1 sous un en-tête) */
} TableData;

/* ─── Ajoute une cellule label dans elm_table ─────────────────────────────── */
static void _table_add_cell(Evas_Object *table, const char *text,
                             int row, int col, int is_header)
{
    Evas_Object *lbl = elm_label_add(table);
    if (!lbl) return;
    if (is_header) {
        /* En-tête en gras via markup EFL */
        char *marked = snprintf_safe("<b>%s</b>", text ? text : "");
        elm_object_text_set(lbl, marked ? marked : "");
        free(marked);
    } else {
        elm_object_text_set(lbl, text ? text : "");
    }
    elm_table_pack(table, lbl, col, row, 1, 1);
    evas_object_show(lbl);
}

/* ─── Découpe une ligne « a|b|c » et remplit les colonnes ─────────────────── */
static int _parse_row(Evas_Object *table, const char *line,
                       int row, int max_cols, int is_header)
{
    char buf[TABLE_LINE_MAX];
    size_t len = strlen(line);
    if (len >= TABLE_LINE_MAX) len = TABLE_LINE_MAX - 1;
    memcpy(buf, line, len); buf[len] = '\0';
    /* Supprimer le \n final */
    while (len > 0 && (buf[len-1]=='\n'||buf[len-1]=='\r')) buf[--len]='\0';
    /* une ligne vide reste une rangée (une cellule vide) : l'étalon la garde */

    int col = 0;
    char *tok = buf, *end;
    while (col < max_cols) {
        end = strchr(tok, '|');
        if (end) *end = '\0';
        _table_add_cell(table, tok, row, col, is_header);
        col++;
        if (!end) break;
        tok = end + 1;
    }
    return col;
}

GtkWidget *widget_table_create(AttributeSet *Attr, tag_attr *attr, gint Type)
{
    Evas_Object *win = efl_main_win_get();
    Evas_Object *parent = win ? win : elm_win_add(NULL, "efl1dialog-tmp", ELM_WIN_BASIC);

    /* Scroller pour gérer les grands tableaux */
    Evas_Object *sc = elm_scroller_add(parent);
    elm_scroller_bounce_set(sc, EINA_FALSE, EINA_TRUE);
    evas_object_size_hint_weight_set(sc, EVAS_HINT_EXPAND, EVAS_HINT_EXPAND);
    evas_object_size_hint_align_set(sc, EVAS_HINT_FILL, EVAS_HINT_FILL);

    Evas_Object *table = elm_table_add(sc);
    elm_table_homogeneous_set(table, EINA_FALSE);
    elm_table_padding_set(table, 4, 2);
    elm_object_content_set(sc, table);

    TableData *td = calloc(1, sizeof(TableData));
    if (!td) { evas_object_show(sc); return (GtkWidget *)sc; }
    td->table    = table;
    td->selected = strdup("");
    td->n_rows   = 0;
    td->n_cols   = TABLE_COLS_MAX;
    evas_object_data_set(sc, "table_data", td);

    int row = 0;

    /* Semantique etalon : <label>A|B</label> = EN-TETES ; chaque <item>
     * est une LIGNE de donnees. (L'ancienne version prenait le premier
     * <item> pour l'en-tete : la seule ligne du banc disparaissait.) */
    if (Attr) {
        GList *el = NULL;
        gchar *hdr = attributeset_get_first(&el, Attr, ATTR_LABEL);
        if (hdr && *hdr) { _parse_row(table, hdr, row, td->n_cols, 1); row++; }
    }
    td->first_data = row;
    if (Attr) {
        GList *el = NULL;
        gchar *item = attributeset_get_first(&el, Attr, ATTR_ITEM);
        while (item && row < TABLE_ROWS_MAX) {
            if (*item) {
                _parse_row(table, item, row, td->n_cols, 0);
                /* Parite : la 1re ligne de donnees est selectionnee par
                 * defaut, et la variable exporte sa PREMIERE colonne. */
                if (td->selected && !*td->selected) {
                    const char *pipe = strchr(item, '|');
                    free(td->selected);
                    /* strndup/strdup : td->selected se libère par free() */
                    td->selected = pipe ? strndup(item, pipe - item)
                                        : strdup(item);
                }
                row++;
            }
            item = attributeset_get_next(&el, Attr, ATTR_ITEM);
        }
    }

    /* <input> : lu par widget_table_refresh(), que le cœur appelle juste après
     * la création (lu ici aussi, la commande tournerait deux fois). */

    td->n_rows = row;
    evas_object_show(table);
    evas_object_show(sc);
    return (GtkWidget *)sc;
}

gchar *widget_table_envvar_construct(GtkWidget *w)
{
    if (!w) return g_strdup("");
    TableData *td = (TableData *)evas_object_data_get((Evas_Object *)w, "table_data");
    if (!td || !td->selected) return g_strdup("");
    return g_strdup(td->selected);
}
gchar *widget_table_envvar_all_construct(variable *var)
{
    if (!var || !var->Widget) return NULL;
    return widget_table_envvar_construct(var->Widget);
}
void widget_table_clear(variable *var)
{
    if (!var || !var->Widget) return;
    TableData *td = (TableData *)evas_object_data_get(
        (Evas_Object *)var->Widget, "table_data");
    if (!td) return;
    elm_table_clear(td->table, EINA_TRUE);
    td->n_rows = 0;
    td->first_data = 0;   /* elm_table_clear emporte aussi l'en-tête */
    free(td->selected); td->selected = strdup("");
}
void widget_table_refresh(variable *var)
{
    TableData *td;
    gchar **lines, **p;
    int vide;

    if (!var || !var->Widget || !var->Attributes) return;
    td = (TableData *)evas_object_data_get((Evas_Object *)var->Widget, "table_data");
    if (!td) return;

    /* <input> (commande ou fichier) : chaque ligne, vide comprise, AJOUTE une
     * rangée de données — jamais d'en-tête, et pas de vidage : c'est l'étalon. */
    lines = widget_input_lines(var->Attributes);
    if (!lines) return;
    vide = td->n_rows <= td->first_data;
    for (p = lines; *p && td->n_rows < TABLE_ROWS_MAX; p++) {
        _parse_row(td->table, *p, td->n_rows, td->n_cols, 0);
        /* Comme après <item> : un tableau sans rangée de données exporte la
         * 1re colonne de sa 1re rangée. */
        if (vide && p == lines) {
            free(td->selected);
            td->selected = strndup(*p, strcspn(*p, "|"));
        }
        td->n_rows++;
    }
    g_strfreev(lines);
}
void widget_table_fileselect(variable *var, const char *n, const char *v) {}
void widget_table_removeselected(variable *var) {}
void widget_table_save(variable *var) {}
