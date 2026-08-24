/* SPDX-License-Identifier: GPL-2.0-or-later */
/*
 * widget_table.c:
 * Gtkdialog - A small utility for fast and easy GUI building.
 * Copyright (C) 2003-2007  László Pere <pipas@linux.pte.hu>
 * Copyright (C) 2011-2012  Thunor <thunorsif@hotmail.com>
 * Copyright (C) 2026  Haplo-Linux <devel@haplo-dialog.fr> (GTK3 port, security)
 *
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation; either version 2 of the License, or
 * (at your option) any later version.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License along
 * with this program; if not, write to the Free Software Foundation, Inc.,
 * 51 Franklin Street, Fifth Floor, Boston, MA 02110-1301 USA.
 *
 * -------------------------------------------------------------------------
 * GTK3 migration note (haplo1):
 *
 * GtkCList was removed from GTK3.  This file replaces every GtkCList call
 * with GtkTreeView + GtkListStore.  The public API declared in
 * widget_table.h is unchanged.
 *
 * Design:
 *  - widget_table_create() builds a GtkListStore (N × G_TYPE_STRING columns)
 *    and wraps it in a GtkTreeView.  The GtkTreeView pointer is what the
 *    rest of the code treats as "var->Widget".
 *  - Internal state is stored as GObject data on the GtkTreeView:
 *      "_n_columns"   GINT_TO_POINTER(n)    — number of data columns
 *      "_sort_func"   GINT_TO_POINTER(0/1/2)— 0=default, 1=natcmp, 2=natcasecmp
 *      "_auto_sort"   GINT_TO_POINTER(bool) — re-sort after each append
 *      "_last-column" GINT_TO_POINTER(col+1)— for click-toggle logic (unused
 *                                             when using set_sort_column_id)
 *      "_initialised" GINT_TO_POINTER(bool) — signal connection guard
 *  - Column-header clicks and ascending/descending toggle are handled by
 *    GTK via gtk_tree_view_column_set_sort_column_id().  No manual callback
 *    is needed; the former widget_table_click_column_callback() is removed.
 *  - Natural-sort is implemented as a GtkTreeIterCompareFunc registered on
 *    the GtkListStore via gtk_tree_sortable_set_sort_func() for each column.
 *  - Callers MUST use fclose(), not pclose(), on FILE* returned by
 *    widget_opencommand() (see safe_exec.c design note 2).
 *
 * gtk3sermo-1.0.0 — Haplo-Linux (devel@haplo-dialog.fr), 2026
 */

/* Includes */
#ifndef _GNU_SOURCE
#define _GNU_SOURCE
#endif
#include <gtk/gtk.h>
#include "config.h"
#include "gtk3d.h"
#include "attributes.h"
#include "automaton.h"
#include "widgets.h"
#include "signals.h"
#include "stringman.h"
#include "tag_attributes.h"

/* Defines */
//#define DEBUG_CONTENT
//#define DEBUG_TRANSITS

/* Local function prototypes */
static void widget_table_input_by_command(variable *var, char *filename,
	gint command_or_file);
static void widget_table_input_by_file(variable *var, char *filename);
static void widget_table_input_by_items(variable *var);

/* -----------------------------------------------------------------------
 * Internal helpers
 * ----------------------------------------------------------------------- */

/*
 * _get_store: retrieve the GtkListStore backing the GtkTreeView widget.
 */
static GtkListStore *_get_store(GtkWidget *widget)
{
	GtkTreeModel *model = gtk_tree_view_get_model(GTK_TREE_VIEW(widget));
	g_return_val_if_fail(model != NULL, NULL);
	return GTK_LIST_STORE(model);
}

/*
 * _get_ncols: retrieve the column count stored at widget creation time.
 */
static gint _get_ncols(GtkWidget *widget)
{
	return GPOINTER_TO_INT(g_object_get_data(G_OBJECT(widget), "_n_columns"));
}

