/* SPDX-License-Identifier: GPL-2.0-or-later */
/*
 * widget_menuitem.c — Éléments de menu GTK4 via GMenuItem + GSimpleAction
 * sermo — haplo-dialog <devel@haplo-dialog.fr>
 * Licence : GPL-2.0-or-later
 *
 * Dans le modèle GTK4 :
 *   - <menuitem> seul (sans <menubar> parent) → GtkPopoverMenu flottant
 *   - <menu> contenant des <menuitem> → GMenu + GtkPopoverMenu
 *
 * Sécurité :
 *   - Actions enregistrées dans un GSimpleActionGroup local
 *   - Callbacks via safe_system()
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
#include "tag_attributes.h"
#include "stack.h"
#include "actions.h"
#include <string.h>
#include <stdlib.h>
#include <ctype.h>

/* Wrapper GClosureNotify (evite -Wcast-function-type sur g_free) */
static void _gtkd_closure_g_free(gpointer data, GClosure *closure) {
	(void)closure;
	g_free(data);
}

/* ─── Callback action ────────────────────────────────────────────────────── */
static void _menuitem_activate(GSimpleAction *a, GVariant *p, gpointer data)
{
    (void)a; (void)p;
    const gchar *cmd = (const gchar *)data;
    if (cmd && *cmd) safe_system(cmd);
}

/* ───────────────────────────────────────────────────────────────────────────
 * <menuitem> et <menuitemseparator> ne sont pas des widgets affichables en
 * GTK4 : ils décrivent une entrée de menu, que le <menu> parent transforme en
 * GMenuItem. Ce port rendait ici un GtkPopoverMenu autonome, qui se retrouvait
 * empilé dans une boîte comme un widget ordinaire — GTK le réalisait alors en
 * surface popup sans parent (« gdk_surface_new_popup: assertion GDK_IS_SURFACE
 * (parent) failed ») et le programme mourait sur un SIGSEGV. Sept des 58
 * exemples livrés plantaient ainsi, tous ceux qui contiennent un <menubar>.
 *
 * On rend donc un porteur inerte, jamais affiché, qui transporte l'étiquette et
 * l'action jusqu'au <menu> parent.
 * ────────────────────────────────────────────────────────────────────────── */

/* Compteur global : les noms d'action doivent être uniques dans le groupe que
 * la barre installe, tous menus confondus. */
static gint _menu_action_counter = 0;

GtkWidget *widget_menuitem_create(AttributeSet *Attr, tag_attr *attr, gint Type)
{
    GtkWidget *porteur = gtk_box_new(GTK_ORIENTATION_HORIZONTAL, 0);
    GList     *el      = NULL;
    gchar     *label   = NULL;
    gchar     *action  = NULL;

    if (Attr) {
        if (attributeset_is_avail(Attr, ATTR_LABEL))
            label = attributeset_get_first(&el, Attr, ATTR_LABEL);
        el = NULL;
        if (attributeset_is_avail(Attr, ATTR_ACTION))
            action = attributeset_get_first(&el, Attr, ATTR_ACTION);
    }

    /* L'étiquette peut aussi venir d'un attribut de balise — <menuitem
     * label="AbiWord"> — ou, faute de mieux, du nom d'icône de stock. */
    if ((!label || !*label) && attr) {
        gchar *v;
        if ((v = get_tag_attribute(attr, "label")))    label = v;
        else if ((v = get_tag_attribute(attr, "stock-id"))) label = v;
        else if ((v = get_tag_attribute(attr, "icon-name"))) label = v;
    }

    /* menuitem cochable : checkbox="true|false" / radiobutton="true|false"
     * — l'etat par defaut est exporte en variable (banc 04). */
    if (attr) {
        gchar *v = get_tag_attribute(attr, "checkbox");
        if (!v) v = get_tag_attribute(attr, "radiobutton");
        if (v) {
            gboolean on = (g_ascii_strcasecmp(v, "true") == 0 ||
                           g_strcmp0(v, "1") == 0);
            g_object_set_data_full(G_OBJECT(porteur), "mi_state",
                g_strdup(on ? "true" : "false"), g_free);
        }
    }

    /* Icône de thème (parité gtk3/qt6) : stock IDs traités comme des noms
     * d'icône freedesktop (morts depuis GTK 3.10). */
    {
        const gchar *icon = NULL;
        if (attr) {
            if (!(icon = get_tag_attribute(attr, "icon")))
            if (!(icon = get_tag_attribute(attr, "icon-name")))
            if (!(icon = get_tag_attribute(attr, "image-icon")))
            if (!(icon = get_tag_attribute(attr, "stock")))
            if (!(icon = get_tag_attribute(attr, "stock-id")))
                icon = get_tag_attribute(attr, "image-stock");
        }
        g_object_set_data_full(G_OBJECT(porteur), "_menu_icon",
            g_strdup(icon ? icon : ""), g_free);
    }

    gtk_widget_set_visible(porteur, FALSE);
    g_object_set_data(G_OBJECT(porteur), "_menu_kind",
        GINT_TO_POINTER(Type == WIDGET_MENUITEMSEPARATOR ? 2 : 1));
    g_object_set_data_full(G_OBJECT(porteur), "_menu_label",
        g_strdup(label  ? label  : ""), g_free);
    g_object_set_data_full(G_OBJECT(porteur), "_menu_action",
        g_strdup(action ? action : ""), g_free);
    return porteur;
}

