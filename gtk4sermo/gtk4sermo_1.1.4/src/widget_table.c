/* SPDX-License-Identifier: GPL-2.0-or-later */
/*
 * widget_table.c — Tableau GTK4 via GtkColumnView + GListStore
 * gtk4sermo 1.0.0 — haplo-dialog <devel@haplo-dialog.fr>
 * Licence : GPL-2.0-or-later
 *
 * GtkCList n'a jamais existé en GTK3/4 (widget GTK2 uniquement).
 * L'API native GTK4 pour les tableaux multi-colonnes est :
 *   GListStore      — modèle de données (GObject)
 *   GtkColumnView   — widget d'affichage
 *   GtkColumnViewColumn — colonne avec factory de cellules
 *   GtkSignalListItemFactory — rendu des cellules
 *
 * Format d'entrée : TSV (colonnes séparées par TAB).
 * Première ligne = en-têtes de colonnes.
 *
 * Sécurité :
 *   - Lignes bornées à TABLE_LINE_MAX caractères
 *   - Lignes bornées à TABLE_ROWS_MAX
 *   - fclose() sur le pipe de commande (jamais pclose)
 */
#ifndef _GNU_SOURCE
#define _GNU_SOURCE
#endif
#include <gtk/gtk.h>
#include "config.h"
#include "gtkdialog.h"
#include "attributes.h"
#include "automaton.h"
#include "widgets.h"
#include "signals.h"
#include "safe_exec.h"
#include "stringman.h"
#include "tag_attributes.h"
#include <string.h>
#include <stdlib.h>
#include <stdio.h>

/* Wrapper GClosureNotify (evite -Wcast-function-type sur g_free) */
static void _gtkd_closure_g_free(gpointer data, GClosure *closure) {
	(void)closure;
	g_free(data);
}

#define TABLE_LINE_MAX  4096
#define TABLE_ROWS_MAX  2048
#define TABLE_COLS_MAX  32

/* ─── RowObject : GObject représentant une ligne ─────────────────────────── */
#define ROW_TYPE_OBJECT (row_object_get_type())
G_DECLARE_FINAL_TYPE(RowObject, row_object, ROW, OBJECT, GObject)

struct _RowObject {
    GObject parent_instance;
    gchar  *cells[TABLE_COLS_MAX];  /* Cellules de la ligne */
    int     n_cells;
};

G_DEFINE_TYPE(RowObject, row_object, G_TYPE_OBJECT)

static void row_object_finalize(GObject *obj)
{
    RowObject *self = ROW_OBJECT(obj);
    for (int i = 0; i < self->n_cells; i++)
        g_free(self->cells[i]);
    G_OBJECT_CLASS(row_object_parent_class)->finalize(obj);
}

static void row_object_class_init(RowObjectClass *klass)
{
    G_OBJECT_CLASS(klass)->finalize = row_object_finalize;
}

static void row_object_init(RowObject *self)
{
    self->n_cells = 0;
    for (int i = 0; i < TABLE_COLS_MAX; i++) self->cells[i] = NULL;
}

static RowObject *row_object_new(gchar **cells, int n)
{
    RowObject *obj = g_object_new(ROW_TYPE_OBJECT, NULL);
    obj->n_cells = (n < TABLE_COLS_MAX) ? n : TABLE_COLS_MAX;
    for (int i = 0; i < obj->n_cells; i++)
        obj->cells[i] = g_strdup(cells[i] ? cells[i] : "");
    return obj;
}

/* ─── Données du widget table ────────────────────────────────────────────── */
typedef struct {
    GtkWidget          *column_view;
    GListStore         *store;
    GtkSingleSelection *selection;
    int                 n_cols;
    gchar              *headers[TABLE_COLS_MAX];
} TableData;

/* ─── Factory de cellules — setup ──────────────────────────────────────────── */
static void _cell_setup(GtkSignalListItemFactory *factory,
                        GtkListItem *item, gpointer data)
{
    (void)factory; (void)data;
    GtkWidget *label = gtk_label_new(NULL);
    gtk_label_set_xalign(GTK_LABEL(label), 0.0f);
    gtk_list_item_set_child(item, label);
}

/* ─── Factory de cellules — bind ────────────────────────────────────────────── */
typedef struct { int col_idx; } ColBindData;

static void _cell_bind(GtkSignalListItemFactory *factory,
                       GtkListItem *item, gpointer data)
{
    (void)factory;
    ColBindData *cbd = (ColBindData *)data;
    RowObject   *row = ROW_OBJECT(gtk_list_item_get_item(item));
    GtkWidget   *lbl = gtk_list_item_get_child(item);
    if (!row || !lbl) return;
    int idx = cbd->col_idx;
    const gchar *text = (idx < row->n_cells && row->cells[idx])
                        ? row->cells[idx] : "";
    gtk_label_set_text(GTK_LABEL(lbl), text);
}