/*
 * _natcmp_func: GtkTreeIterCompareFunc for natural-order sort.
 * user_data = GINT_TO_POINTER(TRUE)  → case-sensitive   (sort-function=1)
 * user_data = GINT_TO_POINTER(FALSE) → case-insensitive (sort-function=2)
 *
 * The active sort column is retrieved from the sortable model so that the
 * same function can be registered for every column.
 */
static gint _natcmp_func(GtkTreeModel *model, GtkTreeIter *a, GtkTreeIter *b,
	gpointer user_data)
{
	gint        sensitive = GPOINTER_TO_INT(user_data);
	gint        sort_col  = 0;
	GtkSortType order;
	gchar      *s1        = NULL;
	gchar      *s2        = NULL;
	gint        result;

	gtk_tree_sortable_get_sort_column_id(GTK_TREE_SORTABLE(model),
		&sort_col, &order);
	if (sort_col < 0)
		sort_col = 0;

	gtk_tree_model_get(model, a, sort_col, &s1, -1);
	gtk_tree_model_get(model, b, sort_col, &s2, -1);

	result = strnatcmp(s1 ? s1 : "", s2 ? s2 : "", sensitive);

	g_free(s1);
	g_free(s2);
	return result;
}

/*
 * _append_row: append a pipe-split list_t row to the GtkListStore.
 * Columns beyond sliced->n_lines are filled with an empty string.
 */
static void _append_row(GtkWidget *widget, list_t *sliced)
{
	GtkListStore *store  = _get_store(widget);
	GtkTreeIter   iter;
	gint          ncols  = _get_ncols(widget);
	gint          c;

	if (!store)
		return;

	gtk_list_store_append(store, &iter);
	for (c = 0; c < ncols; c++) {
		const gchar *cell = (sliced && c < sliced->n_lines && sliced->line[c])
		                    ? sliced->line[c] : "";
		gtk_list_store_set(store, &iter, c, cell, -1);
	}

	/* Re-sort immediately if auto-sort is active */
	if (GPOINTER_TO_INT(g_object_get_data(G_OBJECT(widget), "_auto_sort"))) {
		gint        sc = 0;
		GtkSortType st = GTK_SORT_ASCENDING;
		gtk_tree_sortable_get_sort_column_id(GTK_TREE_SORTABLE(store), &sc, &st);
		gtk_tree_sortable_set_sort_column_id(GTK_TREE_SORTABLE(store), sc, st);
	}
}

/* -----------------------------------------------------------------------
 * Clear
 * ----------------------------------------------------------------------- */

void widget_table_clear(variable *var)
{
	GtkListStore *store;

#ifdef DEBUG_TRANSITS
	fprintf(stderr, "%s(): Entering.\n", __func__);
#endif

	store = _get_store(var->Widget);
	if (store)
		gtk_list_store_clear(store);

#ifdef DEBUG_TRANSITS
	fprintf(stderr, "%s(): Exiting.\n", __func__);
#endif
}

/* -----------------------------------------------------------------------
 * Create
 * ----------------------------------------------------------------------- */