/* <menu> : assemble ses enfants en un GMenuModel et le transporte, lui aussi
 * par un porteur inerte, jusqu'à la <menubar>. */
/* Callback d'un item : ferme le popover puis route l'action par le cœur. */
static void _mi_row_clicked(GtkButton *b, gpointer u)
{
    (void)u;
    const gchar *cmd = g_object_get_data(G_OBJECT(b), "cmd");
    GtkWidget *pop = gtk_widget_get_ancestor(GTK_WIDGET(b), GTK_TYPE_POPOVER);
    if (pop) gtk_popover_popdown(GTK_POPOVER(pop));
    if (cmd && *cmd) execute_action(GTK_WIDGET(b), cmd, NULL);
}

/* Une ligne d'item : bouton plat = [icône 16px] [libellé]. GTK4 n'affiche pas
 * les icônes des menus « modèle » (GMenuModel) ; on construit donc le menu à
 * la main pour pouvoir les montrer (parité gtk3). */
static GtkWidget *_mi_row(const gchar *label, const gchar *icon, const gchar *cmd)
{
    GtkWidget *btn = gtk_button_new();
    gtk_widget_add_css_class(btn, "flat");
    GtkWidget *row = gtk_box_new(GTK_ORIENTATION_HORIZONTAL, 6);
    GtkWidget *img = (icon && *icon) ? gtk_image_new_from_icon_name(icon)
                                     : gtk_image_new();
    gtk_image_set_pixel_size(GTK_IMAGE(img), 16);
    gtk_box_append(GTK_BOX(row), img);
    GtkWidget *lab = gtk_label_new(label && *label ? label : "…");
    gtk_widget_set_halign(lab, GTK_ALIGN_START);
    gtk_widget_set_hexpand(lab, TRUE);
    gtk_box_append(GTK_BOX(row), lab);
    gtk_button_set_child(GTK_BUTTON(btn), row);
    g_object_set_data_full(G_OBJECT(btn), "cmd", g_strdup(cmd ? cmd : ""), g_free);
    g_signal_connect(btn, "clicked", G_CALLBACK(_mi_row_clicked), NULL);
    return btn;
}

/* <menu> : dépile ses <menuitem> et construit un GtkPopover (liste verticale
 * de lignes image+libellé), transporté par un porteur inerte jusqu'à <menubar>. */
