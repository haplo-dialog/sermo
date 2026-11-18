/* sermo_backend_html.c — hook d'init toolkit + ABI neutre du backend HTML statique.
 * SPDX-License-Identifier: GPL-2.0-or-later
 * Copyright (C) 2026 S. Cage
 * haplo-dialog <devel@haplo-dialog.fr>
 *
 * Le cœur (libsermocore, variante NEUTRE) pilote parse/automaton/variables/
 * actions et assemble l'arbre de widgets via une ABI « neutre » nommée qt6_*
 * (héritée du premier port neutre). Ce fichier la réalise sur le modèle
 * arbre-de-WidgetNode, comme les backends sdl3/ncurses ; gtk_main
 * (= sermo_be_run_loop) exporte D'ABORD l'arbre en HTML statique
 * (render_html_export, garanti même si un <timer> sort ensuite le
 * programme), PUIS rejoue les <timer> une fois — comme les autres
 * backends en mode sans affichage (headless CI) — pour rester comparable
 * au banc de comportement de la famille (tests/comportement/). Un
 * <timer><action>exit:...</action></timer> imprime alors les variables
 * (print_variables, via action_exitprogram du cœur) et termine ; sans
 * timer, repli minimal ci-dessous. La page HTML produite reste, elle,
 * sans JavaScript ni logique serveur : ce rejeu ne touche que le
 * PROCESSUS d'export, jamais la sortie. */
#include <stdio.h>
#include <unistd.h>
#include <stdlib.h>

#include "sermo_backend.h"
#include "dialog_state.h"      /* WidgetNode, widget_node_add_child, WT_* */
#include "render_html.h"       /* render_html_export(), html_mode() */
#include "serve_http.h"        /* html_serve_once() */
#include "variables.h"         /* variables_export_all(), print_variables() */
#include "actions.h"           /* execute_action() */
#include "attributes.h"        /* AttributeSet, ATTR_ACTION */
#include "sermo_progress.h"      /* barres de progression lues au fil de l'eau */
#include "jouer_actions.h"     /* jouer_actions() : actions avec leur fonction */
#include "fenetres.h"          /* pile des fenêtres ouvertes, minuteries sans horloge */
void action_exitprogram(GtkWidget *widget, char *string);   /* cœur (actions.c) : bouton nu activé */

/* Globales que le cœur déclare extern (visibilité des widgets). */
GList *widget_show_list = NULL;
GList *widget_hide_list = NULL;

/* Racine capturée par widget_window_create (nom hérité du port neutre). */
WidgetNode *sdl3_root_window = NULL;

extern int option_print_ir;   /* gboolean (gint) défini dans le cœur */

void sermo_backend_toolkit_init(int *argc, char ***argv, int print_ir)
{
    (void)argc; (void)argv; (void)print_ir;
    /* Rien à ouvrir : l'export HTML n'a pas de session graphique. */
}

/* ─── ABI neutre qt6_* sur l'arbre WidgetNode ──────────────────────────────── */
void sermo_be_widget_show(void *w)
{ WidgetNode *n = (WidgetNode *)w; if (n) n->visible = TRUE; }

void sermo_be_widget_hide(void *w)
{ WidgetNode *n = (WidgetNode *)w; if (n) n->visible = FALSE; }

void sermo_be_widget_set_sensitive(void *w, int sensitive)
{ WidgetNode *n = (WidgetNode *)w; if (n) n->sensitive = (sensitive != 0); }

void sermo_be_widget_redraw(void *w)
{ (void)w; /* export en un seul passage : rien à redessiner */ }

void sermo_be_container_add(void *container, void *child)
{
    WidgetNode *c = (WidgetNode *)container, *ch = (WidgetNode *)child;
    if (c && ch) widget_node_add_child(c, ch);
}

void *sermo_be_container_child0(void *container)
{
    WidgetNode *c = (WidgetNode *)container;
    return (c && c->child_count > 0) ? c->children[0] : NULL;
}

void sermo_be_window_move(void *w, int x, int y)
{ (void)w; (void)x; (void)y; /* sans objet pour une page HTML */ }

void *sermo_be_scroll_new(int w, int h)
{
    WidgetNode *n = widget_node_new(WT_VBOX, NULL, NULL);
    if (n) { n->width = w; n->height = h; }
    return n;
}

/* ─── Cycle de vie « application » ─────────────────────────────────────────── */
void sermo_be_app_init(int *argc, char ***argv)
{ (void)argc; (void)argv; }

/* Joue les actions d'un nœud : toutes, chacune avec sa fonction, quand le
 * créateur a gardé le jeu d'attributs ; sinon l'unique commande du nœud.
 * Même règle que le port ncurses (voir jouer_actions.h). */
static void declencher(WidgetNode *n, const char *signal1, const char *signal2)
{
    if (!n) return;
    if (n->actions_attr)
        jouer_actions((GtkWidget *)n, (AttributeSet *)n->actions_attr, signal1, signal2);
    else if (n->action && *n->action)
        execute_action((GtkWidget *)n, n->action, NULL);
}