GtkWidget *widget_table_create(
	AttributeSet *Attr, tag_attr *attr, gint Type)
{
	GList             *element;
	GtkWidget         *widget;
	GtkListStore      *store;
	GtkCellRenderer   *renderer;
	GtkTreeViewColumn *col;
	GtkTreeSelection  *selection;
	gchar             *value;
	GType             *types;
	list_t            *sliced       = NULL;
	list_t            *hdr_active   = NULL;
	list_t            *col_visible  = NULL;
	gint               n_columns    = 1;
	gint               sort_function = 0;  /* 0=GTK default, 1=natcmp, 2=natcasecmp */
	gint               sort_type    = GTK_SORT_ASCENDING;
	gint               sort_column  = 0;
	gboolean           auto_sort    = FALSE;
	gint               c;

#ifdef DEBUG_TRANSITS
	fprintf(stderr, "%s(): Entering.\n", __func__);
#endif

	/* ---- Column headers ---- */
	if (attributeset_is_avail(Attr, ATTR_LABEL)) {
		sliced = linecutter(g_strdup(attributeset_get_first(
			&element, Attr, ATTR_LABEL)), '|');
		n_columns = sliced->n_lines;
	}

	/* ---- Build GtkListStore ---- */
	types = g_new(GType, n_columns);
	for (c = 0; c < n_columns; c++)
		types[c] = G_TYPE_STRING;
	store = gtk_list_store_newv(n_columns, types);
	g_free(types);

	/* ---- Create the GtkTreeView ---- */
	widget = gtk_tree_view_new_with_model(GTK_TREE_MODEL(store));
	g_object_unref(store);   /* GtkTreeView holds its own reference */

	g_object_set_data(G_OBJECT(widget), "_n_columns",
		GINT_TO_POINTER(n_columns));

	/* ---- Parse tag attributes ---- */
	if (attr) {
		if ((value = get_tag_attribute(attr, "sort-function")))
			sort_function = atoi(value);
		if ((value = get_tag_attribute(attr, "sort-type")))
			sort_type = atoi(value);
		if ((value = get_tag_attribute(attr, "sort-column")))
			sort_column = atoi(value);
		if ((value = get_tag_attribute(attr, "auto-sort")) &&
			((strcasecmp(value, "true") == 0) ||
			 (strcasecmp(value, "yes")  == 0) ||
			 (atoi(value) == 1)))
			auto_sort = TRUE;
		if ((value = get_tag_attribute(attr, "column-header-active")))
			hdr_active = linecutter(g_strdup(value), '|');
		if ((value = get_tag_attribute(attr, "column-visible")))
			col_visible = linecutter(g_strdup(value), '|');
		if ((value = get_tag_attribute(attr, "selection-mode"))) {
			selection = gtk_tree_view_get_selection(GTK_TREE_VIEW(widget));
			gtk_tree_selection_set_mode(selection, (GtkSelectionMode)atoi(value));
		}
	}

	g_object_set_data(G_OBJECT(widget), "_auto_sort",
		GINT_TO_POINTER((gint)auto_sort));
	g_object_set_data(G_OBJECT(widget), "_sort_func",
		GINT_TO_POINTER(sort_function));

	/* ---- Register natural-sort compare function for every column ---- */
	if (sort_function == 1 || sort_function == 2) {
		gint sensitive = (sort_function == 1) ? TRUE : FALSE;
		for (c = 0; c < n_columns; c++) {
			gtk_tree_sortable_set_sort_func(GTK_TREE_SORTABLE(store), c,
				_natcmp_func, GINT_TO_POINTER(sensitive), NULL);
		}
	}

	/* ---- Add one GtkTreeViewColumn per header ---- */
	for (c = 0; c < n_columns; c++) {
		const gchar *title = (sliced && c < sliced->n_lines)
		                     ? sliced->line[c] : "";
		renderer = gtk_cell_renderer_text_new();
		col = gtk_tree_view_column_new_with_attributes(
			title, renderer, "text", c, NULL);
		gtk_tree_view_column_set_resizable(col, TRUE);
		/* Enables GTK's built-in header-click → sort-direction toggle */
		gtk_tree_view_column_set_sort_column_id(col, c);

		/* column-header-active */
		if (hdr_active && c < hdr_active->n_lines) {
			gboolean active =
				(strcasecmp(hdr_active->line[c], "true") == 0 ||
				 strcasecmp(hdr_active->line[c], "yes")  == 0 ||
				 atoi(hdr_active->line[c]) == 1);
			gtk_tree_view_column_set_clickable(col, active);
		} else {
			gtk_tree_view_column_set_clickable(col, TRUE);
		}

		/* column-visible */
		if (col_visible && c < col_visible->n_lines) {
			gboolean visible =
				(strcasecmp(col_visible->line[c], "true") == 0 ||
				 strcasecmp(col_visible->line[c], "yes")  == 0 ||
				 atoi(col_visible->line[c]) == 1);
			gtk_tree_view_column_set_visible(col, visible);
		}

		gtk_tree_view_append_column(GTK_TREE_VIEW(widget), col);
	}

	if (sliced)      list_t_free(sliced);
	if (hdr_active)  list_t_free(hdr_active);
	if (col_visible) list_t_free(col_visible);

	/* ---- Apply initial sort column / direction ---- */
	gtk_tree_sortable_set_sort_column_id(GTK_TREE_SORTABLE(store),
		sort_column, sort_type);

#ifdef DEBUG_TRANSITS
	fprintf(stderr, "%s(): Exiting.\n", __func__);
#endif

	return widget;
}