GtkWidget *widget_menu_create(AttributeSet *Attr, tag_attr *attr, gint Type)
{
    stackelement  s;
    GtkWidget    *porteur = gtk_box_new(GTK_ORIENTATION_HORIZONTAL, 0);
    GtkWidget    *popover = gtk_popover_new();
    GtkWidget    *vbox    = gtk_box_new(GTK_ORIENTATION_VERTICAL, 0);
    GList        *el      = NULL;
    gchar        *label   = NULL;
    gint          n;

    (void)Type;
    gtk_popover_set_has_arrow(GTK_POPOVER(popover), FALSE);
    gtk_widget_add_css_class(vbox, "menu");
    gtk_popover_set_child(GTK_POPOVER(popover), vbox);

    s = pop();
    for (n = 0; n < s.nwidgets; n++) {
        GtkWidget *enfant = s.widgets[n];
        gint       genre;
        if (!enfant || !G_IS_OBJECT(enfant)) continue;
        genre = GPOINTER_TO_INT(g_object_get_data(G_OBJECT(enfant), "_menu_kind"));

        if (genre == 2) {
            gtk_box_append(GTK_BOX(vbox), gtk_separator_new(GTK_ORIENTATION_HORIZONTAL));
        } else if (genre == 1) {
            const gchar *ml   = g_object_get_data(G_OBJECT(enfant), "_menu_label");
            const gchar *cmd  = g_object_get_data(G_OBJECT(enfant), "_menu_action");
            const gchar *icon = g_object_get_data(G_OBJECT(enfant), "_menu_icon");
            const gchar *st   = g_object_get_data(G_OBJECT(enfant), "mi_state");
            if (st) {
                /* item cochable */
                GtkWidget *chk = gtk_check_button_new_with_label(ml && *ml ? ml : "…");
                gtk_check_button_set_active(GTK_CHECK_BUTTON(chk),
                    g_strcmp0(st, "true") == 0);
                gtk_widget_add_css_class(chk, "flat");
                gtk_box_append(GTK_BOX(vbox), chk);
            } else {
                gtk_box_append(GTK_BOX(vbox), _mi_row(ml, icon, cmd));
            }
        } else {
            /* sous-menu imbriqué : bouton ouvrant le sous-popover */
            GtkWidget   *sp = g_object_get_data(G_OBJECT(enfant), "_menu_popover");
            const gchar *sl = g_object_get_data(G_OBJECT(enfant), "_menu_label");
            if (sp) {
                GtkWidget *mb = gtk_menu_button_new();
                gtk_menu_button_set_label(GTK_MENU_BUTTON(mb), sl && *sl ? sl : "…");
                gtk_menu_button_set_popover(GTK_MENU_BUTTON(mb), sp);
                gtk_widget_add_css_class(mb, "flat");
                gtk_box_append(GTK_BOX(vbox), mb);
            }
        }
    }

    if (Attr && attributeset_is_avail(Attr, ATTR_LABEL))
        label = attributeset_get_first(&el, Attr, ATTR_LABEL);
    if ((!label || !*label) && attr)
        label = get_tag_attribute(attr, "label");

    gtk_widget_set_visible(porteur, FALSE);
    g_object_set_data(G_OBJECT(porteur), "_menu_kind", GINT_TO_POINTER(0));
    g_object_set_data_full(G_OBJECT(porteur), "_menu_label",
        g_strdup(label ? label : "Menu"), g_free);
    g_object_set_data_full(G_OBJECT(porteur), "_menu_popover",
        g_object_ref_sink(popover), g_object_unref);
    return porteur;
}

gchar *widget_menuitem_envvar_construct(GtkWidget *w)
{
	/* menuitem cochable : exporte son etat true/false */
	const gchar *st;
	if (!w) return g_strdup("");
	st = g_object_get_data(G_OBJECT(w), "mi_state");
	return g_strdup(st ? st : "");
}
gchar *widget_menuitem_envvar_all_construct(variable *var)          { return NULL; }
void   widget_menuitem_clear(variable *var)                         {}
void   widget_menuitem_refresh(variable *var)                       {}
void   widget_menuitem_fileselect(variable *var, const gchar *n, const gchar *v) {}
void   widget_menuitem_removeselected(variable *var)                {}
void   widget_menuitem_save(variable *var)                          {}
