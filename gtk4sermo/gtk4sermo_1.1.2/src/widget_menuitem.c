/* SPDX-License-Identifier: GPL-2.0-or-later */
/*
 * widget_menuitem.c — Éléments de menu GTK4 via GMenuItem + GSimpleAction
 * gtk4sermo 1.0.0 — haplo-dialog <devel@haplo-dialog.fr>
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

/* ─────────────────────────────────────────────────────────────────────────
 * Entrées de menu COCHABLES — <menuitem checkbox="…"> et
 * <menuitem radiobutton="…">
 *
 * GTK 4 a supprimé GtkCheckMenuItem : une entrée de menu n'est plus un widget,
 * c'est un GMenuItem qui désigne une GAction. L'état coché vit donc dans une
 * GSimpleAction ÉTATIQUE, accrochée au porteur par g_object_set_data_full.
 * Le porteur est déjà var->Widget : rien d'autre n'est nécessaire, ni table
 * globale, ni recherche dans un GActionMap.
 *
 * Avant ce correctif, gtk4-compat.h définissait GTK_IS_CHECK_MENU_ITEM(w) à
 * (FALSE) et gtk_check_menu_item_get_active(w) à (FALSE) : les trois branches
 * de signals.c qui lisent l'état d'une entrée cochable étaient du code mort, et
 * la variable shell ne sortait jamais.
 * ───────────────────────────────────────────────────────────────────────── */

#define HP_MI_ACTION  "_menu_gaction"   /* GSimpleAction étatique          */
#define HP_MI_ACTIF   "_menu_actif"     /* dernier état connu (int 0/1)    */
#define HP_MI_ATTRS   "_menu_attrs"     /* AttributeSet* pour les <action> */
#define HP_MI_GROUPE  "_menu_groupe"    /* GPtrArray* des porteurs frères  */

/* Vérité au sens de gtkdialog : « true », « yes » ou un entier valant 1.
 * Reproduit à l'identique gtk3sermo/src/widget_menuitem.c. */
static gboolean hp_mi_verite(const gchar *v)
{
    if (v == NULL) return FALSE;
    return (g_ascii_strcasecmp(v, "true") == 0 ||
            g_ascii_strcasecmp(v, "yes")  == 0 ||
            atoi(v) == 1) ? TRUE : FALSE;
}

gboolean hp_menuitem_est_cochable(gpointer porteur)
{
    if (porteur == NULL || !G_IS_OBJECT(porteur)) return FALSE;
    return g_object_get_data(G_OBJECT(porteur), HP_MI_ACTION) != NULL;
}

gboolean hp_menuitem_get_actif(gpointer porteur)
{
    GSimpleAction *a;
    GVariant      *etat;
    gboolean       r = FALSE;

    if (!hp_menuitem_est_cochable(porteur)) return FALSE;
    a = g_object_get_data(G_OBJECT(porteur), HP_MI_ACTION);
    etat = g_action_get_state(G_ACTION(a));
    if (etat) {
        r = g_variant_get_boolean(etat);
        g_variant_unref(etat);
    }
    return r;
}

/* Pose l'état SANS déclencher les <action> : sert à <default>, <input>,
 * et à la remise à zéro des frères d'un groupe radio. Rend TRUE si la
 * valeur a réellement changé. */
static gboolean hp_mi_set_actif_silencieux(GtkWidget *porteur, gboolean actif)
{
    GSimpleAction *a;

    if (!hp_menuitem_est_cochable(porteur)) return FALSE;
    if (hp_menuitem_get_actif(porteur) == actif) return FALSE;

    a = g_object_get_data(G_OBJECT(porteur), HP_MI_ACTION);
    g_simple_action_set_state(a, g_variant_new_boolean(actif));
    g_object_set_data(G_OBJECT(porteur), HP_MI_ACTIF, GINT_TO_POINTER(actif ? 1 : 0));
    return TRUE;
}

/* « change-state » : GTK ne valide PAS tout seul quand un gestionnaire est
 * branché — c'est à nous de poser l'état. Mesuré : sans set_state ici, l'état
 * ne bouge jamais. */