/* -----------------------------------------------------------------------
 * Environment Variable All Construct
 * ----------------------------------------------------------------------- */

gchar *widget_table_envvar_all_construct(variable *var)
{
	GtkTreeModel *model;
	GtkTreeIter   iter;
	gchar        *value;
	gchar        *cell;
	gchar        *line;
	gchar        *text;
	gint          column  = 0;
	gboolean      first   = TRUE;
	gboolean      valid;

#ifdef DEBUG_TRANSITS
	fprintf(stderr, "%s(): Entering.\n", __func__);
#endif

	if (var->widget_tag_attr) {
		if ((value = get_tag_attribute(var->widget_tag_attr, "exported-column")))
			column = atoi(value);
	}

	model = gtk_tree_view_get_model(GTK_TREE_VIEW(var->Widget));
	line  = g_strdup_printf("%s_ALL=\"", var->Name);

	valid = gtk_tree_model_get_iter_first(model, &iter);
	while (valid) {
		gtk_tree_model_get(model, &iter, column, &cell, -1);
		if (first) {
			text  = g_strconcat(line, "'", cell ? cell : "", "'", NULL);
			first = FALSE;
		} else {
			text = g_strconcat(line, " '", cell ? cell : "", "'", NULL);
		}
		g_free(line);
		line = text;
		g_free(cell);
		valid = gtk_tree_model_iter_next(model, &iter);
	}

	text = g_strconcat(line, "\"\n", NULL);
	g_free(line);

#ifdef DEBUG_TRANSITS
	fprintf(stderr, "%s(): Exiting.\n", __func__);
#endif

	return text;
}

/* -----------------------------------------------------------------------
 * Environment Variable Construct
 * ----------------------------------------------------------------------- */

gchar *widget_table_envvar_construct(GtkWidget *widget)
{
	GtkTreeModel     *model;
	GtkTreeSelection *selection;
	GtkTreeIter       iter;
	GList            *rows;
	GList            *row;
	gchar            *value;
	gchar            *cell;
	gchar            *line;
	gchar            *text;
	gchar            *string;
	gint              column        = 0;
	gint              selectionmode = GTK_SELECTION_SINGLE;
	gboolean          first;
	variable         *var = find_variable_by_widget(widget);

#ifdef DEBUG_TRANSITS
	fprintf(stderr, "%s(): Entering.\n", __func__);
#endif

	if (var->widget_tag_attr) {
		if ((value = get_tag_attribute(var->widget_tag_attr, "selection-mode")))
			selectionmode = atoi(value);
		if ((value = get_tag_attribute(var->widget_tag_attr, "exported-column")))
			column = atoi(value);
	}

#ifdef DEBUG_CONTENT
	fprintf(stderr, "%s(): widget=%p selectionmode=%i column=%i\n",
		__func__, widget, selectionmode, column);
#endif

	model     = gtk_tree_view_get_model(GTK_TREE_VIEW(widget));
	selection = gtk_tree_view_get_selection(GTK_TREE_VIEW(widget));

	if (selectionmode == GTK_SELECTION_NONE) {
		string = g_strdup("");

	} else if (selectionmode == GTK_SELECTION_MULTIPLE) {
		first = TRUE;
		line  = g_strdup("");
		rows  = gtk_tree_selection_get_selected_rows(selection, &model);
		for (row = rows; row != NULL; row = row->next) {
			if (gtk_tree_model_get_iter(model, &iter,
				(GtkTreePath *)row->data)) {
				gtk_tree_model_get(model, &iter, column, &cell, -1);
				if (first) {
					text  = g_strconcat(line, cell ? cell : "", NULL);
					first = FALSE;
				} else {
					text = g_strconcat(line, "\n", cell ? cell : "", NULL);
				}
				g_free(line);
				line = text;
				g_free(cell);
			}
		}
		g_list_free_full(rows, (GDestroyNotify)gtk_tree_path_free);
		string = line;

	} else {
		/* GTK_SELECTION_SINGLE and GTK_SELECTION_BROWSE */
		if (gtk_tree_selection_get_selected(selection, &model, &iter)) {
			gtk_tree_model_get(model, &iter, column, &string, -1);
			if (!string) string = g_strdup("");
		} else {
			string = g_strdup("");
		}
	}

#ifdef DEBUG_TRANSITS
	fprintf(stderr, "%s(): Exiting.\n", __func__);
#endif

	return string;
}