/* ─── Parse une ligne TSV → tableau de cellules ─────────────────────────── */
static int _parse_tsv(const char *line, gchar **cells, int max_cols, char delim)
{
    char buf[TABLE_LINE_MAX];
    size_t len = strlen(line);
    if (len >= TABLE_LINE_MAX) len = TABLE_LINE_MAX - 1;
    memcpy(buf, line, len); buf[len] = '\0';
    while (len > 0 && (buf[len-1]=='\n'||buf[len-1]=='\r')) buf[--len] = '\0';
    if (len == 0) return 0;
    int col = 0;
    char *tok = buf, *end;
    while (col < max_cols) {
        end = strchr(tok, delim);
        if (end) *end = '\0';
        cells[col++] = g_strdup(tok);
        if (!end) break;
        tok = end + 1;
    }
    return col;
}

/* ─── Construit les colonnes GtkColumnView depuis les en-têtes ────────────── */
static void _build_columns(GtkColumnView *cv, TableData *td)
{
    for (int c = 0; c < td->n_cols; c++) {
        ColBindData *cbd = g_new0(ColBindData, 1);
        cbd->col_idx = c;

        GtkListItemFactory *factory = gtk_signal_list_item_factory_new();
        g_signal_connect(factory, "setup",  G_CALLBACK(_cell_setup), NULL);
        g_signal_connect_data(factory, "bind",
                              G_CALLBACK(_cell_bind), cbd,
                              _gtkd_closure_g_free, 0);

        const gchar *hdr = (td->headers[c] && td->headers[c][0])
                           ? td->headers[c] : "";
        GtkColumnViewColumn *col =
            gtk_column_view_column_new(hdr, factory);
        gtk_column_view_column_set_resizable(col, TRUE);
        gtk_column_view_column_set_expand(col, TRUE);
        gtk_column_view_append_column(cv, col);
        g_object_unref(col);
    }
}

