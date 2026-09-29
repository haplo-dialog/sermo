/* SPDX-License-Identifier: GPL-2.0-or-later */
/*
 * Copyright (C) 2026 S. Cage
 * haplo-dialog <devel@haplo-dialog.fr>
 */
/*
 * widget_table.cpp — Tableau FLTK (Fl_Table sous-classée)
 * sermo — haplo-dialog <devel@haplo-dialog.fr>
 * Licence : GPL-2.0-or-later
 *
 * <table> → HaploTable (sous-classe Fl_Table)
 * En-têtes : <label>, colonnes séparées par '|'
 * Format <item> et <input> (commande ou fichier, une ligne = une rangée) :
 *   colonnes séparées par '|' — règle de l'étalon gtk3sermo
 * Export : 1re colonne de la rangée sélectionnée
 *
 * Sécurité :
 *   - <input> lu par widget_input_lines() (fclose, jamais pclose)
 *   - Bornes : TABLE_ROWS_MAX=1024 rangées, TABLE_COLS_MAX=32 colonnes
 */

#ifndef _GNU_SOURCE
#define _GNU_SOURCE
#endif
#include "fltk-compat.h"
#include "gtkdialog.h"
#include "attributes.h"
#include "automaton.h"
#include "widgets.h"
#include "tag_attributes.h"
#include "safe_exec.h"
#include "widget_table.h"

#include <FL/Fl_Table.H>
#include <FL/fl_draw.H>

#include <string>
#include <vector>
#include <cstring>
#include <cstdlib>
#include <cstdio>

/* ── Limites ─────────────────────────────────────────────────────────────── */
static const int TABLE_ROWS_MAX  = 1024;
static const int TABLE_COLS_MAX  = 32;

/* ── Données de la table ─────────────────────────────────────────────────── */
struct TableData {
    std::vector<std::string>               headers;
    std::vector<std::vector<std::string>>  rows;
    int selected_row = -1;
};

/* ══════════════════════════════════════════════════════════════════════════ */
/* Sous-classe Fl_Table                                                       */
/* ══════════════════════════════════════════════════════════════════════════ */
class HaploTable : public Fl_Table {
public:
    TableData *td;

    HaploTable(int X, int Y, int W, int H)
        : Fl_Table(X, Y, W, H), td(nullptr)
    {
        end();   /* Fl_Table est un Fl_Group — fermer le groupe */
        selection_color(FL_SELECTION_COLOR);
        when(FL_WHEN_CHANGED | FL_WHEN_RELEASE);
    }

    ~HaploTable() override {
        delete td;
    }

    void draw_cell(TableContext ctx, int R, int C,
                   int X, int Y, int W, int H) override
    {
        if (!td) return;
        const char *text = "";
        switch (ctx) {
        case CONTEXT_STARTPAGE:
            fl_font(FL_HELVETICA, 14);
            return;
        case CONTEXT_COL_HEADER:
            if (C < (int)td->headers.size())
                text = td->headers[C].c_str();
            fl_push_clip(X, Y, W, H);
            fl_draw_box(FL_THIN_UP_BOX, X, Y, W, H, FL_BACKGROUND_COLOR);
            fl_color(FL_FOREGROUND_COLOR);
            fl_font(FL_HELVETICA_BOLD, 14);
            fl_draw(text, X + 2, Y, W - 4, H, FL_ALIGN_LEFT | FL_ALIGN_CLIP);
            fl_pop_clip();
            return;
        case CONTEXT_ROW_HEADER:
            return;
        case CONTEXT_CELL:
            if (R < (int)td->rows.size() &&
                C < (int)td->rows[R].size())
                text = td->rows[R][C].c_str();
            fl_push_clip(X, Y, W, H);
            /* Coloration alternée + sélection */
            if (R == td->selected_row)
                fl_draw_box(FL_FLAT_BOX, X, Y, W, H, FL_SELECTION_COLOR);
            else if (R % 2 == 0)
                fl_draw_box(FL_FLAT_BOX, X, Y, W, H, fl_rgb_color(240,240,255));
            else
                fl_draw_box(FL_FLAT_BOX, X, Y, W, H, FL_WHITE);
            fl_color(FL_FOREGROUND_COLOR);
            fl_font(FL_HELVETICA, 14);
            fl_draw(text, X + 2, Y, W - 4, H, FL_ALIGN_LEFT | FL_ALIGN_CLIP);
            fl_pop_clip();
            return;
        default:
            return;
        }
    }

    int handle(int ev) override {
        int ret = Fl_Table::handle(ev);
        if (ev == FL_PUSH || ev == FL_RELEASE) {
            int R, C;
            ResizeFlag rf;
            TableContext ctx = cursor2rowcol(R, C, rf);
            if (ctx == CONTEXT_CELL && td && R < (int)td->rows.size()) {
                td->selected_row = R;
                redraw();
            }
        }
        return ret;
    }
};

/* ── Helpers ─────────────────────────────────────────────────────────────── */
static std::vector<std::string> _split_pipe(const char *line)
{
    std::vector<std::string> cols;
    const char *p = line;
    while (*p) {
        const char *pipe = strchr(p, '|');
        if (pipe) {
            cols.emplace_back(p, (size_t)(pipe - p));
            p = pipe + 1;
        } else {
            cols.emplace_back(p);
            break;
        }
    }
    return cols;
}