/* -----------------------------------------------------------------------
 * Fileselect
 * ----------------------------------------------------------------------- */

void widget_table_fileselect(
	variable *var, const char *name, const char *value)
{
#ifdef DEBUG_TRANSITS
	fprintf(stderr, "%s(): Entering.\n", __func__);
#endif

	g_warning( "%s(): Fileselect not implemented for this widget.",
		__func__);

#ifdef DEBUG_TRANSITS
	fprintf(stderr, "%s(): Exiting.\n", __func__);
#endif
}

/* -----------------------------------------------------------------------
 * Refresh
 * ----------------------------------------------------------------------- */

void widget_table_refresh(variable *var)
{
	GList        *element;
	gchar        *act;
	gchar        *value;
	gint          initialised = FALSE;
	gint          selected_row;

#ifdef DEBUG_TRANSITS
	fprintf(stderr, "%s(): Entering.\n", __func__);
#endif

	/* Get initialised state */
	if (g_object_get_data(G_OBJECT(var->Widget), "_initialised") != NULL)
		initialised = GPOINTER_TO_INT(
			g_object_get_data(G_OBJECT(var->Widget), "_initialised"));

	/* The <input> tag... */
	act = attributeset_get_first(&element, var->Attributes, ATTR_INPUT);
	while (act) {
		if (input_is_shell_command(act))
			widget_table_input_by_command(var, act + 8, TRUE);
		/* "File:" prefix → input by file */
		if (strncasecmp(act, "file:", 5) == 0 && strlen(act) > 5) {
			if (!initialised)
				widget_file_monitor_try_create(var, act + 5);
			widget_table_input_by_file(var, act + 5);
		}
		act = attributeset_get_next(&element, var->Attributes, ATTR_INPUT);
	}

	/* The <item> tags... */
	if (attributeset_is_avail(var->Attributes, ATTR_ITEM))
		widget_table_input_by_items(var);

	/* Initialise once at start-up */
	if (!initialised) {
		if (attributeset_is_avail(var->Attributes, ATTR_DEFAULT))
			g_warning(
				"%s(): <default> not implemented for this widget.",
				__func__);

		if ((attributeset_cmp_left(var->Attributes, ATTR_SENSITIVE, "false"))  ||
			(attributeset_cmp_left(var->Attributes, ATTR_SENSITIVE, "disabled")) ||
			(attributeset_cmp_left(var->Attributes, ATTR_SENSITIVE, "no"))     ||
			(attributeset_cmp_left(var->Attributes, ATTR_SENSITIVE, "0")))
			gtk_widget_set_sensitive(var->Widget, FALSE);

		/* GtkTreeView uses "cursor-changed" instead of GtkCList "select-row" */
		g_signal_connect(G_OBJECT(var->Widget), "cursor-changed",
			G_CALLBACK(on_any_widget_cursor_changed_event),
			(gpointer)var->Attributes);

		g_object_set_data(G_OBJECT(var->Widget), "_initialised",
			GINT_TO_POINTER(TRUE));
	}

	/* selected-row: set cursor to the requested row */
	if (var->widget_tag_attr) {
		if ((value = get_tag_attribute(var->widget_tag_attr, "selected-row"))) {
			selected_row = atoi(value);
			if (selected_row >= 0) {
				GtkTreePath *path =
					gtk_tree_path_new_from_indices(selected_row, -1);
				gtk_tree_view_set_cursor(GTK_TREE_VIEW(var->Widget),
					path, NULL, FALSE);
				gtk_tree_path_free(path);
			}
		}
	}

#ifdef DEBUG_TRANSITS
	fprintf(stderr, "%s(): Exiting.\n", __func__);
#endif
}