/* Widgets à bascule (case à cocher, bouton radio, bouton à bascule,
 * interrupteur) : leurs actions jouent à chaque changement d'état, « if true »/
 * « if false » compris. */
static void bascule_jouer(WidgetNode *n, int actif)
{
    if (n->actions_attr && n->type == WT_SWITCH)
        jouer_actions_interrupteur((GtkWidget *)n, (AttributeSet *)n->actions_attr, actif);
    else if (n->actions_attr)
        jouer_actions_bascule((GtkWidget *)n, (AttributeSet *)n->actions_attr, actif);
    else if (n->action && *n->action)
        execute_action((GtkWidget *)n, n->action, NULL);
}

/* Pour widget_radiobutton.c : le bouton radio qu'un autre remplace au départ
 * (son <default> ou son <input>) joue ses actions « décoché », comme chez
 * l'étalon, où elles sont déjà branchées. */
void sermo_be_bascule_jouer(void *w, int actif)
{
    if (w) bascule_jouer((WidgetNode *)w, actif);
}

/* Bascule comme un clic de l'étalon. Un bouton radio choisi le reste ; en
 * choisir un décoche ses frères de la même boîte (le groupe du cœur), qui
 * jouent leurs actions « décoché » d'abord, comme GTK. */
static void basculer(WidgetNode *n)
{
    if (n->type == WT_RADIOBUTTON) {
        if (n->state.toggle.active) return;
        n->state.toggle.active = TRUE;
        if (n->parent)
            for (int i = 0; i < n->parent->child_count; i++) {
                WidgetNode *f = n->parent->children[i];
                if (f == n || f->type != WT_RADIOBUTTON || !f->state.toggle.active) continue;
                f->state.toggle.active = FALSE;
                bascule_jouer(f, FALSE);
            }
        bascule_jouer(n, TRUE);
        return;
    }
    n->state.toggle.active = !n->state.toggle.active;
    bascule_jouer(n, n->state.toggle.active);
}

/* Crochet du contrat : « activate:NOM » bascule les widgets à bascule (le shim
 * fait de gtk_widget_activate() un FALSE). La page porte l'état d'après. */
int sermo_be_widget_activate(void *w)
{
    WidgetNode *n = (WidgetNode *)w;
    if (!n) return 0;
    switch (n->type) {
    case WT_CHECKBOX: case WT_RADIOBUTTON: case WT_TOGGLEBUTTON: case WT_SWITCH:
        basculer(n);
        return 1;
    default:
        return 0;
    }
}

/* Crochet du contrat : « activate:NOM » sur un <button> — ce que fait un clic :
 * ses actions jouent ; un bouton nu ferme le dialogue (EXIT = valeur de
 * famille, sinon le libellé). Le cœur ne l'appelle que pour un bouton. */
int sermo_be_button_click(void *w)
{
    WidgetNode *n = (WidgetNode *)w;
    if (!n || n->type != WT_BUTTON) return 0;
    if (n->var_name && strcmp(n->var_name, "__fontbutton__") == 0) return 0;
    if (n->action && *n->action)
        declencher(n, "clicked", NULL);   /* exit:… peut sortir ici */
    else
        action_exitprogram((GtkWidget *)n,
            (char *)(n->tooltip ? n->tooltip
                     : (n->label && n->label[0] ? n->label : "OK")));
    return 1;
}

/* Crochet du contrat : l'état d'un widget pour condition="active_is_…/
 * sensitive_is_…/visible_is_…(NOM)" (le shim ne sait rien des nœuds).
 * quoi : 0 = actif, 1 = sensible, 2 = visible ; -1 = je ne sais pas. */
int sermo_be_widget_etat(void *w, int quoi)
{
    WidgetNode *n = (WidgetNode *)w;
    if (!n) return -1;
    switch (quoi) {
    case 0:
        switch (n->type) {
        case WT_CHECKBOX: case WT_RADIOBUTTON: case WT_TOGGLEBUTTON: case WT_SWITCH:
            return n->state.toggle.active ? 1 : 0;
        case WT_EXPANDER:
            return n->state.expander.open ? 1 : 0;
        default:
            return -1;
        }
    case 1: return n->sensitive ? 1 : 0;
    case 2: return n->visible ? 1 : 0;
    default: return -1;
    }
}

/* La sélection posée au chargement d'une <list> est jouée une fois, l'arbre
 * entier construit : les variables que l'action vise existent alors. Seule une
 * vraie <list> garde son jeu d'actions ; un <tree> (WT_LIST aussi) non. */
static void listes_chargees(WidgetNode *n)
{
    if (!n) return;
    if (n->type == WT_LIST && n->actions_attr && n->state.list.selected_index >= 0)
        declencher(n, "selection-changed", "row-selected");
    for (int i = 0; i < n->child_count; i++) listes_chargees(n->children[i]);
}

/* Les <timer> : chacun joue une fois, toutes fenêtres ouvertes confondues,
 * dans l'ordre où ils partiraient à l'écran (fenetres.c). Un « exit:… » sort
 * via action_exitprogram (print_variables + exit), sans repasser par ici.
 * Jusqu'à la 2.8.1 : l'ordre du document, et une minuterie de 8 s fermait le
 * dialogue avant celle d'une seconde. */