GtkWidget *widget_table_create(AttributeSet *Attr, tag_attr *attr, gint Type)
{
    TableData *td = g_new0(TableData, 1);
    td->store  = g_list_store_new(ROW_TYPE_OBJECT);
    td->n_cols = 0;

    gchar *header_cells[TABLE_COLS_MAX] = {NULL};
    int    row_count = 0;
    gboolean has_header = FALSE;

    /* En-tetes depuis <label>col1|col2</label> (parite gtk3, PAS le 1er
     * <item>). Si present, TOUS les <item> deviennent des lignes. */
    if (Attr) {
        GList *ell = NULL;
        gchar *lbl = attributeset_get_first(&ell, Attr, ATTR_LABEL);
        if (lbl && *lbl) {
            gchar *hc[TABLE_COLS_MAX] = {NULL};
            int hn = _parse_tsv(lbl, hc, TABLE_COLS_MAX, '|');
            if (hn > 0) {
                td->n_cols = hn;
                for (int i = 0; i < hn; i++) td->headers[i] = g_strdup(hc[i] ? hc[i] : "");
                for (int i = 0; i < hn; i++) g_free(hc[i]);
                has_header = TRUE;
            }
        }
    }

    /* ── Lire les items statiques XML ──────────────────────────────── */
    if (Attr) {
        GList *el = NULL;
        gchar *item = attributeset_get_first(&el, Attr, ATTR_ITEM);
        while (item && row_count < TABLE_ROWS_MAX) {
            if (!*item) { item = attributeset_get_next(&el, Attr, ATTR_ITEM); continue; }
            gchar *cells[TABLE_COLS_MAX] = {NULL};
            int n = _parse_tsv(item, cells, TABLE_COLS_MAX, '|');
            if (n > 0) {
                if (!has_header) {
                    /* Première ligne = en-têtes */
                    td->n_cols = n;
                    for (int i = 0; i < n; i++)
                        td->headers[i] = g_strdup(cells[i] ? cells[i] : "");
                    for (int i = 0; i < n; i++) g_free(cells[i]);
                    has_header = TRUE;
                } else {
                    RowObject *ro = row_object_new(cells, n);
                    g_list_store_append(td->store, ro);
                    g_object_unref(ro);
                    for (int i = 0; i < n; i++) g_free(cells[i]);
                    row_count++;
                }
            }
            item = attributeset_get_next(&el, Attr, ATTR_ITEM);
        }
    }

    /* ── Lire depuis commande (TSV) ────────────────────────────────── */
    if (Attr) {
        GList *el = NULL;
        gchar *cmd = attributeset_get_first(&el, Attr, ATTR_INPUT);
        if (cmd && *cmd) {
            FILE *fp = widget_opencommand(cmd);
            if (fp) {
                char line[TABLE_LINE_MAX];
                gboolean first = !has_header;
                while (fgets(line, sizeof(line), fp) && row_count < TABLE_ROWS_MAX) {
                    gchar *cells[TABLE_COLS_MAX] = {NULL};
                    int n = _parse_tsv(line, cells, TABLE_COLS_MAX, '\t');
                    if (n <= 0) continue;
                    if (first) {
                        td->n_cols = n;
                        for (int i = 0; i < n; i++)
                            td->headers[i] = g_strdup(cells[i] ? cells[i] : "");
                        for (int i = 0; i < n; i++) g_free(cells[i]);
                        first = FALSE;
                    } else {
                        RowObject *ro = row_object_new(cells, n);
                        g_list_store_append(td->store, ro);
                        g_object_unref(ro);
                        for (int i = 0; i < n; i++) g_free(cells[i]);
                        row_count++;
                    }
                }
                /* CRITIQUE : fclose, jamais pclose */
                fclose(fp);
            }
        }
    }

    /* Si aucune colonne détectée, créer une colonne vide */
    if (td->n_cols == 0) { td->n_cols = 1; td->headers[0] = g_strdup(""); }

    /* ── Construire le GtkColumnView ──────────────────────────────── */
    td->selection  = gtk_single_selection_new(G_LIST_MODEL(td->store));
    td->column_view = gtk_column_view_new(GTK_SELECTION_MODEL(td->selection));
    gtk_column_view_set_show_row_separators(GTK_COLUMN_VIEW(td->column_view), TRUE);
    gtk_column_view_set_show_column_separators(GTK_COLUMN_VIEW(td->column_view), TRUE);

    _build_columns(GTK_COLUMN_VIEW(td->column_view), td);

    g_object_set_data(G_OBJECT(td->column_view), "table_data", td);

    /* Scroller */
    GtkWidget *sw = gtk_scrolled_window_new();
    gtk_scrolled_window_set_policy(GTK_SCROLLED_WINDOW(sw),
                                   GTK_POLICY_AUTOMATIC, GTK_POLICY_AUTOMATIC);
    gtk_scrolled_window_set_child(GTK_SCROLLED_WINDOW(sw), td->column_view);
    gtk_widget_set_hexpand(sw, TRUE);
    gtk_widget_set_vexpand(sw, TRUE);
    gtk_widget_set_visible(sw, TRUE);

    g_object_set_data(G_OBJECT(sw), "table_data", td);
    return sw;
}

/* ─── Export variable ────────────────────────────────────────────────────── */
gchar *widget_table_envvar_construct(GtkWidget *widget)
{
    if (!widget) return g_strdup("");
    TableData *td = g_object_get_data(G_OBJECT(widget), "table_data");
    if (!td || !td->selection) return g_strdup("");

    GObject *sel = gtk_single_selection_get_selected_item(td->selection);
    if (!sel) return g_strdup("");
    RowObject *row = ROW_OBJECT(sel);
    if (!row || row->n_cells == 0) return g_strdup("");
    /* Retourne la première colonne de la ligne sélectionnée */
    return g_strdup(row->cells[0] ? row->cells[0] : "");
}

gchar *widget_table_envvar_all_construct(variable *var)
{
    if (!var || !var->Widget) return NULL;
    return widget_table_envvar_construct(var->Widget);
}

void widget_table_clear(variable *var)
{
    if (!var || !var->Widget) return;
    TableData *td = g_object_get_data(G_OBJECT(var->Widget), "table_data");
    if (td && td->store) g_list_store_remove_all(td->store);
}

void widget_table_refresh(variable *var) {}
void widget_table_fileselect(variable *var, const gchar *n, const gchar *v) {}
void widget_table_removeselected(variable *var)
{
    if (!var || !var->Widget) return;
    TableData *td = g_object_get_data(G_OBJECT(var->Widget), "table_data");
    if (!td) return;
    guint pos = gtk_single_selection_get_selected(td->selection);
    if (pos != GTK_INVALID_LIST_POSITION)
        g_list_store_remove(td->store, pos);
}
void widget_table_save(variable *var) {}

/* Stubs de fonctions de tri (interface conservée pour compatibilité) */
gboolean widget_table_click_column_callback(GtkWidget *w, gint col, variable *var)
{ (void)w; (void)col; (void)var; return FALSE; }