/* -----------------------------------------------------------------------
 * Removeselected
 * ----------------------------------------------------------------------- */

void widget_table_removeselected(variable *var)
{
	GtkTreeModel     *model;
	GtkTreeSelection *selection;
	GtkTreeIter       iter;
	GList            *rows;
	GList            *refs;
	GList            *r;
	gchar            *value;
	gint              selectionmode = GTK_SELECTION_SINGLE;

#ifdef DEBUG_TRANSITS
	fprintf(stderr, "%s(): Entering.\n", __func__);
#endif

	if (var->widget_tag_attr) {
		if ((value = get_tag_attribute(var->widget_tag_attr, "selection-mode")))
			selectionmode = atoi(value);
	}

#ifdef DEBUG_CONTENT
	fprintf(stderr, "%s(): widget=%p selectionmode=%i\n",
		__func__, var->Widget, selectionmode);
#endif

	model     = gtk_tree_view_get_model(GTK_TREE_VIEW(var->Widget));
	selection = gtk_tree_view_get_selection(GTK_TREE_VIEW(var->Widget));

	if (selectionmode == GTK_SELECTION_NONE) {
		/* Nothing to do */

	} else if (selectionmode == GTK_SELECTION_MULTIPLE) {
		/*
		 * Convert paths → GtkTreeRowReferences before removal.
		 * Paths become invalid as soon as a row is deleted; row references
		 * stay valid throughout because they track structural changes.
		 */
		rows = gtk_tree_selection_get_selected_rows(selection, &model);
		refs = NULL;
		for (r = rows; r != NULL; r = r->next) {
			refs = g_list_prepend(refs,
				gtk_tree_row_reference_new(model,
					(GtkTreePath *)r->data));
		}
		g_list_free_full(rows, (GDestroyNotify)gtk_tree_path_free);

		for (r = refs; r != NULL; r = r->next) {
			GtkTreePath *path = gtk_tree_row_reference_get_path(
				(GtkTreeRowReference *)r->data);
			if (path) {
				if (gtk_tree_model_get_iter(model, &iter, path))
					gtk_list_store_remove(GTK_LIST_STORE(model), &iter);
				gtk_tree_path_free(path);
			}
		}
		g_list_free_full(refs,
			(GDestroyNotify)gtk_tree_row_reference_free);

	} else {
		/* GTK_SELECTION_SINGLE and GTK_SELECTION_BROWSE */
		if (gtk_tree_selection_get_selected(selection, &model, &iter))
			gtk_list_store_remove(GTK_LIST_STORE(model), &iter);
	}

#ifdef DEBUG_TRANSITS
	fprintf(stderr, "%s(): Exiting.\n", __func__);
#endif
}

/* -----------------------------------------------------------------------
 * Save
 * ----------------------------------------------------------------------- */