static void hp_mi_change_state(GSimpleAction *a, GVariant *val, gpointer data)
{
    GtkWidget    *porteur = GTK_WIDGET(data);
    AttributeSet *Attr;
    GPtrArray    *groupe;
    gboolean      neuf, avant;

    if (val == NULL || porteur == NULL) return;
    neuf  = g_variant_get_boolean(val);
    avant = hp_menuitem_get_actif(porteur);

    g_simple_action_set_state(a, g_variant_new_boolean(neuf));
    g_object_set_data(G_OBJECT(porteur), HP_MI_ACTIF, GINT_TO_POINTER(neuf ? 1 : 0));

    /* Exclusion mutuelle : une entrée radio qui passe à vrai éteint ses
     * frères. On n'émet « toggled » que sur ceux qui CHANGENT réellement —
     * sinon les <action> non préfixées des frères tourneraient à chaque
     * sélection, y compris pour ceux qui n'ont rien gagné ni perdu. */
    groupe = g_object_get_data(G_OBJECT(porteur), HP_MI_GROUPE);
    if (groupe != NULL && neuf) {
        guint i;
        for (i = 0; i < groupe->len; i++) {
            GtkWidget *frere = g_ptr_array_index(groupe, i);
            if (frere == porteur) continue;
            if (hp_mi_set_actif_silencieux(frere, FALSE)) {
                AttributeSet *fa = g_object_get_data(G_OBJECT(frere), HP_MI_ATTRS);
                if (fa) on_any_widget_toggled_event(frere, fa);
            }
        }
    }

    if (neuf == avant) return;   /* rien n'a change pour CE porteur */

    Attr = g_object_get_data(G_OBJECT(porteur), HP_MI_ATTRS);
    if (Attr) {
        on_any_widget_toggled_event(porteur, Attr);
        on_any_widget_activate_event(porteur, Attr);
    }
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

    /* Entrée cochable ? C'est la PRÉSENCE de l'attribut qui décide ; sa
     * valeur ne donne que l'état initial. Même ordre de priorité qu'en
     * GTK 3 : checkbox d'abord, radiobutton ensuite. */
    if (Type != WIDGET_MENUITEMSEPARATOR && attr) {
        const gchar *v = NULL;
        gint         genre_coche = 0;

        if ((v = get_tag_attribute(attr, "checkbox")) != NULL)          genre_coche = 3;
        else if ((v = get_tag_attribute(attr, "radiobutton")) != NULL)  genre_coche = 4;

        if (genre_coche != 0) {
            gchar         *nom  = g_strdup_printf("mi_%d", ++_menu_action_counter);
            gboolean       init = hp_mi_verite(v);
            GSimpleAction *a    = g_simple_action_new_stateful(
                                      nom, NULL, g_variant_new_boolean(init));

            g_signal_connect(a, "change-state",
                             G_CALLBACK(hp_mi_change_state), porteur);

            g_object_set_data_full(G_OBJECT(porteur), HP_MI_ACTION,
                                   a, (GDestroyNotify)g_object_unref);
            g_object_set_data_full(G_OBJECT(porteur), "_menu_nom", nom, g_free);
            g_object_set_data(G_OBJECT(porteur), HP_MI_ACTIF,
                              GINT_TO_POINTER(init ? 1 : 0));
            g_object_set_data(G_OBJECT(porteur), HP_MI_ATTRS, Attr);
            g_object_set_data(G_OBJECT(porteur), "_menu_kind",
                              GINT_TO_POINTER(genre_coche));
            g_object_set_data_full(G_OBJECT(porteur), "_menu_label",
                                   g_strdup(label ? label : ""), g_free);
            g_object_set_data_full(G_OBJECT(porteur), "_menu_action",
                                   g_strdup(action ? action : ""), g_free);
            return porteur;
        }
    }

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
    GPtrArray          *radios  = NULL;
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

    /* Les entrées radio d'un même <menu> forment un groupe : chacune reçoit
     * la liste de ses frères pour pouvoir les éteindre en passant à vrai. */
    if (radios != NULL) {
        guint i;
        for (i = 0; i < radios->len; i++)
            g_object_set_data_full(G_OBJECT(g_ptr_array_index(radios, i)),
                                   HP_MI_GROUPE, g_ptr_array_ref(radios),
                                   (GDestroyNotify)g_ptr_array_unref);
        g_ptr_array_unref(radios);
    }
                section = g_menu_new();
            }
        } else if (genre == 3 || genre == 4) {
            /* Entrée cochable : l'action existe déjà, créée par
             * widget_menuitem_create(). On ne fait que la publier. */
            const gchar   *ml  = g_object_get_data(G_OBJECT(enfant), "_menu_label");
            const gchar   *nom = g_object_get_data(G_OBJECT(enfant), "_menu_nom");
            GSimpleAction *a   = g_object_get_data(G_OBJECT(enfant), HP_MI_ACTION);

            if (a && nom) {
                gchar *cible = g_strdup_printf("menu.%s", nom);
                g_action_map_add_action(G_ACTION_MAP(actions), G_ACTION(a));
                g_menu_append(section, (ml && *ml) ? ml : "…", cible);
                g_free(cible);
            }
            if (genre == 4) {
                if (radios == NULL) radios = g_ptr_array_new();
                g_ptr_array_add(radios, enfant);
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


/* ─────────────────────────────────────────────────────────────────────────
 * enable: / disable: sur un <menu> entier.
 *
 * En GTK 4 une entrée de menu n'est pas un widget : gtk_widget_set_sensitive
 * sur le porteur du <menu> ne grisait donc RIEN, et « disable:mnu01 » restait
 * sans effet.
 *
 * On agit sur ce qui existe réellement : le GSimpleActionGroup que le <menu>
 * transporte déjà. Désactiver toutes ses actions grise toutes ses entrées.
 *
 * ⛔ On ne s'appuie PAS sur l'ordre des enfants d'un GtkPopoverMenuBar pour
 * retrouver « le bon menu » : c'est un détail d'implémentation de GTK, non
 * documenté, et s'il changeait on griserait le mauvais menu sans plantage et
 * sans un mot.
 * ───────────────────────────────────────────────────────────────────────── */
void hp_menu_set_sensitive(gpointer porteur, gboolean actif)
{
	GSimpleActionGroup *grp;
	gchar             **noms;
	gchar             **q;

	if (porteur == NULL || !G_IS_OBJECT(porteur)) return;
	grp = g_object_get_data(G_OBJECT(porteur), "_menu_actions");
	if (grp == NULL) return;

	noms = g_action_group_list_actions(G_ACTION_GROUP(grp));
	for (q = noms; q && *q; q++) {
		GAction *a = g_action_map_lookup_action(G_ACTION_MAP(grp), *q);
		if (a && G_IS_SIMPLE_ACTION(a))
			g_simple_action_set_enabled(G_SIMPLE_ACTION(a), actif);
	}
	g_strfreev(noms);
}

gchar *widget_menuitem_envvar_construct(GtkWidget *w)
{
    /* Seules les entrées cochables ont une valeur, comme en GTK 3. */
    if (hp_menuitem_est_cochable(w))
        return g_strdup(hp_menuitem_get_actif(w) ? "true" : "false");
    return g_strdup("");
}
gchar *widget_menuitem_envvar_all_construct(variable *var)          { return NULL; }
void   widget_menuitem_clear(variable *var)                         {}
void   widget_menuitem_refresh(variable *var)
{
    GList *el = NULL;

    if (var == NULL || !hp_menuitem_est_cochable(var->Widget)) return;

    /* <default>true</default> : appliqué sans déclencher les <action>. */
    if (var->Attributes && attributeset_is_avail(var->Attributes, ATTR_DEFAULT)) {
        const gchar *v = attributeset_get_first(&el, var->Attributes, ATTR_DEFAULT);
        hp_mi_set_actif_silencieux(var->Widget, hp_mi_verite(v));
    }
}
void   widget_menuitem_fileselect(variable *var, const gchar *n, const gchar *v) {}
void   widget_menuitem_removeselected(variable *var)                {}
void   widget_menuitem_save(variable *var)                          {}