/* Une ligne (<item> ou <input>) = une rangée, colonnes séparées par « | » */
static void _append_row(TableData *td, const char *line)
{
    if ((int)td->rows.size() >= TABLE_ROWS_MAX) return;
    auto cols = _split_pipe(line);
    if ((int)cols.size() > TABLE_COLS_MAX)
        cols.resize(TABLE_COLS_MAX);
    td->rows.push_back(cols);
}

/* Parité étalon : la première ligne est sélectionnée par défaut — après
 * <item> comme après <input> (TA="1" sur le banc, comme gtk3sermo). */
static void _select_first_row(TableData *td)
{
    if (td->selected_row < 0 && !td->rows.empty())
        td->selected_row = 0;
}

static void _rebuild(HaploTable *ht)
{
    TableData *td = ht->td;
    if (!td) return;
    int ncols = (int)td->headers.size();
    int nrows = (int)td->rows.size();
    ht->rows(nrows);
    ht->cols(ncols);
    ht->row_header(0);
    ht->col_header(1);
    ht->col_resize(1);
    int col_w = (ncols > 0) ? (ht->w() / ncols) : 100;
    if (col_w < 60) col_w = 60;
    for (int c = 0; c < ncols; ++c)
        ht->col_width(c, col_w);
    ht->row_height_all(22);
    ht->redraw();
}

/* ─────────────────────────────────────────────────────────────────────────── */

extern "C" GtkWidget *widget_table_create(AttributeSet *Attr, tag_attr *attr,
                                           gint /*Type*/)
{
    int ww = 400, hh = 200;
    if (attr) {
        const char *v;
        if ((v = get_tag_attribute(attr, "width-request")))  ww = atoi(v);
        if ((v = get_tag_attribute(attr, "height-request"))) hh = atoi(v);
    }

    HaploTable *ht = new HaploTable(0, 0, ww, hh);
    TableData  *td = new TableData();
    ht->td = td;

    if (Attr) {
        /* Sémantique étalon : <label>A|B</label> = EN-TÊTES ; chaque <item>
         * est une LIGNE. (L'ancienne version prenait le premier <item> pour
         * les en-têtes : la seule ligne du banc devenait l'en-tête et la
         * table restait vide.) */
        GList *element = NULL;
        gchar *hdr = attributeset_get_first(&element, Attr, ATTR_LABEL);
        if (hdr && *hdr) {
            td->headers = _split_pipe(hdr);
            if ((int)td->headers.size() > TABLE_COLS_MAX)
                td->headers.resize(TABLE_COLS_MAX);
        }
        element = NULL;
        gchar *row = attributeset_get_first(&element, Attr, ATTR_ITEM);
        while (row) {
            if (*row) _append_row(td, row);
            row = attributeset_get_next(&element, Attr, ATTR_ITEM);
        }
        /* <input> : dans refresh, que le cœur appelle juste après la création
         * (lu ici aussi, la commande tournait deux fois). */
    }

    _select_first_row(td);

    _rebuild(ht);
    return (GtkWidget *)ht;
}

extern "C" gchar *widget_table_envvar_construct(GtkWidget *widget)
{
    HaploTable *ht = static_cast<HaploTable *>(widget);
    if (!ht || !ht->td) return g_strdup("");
    int r = ht->td->selected_row;
    if (r < 0 || r >= (int)ht->td->rows.size()) return g_strdup("");
    /* Parité étalon (gtk_clist_get_text(..., 0) historique) : la variable
     * exporte la PREMIÈRE colonne de la ligne sélectionnée. */
    const auto &row = ht->td->rows[r];
    return g_strdup(row.empty() ? "" : row[0].c_str());
}

extern "C" gchar *widget_table_envvar_all_construct(variable *var)
{
    if (!var || !var->Widget) return NULL;
    return widget_table_envvar_construct(var->Widget);
}

extern "C" void widget_table_clear(variable *var)
{
    if (!var || !var->Widget) return;
    HaploTable *ht = static_cast<HaploTable *>(var->Widget);
    if (ht->td) {
        ht->td->rows.clear();
        ht->td->selected_row = -1;
    }
    _rebuild(ht);
}

extern "C" void widget_table_refresh(variable *var)
{
    if (!var || !var->Widget) return;
    HaploTable *ht = static_cast<HaploTable *>(var->Widget);

    /* <input> (commande ou fichier, décodé) : chaque ligne devient une rangée
     * AJOUTÉE — l'étalon ne vide pas la table au refresh — et l'en-tête reste
     * <label>. (Le port lisait du TSV à en-tête, et relançait au refresh la
     * directive brute « Command:… ».) */
    gchar **lignes = widget_input_lines(var->Attributes);
    if (lignes && ht->td) {
        for (gchar **l = lignes; *l; l++)
            _append_row(ht->td, *l);
        _select_first_row(ht->td);
    }
    g_strfreev(lignes);
    _rebuild(ht);
}

extern "C" void widget_table_fileselect(variable * /*var*/,
                                         const char * /*n*/,
                                         const char * /*v*/) {}

extern "C" void widget_table_removeselected(variable *var)
{
    if (!var || !var->Widget) return;
    HaploTable *ht = static_cast<HaploTable *>(var->Widget);
    if (!ht->td) return;
    int r = ht->td->selected_row;
    if (r >= 0 && r < (int)ht->td->rows.size()) {
        ht->td->rows.erase(ht->td->rows.begin() + r);
        ht->td->selected_row = -1;
        _rebuild(ht);
    }
}

extern "C" void widget_table_save(variable * /*var*/) {}