static void tic(WidgetNode *minuterie)
{
    declencher(minuterie, "tick", NULL);
}

/* Barres de progression : relever ce que leurs commandes ont écrit. Rend le
 * nombre de lectures encore ouvertes. Sans écran, rien ne presse : on lit
 * chaque commande jusqu'au bout (la première ligne à 100 déclenche ses
 * actions, un « exit:… » sort ici). */
static int a_des_barres(WidgetNode *node)
{
    if (!node) return 0;
    if (node->type == WT_PROGRESSBAR) return 1;
    for (int i = 0; i < node->child_count; i++)
        if (a_des_barres(node->children[i])) return 1;
    return 0;
}

static int servir_barres(WidgetNode *node)
{
    int ouvertes = 0;
    if (!node) return 0;
    if (node->type == WT_PROGRESSBAR && node->lecture) {
        if (sermo_progress_poll((sermo_progress *)node->lecture))
            ouvertes++;
        else
            node->lecture = NULL;   /* rendue par le cœur */
    }
    for (int i = 0; i < node->child_count; i++) ouvertes += servir_barres(node->children[i]);
    return ouvertes;
}

int sermo_be_run_loop(void)
{
    WidgetNode *root = sdl3_root_window;
    if (option_print_ir || !root) return 0;   /* headless : le cœur imprime l'IR */

    const char *title = (root->label && *root->label) ? root->label : "htmlsermo";

    /* Mode « serve » : la page n'est pas ecrite sur disque, elle est servie sur
     * la boucle locale et le formulaire renvoie les valeurs. C'est le cycle de
     * vie d'une fenetre de dialogue, transpose au navigateur — donc on ne
     * rejoue PAS les timers et on ne sort pas par le repli EXIT="abort". */
    if (html_mode() == 2 /* MODE_SERVE */) {
        char *buf = NULL; size_t blen = 0;
        FILE *mem = open_memstream(&buf, &blen);
        if (!mem) { perror("htmlsermo: open_memstream"); exit(EXIT_FAILURE); }
        render_html_write(mem, root, title);
        fclose(mem);
        int rc = html_serve_once(buf, (unsigned long)blen);
        free(buf);
        /* Les variables du dialogue, comme les autres ports : la valeur reçue
         * du navigateur si elle existe, sinon celle de départ. Jamais un nom
         * que le dialogue ne déclare pas (voir serve_http.c, print_form). */
        variables_export_all();
        print_variables(NULL);
        const char *sortie = rc == 0 ? html_valeur_soumise("EXIT") : NULL;
        gchar *e = shell_escape_value(sortie ? sortie : "abort");
        printf("EXIT=\"%s\"\n", e);
        g_free(e);
        g_setenv("EXIT", sortie ? sortie : "abort", TRUE);   /* --do lit EXIT dans l'environnement (manuel §3.2) */
        fflush(stdout);
        exit(EXIT_SUCCESS);
    }

    /* launch: — le cœur rappelle cette fonction depuis une action. La page
     * reste celle de la première fenêtre : jusqu'à la 3.3.0 la fenêtre lancée
     * l'écrasait, ne jouait que ses propres minuteries et le dialogue sortait
     * en EXIT="abort", celles de la première fenêtre perdues. */
    static int profondeur = 0;
    int lancee = profondeur++ > 0;
    int admise = fenetre_empiler(root);

    /* 1) L'export HTML d'abord, INCONDITIONNELLEMENT : c'est la promesse du
     * backend, elle ne doit jamais dépendre de ce qu'un <timer> fait ensuite. */
    if (!lancee)
        render_html_export(root, title);

    /* 2) Rejeu des timers, comme les autres backends en mode headless.
     * Si l'un déclenche « exit:... », action_exitprogram imprime les
     * variables et termine ici — on ne revient pas. */
    if (admise) {
        listes_chargees(root);
        int barres = servir_barres(root);
        while (barres > 0) {
            usleep(20000);
            barres = servir_barres(root);
        }
        /* Les barres ont fini de lire leur commande : la page est réécrite avec
         * leurs vraies valeurs. Écrite d'abord sans elles (promesse ci-dessus,
         * une barre à 100 % peut faire sortir par « exit:… »), elle montrait
         * toujours 0 % — « Progression (doit afficher 65 %) » vide dans la
         * galerie du site. */
        if (!lancee && a_des_barres(root))
            render_html_export(root, title);
        minuteries_une_fois(tic);
    }

    /* 3) Repli si aucun timer n'a fait sortir le programme : mêmes lignes
     * que les autres backends dans ce cas (variables + EXIT="abort"). */
    print_variables(NULL);
    printf("EXIT=\"abort\"\n");
    g_setenv("EXIT", "abort", TRUE);   /* --do lit EXIT dans l'environnement (manuel §3.2) : il n'y était pas. */
    fflush(stdout);
    exit(EXIT_SUCCESS);
}

void sermo_be_app_quit(void)
{ /* pas de boucle à interrompre */ }
