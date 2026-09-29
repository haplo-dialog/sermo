/* SPDX-License-Identifier: GPL-2.0-or-later */
/*
 * Copyright (C) 2026 S. Cage
 * haplo-dialog <devel@haplo-dialog.fr>
 */
/* widget_treetable.cpp — Arbre hiérarchique multi-colonnes FLTK
 * sermo — haplo-dialog — GPL-2.0-or-later
 *
 * <treetable> : arbre avec colonnes, expand/collapse, indentation visuelle.
 *
 * Format d'entrée (attributs <item>) :
 *   "Parent\tCol2\tCol3"        → nœud de premier niveau
 *   "Parent/Enfant\tCol2\tCol3" → nœud imbriqué
 *
 * Format <column> : noms des en-têtes, séparés par '|'
 *   ex: <column>Nom|Taille|Date</column>
 *
 * Export :
 *   Chemin complet du nœud sélectionné (e.g. "Parent/Enfant")
 *
 * Sécurité :
 *   fclose() sur tout FILE* issu de widget_opencommand().
 *
 * Limites :
 *   TREETABLE_COLS_MAX=16, TREETABLE_ROWS_MAX=2048, profondeur max=32
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
#include "widget_treetable.h"

#include <FL/Fl.H>
#include <FL/Fl_Table.H>
#include <FL/fl_draw.H>
#include <FL/Fl_Pixmap.H>

#include <string.h>
#include <stdlib.h>
#include <stdio.h>
#include <vector>
#include <string>
#include <algorithm>

/* ─── Constantes ─────────────────────────────────────────────────────────── */
static const int TREETABLE_COLS_MAX  = 16;
static const int TREETABLE_ROWS_MAX  = 2048;
static const int TREETABLE_COL_W     = 160;
static const int TREETABLE_ROW_H     = 20;
static const int TREETABLE_INDENT_PX = 16;   /* px par niveau d'imbrication */

/* ─── Couleurs Catppuccin Mocha ──────────────────────────────────────────── */
static const Fl_Color CLR_BASE    = fl_rgb_color(0x1e,0x1e,0x2e);
static const Fl_Color CLR_TEXT    = fl_rgb_color(0xcd,0xd6,0xf4);
static const Fl_Color CLR_SURFACE = fl_rgb_color(0x31,0x32,0x44);
static const Fl_Color CLR_HEADER  = fl_rgb_color(0x45,0x47,0x5a);
static const Fl_Color CLR_SEL     = fl_rgb_color(0x58,0x5b,0x70);
static const Fl_Color CLR_BLUE    = fl_rgb_color(0x89,0xb4,0xfa);

/* ─── Structure d'un nœud ────────────────────────────────────────────────── */
struct TreeNode {
    std::string          path;       /* "Parent/Enfant/Feuille" */
    std::string          label;      /* dernier segment du path */
    int                  depth;      /* 0 = racine */
    bool                 expanded;
    bool                 visible;
    std::vector<std::string> cols;   /* colonnes 1..N (col 0 = label) */
    int                  parent_idx; /* index dans le vecteur global, -1=racine */
    std::vector<int>     children;
};

/* ─── Classe HaploTreeTable ──────────────────────────────────────────────── */
class HaploTreeTable : public Fl_Table {
public:
    std::vector<TreeNode>   nodes;        /* tous les nœuds (plats) */
    std::vector<int>        visible_rows; /* indices visibles (après expand/collapse) */
    std::vector<std::string> headers;
    int                     selected_visible; /* index dans visible_rows, -1=aucun */

    HaploTreeTable(int X, int Y, int W, int H)
        : Fl_Table(X, Y, W, H), selected_visible(-1)
    {
        color(CLR_BASE);
        selection_color(CLR_SEL);
        col_header(1);
        col_resize(1);
        row_header(0);
        row_height_all(TREETABLE_ROW_H);
        col_header_height(TREETABLE_ROW_H + 2);
        end();
    }

    /* Recalculer les lignes visibles */
    void rebuild_visible()
    {
        visible_rows.clear();
        for (int i = 0; i < (int)nodes.size(); i++) {
            if (nodes[i].visible)
                visible_rows.push_back(i);
        }
        rows((int)visible_rows.size());
        redraw();
    }