void widget_table_save(variable *var)
{
	FILE         *outfile;
	GList        *element;
	gchar        *act;
	gchar        *filename = NULL;
	gchar        *cellstr;
	gchar        *line;
	gchar        *text;
	gint          c;
	gint          ncols;
	gint          row = 0;
	GtkTreeModel *model;
	GtkTreeIter   iter;
	gboolean      valid;

#ifdef DEBUG_TRANSITS
	fprintf(stderr, "%s(): Entering.\n", __func__);
#endif

	act = attributeset_get_first(&element, var->Attributes, ATTR_OUTPUT);
	while (act) {
		if (strncasecmp(act, "file:", 5) == 0 && strlen(act) > 5) {
			filename = act + 5;
			break;
		}
		act = attributeset_get_next(&element, var->Attributes, ATTR_OUTPUT);
	}

	if (filename) {
		if ((outfile = fopen(filename, "w"))) {
			model = gtk_tree_view_get_model(GTK_TREE_VIEW(var->Widget));
			ncols = _get_ncols(var->Widget);
			valid = gtk_tree_model_get_iter_first(model, &iter);
			while (valid) {
				line = g_strdup("");
				for (c = 0; c < ncols; c++) {
					gtk_tree_model_get(model, &iter, c, &cellstr, -1);
					if (c == 0) {
						text = g_strconcat(line,
							cellstr ? cellstr : "", NULL);
					} else {
						text = g_strconcat(line, "|",
							cellstr ? cellstr : "", NULL);
					}
					g_free(line);
					line = text;
					g_free(cellstr);
				}
				if (row == 0) {
					fprintf(outfile, "%s", line);
				} else {
					fprintf(outfile, "\n%s", line);
				}
				g_free(line);
				row++;
				valid = gtk_tree_model_iter_next(model, &iter);
			}
			fclose(outfile);
		} else {
			g_warning( "%s(): Couldn't open '%s' for writing.",
				__func__, filename);
		}
	} else {
		g_warning( "%s(): No <output file> directive found.",
			__func__);
	}

#ifdef DEBUG_TRANSITS
	fprintf(stderr, "%s(): Exiting.\n", __func__);
#endif
}

/* -----------------------------------------------------------------------
 * Input by Command
 * ----------------------------------------------------------------------- */

static void widget_table_input_by_command(variable *var, char *filename,
	gint command_or_file)
{
	FILE   *infile;
	gchar   line[512];
	gint    count;
	list_t *sliced;

#ifdef DEBUG_TRANSITS
	fprintf(stderr, "%s(): Entering.\n", __func__);
#endif

	infile = command_or_file
	         ? widget_opencommand(filename)
	         : fopen(filename, "r");

	if (infile) {
		while (fgets(line, sizeof(line), infile) != NULL) {
			line[sizeof(line) - 1] = 0;
			/* Strip trailing CR/LF */
			for (count = (gint)strlen(line) - 1; count >= 0; count--)
				if (line[count] == 13 || line[count] == 10)
					line[count] = 0;
			sliced = linecutter(g_strdup(line), '|');
			_append_row(var->Widget, sliced);
			if (sliced) list_t_free(sliced);
		}
		fclose(infile);   /* NOTE: fclose(), NOT pclose() — see safe_exec.c */
	} else {
		g_warning( "%s(): Couldn't open '%s' for reading.",
			__func__, filename);
	}

#ifdef DEBUG_TRANSITS
	fprintf(stderr, "%s(): Exiting.\n", __func__);
#endif
}

/* -----------------------------------------------------------------------
 * Input by File
 * ----------------------------------------------------------------------- */

static void widget_table_input_by_file(variable *var, char *filename)
{
#ifdef DEBUG_TRANSITS
	fprintf(stderr, "%s(): Entering.\n", __func__);
#endif

	widget_table_input_by_command(var, filename, FALSE);

#ifdef DEBUG_TRANSITS
	fprintf(stderr, "%s(): Exiting.\n", __func__);
#endif
}

/* -----------------------------------------------------------------------
 * Input by Items
 * ----------------------------------------------------------------------- */

static void widget_table_input_by_items(variable *var)
{
	GList  *element;
	gchar  *text;
	list_t *sliced;

#ifdef DEBUG_TRANSITS
	fprintf(stderr, "%s(): Entering.\n", __func__);
#endif

	g_assert(var->Attributes != NULL && var->Widget != NULL);

	text = attributeset_get_first(&element, var->Attributes, ATTR_ITEM);
	while (text != NULL) {
		sliced = linecutter(g_strdup(text), '|');
		_append_row(var->Widget, sliced);
		if (sliced) list_t_free(sliced);
		text = attributeset_get_next(&element, var->Attributes, ATTR_ITEM);
	}

#ifdef DEBUG_TRANSITS
	fprintf(stderr, "%s(): Exiting.\n", __func__);
#endif
}
