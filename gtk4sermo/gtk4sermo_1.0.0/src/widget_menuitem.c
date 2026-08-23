/*
 * widget_menuitem.c — Éléments de menu GTK4 via GMenuItem + GSimpleAction
 * gtk4sermo 1.0.0 — Haplo-Linux <devel@haplo-dialog.fr>
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
GtkWidget *widget_menu_create(AttributeSet *Attr, tag_attr *attr, gint Type)
{
    stackelement        s;
    GtkWidget          *porteur = gtk_box_new(GTK_ORIENTATION_HORIZONTAL, 0);
    GMenu              *modele  = g_menu_new();
    GMenu              *section = g_menu_new();
    GSimpleActionGroup *actions = g_simple_action_group_new();
    GList              *el      = NULL;
    gchar              *label   = NULL;
    gint                n;

    (void)Type;

    s = pop();

    /* La pile est déjà dans l'ordre du document pour les menus. */
    for (n = 0; n < s.nwidgets; n++) {
        GtkWidget *enfant = s.widgets[n];
        gint       genre;

        if (!enfant || !G_IS_OBJECT(enfant)) continue;
        genre = GPOINTER_TO_INT(g_object_get_data(G_OBJECT(enfant), "_menu_kind"));

        if (genre == 2) {
            /* séparateur : on clôt la section courante */
            if (g_menu_model_get_n_items(G_MENU_MODEL(section)) > 0) {
                g_menu_append_section(modele, NULL, G_MENU_MODEL(section));
                g_object_unref(section);
                section = g_menu_new();
            }
        } else if (genre == 1) {
            const gchar *ml  = g_object_get_data(G_OBJECT(enfant), "_menu_label");
            const gchar *cmd = g_object_get_data(G_OBJECT(enfant), "_menu_action");
            gchar *nom    = g_strdup_printf("mi_%d", ++_menu_action_counter);
            gchar *cible  = g_strdup_printf("menu.%s", nom);
            GSimpleAction *a = g_simple_action_new(nom, NULL);

            g_signal_connect_data(a, "activate", G_CALLBACK(_menuitem_activate),
                g_strdup(cmd ? cmd : ""), _gtkd_closure_g_free, 0);
            g_action_map_add_action(G_ACTION_MAP(actions), G_ACTION(a));
            g_object_unref(a);

            g_menu_append(section, (ml && *ml) ? ml : "…", cible);
            g_free(nom); g_free(cible);
        } else {
            /* sous-menu imbriqué */
            GMenu       *sm = g_object_get_data(G_OBJECT(enfant), "_menu_model");
            const gchar *sl = g_object_get_data(G_OBJECT(enfant), "_menu_label");
            GSimpleActionGroup *sa =
                g_object_get_data(G_OBJECT(enfant), "_menu_actions");
            if (sm) {
                g_menu_append_submenu(section, (sl && *sl) ? sl : "…",
                                      G_MENU_MODEL(sm));
                /* les actions du sous-menu remontent avec lui */
                if (sa) {
                    gchar **noms = g_action_group_list_actions(G_ACTION_GROUP(sa));
                    for (gchar **q = noms; q && *q; q++) {
                        GAction *act = g_action_map_lookup_action(G_ACTION_MAP(sa), *q);
                        if (act) g_action_map_add_action(G_ACTION_MAP(actions), act);
                    }
                    g_strfreev(noms);
                }
            }
        }
    }

    if (g_menu_model_get_n_items(G_MENU_MODEL(section)) > 0)
        g_menu_append_section(modele, NULL, G_MENU_MODEL(section));
    g_object_unref(section);

    if (Attr && attributeset_is_avail(Attr, ATTR_LABEL))
        label = attributeset_get_first(&el, Attr, ATTR_LABEL);
    /* <menu label="_Stock Images"> : l'étiquette en attribut de balise */
    if ((!label || !*label) && attr)
        label = get_tag_attribute(attr, "label");

    gtk_widget_set_visible(porteur, FALSE);
    g_object_set_data(G_OBJECT(porteur), "_menu_kind", GINT_TO_POINTER(0));
    g_object_set_data_full(G_OBJECT(porteur), "_menu_label",
        g_strdup(label ? label : "Menu"), g_free);
    g_object_set_data_full(G_OBJECT(porteur), "_menu_model", modele, g_object_unref);
    g_object_set_data_full(G_OBJECT(porteur), "_menu_actions", actions, g_object_unref);
    return porteur;
}

gchar *widget_menuitem_envvar_construct(GtkWidget *w)               { return g_strdup(""); }
gchar *widget_menuitem_envvar_all_construct(variable *var)          { return NULL; }
void   widget_menuitem_clear(variable *var)                         {}
void   widget_menuitem_refresh(variable *var)                       {}
void   widget_menuitem_fileselect(variable *var, const gchar *n, const gchar *v) {}
void   widget_menuitem_removeselected(variable *var)                {}
void   widget_menuitem_save(variable *var)                          {}