    /* Ajouter un nœud depuis une ligne "Path\tCol1\tCol2..." */
    void add_node_line(const char *line)
    {
        if (!line || !*line) return;

        TreeNode nd;
        nd.expanded    = true;
        nd.visible     = true;
        nd.parent_idx  = -1;

        /* Séparer le chemin (avant premier \t) des colonnes */
        const char *tab = strchr(line, '\t');
        std::string path_part = tab ? std::string(line, tab - line) : std::string(line);
        nd.path = path_part;

        /* Calculer depth et label à partir du chemin */
        nd.depth = (int)std::count(nd.path.begin(), nd.path.end(), '/');
        size_t slash = nd.path.rfind('/');
        nd.label = (slash == std::string::npos) ? nd.path : nd.path.substr(slash + 1);

        /* Colonnes supplémentaires */
        if (tab) {
            const char *p = tab + 1;
            while (p && *p) {
                const char *next = strchr(p, '\t');
                nd.cols.push_back(next ? std::string(p, next - p) : std::string(p));
                p = next ? next + 1 : nullptr;
            }
        }

        /* Trouver le parent */
        if (nd.depth > 0) {
            size_t sl = nd.path.rfind('/');
            std::string parent_path = (sl != std::string::npos) ? nd.path.substr(0, sl) : "";
            for (int i = (int)nodes.size() - 1; i >= 0; i--) {
                if (nodes[i].path == parent_path) {
                    nd.parent_idx = i;
                    nodes[i].children.push_back((int)nodes.size());
                    /* Si le parent est réduit, ce nœud est invisible */
                    if (!nodes[i].expanded) nd.visible = false;
                    break;
                }
            }
        }

        if ((int)nodes.size() < TREETABLE_ROWS_MAX)
            nodes.push_back(nd);
    }

    /* Toggle expand/collapse sur un nœud (par index global) */
    void toggle_node(int node_idx)
    {
        if (node_idx < 0 || node_idx >= (int)nodes.size()) return;
        TreeNode &nd = nodes[node_idx];
        if (nd.children.empty()) return;  /* feuille, rien à faire */
        nd.expanded = !nd.expanded;
        _set_children_visible(node_idx, nd.expanded);
        rebuild_visible();
    }

    void _set_children_visible(int parent_idx, bool vis)
    {
        for (int ci : nodes[parent_idx].children) {
            nodes[ci].visible = vis;
            if (!vis || nodes[ci].expanded)
                _set_children_visible(ci, vis && nodes[ci].expanded);
        }
    }

    /* Chemin du nœud sélectionné */
    std::string selected_path() const
    {
        if (selected_visible < 0 || selected_visible >= (int)visible_rows.size())
            return "";
        return nodes[visible_rows[selected_visible]].path;
    }

protected:
    void draw_cell(TableContext ctx, int R, int C, int X, int Y, int W, int H) override
    {
        switch (ctx) {
        case CONTEXT_COL_HEADER:
            fl_push_clip(X, Y, W, H);
            fl_draw_box(FL_FLAT_BOX, X, Y, W, H, CLR_HEADER);
            fl_color(CLR_TEXT);
            fl_font(FL_HELVETICA_BOLD, 11);
            {
                std::string hdr = (C < (int)headers.size()) ? headers[C] : "";
                fl_draw(hdr.c_str(), X + 4, Y, W - 4, H, FL_ALIGN_LEFT | FL_ALIGN_CLIP);
            }
            fl_color(CLR_SURFACE);
            fl_line(X + W - 1, Y, X + W - 1, Y + H);
            fl_pop_clip();
            return;

        case CONTEXT_CELL: {
            if (R < 0 || R >= (int)visible_rows.size()) return;
            int ni = visible_rows[R];
            const TreeNode &nd = nodes[ni];

            fl_push_clip(X, Y, W, H);
            Fl_Color bg = (R == selected_visible) ? CLR_SEL
                        : (R % 2 == 0)            ? CLR_BASE
                                                  : CLR_SURFACE;
            fl_draw_box(FL_FLAT_BOX, X, Y, W, H, bg);

            if (C == 0) {
                /* Colonne 0 : indentation + triangle expand/collapse + label */
                int indent = nd.depth * TREETABLE_INDENT_PX + 4;

                /* Triangle expand/collapse si enfants */
                if (!nd.children.empty()) {
                    int tx = X + indent;
                    int ty = Y + H / 2;
                    fl_color(CLR_BLUE);
                    if (nd.expanded) {
                        /* ▼ */
                        fl_polygon(tx, ty - 4, tx + 8, ty - 4, tx + 4, ty + 4);
                    } else {
                        /* ▶ */
                        fl_polygon(tx, ty - 5, tx + 8, ty, tx, ty + 5);
                    }
                    indent += 12;
                }

                fl_color(CLR_TEXT);
                fl_font(FL_HELVETICA, 11);
                fl_draw(nd.label.c_str(), X + indent, Y, W - indent, H,
                        FL_ALIGN_LEFT | FL_ALIGN_CLIP);
            } else {
                /* Colonnes supplémentaires */
                int ci = C - 1;
                std::string val = (ci < (int)nd.cols.size()) ? nd.cols[ci] : "";
                fl_color(CLR_TEXT);
                fl_font(FL_HELVETICA, 11);
                fl_draw(val.c_str(), X + 4, Y, W - 4, H, FL_ALIGN_LEFT | FL_ALIGN_CLIP);
            }

            /* Séparateur de colonne */
            fl_color(CLR_SURFACE);
            fl_line(X + W - 1, Y, X + W - 1, Y + H);
            fl_pop_clip();
            return;
        }

        default:
            return;
        }
    }

    int handle(int event) override
    {
        int ret = Fl_Table::handle(event);

        if (event == FL_PUSH) {
            int R, C;
            ResizeFlag resized;
            TableContext ctx = cursor2rowcol(R, C, resized);
            if (ctx == CONTEXT_CELL && R >= 0 && R < (int)visible_rows.size()) {
                /* Sélection */
                selected_visible = R;
                int ni = visible_rows[R];

                /* Vérifier si clic sur le triangle (col 0) */
                if (C == 0 && !nodes[ni].children.empty()) {
                    /* Calculer la zone du triangle */
                    int indent = nodes[ni].depth * TREETABLE_INDENT_PX + 4;
                    int cx = 0, cy = 0, cw = 0, ch = 0;
                    find_cell(ctx, R, C, cx, cy, cw, ch);
                    int mx = Fl::event_x();
                    if (mx >= cx + indent && mx <= cx + indent + 12)
                        toggle_node(ni);
                    else
                        redraw();
                } else {
                    redraw();
                }
                return 1;
            }
        }
        return ret;
    }
};

/* ─── API widget_treetable ───────────────────────────────────────────────── */

GtkWidget *widget_treetable_create(AttributeSet *Attr, tag_attr *attr, gint Type)
{
    int ww = 300, hh = 250;
    if (attr) {
        const char *v;
        if ((v = get_tag_attribute(attr, "width-request")))  ww = atoi(v);
        if ((v = get_tag_attribute(attr, "height-request"))) hh = atoi(v);
    }

    HaploTreeTable *tt = new HaploTreeTable(0, 0, ww, hh);

    /* En-têtes de colonnes */
    int ncols = 1;
    if (Attr) {
        GList *el = NULL;
        gchar *colspec = attributeset_get_first(&el, Attr, ATTR_LABEL);
        if (colspec && *colspec) {
            char buf[512];
            g_strlcpy(buf, colspec, sizeof(buf));
            char *tok = strtok(buf, "|");
            while (tok && ncols <= TREETABLE_COLS_MAX) {
                tt->headers.push_back(tok);
                tok = strtok(NULL, "|");
                ncols++;
            }
        }
    }
    if (tt->headers.empty()) tt->headers.push_back("Élément");
    ncols = (int)tt->headers.size();

    /* Largeurs des colonnes */
    tt->cols(ncols);
    for (int c = 0; c < ncols; c++)
        tt->col_width(c, (c == 0) ? TREETABLE_COL_W * 2 : TREETABLE_COL_W);

    /* Ajouter les nœuds depuis ATTR_ITEM */
    if (Attr) {
        GList *el = NULL;
        gchar *item = attributeset_get_first(&el, Attr, ATTR_ITEM);
        while (item) {
            if (*item) tt->add_node_line(item);
            item = attributeset_get_next(&el, Attr, ATTR_ITEM);
        }

        /* Commande externe */
        el = NULL;
        gchar *cmd = attributeset_get_first(&el, Attr, ATTR_INPUT);
        if (cmd && *cmd) {
            FILE *fp = widget_opencommand(cmd);
            if (fp) {
                char line[2048];
                while (fgets(line, sizeof(line), fp)) {
                    size_t len = strlen(line);
                    while (len > 0 && (line[len-1]=='\n'||line[len-1]=='\r')) line[--len]='\0';
                    if (len > 0) tt->add_node_line(line);
                }
                fclose(fp);  /* safe_popen() → fdopen() → fclose() obligatoire */
            }
        }
    }

    tt->rebuild_visible();
    return (GtkWidget *)tt;
}

gchar *widget_treetable_envvar_construct(GtkWidget *w)
{
    if (!w) return g_strdup("");
    HaploTreeTable *tt = (HaploTreeTable *)w;
    return g_strdup(tt->selected_path().c_str());
}

gchar *widget_treetable_envvar_all_construct(variable *v)
{
    return v && v->Widget ? widget_treetable_envvar_construct(v->Widget) : NULL;
}

void widget_treetable_clear(variable *v)
{
    if (!v || !v->Widget) return;
    HaploTreeTable *tt = (HaploTreeTable *)v->Widget;
    tt->nodes.clear();
    tt->visible_rows.clear();
    tt->headers.clear();
    tt->selected_visible = -1;
    tt->rows(0);
    tt->redraw();
}

void widget_treetable_refresh(variable *v)
{
    if (v && v->Widget) ((HaploTreeTable *)v->Widget)->redraw();
}

void widget_treetable_fileselect(variable *v, const char *n, const char *val) {
    (void)v; (void)n; (void)val;
}

void widget_treetable_removeselected(variable *v)
{
    if (!v || !v->Widget) return;
    HaploTreeTable *tt = (HaploTreeTable *)v->Widget;
    if (tt->selected_visible < 0 || tt->selected_visible >= (int)tt->visible_rows.size()) return;
    int ni = tt->visible_rows[tt->selected_visible];
    /* Supprimer récursivement les enfants puis le nœud */
    tt->nodes.erase(tt->nodes.begin() + ni);
    tt->selected_visible = -1;
    tt->rebuild_visible();
}

void widget_treetable_save(variable *v) { (void)v; }
