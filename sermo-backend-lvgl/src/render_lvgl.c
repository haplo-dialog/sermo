/* SPDX-License-Identifier: GPL-2.0-or-later */
/*
 * Copyright (C) 2026 S. Cage
 * haplo-dialog <devel@haplo-dialog.fr>
 */
/* render_lvgl.c — rendu + boucle d'évènements du backend LVGL.
 *
 * Même modèle que les ports neutres sdl3 et ncurses : le cœur construit un
 * arbre de WidgetNode (parse, variables, actions, minuteries) ; ce fichier
 * (a) fabrique un objet LVGL par nœud, (b) recopie dans le nœud ce que
 * l'utilisateur change (texte tapé, case cochée, ligne choisie…) — c'est le
 * nœud que le cœur exporte —, (c) route les évènements vers le cœur
 * (jouer_actions / execute_action), (d) remet à l'écran ce que le cœur change
 * (refresh:, clear:…) à chaque tour de boucle.
 *
 * Deux chemins :
 *  - SANS ÉCRAN (SERMO_LVGL_BATCH=1, ou pas de DISPLAY) : n'ouvre rien ;
 *    exécute la logique (listes, barres, minuteries → actions ; un « exit:… »
 *    fait exporter et sortir le cœur). Même conduite que ncurses.
 *  - FENÊTRE : LVGL v9.6.0 embarqué (tiers/lvgl, réglage ../lv_conf.h), par
 *    Wayland si WAYLAND_DISPLAY est posé, sinon par X11 — l'ordre de GTK.
 *    SERMO_LVGL_X11=1 force X11 (XWayland).
 *  - FRAMEBUFFER (écran sans serveur d'affichage : console, borne) : SEULEMENT
 *    sur demande, SERMO_LVGL_FBDEV=/dev/fbN ; entrées par SERMO_LVGL_EVDEV=
 *    /dev/input/eventN, ou « auto » (tous les périphériques lisibles). Jamais
 *    choisi tout seul : un script lancé sans écran (cron, ssh) dessinerait
 *    sur la console de la machine.
 *
 * Police : LVGL n'a que ses Montserrat, sans « é », « à », « ç ». On charge une
 * TTF du système (DejaVu Sans) ; à défaut, Montserrat 14.
 */
#include "lvgl.h"
#if LV_USE_THORVG
#include "src/libs/thorvg/thorvg_capi.h"   /* lecteur SVG de ThorVG (icônes) */
#endif

#include <X11/Xlib.h>
#include <X11/Xatom.h>
#include <X11/Xutil.h>
#include <X11/Xlibint.h>   /* resource_base/resource_mask : reconnaître NOTRE fenêtre */
#include <png.h>
#include <dirent.h>
#include <langinfo.h>
#include <limits.h>
#include <sys/stat.h>
#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <time.h>
#include <unistd.h>

#include "render_lvgl.h"
#include "dialog_state.h"
#include "variables.h"
#include "actions.h"
#include "attributes.h"
#include "tag_attributes.h"
#include "sermo_progress.h"
#include "sermo_open_uri.h"
#include "jouer_actions.h"

/* Fourni par le cœur (actions.c), non exporté par ses en-têtes — comme dans sdl3. */
void action_exitprogram(GtkWidget *widget, char *string);
/* --render-png FILE : chemin du PNG à produire (option globale, gtkdialog.c). */
extern char *option_render_png;

/* ═══════════════════════════════════════════════════════════════════════════
 *  Logique commune (sans écran et fenêtre) — reprise du port ncurses
 * ═══════════════════════════════════════════════════════════════════════════ */
/* Joue les actions d'un nœud : toutes, chacune avec sa fonction, quand le
 * créateur a gardé le jeu d'attributs ; sinon l'unique commande du nœud. */
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
    if (n->lv) {
        if (n->state.toggle.active) lv_obj_add_state((lv_obj_t *)n->lv, LV_STATE_CHECKED);
        else                        lv_obj_remove_state((lv_obj_t *)n->lv, LV_STATE_CHECKED);
    }
        if (n->parent)
            for (int i = 0; i < n->parent->child_count; i++) {
                WidgetNode *f = n->parent->children[i];
                if (f == n || f->type != WT_RADIOBUTTON || !f->state.toggle.active) continue;
                f->state.toggle.active = FALSE;
                if (f->lv) lv_obj_remove_state((lv_obj_t *)f->lv, LV_STATE_CHECKED);
                bascule_jouer(f, FALSE);
            }
        bascule_jouer(n, TRUE);
        return;
    }
    n->state.toggle.active = !n->state.toggle.active;
    if (n->lv) {
        if (n->state.toggle.active) lv_obj_add_state((lv_obj_t *)n->lv, LV_STATE_CHECKED);
        else                        lv_obj_remove_state((lv_obj_t *)n->lv, LV_STATE_CHECKED);
    }
    bascule_jouer(n, n->state.toggle.active);
}

/* Crochet du contrat : « activate:NOM » bascule les widgets à bascule (le shim
 * fait de gtk_widget_activate() un FALSE). La case affichée suit l'état. */
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

/* <list> : joue ses actions (signal « selection-changed » de l'étalon). Seule
 * une vraie <list> garde son jeu d'actions ; un <tree> (WT_LIST aussi) non. */
static void liste_selection_changee(WidgetNode *n)
{
    if (n && n->type == WT_LIST && n->actions_attr)
        declencher(n, "selection-changed", "row-selected");
}

/* La sélection posée au chargement est jouée une fois, l'arbre entier
 * construit : les variables que l'action vise existent alors. */
static void listes_chargees(WidgetNode *n)
{
    if (!n) return;
    if (n->type == WT_LIST && n->state.list.selected_index >= 0)
        liste_selection_changee(n);
    for (int i = 0; i < n->child_count; i++) listes_chargees(n->children[i]);
}

static void fire_timers(WidgetNode *node)
{
    if (!node) return;
    if (node->type == WT_TIMER)
        declencher(node, "tick", NULL);
    for (int i = 0; i < node->child_count; i++) fire_timers(node->children[i]);
}

/* Barres de progression : relever ce que leurs commandes ont écrit.
 * Rend le nombre de lectures encore ouvertes. */
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

static int export_abort(void)
{
    variables_export_all();
    printf("EXIT=\"abort\"\n");
    g_setenv("EXIT", "abort", TRUE);   /* --do lit EXIT dans l'environnement (manuel §3.2) : il n'y était pas. */
    fflush(stdout);
    return 0;
}

static int run_headless(WidgetNode *root)
{
    listes_chargees(root);  /* un « exit:… » peut sortir ici, via le cœur */
    while (servir_barres(root) > 0)
        usleep(20000);
    fire_timers(root);      /* un « exit:… » sort ici via action_exitprogram */
    return export_abort();
}

/* ═══════════════════════════════════════════════════════════════════════════
 *  Fenêtre LVGL
 * ═══════════════════════════════════════════════════════════════════════════ */
static const lv_font_t *g_police;
static volatile int     g_fermee;     /* la fenêtre a été fermée par le gestionnaire */
static int              g_mesure;     /* construction pour mesurer : 1 = tailles naturelles,
                                       * 2 = règles de largeur à la largeur g_mesure_l */
static int              g_mesure_l;

static const lv_font_t *police_systeme(int px)
{
    static const char *candidates[] = {
        "/usr/share/fonts/truetype/dejavu/DejaVuSans.ttf",
        "/usr/share/fonts/TTF/DejaVuSans.ttf",
        "/usr/share/fonts/dejavu/DejaVuSans.ttf",
        NULL
    };
    for (int i = 0; candidates[i]; i++) {
        if (access(candidates[i], R_OK) != 0) continue;
        char chemin[512];
        snprintf(chemin, sizeof chemin, "A:%s", candidates[i]);
        lv_font_t *f = lv_tiny_ttf_create_file(chemin, px);
        /* Les symboles de LVGL (flèche du menu déroulant, coche…) vivent dans
         * les Montserrat embarquées, pas dans la TTF : repli sur elles. */
        if (f) { f->fallback = &lv_font_montserrat_14; return f; }
    }
    return &lv_font_montserrat_14;
}

/* ═══════════════════════════════════════════════════════════════════════════
 *  Dialogues : fichier, couleur, police
 *
 *  Une fenêtre modale dessinée par-dessus le dialogue (calque du haut de LVGL) :
 *  un voile qui bloque le reste, une carte au centre. Le contexte de chaque
 *  dialogue est libéré avec le voile (LV_EVENT_DELETE).
 * ═══════════════════════════════════════════════════════════════════════════ */
/* Tête commune des contextes : le nœud, puis le voile. */
typedef struct { WidgetNode *n; lv_obj_t *voile; } CtxTete;

static void voile_libere(lv_event_t *e)
{
    free(lv_event_get_user_data(e));
}

/* Ouvre le voile et rend la carte, en colonne, où poser le contenu. */
static lv_obj_t *modale_ouvrir(const char *titre, void *ctx, lv_obj_t **voile)
{
    lv_obj_t *v = lv_obj_create(lv_layer_top());
    lv_obj_remove_style_all(v);
    lv_obj_set_size(v, lv_pct(100), lv_pct(100));
    lv_obj_set_style_bg_color(v, lv_color_black(), 0);
    lv_obj_set_style_bg_opa(v, LV_OPA_50, 0);
    lv_obj_set_clickable(v, true);                      /* le dialogue dessous ne reçoit plus rien */
    if (ctx) lv_obj_add_event_cb(v, voile_libere, LV_EVENT_DELETE, ctx);
    lv_obj_t *c = lv_obj_create(v);
    lv_obj_set_size(c, lv_pct(90), lv_pct(90));
    lv_obj_center(c);
    lv_obj_set_flex_flow(c, LV_FLEX_FLOW_COLUMN);
    lv_obj_set_style_pad_gap(c, 6, 0);
    lv_label_set_text(lv_label_create(c), titre);
    *voile = v;
    return c;
}

static lv_obj_t *modale_rangee_boutons(lv_obj_t *carte)
{
    lv_obj_t *r = lv_obj_create(carte);
    lv_obj_remove_style_all(r);
    lv_obj_set_size(r, lv_pct(100), LV_SIZE_CONTENT);
    lv_obj_set_flex_flow(r, LV_FLEX_FLOW_ROW);
    lv_obj_set_flex_align(r, LV_FLEX_ALIGN_END, LV_FLEX_ALIGN_CENTER, LV_FLEX_ALIGN_CENTER);
    lv_obj_set_style_pad_gap(r, 6, 0);
    return r;
}

static lv_obj_t *modale_bouton(lv_obj_t *rangee, const char *texte, lv_event_cb_t cb, void *ctx)
{
    lv_obj_t *b = lv_button_create(rangee);
    lv_label_set_text(lv_label_create(b), texte);
    lv_obj_add_event_cb(b, cb, LV_EVENT_CLICKED, ctx);
    return b;
}

/* Libellé du bouton d'un nœud (premier enfant étiquette du bouton LVGL). */
static void bouton_libelle(WidgetNode *n, const char *texte)
{
    lv_obj_t *b = (lv_obj_t *)n->lv;
    if (!b || lv_obj_get_child_count(b) < 1) return;
    lv_label_set_text(lv_obj_get_child(b, 0), texte);
}

/* ─── Fichier ─────────────────────────────────────────────────────────────
 * Comme le navigateur du port ncurses : un dossier et sa liste. Les dossiers
 * d'abord (suivis de « / »), puis les fichiers ; « .. » pour remonter ; les
 * noms cachés ne sont pas montrés. Clic sur un dossier : on y descend ; sur un
 * fichier : il est retenu, le dialogue se ferme. */
typedef struct {
    WidgetNode *n;
    lv_obj_t   *voile, *titre, *liste;
    char        dossier[PATH_MAX];
    char      **noms;
    int         nb;
} CtxFichier;

static void fichier_remplir(CtxFichier *c);

static void fichier_vider(CtxFichier *c)
{
    for (int i = 0; i < c->nb; i++) free(c->noms[i]);
    free(c->noms);
    c->noms = NULL; c->nb = 0;
}

static void fichier_voile_libere(lv_event_t *e)
{
    CtxFichier *c = (CtxFichier *)lv_event_get_user_data(e);
    fichier_vider(c);
    free(c);
}

static int fichier_comparer(const void *a, const void *b)
{
    const char *x = *(const char * const *)a, *y = *(const char * const *)b;
    size_t lx = strlen(x), ly = strlen(y);
    int dx = lx && x[lx - 1] == '/', dy = ly && y[ly - 1] == '/';
    if (dx != dy) return dy - dx;                 /* dossiers d'abord */
    return strcmp(x, y);
}

static void fichier_chemin(char *out, size_t n, const char *dossier, const char *nom)
{
    if (strcmp(dossier, "/") == 0) snprintf(out, n, "/%s", nom);
    else                           snprintf(out, n, "%s/%s", dossier, nom);
}

static void fichier_clic(lv_event_t *e)
{
    CtxFichier *c = (CtxFichier *)lv_event_get_user_data(e);
    int i = (int)(intptr_t)lv_obj_get_user_data(lv_event_get_target_obj(e));
    if (i < 0 || i >= c->nb) return;
    const char *nom = c->noms[i];
    size_t l = strlen(nom);
    if (strcmp(nom, "../") == 0) {
        char *barre = strrchr(c->dossier, '/');
        if (barre && barre != c->dossier) *barre = '\0';
        else snprintf(c->dossier, sizeof c->dossier, "/");
        fichier_remplir(c);
        return;
    }
    char plein[PATH_MAX];
    char propre[NAME_MAX + 1];
    snprintf(propre, sizeof propre, "%.*s", (int)(l && nom[l - 1] == '/' ? l - 1 : l), nom);
    fichier_chemin(plein, sizeof plein, c->dossier, propre);
    if (l && nom[l - 1] == '/') {
        snprintf(c->dossier, sizeof c->dossier, "%s", plein);
        fichier_remplir(c);
        return;
    }
    WidgetNode *n = c->n;
    snprintf(n->state.entry.buf, sizeof(n->state.entry.buf), "%s", plein);
    bouton_libelle(n, propre);
    lv_obj_delete_async(c->voile);
    if (n->action && *n->action) declencher(n, "file-set", "selection-changed");
}

static void fichier_annuler(lv_event_t *e)
{
    CtxFichier *c = (CtxFichier *)lv_event_get_user_data(e);
    lv_obj_delete_async(c->voile);
}

static void fichier_remplir(CtxFichier *c)
{
    fichier_vider(c);
    lv_obj_clean(c->liste);
    /* Un long chemin ne tient pas : on en montre la FIN, la partie utile. */
    size_t ld = strlen(c->dossier);
    if (ld > 40) lv_label_set_text_fmt(c->titre, "…%s", c->dossier + ld - 39);
    else         lv_label_set_text(c->titre, c->dossier);
    DIR *d = opendir(c->dossier);
    int cap = 0;
    if (strcmp(c->dossier, "/") != 0) {
        cap = 16; c->noms = malloc(sizeof(char *) * (size_t)cap);
        if (c->noms) c->noms[c->nb++] = strdup("../");
    }
    if (d) {
        struct dirent *de;
        while ((de = readdir(d))) {
            if (de->d_name[0] == '.') continue;   /* noms cachés */
            char plein[PATH_MAX], nom[NAME_MAX + 2];
            struct stat st;
            fichier_chemin(plein, sizeof plein, c->dossier, de->d_name);
            int dossier = stat(plein, &st) == 0 && S_ISDIR(st.st_mode);
            snprintf(nom, sizeof nom, "%s%s", de->d_name, dossier ? "/" : "");
            if (c->nb >= cap) {
                int ncap = cap ? cap * 2 : 16;
                char **t = realloc(c->noms, sizeof(char *) * (size_t)ncap);
                if (!t) break;
                c->noms = t; cap = ncap;
            }
            c->noms[c->nb++] = strdup(nom);
        }
        closedir(d);
    }
    int debut = (c->nb && strcmp(c->noms[0], "../") == 0) ? 1 : 0;
    if (c->nb - debut > 1)
        qsort(c->noms + debut, (size_t)(c->nb - debut), sizeof(char *), fichier_comparer);
    for (int i = 0; i < c->nb; i++) {
        lv_obj_t *b = lv_button_create(c->liste);
        lv_obj_set_width(b, lv_pct(100));
        lv_label_set_text(lv_label_create(b), c->noms[i]);
        lv_obj_set_user_data(b, (void *)(intptr_t)i);
        lv_obj_add_event_cb(b, fichier_clic, LV_EVENT_CLICKED, c);
    }
}

static void dialogue_fichier(WidgetNode *n)
{
    CtxFichier *c = calloc(1, sizeof *c);
    if (!c) return;
    c->n = n;
    /* Point de départ : le dossier du chemin retenu, sinon le dossier courant. */
    struct stat st;
    if (n->state.entry.buf[0]) {
        snprintf(c->dossier, sizeof c->dossier, "%s", n->state.entry.buf);
        if (!(stat(c->dossier, &st) == 0 && S_ISDIR(st.st_mode))) {
            char *barre = strrchr(c->dossier, '/');
            if (barre && barre != c->dossier) *barre = '\0';
            else if (barre) snprintf(c->dossier, sizeof c->dossier, "/");
            else c->dossier[0] = '\0';
        }
    }
    if (!c->dossier[0] || !(stat(c->dossier, &st) == 0 && S_ISDIR(st.st_mode))) {
        if (!getcwd(c->dossier, sizeof c->dossier)) snprintf(c->dossier, sizeof c->dossier, "/");
    }
    /* contexte NULL : c'est fichier_voile_libere qui libère, noms compris */
    lv_obj_t *carte = modale_ouvrir(n->label && *n->label ? n->label : "Choisir un fichier", NULL, &c->voile);
    lv_obj_add_event_cb(c->voile, fichier_voile_libere, LV_EVENT_DELETE, c);
    c->titre = lv_label_create(carte);
    c->liste = lv_obj_create(carte);
    lv_obj_set_width(c->liste, lv_pct(100));
    lv_obj_set_flex_grow(c->liste, 1);
    lv_obj_set_flex_flow(c->liste, LV_FLEX_FLOW_COLUMN);
    lv_obj_set_style_pad_gap(c->liste, 2, 0);
    modale_bouton(modale_rangee_boutons(carte), "Annuler", fichier_annuler, c);
    fichier_remplir(c);
}

/* ─── Couleur ───────────────────────────────────────────────────────────── */
typedef struct {
    WidgetNode *n;
    lv_obj_t   *voile, *apercu, *glissieres[3], *valeur;
} CtxCouleur;

static void couleur_apercu(CtxCouleur *c)
{
    int v[3];
    for (int i = 0; i < 3; i++) v[i] = (int)lv_slider_get_value(c->glissieres[i]);
    lv_obj_set_style_bg_color(c->apercu, lv_color_make((uint8_t)v[0], (uint8_t)v[1], (uint8_t)v[2]), 0);
    lv_label_set_text_fmt(c->valeur, "#%02x%02x%02x", v[0], v[1], v[2]);
}

static void couleur_glisse(lv_event_t *e)
{
    couleur_apercu((CtxCouleur *)lv_event_get_user_data(e));
}

static void couleur_ok(lv_event_t *e)
{
    CtxCouleur *c = (CtxCouleur *)lv_event_get_user_data(e);
    WidgetNode *n = c->n;
    n->state.color.r = (float)lv_slider_get_value(c->glissieres[0]) / 255.0f;
    n->state.color.g = (float)lv_slider_get_value(c->glissieres[1]) / 255.0f;
    n->state.color.b = (float)lv_slider_get_value(c->glissieres[2]) / 255.0f;
    if (n->lv) {
        lv_color_t col = lv_color_make((uint8_t)lv_slider_get_value(c->glissieres[0]),
                                       (uint8_t)lv_slider_get_value(c->glissieres[1]),
                                       (uint8_t)lv_slider_get_value(c->glissieres[2]));
        lv_obj_set_style_bg_color((lv_obj_t *)n->lv, col, 0);
        lv_obj_set_style_text_color((lv_obj_t *)n->lv,
            lv_color_luminance(col) > 140 ? lv_color_black() : lv_color_white(), 0);
    }
    lv_obj_delete_async(c->voile);
    if (n->action && *n->action) declencher(n, "color-set", NULL);
}

static void modale_annuler(lv_event_t *e)
{
    CtxTete *c = (CtxTete *)lv_event_get_user_data(e);   /* CtxCouleur, CtxPolice */
    lv_obj_delete_async(c->voile);
}

static void dialogue_couleur(WidgetNode *n)
{
    static const char *noms[3] = { "Rouge", "Vert", "Bleu" };
    CtxCouleur *c = calloc(1, sizeof *c);
    if (!c) return;
    c->n = n;
    lv_obj_t *carte = modale_ouvrir(n->label && *n->label ? n->label : "Choisir une couleur", c, &c->voile);
    float comp[3] = { n->state.color.r, n->state.color.g, n->state.color.b };
    for (int i = 0; i < 3; i++) {
        lv_label_set_text(lv_label_create(carte), noms[i]);
        c->glissieres[i] = lv_slider_create(carte);
        lv_obj_set_width(c->glissieres[i], lv_pct(95));
        lv_slider_set_range(c->glissieres[i], 0, 255);
        lv_slider_set_value(c->glissieres[i], (int32_t)(comp[i] * 255.0f + 0.5f), LV_ANIM_OFF);
        lv_obj_add_event_cb(c->glissieres[i], couleur_glisse, LV_EVENT_VALUE_CHANGED, c);
    }
    c->apercu = lv_obj_create(carte);
    lv_obj_set_size(c->apercu, lv_pct(100), 40);
    lv_obj_set_style_bg_opa(c->apercu, LV_OPA_COVER, 0);
    c->valeur = lv_label_create(carte);
    lv_obj_t *r = modale_rangee_boutons(carte);
    modale_bouton(r, "Annuler", modale_annuler, c);
    modale_bouton(r, "OK", couleur_ok, c);
    couleur_apercu(c);
}

/* ─── Police ────────────────────────────────────────────────────────────
 * Le cœur neutre garde la police dans le libellé du nœud (« Sans 12 »,
 * widget_fontbutton.c). Trois familles génériques et une taille : ce qu'un
 * script peut passer tel quel à une application. */
typedef struct {
    WidgetNode *n;
    lv_obj_t   *voile, *famille, *taille;
} CtxPolice;

static void police_ok(lv_event_t *e)
{
    CtxPolice *c = (CtxPolice *)lv_event_get_user_data(e);
    char fam[32], tai[8], tout[48];
    lv_dropdown_get_selected_str(c->famille, fam, sizeof fam);
    lv_dropdown_get_selected_str(c->taille, tai, sizeof tai);
    snprintf(tout, sizeof tout, "%s %s", fam, tai);
    free(c->n->label);
    c->n->label = strdup(tout);
    bouton_libelle(c->n, tout);
    lv_obj_delete_async(c->voile);
    if (c->n->action && *c->n->action) declencher(c->n, "font-set", NULL);
}

static void dialogue_police(WidgetNode *n)
{
    CtxPolice *c = calloc(1, sizeof *c);
    if (!c) return;
    c->n = n;
    lv_obj_t *carte = modale_ouvrir("Choisir une police", c, &c->voile);
    c->famille = lv_dropdown_create(carte);
    lv_dropdown_set_options(c->famille, "Sans\nSerif\nMonospace");
    c->taille = lv_dropdown_create(carte);
    lv_dropdown_set_options(c->taille, "8\n9\n10\n11\n12\n14\n16\n18\n20\n24");
    /* Pré-choix d'après la police courante (« Famille Taille »). */
    const char *cour = n->label ? n->label : "Sans 12";
    const char *esp = strrchr(cour, ' ');
    if (esp) {
        char fam[32];
        snprintf(fam, sizeof fam, "%.*s", (int)(esp - cour), cour);
        int32_t i = lv_dropdown_get_option_index(c->famille, fam);
        if (i >= 0) lv_dropdown_set_selected(c->famille, (uint32_t)i);
        i = lv_dropdown_get_option_index(c->taille, esp + 1);
        if (i >= 0) lv_dropdown_set_selected(c->taille, (uint32_t)i);
    } else {
        lv_dropdown_set_selected(c->taille, 4);   /* 12 */
    }
    lv_obj_t *r = modale_rangee_boutons(carte);
    modale_bouton(r, "Annuler", modale_annuler, c);
    modale_bouton(r, "OK", police_ok, c);
}

/* ─── Évènements : l'utilisateur change un widget ─────────────────────────── */
static void bouton_clic(lv_event_t *e)
{
    WidgetNode *n = (WidgetNode *)lv_event_get_user_data(e);
    switch (n->type) {
    case WT_FILECHOOSER: dialogue_fichier(n); return;
    case WT_COLORBUTTON: dialogue_couleur(n); return;
    case WT_FONTBUTTON:  dialogue_police(n);  return;
    case WT_LINKBUTTON:
        if (n->state.entry.buf[0]) sermo_open_uri(n->state.entry.buf);
        break;
    default:
        /* un <button> marqué __fontbutton__ (widget_fontbutton.c) est une police */
        if (n->var_name && strcmp(n->var_name, "__fontbutton__") == 0) { dialogue_police(n); return; }
        break;
    }
    if (n->action && *n->action) {
        declencher(n, n->type == WT_MENUITEM ? "activate" : "clicked", NULL);   /* exit:… peut sortir ici */
    } else if (n->type == WT_BUTTON
               && !(n->var_name && strcmp(n->var_name, "__fontbutton__") == 0)) {
        /* SEUL un vrai <button> nu ferme le dialogue : un colorbutton, un
         * filechooser, un lien ou un menu cliqué sans action ne sortent pas
         * chez l'étalon (garde_clic_widgets l'a vu : colorbutton fermait). */
        /* gtkdialog : un bouton nu (<button ok>, <button cancel>…) ferme le
         * dialogue, EXIT = valeur de famille sinon libellé — comme sdl3/ncurses. */
        action_exitprogram((GtkWidget *)n,
            (char *)(n->tooltip ? n->tooltip
                     : (n->label && n->label[0] ? n->label : "OK")));
    }
}

static void bascule_change(lv_event_t *e)
{
    WidgetNode *n = (WidgetNode *)lv_event_get_user_data(e);
    lv_obj_t   *o = lv_event_get_target_obj(e);
    gboolean    actif = lv_obj_has_state(o, LV_STATE_CHECKED);
    /* <radiobutton> : un clic ne décoche pas le bouton choisi, et en choisir un
     * décoche ses frères de la même boîte — le groupe de l'étalon. LVGL n'a pas
     * de bouton radio : c'est une case ronde, qui se comportait en case à cocher. */
    if (n->type == WT_RADIOBUTTON) {
        if (!actif) { lv_obj_add_state(o, LV_STATE_CHECKED); return; }
        if (n->parent)
            for (int i = 0; i < n->parent->child_count; i++) {
                WidgetNode *f = n->parent->children[i];
                if (f == n || f->type != WT_RADIOBUTTON || !f->state.toggle.active) continue;
                f->state.toggle.active = FALSE;
                if (f->lv) lv_obj_remove_state((lv_obj_t *)f->lv, LV_STATE_CHECKED);
                bascule_jouer(f, FALSE);   /* le frère décoché joue « if false » */
            }
    }
    if (actif == n->state.toggle.active) return;
    n->state.toggle.active = actif;
    bascule_jouer(n, actif);   /* exit:… peut sortir ici */
}

static void champ_change(lv_event_t *e)
{
    WidgetNode *n = (WidgetNode *)lv_event_get_user_data(e);
    const char *t = lv_textarea_get_text(lv_event_get_target_obj(e));
    if (n->type == WT_EDIT) {
        free(n->state.text.content);
        n->state.text.content = strdup(t ? t : "");
        n->state.text.len = (int)strlen(n->state.text.content);
        n->state.text.cap = n->state.text.len + 1;
    } else if (n->type == WT_SEARCHENTRY) {
        snprintf(n->state.list.filter, sizeof(n->state.list.filter), "%s", t ? t : "");
    } else {
        snprintf(n->state.entry.buf, sizeof(n->state.entry.buf), "%s", t ? t : "");
    }
}

static void liste_clic(lv_event_t *e)
{
    WidgetNode *n = (WidgetNode *)lv_event_get_user_data(e);
    lv_obj_t   *b = lv_event_get_target_obj(e);
    int         i = (int)(intptr_t)lv_obj_get_user_data(b);
    /* Une seule ligne choisie : chaque ligne est cochable, on décoche les autres
     * (jusqu'à la 2.8.0, deux lignes restaient marquées). */
    lv_obj_t *l = lv_obj_get_parent(b);
    for (uint32_t k = 0; k < lv_obj_get_child_count(l); k++) {
        lv_obj_t *r = lv_obj_get_child(l, (int32_t)k);
        if (r != b) lv_obj_remove_state(r, LV_STATE_CHECKED);
    }
    lv_obj_add_state(b, LV_STATE_CHECKED);
    if (i == n->state.list.selected_index) return;
    n->state.list.selected_index = i;
    liste_selection_changee(n);
}

static void deroulant_change(lv_event_t *e)
{
    WidgetNode *n = (WidgetNode *)lv_event_get_user_data(e);
    int i = (int)lv_dropdown_get_selected(lv_event_get_target_obj(e));
    if (i == n->state.list.selected_index) return;
    n->state.list.selected_index = i;
    if (n->action && *n->action) declencher(n, "changed", NULL);
}

static void glissiere_change(lv_event_t *e)
{
    WidgetNode *n = (WidgetNode *)lv_event_get_user_data(e);
    lv_obj_t   *o = lv_event_get_target_obj(e);
    double pas  = n->state.scale.step > 0 ? n->state.scale.step : 1;
    n->state.scale.value = n->state.scale.min + lv_slider_get_value(o) * pas;
    if (n->action && *n->action) declencher(n, "value-changed", NULL);
}

/* <spinbutton> : un champ numérique entre « − » et « + ». Pas de lv_spinbox : il
 * complète toujours par des zéros (« 030 » pour 30, mesuré), là où les autres
 * ports écrivent « 30 ». La valeur vit dans le cœur (state.spin) ; le champ
 * l'affiche avec ses décimales (digits). */
static void spin_afficher(WidgetNode *n, lv_obj_t *champ)
{
    char t[64];
    snprintf(t, sizeof t, "%.*f", n->state.spin.digits, n->state.spin.value);
    for (char *c = t; *c; c++) if (*c == ',') *c = '.';   /* locale : point décimal, comme l'export */
    if (strcmp(lv_textarea_get_text(champ), t) != 0) lv_textarea_set_text(champ, t);
}
static void spin_poser(WidgetNode *n, double v)
{
    if (v < n->state.spin.min) v = n->state.spin.min;
    if (v > n->state.spin.max) v = n->state.spin.max;
    if (v != n->state.spin.value) {
        n->state.spin.value = v;
        if (n->action && *n->action) declencher(n, "value-changed", NULL);
    }
}
static void spin_saisie(lv_event_t *e)
{
    WidgetNode *n = (WidgetNode *)lv_event_get_user_data(e);
    const char *t = lv_textarea_get_text(lv_event_get_target_obj(e));
    char *fin = NULL;
    double v = g_ascii_strtod(t, &fin);
    if (fin != t) spin_poser(n, v);
}
static void spin_pas(lv_event_t *e, int sens)
{
    WidgetNode *n = (WidgetNode *)lv_event_get_user_data(e);
    double pas = 1;
    for (int i = 0; i < n->state.spin.digits; i++) pas /= 10;
    spin_poser(n, n->state.spin.value + sens * pas);
    spin_afficher(n, lv_obj_get_child((lv_obj_t *)n->lv, 1));
}
static void spin_moins(lv_event_t *e) { spin_pas(e, -1); }
static void spin_plus(lv_event_t *e)  { spin_pas(e, +1); }

/* <colorbutton> : le bouton montre la couleur retenue, texte clair ou foncé
 * selon la clarté du fond pour rester lisible. À la construction et à chaque
 * synchronisation : un « refresh » rejoue l'<input> (widget_colorbutton.c,
 * 2026-10-13). Composantes arrondies (× 255 + 0,5), comme l'export. */
static void couleur_peindre(WidgetNode *n, lv_obj_t *o)
{
    lv_color_t c = lv_color_make((uint8_t)(n->state.color.r * 255.0f + 0.5f),
                                 (uint8_t)(n->state.color.g * 255.0f + 0.5f),
                                 (uint8_t)(n->state.color.b * 255.0f + 0.5f));
    if (lv_color_eq(lv_obj_get_style_bg_color(o, 0), c)) return;
    lv_obj_set_style_bg_color(o, c, 0);
    lv_obj_set_style_text_color(o, lv_color_luminance(c) > 140 ? lv_color_black() : lv_color_white(), 0);
}

/* <expander> : titre et visibilité du corps suivent state.expander.open. */
static void expander_titre(WidgetNode *n, lv_obj_t *titre, lv_obj_t *corps)
{
    char t[512];
    snprintf(t, sizeof t, "%s  %s", n->state.expander.open ? LV_SYMBOL_DOWN : LV_SYMBOL_RIGHT,
             n->label ? n->label : "");
    lv_label_set_text(titre, t);
    lv_obj_set_hidden(corps, !n->state.expander.open);
}
static void expander_clic(lv_event_t *e)
{
    WidgetNode *n = (WidgetNode *)lv_event_get_user_data(e);
    lv_obj_t   *o = (lv_obj_t *)n->lv;
    n->state.expander.open = !n->state.expander.open;
    expander_titre(n, lv_obj_get_child(o, 0), lv_obj_get_child(o, 1));
    if (n->action && *n->action) declencher(n, "activate", NULL);
}

/* <stack> et <wizard> : seule la page courante (state.notebook.current_tab)
 * se montre. Jusqu'à la 2.8.0, toutes les pages s'empilaient à l'écran. */
static void pages_montrer(WidgetNode *n, lv_obj_t *pages)
{
    int cur = n->state.notebook.current_tab;
    if (cur < 0 || cur >= n->child_count) cur = 0;
    for (int i = 0; i < n->child_count; i++)
        if (n->children[i]->lv) lv_obj_set_hidden((lv_obj_t *)n->children[i]->lv, i != cur);
    (void)pages;
}
static void pile_choisir(lv_event_t *e)
{
    lv_obj_t   *b = lv_event_get_target_obj(e);
    WidgetNode *n = (WidgetNode *)lv_event_get_user_data(e);
    n->state.notebook.current_tab = (int)(intptr_t)lv_obj_get_user_data(b);
    pages_montrer(n, NULL);
}
static void assistant_etat(WidgetNode *n)
{
    lv_obj_t *o = (lv_obj_t *)n->lv;
    lv_obj_t *barre = lv_obj_get_child(o, (int32_t)lv_obj_get_child_count(o) - 1);
    int cur = n->state.notebook.current_tab;
    char t[64];
    snprintf(t, sizeof t, "étape %d/%d", cur + 1, n->child_count);
    lv_label_set_text(lv_obj_get_child(barre, 3), t);
    if (cur > 0) lv_obj_remove_state(lv_obj_get_child(barre, 0), LV_STATE_DISABLED);
    else         lv_obj_add_state(lv_obj_get_child(barre, 0), LV_STATE_DISABLED);
    if (cur < n->child_count - 1) lv_obj_remove_state(lv_obj_get_child(barre, 1), LV_STATE_DISABLED);
    else                          lv_obj_add_state(lv_obj_get_child(barre, 1), LV_STATE_DISABLED);
    pages_montrer(n, NULL);
}
static void assistant_precedent(lv_event_t *e)
{
    WidgetNode *n = (WidgetNode *)lv_event_get_user_data(e);
    if (n->state.notebook.current_tab > 0) n->state.notebook.current_tab--;
    assistant_etat(n);
}
static void assistant_suivant(lv_event_t *e)
{
    WidgetNode *n = (WidgetNode *)lv_event_get_user_data(e);
    if (n->state.notebook.current_tab < n->child_count - 1) n->state.notebook.current_tab++;
    assistant_etat(n);
}
static void assistant_terminer(lv_event_t *e)
{
    WidgetNode *n = (WidgetNode *)lv_event_get_user_data(e);
    if (n->actions_attr) declencher(n, NULL, NULL);   /* comme sdl3 : les actions du <wizard> */
}

/* <menubutton> : une liste déroulante qui garde son titre ; un choix retient
 * le libellé (la variable) et joue l'action du <menuitem>, comme sdl3. */
static void menu_choisi(lv_event_t *e)
{
    WidgetNode *n = (WidgetNode *)lv_event_get_user_data(e);
    uint32_t    i = lv_dropdown_get_selected(lv_event_get_target_obj(e));
    int k = 0;
    for (int c = 0; c < n->child_count; c++) {
        WidgetNode *it = n->children[c];
        if (it->type != WT_MENUITEM || !it->label || !it->label[0]) continue;
        if ((uint32_t)k++ != i) continue;
        snprintf(n->state.entry.buf, sizeof(n->state.entry.buf), "%s", it->label);
        if (it->action && it->action[0]) declencher(it, "activate", NULL);
        break;
    }
}

static void calendrier_change(lv_event_t *e)
{
    WidgetNode *n = (WidgetNode *)lv_event_get_user_data(e);
    lv_calendar_date_t d;
    if (lv_calendar_get_pressed_date(lv_event_get_current_target_obj(e), &d) != LV_RESULT_OK) return;
    n->state.calendar.year = d.year; n->state.calendar.month = d.month; n->state.calendar.day = d.day;
    lv_calendar_set_highlighted_dates(lv_event_get_current_target_obj(e), NULL, 0);
    lv_calendar_set_today_date(lv_event_get_current_target_obj(e), d.year, d.month, d.day);
    if (n->action && *n->action) declencher(n, "day-selected", NULL);
}

/* En-tête du calendrier : « ‹ mai 2026 › » dans la langue du poste (strftime) ;
 * celui de LVGL écrit les mois en anglais. */
static void calendrier_titre(lv_obj_t *cal, lv_obj_t *titre)
{
    const lv_calendar_date_t *m = lv_calendar_get_showed_date(cal);
    struct tm t = { .tm_year = m->year - 1900, .tm_mon = m->month - 1, .tm_mday = 1 };
    char buf[64];
    if (!strftime(buf, sizeof buf, "%B %Y", &t)) snprintf(buf, sizeof buf, "%d/%d", m->month, m->year);
    lv_label_set_text(titre, buf);
}
static void calendrier_mois(lv_event_t *e, int sens)
{
    lv_obj_t *barre = lv_obj_get_parent(lv_event_get_target_obj(e));
    lv_obj_t *cal   = (lv_obj_t *)lv_event_get_user_data(e);
    const lv_calendar_date_t *m = lv_calendar_get_showed_date(cal);
    int an = m->year, mois = m->month + sens;
    if (mois < 1)  { mois = 12; an--; }
    if (mois > 12) { mois = 1;  an++; }
    lv_calendar_set_month_shown(cal, (uint32_t)an, (uint32_t)mois);
    calendrier_titre(cal, lv_obj_get_child(barre, 1));
}
static void calendrier_avant(lv_event_t *e) { calendrier_mois(e, -1); }
static void calendrier_apres(lv_event_t *e) { calendrier_mois(e, +1); }

static void onglet_change(lv_event_t *e)
{
    WidgetNode *n = (WidgetNode *)lv_event_get_user_data(e);
    n->state.notebook.current_tab = (int)lv_tabview_get_tab_active(lv_event_get_target_obj(e));
}

static void minuterie_tic(lv_timer_t *t)
{
    WidgetNode *n = (WidgetNode *)lv_timer_get_user_data(t);
    declencher(n, "tick", NULL);                 /* exit:… sort ici */
}

static void ecran_ferme(lv_event_t *e)
{
    (void)e;
    g_fermee = 1;
}

/* ─── Construction : un objet LVGL par nœud ───────────────────────────────── */
static lv_obj_t *boite(lv_obj_t *parent, lv_flex_flow_t sens)
{
    lv_obj_t *b = lv_obj_create(parent);
    lv_obj_remove_style_all(b);
    lv_obj_set_size(b, LV_SIZE_CONTENT, LV_SIZE_CONTENT);
    lv_obj_set_flex_flow(b, sens);
    lv_obj_set_style_pad_gap(b, 6, 0);
    /* Une boîte GTK ne défile jamais. Défilante, LVGL la faisait glisser vers
     * le champ qui prend le focus (mesuré : 18 px de côté sans aucun
     * débordement). Seul l'écran défile, en hauteur. */
    lv_obj_set_scrollable(b, false);
    return b;
}

static void construire(WidgetNode *n, lv_obj_t *parent);

/* <levelbar> : la valeur, ramenée entre min et max, en pour cent de la barre.
 * À la construction et à chaque synchronisation : un « refresh » rejoue
 * l'<input> (widget_levelbar.c) et la barre doit suivre. */
static void niveau_afficher(WidgetNode *n, lv_obj_t *o)
{
    double r = n->state.levelbar.max - n->state.levelbar.min;
    double f = r > 0 ? (n->state.levelbar.value - n->state.levelbar.min) / r : 0;
    if (f < 0) f = 0;
    if (f > 1) f = 1;
    lv_bar_set_value(o, (int32_t)(f * 100.0 + 0.5), LV_ANIM_OFF);
}

/* Boîtes qui empilent leurs enfants en colonne. Chez l'étalon (GtkBox
 * verticale), chaque enfant y prend TOUTE la largeur ; le texte d'un <text>
 * y est centré (xalign 0.5). */
static int colonne(const WidgetNode *p)
{
    if (!p) return 0;
    switch (p->type) {
    case WT_WINDOW: case WT_VBOX: case WT_FRAME: case WT_NOTEBOOK: case WT_EXPANDER:
    case WT_EVENTBOX: case WT_STACK: case WT_WIZARD: case WT_REVEALER:
    case WT_OVERLAY: case WT_ASPECTFRAME: case WT_PANED:
        return 1;
    default:
        return 0;
    }
}

/* Une colonne a-t-elle une largeur ? Oui si elle remonte jusqu'à la fenêtre par
 * des colonnes, ou si on lui en donne une. Non dès qu'elle est dans une rangée :
 * taillée sur son contenu, ses enfants à 100 % tomberaient à 0 px (mesuré :
 * <hbox><vbox><text> n'affichait pas son texte). */
static int rangee(const WidgetNode *p);

/* Le défaut de gtkdialog (widget_hbox.c / widget_vbox.c de gtk3sermo) quand
 * rien n'est dit, pour space-expand comme pour space-fill : <edit>, <frame>,
 * fenêtre défilante, et <entry> dans une rangée, <paned> dans une colonne. */
static int etire_par_defaut(const WidgetNode *n)
{
    if (n->defile || n->type == WT_EDIT || n->type == WT_FRAME) return 1;
    return rangee(n->parent) ? n->type == WT_ENTRY : n->type == WT_PANED;
}

/* Une <hbox>/<vbox> homogeneous="true" : chaque enfant a une case de même
 * taille, et l'ensemble occupe toute la boîte. */
static int homogene(const WidgetNode *p)
{
    return p && (p->type == WT_HBOX || p->type == WT_VBOX) && p->homogeneous;
}

/* La fenêtre et la page d'onglet n'ont qu'un enfant, qui prend toute leur
 * place (GtkWindow, page de GtkNotebook) : t4, F1 occupe toute la hauteur. */
static int seul_enfant(const WidgetNode *p)
{
    return p && (p->type == WT_WINDOW || p->type == WT_NOTEBOOK);
}

/* S'étire-t-il ? space-expand (celui de la boîte, reporté d'un enfant à
 * l'autre : boite_repartir, widgets.c) ; sans réglage, le défaut du type.
 * L'enfant d'une fenêtre ou d'une page d'onglet prend toute la place ; dans
 * une boîte homogène, chaque enfant reçoit sa case. */
static int etire(const WidgetNode *n)
{
    if (!n->parent) return 0;
    if (seul_enfant(n->parent) || homogene(n->parent)) return 1;
    if (n->expand) return n->expand > 0;
    return etire_par_defaut(n);
}

/* Remplit-il la place qu'il reçoit (space-fill) ? Sinon il garde sa taille,
 * centré dans cette place (t3 : « Etire » au milieu de la place libre). */
static int remplit(const WidgetNode *n)
{
    if (!n->parent || seul_enfant(n->parent)) return 1;
    if (n->fill) return n->fill > 0;
    return etire_par_defaut(n);
}

/* Un enfant d'une <hbox>/<vbox> qui reçoit de la place sans la remplir est
 * posé dans une case transparente, qui prend la place et le centre. */
static int en_case(const WidgetNode *n)
{
    return n->parent && (n->parent->type == WT_HBOX || n->parent->type == WT_VBOX)
           && n->type != WT_TIMER && etire(n) && !remplit(n);
}

/* … ou dans une rangée de largeur connue, où il s'étire : sa largeur est alors
 * la place libre de la rangée (jusqu'à la 2.8.1, « inconnue » : les lignes du
 * cadre « Systeme » de system-tools restaient à gauche, ses cadres étroits). */
static int largeur_connue(const WidgetNode *c)
{
    for (; c; c = c->parent) {
        if (c->type == WT_WINDOW || c->width > 0 || !c->parent) return 1;
        if (!colonne(c->parent) && !(rangee(c->parent) && etire(c) && remplit(c))) return 0;
    }
    return 1;
}

/* Une boîte a-t-elle une hauteur ? Oui si on la lui donne, si c'est une page
 * d'onglet (le lv_tabview a une hauteur fixée), si elle s'étire dans une
 * colonne de hauteur connue, ou si elle remplit une rangée de hauteur connue. */
static int hauteur_connue(const WidgetNode *c)
{
    for (; c; c = c->parent) {
        if (c->height > 0) return 1;
        /* la fenêtre a la taille de l'écran — pas pendant la mesure, où elle
         * est taillée sur son contenu */
        if (c->type == WT_WINDOW) return !g_mesure;
        if (!c->parent) return 0;
        if (c->parent->type == WT_NOTEBOOK) return 1;
        if (rangee(c->parent)) continue;
        if (colonne(c->parent) && etire(c) && remplit(c)) continue;
        return 0;
    }
    return 0;
}

/* Ce que GTK dessine à sa hauteur propre, centré dans la place reçue : une
 * barre, un curseur, une jauge, un champ. LVGL, lui, le dessinerait sur toute
 * la hauteur (barre de 120 px de haut, mesuré sur 04-indicateurs) : ces
 * widgets ne s'étirent qu'en largeur. */
static int hauteur_propre(const WidgetNode *n)
{
    switch (n->type) {
    case WT_PROGRESSBAR: case WT_PULSE: case WT_HSCALE: case WT_LEVELBAR: case WT_SPINBUTTON:
    case WT_ENTRY: case WT_PASSWORD: case WT_SEARCHENTRY: case WT_COMBOBOX: case WT_SPINNER:
        return 1;
    default:
        return 0;
    }
}

/* Remplit la hauteur de sa rangée, comme dans GTK (une rangée donne toute sa
 * hauteur à ses enfants) : les conteneurs et ce qui s'étire. */
static int remplit_rangee(const WidgetNode *n)
{
    if (hauteur_propre(n)) return 0;
    switch (n->type) {
    case WT_FRAME: case WT_VBOX: case WT_HBOX: case WT_NOTEBOOK: case WT_EDIT:
    case WT_TREE: case WT_LIST: case WT_TABLE: case WT_PANED: case WT_EXPANDER:
    case WT_BUTTON:   /* t2 : OK / Cancel prennent la hauteur de leur rangée */
        return 1;
    default:
        return etire(n);
    }
}

static void construire_enfants(WidgetNode *n, lv_obj_t *o)
{
    for (int i = 0; i < n->child_count; i++) construire(n->children[i], o);
}

/* <list> et <tree> : une colonne de lignes cochables qui défile (lv_list est
 * déprécié en 9.6 au profit de ce montage). Un <tree> sépare ses colonnes par
 * « | » : chaque ligne devient une rangée de cellules, sous les en-têtes de
 * son <label> — jusqu'à la 2.8.0, la ligne brute « a|b|c » s'affichait. */
static int colonnes_compter(const char *t)
{
    int k = 1;
    for (; t && *t; t++) if (*t == '|') k++;
    return k;
}
static void cellules(lv_obj_t *rangee_o, const char *texte, int cols, const int32_t *large)
{
    gchar **c = g_strsplit(texte ? texte : "", "|", cols);
    int nb = (int)g_strv_length(c);
    for (int j = 0; j < cols; j++) {
        lv_obj_t *l = lv_label_create(rangee_o);
        lv_label_set_text(l, j < nb ? c[j] : "");
        lv_label_set_long_mode(l, LV_LABEL_LONG_MODE_DOTS);
        lv_obj_set_width(l, large[j]);
    }
    g_strfreev(c);
}
/* Largeur d'une colonne d'arbre : son plus long texte (en-tête compris). */
static void colonnes_mesurer(const WidgetNode *n, int cols, int32_t *large)
{
    for (int j = 0; j < cols; j++) large[j] = 24;
    for (int i = -1; i < n->state.list.item_count; i++) {
        const char *t = i < 0 ? n->label : n->state.list.items[i];
        if (!t) continue;
        gchar **c = g_strsplit(t, "|", cols);
        for (int j = 0; c[j] && j < cols; j++) {
            lv_point_t sz;
            lv_text_get_size(&sz, c[j], g_police, 0, 0, LV_COORD_MAX, LV_TEXT_FLAG_NONE);
            if (sz.x + 16 > large[j]) large[j] = sz.x + 16;
        }
        g_strfreev(c);
    }
}
static int liste_colonnes(const WidgetNode *n)
{
    int cols = 1;
    for (int i = 0; i < n->state.list.item_count; i++) {
        int k = colonnes_compter(n->state.list.items[i]);
        if (k > cols) cols = k;
    }
    return cols;
}
static int liste_entete(const WidgetNode *n)
{
    return liste_colonnes(n) > 1 && n->label && strchr(n->label, '|');
}
static void construire_liste(WidgetNode *n, lv_obj_t *parent)
{
    int cols = liste_colonnes(n);
    int32_t *large = g_new0(int32_t, cols), total = 0;
    colonnes_mesurer(n, cols, large);
    for (int j = 0; j < cols; j++) total += large[j];
    lv_obj_t *l = lv_obj_create(parent);
    lv_obj_set_size(l, n->width > 0 ? n->width : (cols > 1 ? total + 40 : 220),
                    n->height > 0 ? n->height : 140);
    /* Largeur naturelle : en pour-cent (largeur ou largeur minimale), ce
     * conteneur défilant tombait à 0 px ou n'en tenait pas compte (mesuré). */
    lv_obj_set_flex_flow(l, LV_FLEX_FLOW_COLUMN);
    lv_obj_set_style_pad_gap(l, 0, 0);
    lv_obj_set_style_pad_all(l, 4, 0);
    n->lv = l;
    if (liste_entete(n)) {
        lv_obj_t *t = boite(l, LV_FLEX_FLOW_ROW);
        lv_obj_set_style_pad_hor(t, 8, 0);
        lv_obj_set_style_text_color(t, lv_color_hex(0x555555), 0);   /* gris foncé : 7,5:1 sur blanc */
        lv_obj_set_style_pad_gap(t, 0, 0);
        cellules(t, n->label, cols, large);
    }
    for (int i = 0; i < n->state.list.item_count; i++) {
        lv_obj_t *b = lv_button_create(l);
        lv_obj_set_width(b, lv_pct(100));
        lv_obj_set_flex_flow(b, LV_FLEX_FLOW_ROW);
        lv_obj_set_style_pad_ver(b, 4, 0);
        lv_obj_set_style_pad_hor(b, 8, 0);
        lv_obj_set_style_radius(b, 0, 0);
        lv_obj_set_style_shadow_width(b, 0, 0);
        if (cols > 1) cellules(b, n->state.list.items[i], cols, large);
        else          lv_label_set_text(lv_label_create(b), n->state.list.items[i]);
        if (cols > 1) lv_obj_set_style_pad_gap(b, 0, 0);
        lv_obj_set_user_data(b, (void *)(intptr_t)i);
        lv_obj_set_checkable(b, true);
        /* Lignes comme l'étalon : fond blanc, texte noir. Ligne choisie : bleu
         * nuit #1A3A5C, texte blanc (le thème mettait du rouge, que rien ne
         * distingue du bleu pour un daltonien) — elle se lit par la clarté. */
        lv_obj_set_style_bg_color(b, lv_color_white(), 0);
        lv_obj_set_style_text_color(b, lv_color_black(), 0);
        lv_obj_set_style_bg_color(b, lv_color_hex(0x1A3A5C), LV_STATE_CHECKED);
        lv_obj_set_style_text_color(b, lv_color_white(), LV_STATE_CHECKED);
        if (i == n->state.list.selected_index) lv_obj_add_state(b, LV_STATE_CHECKED);
        lv_obj_add_event_cb(b, liste_clic, LV_EVENT_CLICKED, n);
    }
    g_free(large);
}

/* Fin commune à tous les nœuds : visibilité, pleine largeur, expansion. */
static void finir(WidgetNode *n, lv_obj_t *o);

/* <notebook> : de vrais onglets (lv_tabview). Une page par enfant, libellés de
 * tab-labels="A|B|C" (repli : numéros). Pendant la 1re mesure, les pages sont
 * posées l'une SUR l'autre dans une boîte sans mise en page : la boîte prend
 * la taille de la plus grande, qu'on retient pour la fenêtre. */
static void construire_onglets(WidgetNode *n, lv_obj_t *parent)
{
    const int barre = 36;
    if (g_mesure == 1) {
        /* Marge intérieure d'une page d'onglet, lue sur un onglet témoin
         * (elle vient du thème) : sans elle, la fenêtre coupait le contenu. */
        lv_obj_t *temoin = lv_tabview_create(parent);
        lv_obj_t *p0 = lv_tabview_add_tab(temoin, "x");
        int32_t ph = lv_obj_get_style_pad_left(p0, 0) + lv_obj_get_style_pad_right(p0, 0);
        int32_t pv = lv_obj_get_style_pad_top(p0, 0) + lv_obj_get_style_pad_bottom(p0, 0);
        lv_obj_delete(temoin);
        lv_obj_t *pile = lv_obj_create(parent);
        lv_obj_remove_style_all(pile);
        lv_obj_set_size(pile, LV_SIZE_CONTENT, LV_SIZE_CONTENT);
        lv_obj_set_style_pad_hor(pile, ph / 2, 0);
        lv_obj_set_style_pad_ver(pile, pv / 2, 0);
        lv_obj_set_scrollable(pile, false);
        for (int i = 0; i < n->child_count; i++) construire(n->children[i], pile);
        lv_obj_update_layout(pile);
        n->lv_mesure_h = lv_obj_get_height(pile) + barre;
        lv_obj_set_height(pile, n->lv_mesure_h);
        finir(n, pile);
        return;
    }
    lv_obj_t *tv = lv_tabview_create(parent);
    const int cote = n->state.notebook.side;   /* 0 haut, 1 gauche, 2 droite, 3 bas */
    if (cote == 1 || cote == 2) {
        /* Barre sur le côté : sa « taille » est sa LARGEUR — 36 px coupaient
         * les libellés (« ystem » pour « Systeme », jusqu'à la 2.8.1). Elle
         * prend la largeur du plus long libellé, plus la marge du bouton. */
        const lv_font_t *police = lv_obj_get_style_text_font(tv, 0);
        int32_t large = 0;
        for (int i = 0; i < n->child_count && i < n->state.notebook.tab_count; i++) {
            const char *t = n->state.notebook.tab_labels[i];
            if (!t) continue;
            lv_point_t sz;
            lv_text_get_size(&sz, t, police, 0, 0, LV_COORD_MAX, LV_TEXT_FLAG_NONE);
            if (sz.x > large) large = sz.x;
        }
        lv_tabview_set_tab_bar_size(tv, large > 0 ? large + 32 : 80);
        lv_tabview_set_tab_bar_position(tv, cote == 1 ? LV_DIR_LEFT : LV_DIR_RIGHT);
    } else {
        lv_tabview_set_tab_bar_size(tv, barre);
        if (cote == 3) lv_tabview_set_tab_bar_position(tv, LV_DIR_BOTTOM);
    }
    lv_obj_set_height(tv, n->height > 0 ? n->height
                          : (n->lv_mesure_h > 0 ? n->lv_mesure_h : 240));
    for (int i = 0; i < n->child_count; i++) {
        char num[16];
        const char *nom = (i < n->state.notebook.tab_count && n->state.notebook.tab_labels[i])
                          ? n->state.notebook.tab_labels[i] : (snprintf(num, sizeof num, "%d", i + 1), num);
        lv_obj_t *page = lv_tabview_add_tab(tv, nom);
        lv_obj_set_flex_flow(page, LV_FLEX_FLOW_COLUMN);
        lv_obj_set_style_pad_gap(page, 6, 0);
        construire(n->children[i], page);
    }
    if (cote == 1 || cote == 2) {
        /* Onglets à leur hauteur, empilés en haut, comme GTK : LVGL les étirait
         * sur toute la hauteur de la barre. */
        lv_obj_t *bar = lv_tabview_get_tab_bar(tv);
        lv_obj_set_flex_align(bar, LV_FLEX_ALIGN_START, LV_FLEX_ALIGN_START, LV_FLEX_ALIGN_START);
        for (uint32_t i = 0; i < lv_obj_get_child_count(bar); i++) {
            lv_obj_t *b = lv_obj_get_child(bar, (int32_t)i);
            lv_obj_set_flex_grow(b, 0);
            lv_obj_set_size(b, lv_pct(100), barre);
        }
    }
    if (n->state.notebook.current_tab > 0 && n->state.notebook.current_tab < n->child_count)
        lv_tabview_set_active(tv, (uint32_t)n->state.notebook.current_tab, LV_ANIM_OFF);
    lv_obj_add_event_cb(tv, onglet_change, LV_EVENT_VALUE_CHANGED, n);
    finir(n, tv);
}

/* <table> : lv_table, en-têtes en 1re rangée. Le cœur neutre exporte toujours
 * la 1re colonne de la 1re rangée (widget_table.c) : le tableau se lit, la
 * sélection n'est pas suivie. */
static void construire_tableau(WidgetNode *n, lv_obj_t *parent)
{
    int cols = n->state.table.cols, rangs = n->state.table.rows;
    lv_obj_t *t = lv_table_create(parent);
    if (cols < 1) { finir(n, t); return; }
    lv_table_set_column_count(t, (uint32_t)cols);
    lv_table_set_row_count(t, (uint32_t)(rangs + 1));
    for (int c = 0; c < cols; c++) {
        const char *h = n->state.table.headers ? n->state.table.headers[c] : NULL;
        lv_table_set_cell_value(t, 0, (uint32_t)c, h ? h : "");
        lv_table_set_column_width(t, (uint32_t)c, 120);
    }
    for (int r = 0; r < rangs; r++)
        for (int c = 0; c < cols; c++) {
            const char *v = n->state.table.cells ? n->state.table.cells[r * cols + c] : NULL;
            lv_table_set_cell_value(t, (uint32_t)(r + 1), (uint32_t)c, v ? v : "");
        }
    if (n->height > 0) lv_obj_set_height(t, n->height);
    else if (rangs > 8) lv_obj_set_height(t, 260);   /* au-delà, il défile */
    finir(n, t);
}

/* Texte montré par un <text> : ce que <input>/<default> ont posé, sinon le
 * <label> — comme l'étalon. Le saut de ligne final de la commande ne se montre
 * pas (fltk1 l'affichait « ^J »). */
static void texte_poser(WidgetNode *n, lv_obj_t *o)
{
    const char *t = n->label ? n->label : "";
    if (n->type == WT_TEXT && n->state.text.content && n->state.text.content[0]) t = n->state.text.content;
    size_t l = strlen(t);
    while (l > 0 && (t[l - 1] == '\n' || t[l - 1] == '\r')) l--;
    char *c = g_strndup(t, l);
    if (strcmp(lv_label_get_text(o), c) != 0) lv_label_set_text(o, c);
    g_free(c);
}

/* Une rangée (hbox, barre d'outils…) aligne ses enfants en ligne. */
static int rangee(const WidgetNode *p)
{
    return p && (p->type == WT_HBOX || p->type == WT_TOOLBAR || p->type == WT_FLOWBOX || p->type == WT_GRID);
}

/* Pose le fichier `f` (PNG, ou SVG par ThorVG) dans l'image `o`, à `px`
 * pixels de large si px > 0. Une icône SYMBOLIQUE (« …-symbolic.svg ») prend la
 * couleur du texte, comme dans GTK. Rend 1 si l'image est posée. Jusqu'à la
 * 2.8.1 : PNG seulement — les thèmes n'ayant presque plus que du SVG, aucune
 * icône ne s'affichait. */
/* Un SVG dessiné par ThorVG dans une image ARGB8888 de `px` pixels de large
 * (hauteur au ratio). Rend le descripteur (à garder tant que l'image vit), ou
 * NULL. L'analyseur SVG de LVGL plante sur les icônes d'Adwaita : ThorVG a le
 * sien, solide. */
#if LV_USE_THORVG
static lv_image_dsc_t *icone_svg(const char *chemin, int px)
{
    Tvg_Paint *pic = tvg_picture_new();
    if (!pic) return NULL;
    float w = 0, h = 0;
    if (tvg_picture_load(pic, chemin) != TVG_RESULT_SUCCESS ||
        tvg_picture_get_size(pic, &w, &h) != TVG_RESULT_SUCCESS || w <= 0 || h <= 0) {
        tvg_paint_del(pic);
        return NULL;
    }
    if (px <= 0) px = (int)w;
    int pw = px, ph = (int)(px * h / w + 0.5f);
    if (pw > 1024 || ph < 1 || ph > 1024) { tvg_paint_del(pic); return NULL; }
    tvg_picture_set_size(pic, (float)pw, (float)ph);
    lv_image_dsc_t *d = calloc(1, sizeof *d);
    uint32_t *buf = calloc((size_t)pw * (size_t)ph, sizeof *buf);
    Tvg_Canvas *cv = tvg_swcanvas_create();
    if (!d || !buf || !cv) { free(d); free(buf); if (cv) tvg_canvas_destroy(cv); tvg_paint_del(pic); return NULL; }
    tvg_swcanvas_set_target(cv, buf, (uint32_t)pw, (uint32_t)pw, (uint32_t)ph, TVG_COLORSPACE_ARGB8888S);
    tvg_canvas_push(cv, pic);            /* le canevas possède désormais l'image */
    tvg_canvas_draw(cv);
    tvg_canvas_sync(cv);
    tvg_canvas_destroy(cv);
    d->header.magic = LV_IMAGE_HEADER_MAGIC;
    d->header.cf = LV_COLOR_FORMAT_ARGB8888;
    d->header.w = (uint32_t)pw;
    d->header.h = (uint32_t)ph;
    d->header.stride = (uint32_t)pw * 4;
    d->data = (const uint8_t *)buf;
    d->data_size = (uint32_t)pw * (uint32_t)ph * 4;
    return d;
}
#else
/* Version légère (SERMO_LVGL_LEGER, sans ThorVG) : pas d'icône SVG ; les
 * icônes PNG restent affichées. */
static lv_image_dsc_t *icone_svg(const char *chemin, int px)
{
    (void)chemin;
    (void)px;
    return NULL;
}
#endif

static void icone_liberer(lv_event_t *e)
{
    lv_image_dsc_t *d = lv_event_get_user_data(e);
    if (d) { free((void *)d->data); free(d); }
}

static int icone_poser(lv_obj_t *o, const char *f, int px)
{
    if (f && g_ascii_strncasecmp(f, "file:", 5) == 0) f += 5;
    while (f && (*f == ' ' || *f == '\t')) f++;
    if (!f || !*f || !(g_str_has_suffix(f, ".png") || g_str_has_suffix(f, ".svg"))) return 0;
    struct stat st;
    /* chemin relatif : au dossier courant, comme l'étalon (fond.png d'<overlay>) */
    char *abs = realpath(f, NULL);
    int ok = 0;
    if (abs && stat(abs, &st) == 0 && S_ISREG(st.st_mode) && g_str_has_suffix(abs, ".svg")) {
        lv_image_dsc_t *d = icone_svg(abs, px > 0 ? px : 0);
        if (d) {
            lv_image_set_src(o, d);
            lv_obj_add_event_cb(o, icone_liberer, LV_EVENT_DELETE, d);
            ok = 1;
        }
    } else if (abs && stat(abs, &st) == 0 && S_ISREG(st.st_mode)) {
        char *src = g_strconcat("A:", abs, NULL);
        lv_image_set_src(o, src);   /* LVGL copie la chaîne */
        g_free(src);
        ok = 1;
        if (px > 0) {
            lv_image_header_t h;
            if (lv_image_decoder_get_info(lv_image_get_src(o), &h) == LV_RESULT_OK && h.w > 0) {
                lv_image_set_scale(o, (uint32_t)(256 * px / h.w));
                lv_obj_set_size(o, px, px * h.h / h.w);
                lv_image_set_inner_align(o, LV_IMAGE_ALIGN_CENTER);
            }
        }
    }
    if (ok && strstr(abs, "-symbolic.svg")) {
        lv_obj_set_style_image_recolor(o, lv_obj_get_style_text_color(lv_obj_get_parent(o), 0), 0);
        lv_obj_set_style_image_recolor_opa(o, LV_OPA_COVER, 0);
    }
    free(abs);
    return ok;
}

/* <pixmap>, <image> : un fichier PNG ou SVG, ou une icône du thème. */
static lv_obj_t *image_creer(WidgetNode *n, lv_obj_t *parent)
{
    lv_obj_t *o = lv_image_create(parent);
    icone_poser(o, n->type == WT_IMAGE_W ? n->icon_path : n->label,
                n->type == WT_IMAGE_W ? n->icon_px : 0);
    return o;
}

/* L'objet qui occupe la place d'un nœud dans sa boîte : sa case, s'il en a. */
static lv_obj_t *place_de(const WidgetNode *n)
{
    return n->lv_case ? (lv_obj_t *)n->lv_case : (lv_obj_t *)n->lv;
}

/* homogeneous="true" : chaque enfant reçoit une case de MÊME taille sur l'axe
 * de la boîte — au moins la plus grande taille naturelle, et toute la longueur
 * de la boîte quand elle est connue (flex-grow posé par finir : etire() vaut 1
 * dans une boîte homogène). Un enfant qui ne remplit pas sa case y est centré
 * (en_case). Étalon : t1, cinq colonnes égales sur toute la largeur. */
static void cases_egales(WidgetNode *n, lv_obj_t *o, int r)
{
    int32_t plus = 0;
    lv_obj_update_layout(o);
    for (int i = 0; i < n->child_count; i++) {
        lv_obj_t *p = place_de(n->children[i]);
        if (!p || !n->children[i]->visible) continue;
        int32_t t = r ? lv_obj_get_width(p) : lv_obj_get_height(p);
        if (t > plus) plus = t;
    }
    for (int i = 0; i < n->child_count; i++) {
        lv_obj_t *p = place_de(n->children[i]);
        if (!p) continue;
        if (r) lv_obj_set_style_min_width(p, plus, 0);
        else   lv_obj_set_style_min_height(p, plus, 0);
    }
}

/* Les hauteurs qui dépendent de la place reçue : posées APRÈS la construction
 * de tout l'arbre (hauteurs_poser). Mesurée au fil de la construction, la
 * hauteur naturelle d'un nœud se lit sur des parents encore taillés sur leur
 * contenu : un texte en pleine largeur d'un cadre de 0 px y passe à la ligne à
 * chaque lettre (mesuré : 544 px pour une ligne, une colonne de 1501 px dans
 * system-tools). */
enum {
    H_GRANDIT      = 1,   /* s'étire en hauteur dans sa colonne (flex-grow) */
    H_REMPLIT      = 2,   /* prend toute la hauteur de sa rangée */
    H_CASE_REMPLIT = 4,   /* sa case prend toute la hauteur de sa rangée */
};
static void hauteurs_relever(WidgetNode *n)
{
    lv_obj_t *o = (lv_obj_t *)n->lv, *c = (lv_obj_t *)n->lv_case;
    if (!o) n->lv_hauteur = 0;
    if (n->lv_hauteur & (H_GRANDIT | H_CASE_REMPLIT))
        n->lv_haut_nat = lv_obj_get_height(c ? c : o);
    else if (n->lv_hauteur & H_REMPLIT)
        n->lv_haut_nat = lv_obj_get_height(o);
    for (int i = 0; i < n->child_count; i++) hauteurs_relever(n->children[i]);
}
static void hauteurs_appliquer(WidgetNode *n)
{
    lv_obj_t *o = (lv_obj_t *)n->lv, *c = (lv_obj_t *)n->lv_case;
    if (!o) n->lv_hauteur = 0;
    /* AU MOINS la hauteur naturelle : une boîte taillée sur son contenu ne
     * compte pas un enfant en pour-cent ou qui grandit, et LVGL l'écraserait
     * à 0 px (les boutons de « Actions rapides » et le cadre « dmesg » de
     * system-tools disparaissaient, jusqu'à la 2.8.1). */
    if (n->lv_hauteur & H_GRANDIT) {
        lv_obj_t *t = c ? c : o;
        lv_obj_set_style_min_height(t, n->lv_haut_nat, 0);
        lv_obj_set_flex_grow(t, 1);
    }
    if (n->lv_hauteur & H_CASE_REMPLIT) {
        lv_obj_set_style_min_height(c, n->lv_haut_nat, 0);
        lv_obj_set_height(c, lv_pct(100));
    }
    if (n->lv_hauteur & H_REMPLIT) {
        if (!c) lv_obj_set_style_min_height(o, n->lv_haut_nat, 0);
        lv_obj_set_height(o, lv_pct(100));
    }
    for (int i = 0; i < n->child_count; i++) hauteurs_appliquer(n->children[i]);
}
static void textes_poser(WidgetNode *n)
{
    if (n->type == WT_EDIT && n->lv)
        lv_textarea_set_text((lv_obj_t *)n->lv, n->state.text.content ? n->state.text.content : "");
    for (int i = 0; i < n->child_count; i++) textes_poser(n->children[i]);
}
static void hauteurs_poser(WidgetNode *root)
{
    lv_obj_update_layout(lv_screen_active());
    hauteurs_relever(root);
    hauteurs_appliquer(root);
}

static void construire(WidgetNode *n, lv_obj_t *parent)
{
    lv_obj_t *o = NULL;
    const char *lbl = n->label ? n->label : "";
    int boite_n = 0;   /* une boîte : on lui applique width/height-request */

    /* Étiré sans remplir (space-fill faux) : une case transparente prend la
     * place reçue et y centre le widget, à sa taille propre — sur la largeur
     * dans une rangée, sur la hauteur dans une colonne (où il garde toute la
     * largeur, comme dans une GtkBox verticale). */
    n->lv_case = NULL;
    if (en_case(n)) {
        int r = rangee(n->parent);
        lv_obj_t *c = boite(parent, r ? LV_FLEX_FLOW_ROW : LV_FLEX_FLOW_COLUMN);
        lv_obj_set_style_pad_gap(c, 0, 0);
        lv_obj_set_flex_align(c, LV_FLEX_ALIGN_CENTER, r ? LV_FLEX_ALIGN_CENTER : LV_FLEX_ALIGN_START,
                              LV_FLEX_ALIGN_CENTER);
        n->lv_case = c;
        parent = c;
    }

    switch (n->type) {
    case WT_WINDOW:
        o = boite(parent, LV_FLEX_FLOW_COLUMN);
        if (g_mesure == 2)  lv_obj_set_width(o, g_mesure_l);
        else if (!g_mesure) lv_obj_set_size(o, lv_pct(100), lv_pct(100));
        construire_enfants(n, o);
        break;
    case WT_NOTEBOOK:
        construire_onglets(n, parent);
        return;
    case WT_TABLE:
        construire_tableau(n, parent);
        return;
    case WT_EXPANDER: {
        /* Un titre (« ▼ » ouvert, « ▶ » fermé) et un corps qui se montre ou se
         * cache. Jusqu'à la 2.8.0, une simple colonne : ni titre, ni repli. */
        o = boite(parent, LV_FLEX_FLOW_COLUMN);
        lv_obj_t *titre = lv_label_create(o);
        lv_obj_set_clickable(titre, true);
        lv_obj_t *corps = boite(o, LV_FLEX_FLOW_COLUMN);
        lv_obj_set_width(corps, lv_pct(100));
        n->lv = o;
        expander_titre(n, titre, corps);
        lv_obj_add_event_cb(titre, expander_clic, LV_EVENT_CLICKED, n);
        construire_enfants(n, corps);
        break;
    }
    case WT_STACK: case WT_WIZARD: {
        /* Une page à la fois. <stack> à sélecteur (side) : une rangée de
         * boutons au-dessus ; <wizard> : Précédent / Suivant / Terminer
         * dessous, comme les autres ports. */
        o = boite(parent, LV_FLEX_FLOW_COLUMN);
        boite_n = 1;
        n->lv = o;
        if (n->type == WT_STACK && n->state.notebook.side) {
            lv_obj_t *sel = boite(o, LV_FLEX_FLOW_ROW);
            for (int i = 0; i < n->child_count; i++) {
                char num[16];
                snprintf(num, sizeof num, "%d", i + 1);
                lv_obj_t *b = lv_button_create(sel);
                lv_label_set_text(lv_label_create(b), num);
                lv_obj_set_user_data(b, (void *)(intptr_t)i);
                lv_obj_add_event_cb(b, pile_choisir, LV_EVENT_CLICKED, n);
            }
        }
        construire_enfants(n, o);
        if (n->type == WT_WIZARD) {
            lv_obj_t *barre = boite(o, LV_FLEX_FLOW_ROW);
            lv_obj_set_flex_align(barre, LV_FLEX_ALIGN_START, LV_FLEX_ALIGN_CENTER, LV_FLEX_ALIGN_CENTER);
            static const char *noms[3] = { "Précédent", "Suivant", "Terminer" };
            lv_event_cb_t cbs[3] = { assistant_precedent, assistant_suivant, assistant_terminer };
            for (int i = 0; i < 3; i++) {
                lv_obj_t *b = lv_button_create(barre);
                lv_label_set_text(lv_label_create(b), noms[i]);
                lv_obj_add_event_cb(b, cbs[i], LV_EVENT_CLICKED, n);
            }
            lv_label_create(barre);   /* « étape i/n » */
            assistant_etat(n);
        } else {
            pages_montrer(n, o);
        }
        break;
    }
    case WT_OVERLAY: {
        /* Les couches l'une SUR l'autre, centrées : une grille d'une seule
         * cellule. En colonne, le texte passait sous l'image. */
        static const int32_t une[] = { LV_GRID_CONTENT, LV_GRID_TEMPLATE_LAST };
        o = boite(parent, LV_FLEX_FLOW_COLUMN);
        boite_n = 1;
        lv_obj_set_grid_dsc_array(o, une, une);
        construire_enfants(n, o);
        for (uint32_t i = 0; i < lv_obj_get_child_count(o); i++)
            lv_obj_set_grid_cell(lv_obj_get_child(o, (int32_t)i), LV_GRID_ALIGN_CENTER, 0, 1,
                                 LV_GRID_ALIGN_CENTER, 0, 1);
        break;
    }
    case WT_VBOX:
    case WT_EVENTBOX: case WT_REVEALER:
    case WT_PANED:
        o = boite(parent, n->type == WT_PANED && !n->state.paned.vertical
                          ? LV_FLEX_FLOW_ROW : LV_FLEX_FLOW_COLUMN);
        boite_n = 1;
        construire_enfants(n, o);
        if (homogene(n)) cases_egales(n, o, 0);
        break;
    case WT_GRID: {
        /* <grid columns="N"> : des rangées de N cellules, chaque colonne à la
         * largeur de sa plus large cellule. En flot à la ligne, la 2e rangée
         * sortait de la fenêtre (mesure faite sans le retour à la ligne). */
        int nc = n->state.grid.columns > 0 ? n->state.grid.columns : 1;
        o = boite(parent, LV_FLEX_FLOW_COLUMN);
        boite_n = 1;
        lv_obj_set_style_pad_gap(o, n->state.grid.row_spacing, 0);
        lv_obj_t *r = NULL;
        for (int i = 0; i < n->child_count; i++) {
            if (i % nc == 0) {
                r = boite(o, LV_FLEX_FLOW_ROW);
                lv_obj_set_style_pad_gap(r, n->state.grid.col_spacing, 0);
                lv_obj_set_flex_align(r, LV_FLEX_ALIGN_START, LV_FLEX_ALIGN_CENTER, LV_FLEX_ALIGN_CENTER);
            }
            construire(n->children[i], r);
        }
        /* largeur de colonne = la plus large de ses cellules */
        lv_obj_update_layout(o);
        for (int c = 0; c < nc; c++) {
            int32_t lmax = 0;
            for (int i = c; i < n->child_count; i += nc)
                if (n->children[i]->lv) {
                    int32_t w = lv_obj_get_width((lv_obj_t *)n->children[i]->lv);
                    if (w > lmax) lmax = w;
                }
            for (int i = c; i < n->child_count; i += nc)
                if (n->children[i]->lv) lv_obj_set_width((lv_obj_t *)n->children[i]->lv, lmax);
        }
        break;
    }
    case WT_HBOX: case WT_TOOLBAR: case WT_FLOWBOX:
        o = boite(parent, n->type == WT_FLOWBOX ? LV_FLEX_FLOW_ROW_WRAP : LV_FLEX_FLOW_ROW);
        boite_n = 1;
        /* Une étiquette se centre sur la hauteur de son champ, comme chez l'étalon.
         * gtkdialog range une rangée PAR LA FIN (gtk_box_pack_end) : ce qui ne
         * s'étire pas se cale à droite (jusqu'à la 2.8.1 : à gauche). */
        lv_obj_set_flex_align(o, n->type == WT_HBOX ? LV_FLEX_ALIGN_END : LV_FLEX_ALIGN_START,
                              LV_FLEX_ALIGN_CENTER, LV_FLEX_ALIGN_CENTER);
        construire_enfants(n, o);
        if (homogene(n)) cases_egales(n, o, 1);
        break;
    case WT_FRAME: case WT_ASPECTFRAME: {
        /* <aspectframe label="…"> se dessine comme un cadre titré. */
        o = lv_obj_create(parent);
        boite_n = 1;
        lv_obj_set_size(o, LV_SIZE_CONTENT, LV_SIZE_CONTENT);
        lv_obj_set_flex_flow(o, LV_FLEX_FLOW_COLUMN);
        lv_obj_set_scrollable(o, false);
        if (lbl[0]) lv_label_set_text(lv_label_create(o), lbl);
        construire_enfants(n, o);
        break;
    }
    case WT_TEXT: case WT_STATUSBAR:
        o = lv_label_create(parent);
        lv_label_set_text(o, "");
        texte_poser(n, o);
        break;
    case WT_INFOBAR: {
        /* Bandeau teinté selon message-type, texte noir : le sens se lit aussi
         * par le texte (contraste ≥ 12:1 sur ces fonds clairs). */
        const char *mt = n->state.infobar.message_type;
        uint32_t fond = !strcmp(mt, "error") ? 0xF6D5D5 : !strcmp(mt, "warning") ? 0xF4E7D3
                      : !strcmp(mt, "question") ? 0xDCE6F4 : 0xE3EEF9;
        o = lv_label_create(parent);
        lv_label_set_text(o, lbl);
        lv_obj_set_style_bg_opa(o, LV_OPA_COVER, 0);
        lv_obj_set_style_bg_color(o, lv_color_hex(fond), 0);
        lv_obj_set_style_text_color(o, lv_color_black(), 0);
        lv_obj_set_style_pad_all(o, 10, 0);
        lv_obj_set_style_radius(o, 4, 0);
        break;
    }
    case WT_LINKBUTTON:
        /* Un lien souligné, pas un bouton plein : comme l'étalon. Bleu
         * #1A5FB4 sur fond clair (contraste 6,3:1), souligné pour se lire
         * sans la couleur. */
        o = lv_label_create(parent);
        lv_label_set_text(o, lbl[0] ? lbl : n->state.entry.buf);
        lv_obj_set_style_text_color(o, lv_color_hex(0x1A5FB4), 0);
        lv_obj_set_style_text_decor(o, LV_TEXT_DECOR_UNDERLINE, 0);
        lv_obj_set_clickable(o, true);
        lv_obj_add_event_cb(o, bouton_clic, LV_EVENT_CLICKED, n);
        break;
    case WT_MENUBUTTON: {
        o = lv_dropdown_create(parent);
        lv_dropdown_clear_options(o);
        for (int i = 0; i < n->child_count; i++) {
            WidgetNode *it = n->children[i];
            if (it->type == WT_MENUITEM && it->label && it->label[0])
                lv_dropdown_add_option(o, it->label, LV_DROPDOWN_POS_LAST);
        }
        lv_dropdown_set_text(o, lbl[0] ? lbl : "Menu");   /* le titre reste affiché */
        lv_dropdown_set_selected_highlight(o, false);
        lv_obj_set_width(o, LV_SIZE_CONTENT);
        lv_obj_add_event_cb(o, menu_choisi, LV_EVENT_VALUE_CHANGED, n);
        break;
    }
    case WT_CALENDAR: {
        o = boite(parent, LV_FLEX_FLOW_COLUMN);
        lv_obj_t *cal = lv_calendar_create(o);
        lv_obj_set_size(cal, 280, 230);
        /* noms de jours de la langue du poste, dimanche d'abord (ordre LVGL) */
        static const char *jours[7];
        static char jours_buf[7][16];
        static const nl_item ab[7] = { ABDAY_1, ABDAY_2, ABDAY_3, ABDAY_4, ABDAY_5, ABDAY_6, ABDAY_7 };
        for (int i = 0; i < 7; i++) {
            snprintf(jours_buf[i], sizeof jours_buf[i], "%s", nl_langinfo(ab[i]));
            jours[i] = jours_buf[i];
        }
        lv_calendar_set_day_names(cal, jours);
        lv_calendar_set_today_date(cal, (uint32_t)n->state.calendar.year, (uint32_t)n->state.calendar.month,
                                   (uint32_t)n->state.calendar.day);
        lv_calendar_set_month_shown(cal, (uint32_t)n->state.calendar.year, (uint32_t)n->state.calendar.month);
        lv_obj_t *barre = boite(o, LV_FLEX_FLOW_ROW);
        lv_obj_set_flex_align(barre, LV_FLEX_ALIGN_SPACE_BETWEEN, LV_FLEX_ALIGN_CENTER, LV_FLEX_ALIGN_CENTER);
        lv_obj_set_width(barre, 280);
        lv_obj_move_to_index(barre, 0);
        lv_obj_t *av = lv_button_create(barre);
        lv_label_set_text(lv_label_create(av), LV_SYMBOL_LEFT);
        calendrier_titre(cal, lv_label_create(barre));
        lv_obj_t *ap = lv_button_create(barre);
        lv_label_set_text(lv_label_create(ap), LV_SYMBOL_RIGHT);
        lv_obj_add_event_cb(av, calendrier_avant, LV_EVENT_CLICKED, cal);
        lv_obj_add_event_cb(ap, calendrier_apres, LV_EVENT_CLICKED, cal);
        lv_obj_add_event_cb(cal, calendrier_change, LV_EVENT_VALUE_CHANGED, n);
        break;
    }
    case WT_PIXMAP: case WT_IMAGE_W:
        o = image_creer(n, parent);
        break;
    case WT_BUTTON: case WT_MENUITEM: case WT_FONTBUTTON:
    case WT_COLORBUTTON: case WT_FILECHOOSER: {
        o = lv_button_create(parent);
        /* Le contenu (icône et libellé) est un groupe CENTRÉ dans le bouton,
         * sur les deux axes, comme l'étalon : un bouton élargi ou étiré en
         * hauteur (t2, t4) gardait son libellé en haut à gauche. */
        lv_obj_set_flex_flow(o, LV_FLEX_FLOW_ROW);
        lv_obj_set_flex_align(o, LV_FLEX_ALIGN_CENTER, LV_FLEX_ALIGN_CENTER, LV_FLEX_ALIGN_CENTER);
        /* <input file icon="…"> : l'icône du thème avant le libellé, comme
         * l'étalon (jusqu'à la 2.8.1, lvgl n'en dessinait aucune). */
        if (n->icon_path && (n->type == WT_BUTTON || n->type == WT_MENUITEM)) {
            /* image-position : l'icône à droite, dessus ou dessous du libellé ;
             * le groupe reste centré (flux inversé, alignement au centre) */
            static const lv_flex_flow_t flux[4] = { LV_FLEX_FLOW_ROW, LV_FLEX_FLOW_ROW_REVERSE,
                                                    LV_FLEX_FLOW_COLUMN, LV_FLEX_FLOW_COLUMN_REVERSE };
            lv_obj_set_flex_flow(o, flux[n->icone_pos & 3]);
            lv_obj_set_style_pad_column(o, 6, 0);
            lv_obj_set_style_pad_row(o, 4, 0);
            lv_obj_t *ic = lv_image_create(o);
            if (!icone_poser(ic, n->icon_path, n->icon_px > 0 ? n->icon_px : 16)) lv_obj_delete(ic);
        }
        /* un <chooser> sans libellé montre son dossier, pas « OK » */
        lv_label_set_text(lv_label_create(o), lbl[0] ? lbl
                          : n->type == WT_FILECHOOSER ? (n->state.entry.buf[0] ? n->state.entry.buf : "…") : "OK");
        lv_obj_add_event_cb(o, bouton_clic, LV_EVENT_CLICKED, n);
        if (n->type == WT_COLORBUTTON)
            couleur_peindre(n, o);
        break;
    }
    case WT_TOGGLEBUTTON:
        /* Un bouton qui reste enfoncé, pas une case à cocher : comme l'étalon.
         * Enfoncé : bleu nuit #1A3A5C, texte blanc — il se lit par la clarté. */
        o = lv_button_create(parent);
        lv_label_set_text(lv_label_create(o), lbl);
        lv_obj_set_checkable(o, true);
        lv_obj_set_style_bg_color(o, lv_color_hex(0x1A3A5C), LV_STATE_CHECKED);
        if (n->state.toggle.active) lv_obj_add_state(o, LV_STATE_CHECKED);
        lv_obj_add_event_cb(o, bascule_change, LV_EVENT_VALUE_CHANGED, n);
        break;
    case WT_CHECKBOX: case WT_RADIOBUTTON:
        o = lv_checkbox_create(parent);
        lv_checkbox_set_text(o, lbl);
        if (n->type == WT_RADIOBUTTON)   /* rond, comme un bouton radio */
            lv_obj_set_style_radius(o, LV_RADIUS_CIRCLE, LV_PART_INDICATOR);
        if (n->state.toggle.active) lv_obj_add_state(o, LV_STATE_CHECKED);
        lv_obj_add_event_cb(o, bascule_change, LV_EVENT_VALUE_CHANGED, n);
        break;
    case WT_SWITCH:
        o = lv_switch_create(parent);
        if (n->state.toggle.active) lv_obj_add_state(o, LV_STATE_CHECKED);
        lv_obj_add_event_cb(o, bascule_change, LV_EVENT_VALUE_CHANGED, n);
        break;
    case WT_ENTRY: case WT_PASSWORD: case WT_SEARCHENTRY:
        o = lv_textarea_create(parent);
        lv_textarea_set_one_line(o, true);
        lv_obj_set_width(o, n->width > 0 ? n->width : 220);
        if (n->type == WT_PASSWORD) lv_textarea_set_password_mode(o, true);
        /* ⛔ <searchentry> range son texte dans state.list.filter, pas dans
         * entry.buf : les deux partagent l'union. Écrire entry.buf écrasait
         * list.items (pointeur) jusqu'à la 2.8.0. */
        lv_textarea_set_text(o, n->type == WT_SEARCHENTRY ? n->state.list.filter : n->state.entry.buf);
        lv_obj_add_event_cb(o, champ_change, LV_EVENT_VALUE_CHANGED, n);
        break;
    case WT_EDIT:
        o = lv_textarea_create(parent);
        lv_obj_set_size(o, n->width > 0 ? n->width : 300, n->height > 0 ? n->height : 120);
        /* Le texte est posé après la construction (textes_poser) : sa taille
         * est fixée ci-dessus, il ne compte pas dans la mesure, et
         * lv_textarea_set_text refait la mise en page de tout l'écran — un
         * texte de 20000 caractères sans saut de ligne coûtait 3 s à chaque
         * fois (tests/garde_input_sans_fin.sh figé au-delà de 40 s). */
        lv_obj_add_event_cb(o, champ_change, LV_EVENT_VALUE_CHANGED, n);
        break;
    case WT_LIST:
        construire_liste(n, parent);
        return;
    case WT_COMBOBOX: {
        o = lv_dropdown_create(parent);
        lv_dropdown_clear_options(o);
        for (int i = 0; i < n->state.list.item_count; i++)
            lv_dropdown_add_option(o, n->state.list.items[i], LV_DROPDOWN_POS_LAST);
        if (n->state.list.selected_index >= 0)
            lv_dropdown_set_selected(o, (uint32_t)n->state.list.selected_index);
        lv_obj_add_event_cb(o, deroulant_change, LV_EVENT_VALUE_CHANGED, n);
        break;
    }
    case WT_HSCALE: case WT_VSCALE: {
        double pas = n->state.scale.step > 0 ? n->state.scale.step : 1;
        o = lv_slider_create(parent);
        if (n->type == WT_VSCALE) {   /* verticale, le maximum en haut comme l'étalon */
            lv_obj_set_size(o, 16, 120);
            lv_slider_set_orientation(o, LV_SLIDER_ORIENTATION_VERTICAL);
        }
        else                      lv_obj_set_width(o, 200);
        lv_slider_set_range(o, 0, (int32_t)((n->state.scale.max - n->state.scale.min) / pas));
        lv_slider_set_value(o, (int32_t)((n->state.scale.value - n->state.scale.min) / pas), LV_ANIM_OFF);
        lv_obj_add_event_cb(o, glissiere_change, LV_EVENT_VALUE_CHANGED, n);
        break;
    }
    case WT_SPINBUTTON: {
        /* « − », le nombre, « + » : comme les autres ports. Jusqu'à la 2.8.0, ce
         * widget tombait dans le cas par défaut et n'était pas dessiné du tout. */
        o = boite(parent, LV_FLEX_FLOW_ROW);
        lv_obj_set_flex_align(o, LV_FLEX_ALIGN_START, LV_FLEX_ALIGN_CENTER, LV_FLEX_ALIGN_CENTER);
        n->lv = o;   /* les boutons retrouvent le champ par la rangée */
        lv_obj_t *moins = lv_button_create(o);
        lv_label_set_text(lv_label_create(moins), LV_SYMBOL_MINUS);
        lv_obj_t *champ = lv_textarea_create(o);
        lv_textarea_set_one_line(champ, true);
        lv_textarea_set_accepted_chars(champ, "0123456789.-");
        lv_obj_set_width(champ, n->width > 0 ? n->width : 90);
        spin_afficher(n, champ);
        lv_obj_t *plus = lv_button_create(o);
        lv_label_set_text(lv_label_create(plus), LV_SYMBOL_PLUS);
        lv_obj_add_event_cb(champ, spin_saisie, LV_EVENT_VALUE_CHANGED, n);
        lv_obj_add_event_cb(moins, spin_moins, LV_EVENT_CLICKED, n);
        lv_obj_add_event_cb(plus, spin_plus, LV_EVENT_CLICKED, n);
        break;
    }
    case WT_PROGRESSBAR: case WT_LEVELBAR: case WT_PULSE:
        o = lv_bar_create(parent);
        lv_obj_set_width(o, 200);
        lv_bar_set_range(o, 0, 100);
        if (n->type == WT_PROGRESSBAR)   /* fraction 0..1 du cœur, dès l'ouverture (et dans --render-png) */
            lv_bar_set_value(o, (int32_t)(n->state.progress.value * 100.0 + 0.5), LV_ANIM_OFF);
        if (n->type == WT_LEVELBAR)      /* valeur entre min et max ; elle restait vide */
            niveau_afficher(n, o);
        break;
    case WT_SPINNER:
        o = lv_spinner_create(parent);
        lv_obj_set_size(o, 32, 32);
        break;
    case WT_SEPARATOR:
        o = lv_obj_create(parent);
        /* Dans une rangée, un trait vertical : en pleine largeur, il poussait
         * ses voisins hors de la fenêtre (« Droite » disparaissait). */
        if (rangee(n->parent)) lv_obj_set_size(o, 2, 20);
        else                   lv_obj_set_size(o, lv_pct(100), 2);
        break;
    case WT_TIMER: {
        if (g_mesure) return;
        int ms = (n->label && *n->label) ? atoi(n->label) : 1000;
        lv_timer_create(minuterie_tic, (uint32_t)(ms > 0 ? ms : 1000), n);
        return;   /* rien à montrer */
    }
    default:
        /* Widgets pas encore dessinés : leurs enfants, s'il y en a. */
        o = boite(parent, LV_FLEX_FLOW_COLUMN);
        boite_n = 1;
        construire_enfants(n, o);
        break;
    }

    /* width-request / height-request d'une boîte. Ignorés jusqu'à la 2.8.0 :
     * le cœur enveloppe un <edit> dans une vbox de 200 px, qui restait taillée
     * sur son contenu ; le champ, à 100 % de 0 px, n'était pas dessiné. */
    if (o && boite_n) {
        if (n->width > 0) {
            /* un minimum, comme chez GTK : un enfant de largeur fixe plus large
             * (un <tree> à colonnes) élargit la boîte au lieu d'être rogné */
            lv_obj_set_width(o, n->width);
            lv_obj_update_layout(o);
            int32_t besoin = 0;
            for (uint32_t i = 0; i < lv_obj_get_child_count(o); i++) {
                int32_t w = lv_obj_get_width(lv_obj_get_child(o, (int32_t)i));
                if (w > besoin) besoin = w;
            }
            besoin += lv_obj_get_style_pad_left(o, 0) + lv_obj_get_style_pad_right(o, 0);
            if (besoin > n->width) lv_obj_set_width(o, besoin);
        }
        if (n->height > 0) lv_obj_set_height(o, n->height);
    }
    finir(n, o);
}

static void finir(WidgetNode *n, lv_obj_t *o)
{
    n->lv = o;
    if (o && n->type == WT_TEXT) lv_obj_set_style_text_align(o, LV_TEXT_ALIGN_CENTER, 0);
    /* Pleine largeur dans une colonne — pas pendant la mesure : une largeur en
     * pour-cent d'une boîte taillée sur son contenu ne se mesure pas. */
    /* Un interrupteur garde sa taille, comme chez l'étalon : étiré, il devenait
     * une longue barre (capture du site, 2026-10-10). */
    /* Sa case (en_case), s'il en a une : c'est elle qui prend la place dans la
     * boîte ; le widget y garde sa taille sur l'axe de la boîte. */
    lv_obj_t *cel = (lv_obj_t *)n->lv_case;
    n->lv_hauteur = 0;
    if (cel && g_mesure != 1 && colonne(n->parent) && largeur_connue(n->parent))
        lv_obj_set_width(cel, lv_pct(100));
    if (cel && g_mesure != 1 && rangee(n->parent) && hauteur_connue(n->parent))
        n->lv_hauteur |= H_CASE_REMPLIT;
    if (o && g_mesure != 1 && colonne(n->parent) && largeur_connue(n->parent)
        && !(n->width > 0) && n->type != WT_TABLE && n->type != WT_SWITCH && n->type != WT_VSCALE
        && n->type != WT_MENUBUTTON)
        lv_obj_set_width(o, lv_pct(100));   /* lv_table : une largeur en pour-cent la met à 0 px (mesuré) */
    /* space-expand="true" : l'enfant prend la place restante dans sa boîte,
     * en largeur dans un hbox, en hauteur dans un vbox — et remplit l'autre
     * sens, comme chez l'étalon (space-fill). Posé après coup, par
     * hauteurs_poser : voir là. */
    if (o && g_mesure != 1 && rangee(n->parent) && hauteur_connue(n->parent) && remplit_rangee(n)
        && !(n->height > 0))
        n->lv_hauteur |= H_REMPLIT;
    lv_obj_t *t = cel ? cel : o;   /* ce qui s'étire dans la boîte */
    if (o && etire(n) && g_mesure != 1 && n->type != WT_VSCALE) {
        /* Pas de flex-grow pendant la mesure : dans une boîte taillée sur son
         * contenu, il n'y a rien à prendre et LVGL écrase l'enfant à 0 px (le
         * formulaire mesurait 80 px de haut et sortait coupé). */
        if (!g_mesure) {
            /* Il garde AU MOINS sa taille naturelle : dans une boîte taillée
             * sur son contenu, LVGL écrase sinon un enfant qui grandit à 0 px
             * (les boutons de « Actions rapides » et le cadre « dmesg » de
             * system-tools disparaissaient, jusqu'à la 2.8.1). */
            /* Une mise en page complète ne sert qu'à la largeur d'une rangée :
             * la hauteur d'une colonne est relevée une seule fois, par
             * hauteurs_poser. Chacune refait tout l'écran — un <edit> de
             * 20000 caractères sans saut de ligne y passait plus de 40 s
             * (tests/garde_input_sans_fin.sh). */
            if (rangee(n->parent)) {
                lv_obj_update_layout(t);
                lv_obj_set_style_min_width(t, lv_obj_get_width(t), 0);
                lv_obj_set_flex_grow(t, 1);
            } else if (cel || !hauteur_propre(n)) {
                n->lv_hauteur |= H_GRANDIT;   /* hauteurs_poser */
            }
        }
        if (rangee(n->parent)) {
            /* La hauteur d'une rangée : ci-dessus (remplit_rangee), et seulement
             * si la rangée en a une — taillée sur son contenu, un 100 % devient
             * circulaire et LVGL écrase le champ (mesuré sur 01-formulaire).
             * <grid> et <flowbox> sont aussi des rangées : jusqu'ici, une <entry>
             * de <grid> y recevait 100 % de largeur d'une rangée taillée sur son
             * contenu — la rangée gonflait jusqu'à 25 millions de px et
             * débordait l'int de LVGL (UBSan, cas 29-grid-flot, 2026-10-14). */
        } else if (largeur_connue(n->parent))
            /* Même piège en largeur : 100 % d'une colonne taillée sur son
             * contenu vaut 0 px — les boutons de « Actions rapides » de
             * system-tools n'apparaissaient pas (jusqu'à la 2.8.1). */
            lv_obj_set_width(t, lv_pct(100));
    }
    if (cel && !n->visible) lv_obj_set_hidden(cel, true);
    if (o && !n->visible) lv_obj_set_hidden(o, true);
    if (o && !n->sensitive) lv_obj_add_state(o, LV_STATE_DISABLED);
}

/* ─── Synchronisation : ce que le cœur a changé revient à l'écran ─────────── */
static void synchroniser(WidgetNode *n)
{
    lv_obj_t *o = (lv_obj_t *)n->lv;
    if (n->lv_case) lv_obj_set_hidden((lv_obj_t *)n->lv_case, !n->visible);
    if (o) {
        lv_obj_set_hidden(o, !n->visible);
        if (n->sensitive) lv_obj_remove_state(o, LV_STATE_DISABLED);
        else              lv_obj_add_state(o, LV_STATE_DISABLED);

        switch (n->type) {
        case WT_TEXT: case WT_STATUSBAR:
            texte_poser(n, o);
            break;
        case WT_INFOBAR: {
            const char *t = n->label ? n->label : "";
            if (strcmp(lv_label_get_text(o), t) != 0) lv_label_set_text(o, t);
            break;
        }
        /* Famille C (2026-10-13) : un « refresh » rejoue l'<input> de ces
         * widgets (widget_*.c) ; l'écran suit ici. Aucune de ces poses n'appelle
         * un rappel qui joue des actions : lv_tabview_set_active n'envoie pas
         * VALUE_CHANGED (la fin de défilement qu'il cause non plus : l'onglet
         * courant est déjà posé ; onglet_change ne joue rien), lv_calendar_*
         * n'en envoie qu'aux en-têtes enfants du calendrier (il n'en a pas ici,
         * calendrier_change n'est pas touché), lv_label_set_text et les styles
         * n'en envoient aucun. Statusbar, infobar et searchentry passent
         * par les cas déjà là (texte_poser, libellé, champ : champ_change ne
         * fait que recopier le texte). Le linkbutton montre son libellé,
         * que l'<input> ne change pas (seule l'URI change, comme l'étalon). */
        case WT_COLORBUTTON:
            couleur_peindre(n, o);
            break;
        case WT_BUTTON: case WT_FONTBUTTON:
            /* <fontbutton> : un bouton marqué __fontbutton__ dont le libellé
             * est la police (widget_fontbutton.c) ; le libellé est le dernier
             * enfant du bouton (une icône peut le précéder) */
            if (n->type == WT_FONTBUTTON || (n->var_name && strcmp(n->var_name, "__fontbutton__") == 0)) {
                uint32_t k = lv_obj_get_child_count(o);
                lv_obj_t *l = k ? lv_obj_get_child(o, (int32_t)k - 1) : NULL;
                const char *t = n->label && n->label[0] ? n->label : "OK";
                if (l && lv_obj_check_type(l, &lv_label_class) && strcmp(lv_label_get_text(l), t) != 0)
                    lv_label_set_text(l, t);
            }
            break;
        case WT_CALENDAR:
            /* la boîte : la barre « ‹ mois › » d'abord, puis le calendrier */
            if (lv_obj_get_child_count(o) == 2 && lv_obj_check_type(lv_obj_get_child(o, 1), &lv_calendar_class)) {
                lv_obj_t *cal = lv_obj_get_child(o, 1);
                const lv_calendar_date_t *j = lv_calendar_get_today_date(cal);
                if (j->year != n->state.calendar.year || j->month != n->state.calendar.month
                    || j->day != n->state.calendar.day) {
                    lv_calendar_set_today_date(cal, (uint32_t)n->state.calendar.year,
                                               (uint32_t)n->state.calendar.month, (uint32_t)n->state.calendar.day);
                    lv_calendar_set_month_shown(cal, (uint32_t)n->state.calendar.year,
                                                (uint32_t)n->state.calendar.month);
                    calendrier_titre(cal, lv_obj_get_child(lv_obj_get_child(o, 0), 1));
                }
            }
            break;
        case WT_EXPANDER: {
            /* le titre, puis le corps (caché quand l'expander est replié) */
            lv_obj_t *corps = lv_obj_get_child(o, 1);
            if (corps && lv_obj_is_hidden(corps) == (bool)n->state.expander.open)
                expander_titre(n, lv_obj_get_child(o, 0), corps);
            break;
        }
        case WT_NOTEBOOK: {
            int p = n->state.notebook.current_tab;
            if (lv_obj_check_type(o, &lv_tabview_class) && p >= 0 && p < n->child_count
                && lv_tabview_get_tab_active(o) != (uint32_t)p)
                lv_tabview_set_active(o, (uint32_t)p, LV_ANIM_OFF);
            break;
        }
        case WT_ENTRY: case WT_PASSWORD: case WT_SEARCHENTRY: {
            const char *t = n->type == WT_SEARCHENTRY ? n->state.list.filter : n->state.entry.buf;
            if (strcmp(lv_textarea_get_text(o), t) != 0) lv_textarea_set_text(o, t);
            break;
        }
        case WT_EDIT: {
            const char *t = n->state.text.content ? n->state.text.content : "";
            if (strcmp(lv_textarea_get_text(o), t) != 0) lv_textarea_set_text(o, t);
            break;
        }
        case WT_CHECKBOX: case WT_RADIOBUTTON: case WT_TOGGLEBUTTON: case WT_SWITCH:
            if (n->state.toggle.active) lv_obj_add_state(o, LV_STATE_CHECKED);
            else                        lv_obj_remove_state(o, LV_STATE_CHECKED);
            break;
        case WT_SPINBUTTON:
            spin_afficher(n, lv_obj_get_child(o, 1));   /* « − », le champ, « + » */
            break;
        case WT_HSCALE: case WT_VSCALE: {   /* un « refresh » rejoue l'<input> */
            double pas = n->state.scale.step > 0 ? n->state.scale.step : 1;
            lv_slider_set_value(o, (int32_t)((n->state.scale.value - n->state.scale.min) / pas), LV_ANIM_OFF);
            break;
        }
        case WT_PROGRESSBAR:
            /* le cœur pose une fraction 0..1 (widget_progressbar.c) */
            lv_bar_set_value(o, (int32_t)(n->state.progress.value * 100.0 + 0.5), LV_ANIM_OFF);
            break;
        case WT_LEVELBAR:
            niveau_afficher(n, o);
            break;
        case WT_LIST:
            if ((int)lv_obj_get_child_count(o) != n->state.list.item_count + liste_entete(n)) {
                lv_obj_t *parent = lv_obj_get_parent(o);
                uint32_t  place  = lv_obj_get_index(o);
                lv_obj_delete(o);
                construire_liste(n, parent);
                lv_obj_move_to_index((lv_obj_t *)n->lv, (int32_t)place);
            }
            break;
        default: break;
        }
    }
    for (int i = 0; i < n->child_count; i++) synchroniser(n->children[i]);
    /* après les enfants : leur « visible » vient d'être remis à l'écran */
    if (o && (n->type == WT_STACK || n->type == WT_WIZARD)) pages_montrer(n, o);
}

/* ─── Fenêtre X11 : ce que le pilote de LVGL ne fait pas ─────────────────────
 * Le pilote pose le titre par XSetStandardProperties (WM_NAME en STRING : un
 * titre UTF-8 s'y affiche de travers) et aucune classe. On retrouve NOTRE
 * fenêtre parmi les enfants de la racine — ses identifiants portent la base
 * de ressources de notre connexion — et on pose _NET_WM_NAME (UTF-8) et
 * WM_CLASS = lvglsermo, que les gestionnaires et les bancs lisent. */
static Display *g_x;
static Window   g_fenetre;

static void fenetre_nommer(lv_display_t *d, const char *title)
{
    _x11_user_hdr_t *h = (_x11_user_hdr_t *)lv_display_get_driver_data(d);
    if (!h || !h->display) return;
    Display *x = h->display;
    Window racine, mere, *enfants = NULL;
    unsigned int nb = 0;
    g_x = x;
    if (!XQueryTree(x, DefaultRootWindow(x), &racine, &mere, &enfants, &nb)) return;
    for (unsigned int i = 0; i < nb; i++)
        if ((enfants[i] & ~x->resource_mask) == x->resource_base) { g_fenetre = enfants[i]; break; }
    if (enfants) XFree(enfants);
    if (!g_fenetre) return;

    XClassHint classe = { (char *)"lvglsermo", (char *)"lvglsermo" };
    XSetClassHint(x, g_fenetre, &classe);
    Atom utf8 = XInternAtom(x, "UTF8_STRING", False);
    XChangeProperty(x, g_fenetre, XInternAtom(x, "_NET_WM_NAME", False), utf8, 8,
                    PropModeReplace, (const unsigned char *)title, (int)strlen(title));
    XFlush(x);
}

/* Le pilote relit la souris toutes les LV_DEF_REFR_PERIOD ms et n'en garde que
 * l'état du moment : un appui et sa relâche arrivés entre deux relevés (un
 * clic d'outil, un toucher bref) ne font AUCUN clic. Avant chaque tour, une
 * relâche qui suit un appui encore en file est mise de côté, et rendue à la
 * file 60 ms plus tard — après au moins un relevé « appuyé ». */
static XEvent   g_relache;
static int      g_relache_tenue;
static uint32_t g_relache_quand;

static Bool est_appui(Display *x, XEvent *e, XPointer a)
{ (void)x; (void)a; return e->type == ButtonPress && e->xbutton.button == Button1; }
static Bool est_relache(Display *x, XEvent *e, XPointer a)
{ (void)x; (void)a; return e->type == ButtonRelease && e->xbutton.button == Button1; }

static void clic_bref_tenir(void)
{
    if (!g_x) return;
    if (g_relache_tenue) {
        if (lv_tick_elaps(g_relache_quand) >= 60) {
            XPutBackEvent(g_x, &g_relache);
            g_relache_tenue = 0;
        }
        return;
    }
    XEvent e;
    if (!XCheckIfEvent(g_x, &e, est_appui, NULL)) return;
    XEvent r;
    if (XCheckIfEvent(g_x, &r, est_relache, NULL)) {
        g_relache = r; g_relache_tenue = 1; g_relache_quand = lv_tick_get();
    }
    XPutBackEvent(g_x, &e);   /* l'appui retourne en tête de file */
}

/* ─── --render-png : le dialogue dessiné dans une image, sans fenêtre ─────── */
static uint8_t *g_image;
static int      g_image_l, g_image_h;

static void image_recevoir(lv_display_t *d, const lv_area_t *zone, uint8_t *px)
{
    int32_t l = lv_area_get_width(zone);
    for (int32_t y = zone->y1; y <= zone->y2; y++)
        memcpy(g_image + ((size_t)y * g_image_l + zone->x1) * 4,
               px + (size_t)(y - zone->y1) * l * 4, (size_t)l * 4);
    lv_display_flush_ready(d);
}

static int image_ecrire(const char *chemin)
{
    FILE *f = fopen(chemin, "wb");
    if (!f) return 1;
    png_structp png = png_create_write_struct(PNG_LIBPNG_VER_STRING, NULL, NULL, NULL);
    png_infop   info = png ? png_create_info_struct(png) : NULL;
    if (!png || !info || setjmp(png_jmpbuf(png))) {
        png_destroy_write_struct(&png, &info); fclose(f); return 1;
    }
    png_init_io(png, f);
    png_set_IHDR(png, info, (png_uint_32)g_image_l, (png_uint_32)g_image_h, 8,
                 PNG_COLOR_TYPE_RGBA, PNG_INTERLACE_NONE, PNG_COMPRESSION_TYPE_DEFAULT,
                 PNG_FILTER_TYPE_DEFAULT);
    png_write_info(png, info);
    png_set_bgr(png);                                /* LVGL : B, G, R, X en mémoire */
    for (int y = 0; y < g_image_h; y++) {
        uint8_t *ligne = g_image + (size_t)y * g_image_l * 4;
        for (int x = 0; x < g_image_l; x++) ligne[x * 4 + 3] = 0xFF;   /* opaque */
        png_write_row(png, ligne);
    }
    png_write_end(png, NULL);
    png_destroy_write_struct(&png, &info);
    return fclose(f) == 0 ? 0 : 1;
}

static void preparer_ecran(WidgetNode *root);
static void mesurer(WidgetNode *root, int *l, int *h);

static int run_png(WidgetNode *root, int win_w, int win_h)
{
    lv_init();
    /* Sans taille demandée, l'image prend celle du contenu, comme la fenêtre
     * (run_window) et comme l'étalon. Elle faisait toujours 640×480 : une case
     * à cocher tenait dans un coin d'une image vide (captures du site). */
    /* Une taille demandée (default-width/height) est un point de départ, pas
     * un plafond : comme GTK, jamais plus petite que le contenu. Le formulaire
     * du site (480×420) sortait coupé sous ses boutons. */
    {
        int l, h;
        mesurer(root, &l, &h);
        if (win_w < l) win_w = l;
        if (win_h < h) win_h = h;
    }
    g_image_l = win_w;
    g_image_h = win_h;
    g_image = calloc((size_t)g_image_l * g_image_h, 4);
    size_t taille = (size_t)g_image_l * g_image_h * 4;
    uint8_t *tampon = malloc(taille);
    if (!g_image || !tampon) return 1;
    lv_display_t *d = lv_display_create(g_image_l, g_image_h);
    lv_display_set_color_format(d, LV_COLOR_FORMAT_XRGB8888);
    lv_display_set_buffers(d, tampon, NULL, (uint32_t)taille, LV_DISPLAY_RENDER_MODE_DIRECT);
    lv_display_set_flush_cb(d, image_recevoir);
    preparer_ecran(root);
    /* Les barres lisent leur commande <input> au fil de l'eau : on relève ce
     * qu'elles ont écrit, 2 s au plus (une commande sans fin ne bloque pas
     * l'image), puis on remet l'état du cœur à l'écran. */
    for (int t = 0; t < 100 && servir_barres(root) > 0; t++) usleep(20000);
    synchroniser(root);
    for (int i = 0; i < 6; i++) { lv_tick_inc(40); lv_timer_handler(); }
    lv_refr_now(d);
    int rc = image_ecrire(option_render_png);
    if (rc) fprintf(stderr, "lvglsermo: --render-png : écriture de %s impossible\n", option_render_png);
    return rc;
}

/* Police, mise en page de l'écran, clavier, puis l'arbre. */
static void preparer_ecran(WidgetNode *root)
{
    g_police = police_systeme(15);
    /* La police va au THÈME, pas seulement à l'écran : le thème pose la sienne
     * (Montserrat, sans accents) sur les onglets, les tableaux, les menus —
     * « Général » y sortait « G?n?ral ». */
    lv_display_t *d = lv_display_get_default();
    lv_theme_t *th = lv_theme_default_init(d, lv_palette_main(LV_PALETTE_BLUE),
                                           lv_palette_main(LV_PALETTE_RED), false, g_police);
    if (th) lv_display_set_theme(d, th);
    lv_obj_t *ecran = lv_screen_active();
    lv_obj_set_style_text_font(ecran, g_police, 0);
    lv_obj_set_style_pad_all(ecran, 8, 0);
    lv_obj_set_scroll_dir(ecran, LV_DIR_VER);   /* un dialogue trop haut défile, jamais de côté */

    /* Clavier : les champs rejoignent le groupe par défaut, que le clavier X11 sert. */
    lv_group_t *g = lv_group_create();
    lv_group_set_default(g);
    for (lv_indev_t *i = lv_indev_get_next(NULL); i; i = lv_indev_get_next(i))
        if (lv_indev_get_type(i) == LV_INDEV_TYPE_KEYPAD) lv_indev_set_group(i, g);

    construire(root, ecran);
    if (!g_mesure) textes_poser(root);
    hauteurs_poser(root);
}

/* ─── Taille de la fenêtre : celle du contenu, comme chez l'étalon ──────────
 * Le dialogue est construit une fois sur un écran hors champ, mesuré, puis
 * défait ; la fenêtre X11 s'ouvre à cette taille (bornée), sauf taille
 * demandée par default-width/height. */
static void oublier(WidgetNode *n)
{
    n->lv = NULL;
    n->lv_case = NULL;
    n->lv_hauteur = 0;
    for (int i = 0; i < n->child_count; i++) oublier(n->children[i]);
}

static void mesure_recevoir(lv_display_t *d, const lv_area_t *z, uint8_t *px)
{ (void)z; (void)px; lv_display_flush_ready(d); }


/* Un bouton de fichier, de couleur ou de police ouvre un dialogue DANS la
 * fenêtre : elle doit pouvoir le contenir. */
static int a_un_dialogue(const WidgetNode *n)
{
    if (n->type == WT_FILECHOOSER || n->type == WT_COLORBUTTON || n->type == WT_FONTBUTTON
        || (n->var_name && strcmp(n->var_name, "__fontbutton__") == 0))
        return 1;
    for (int i = 0; i < n->child_count; i++) if (a_un_dialogue(n->children[i])) return 1;
    return 0;
}

static void mesurer(WidgetNode *root, int *l, int *h)
{
    static uint8_t tampon[64 * 64 * 4];
    lv_display_t *d = lv_display_create(1600, 1000);
    lv_display_set_buffers(d, tampon, NULL, sizeof tampon, LV_DISPLAY_RENDER_MODE_PARTIAL);
    lv_display_set_flush_cb(d, mesure_recevoir);
    lv_display_set_default(d);
    int pad = 2 * 8;   /* marge de l'écran, posée par preparer_ecran */
    /* 1re passe : largeur naturelle du contenu. */
    g_mesure = 1;
    preparer_ecran(root);
    lv_obj_update_layout(lv_screen_active());
    lv_obj_t *b = (lv_obj_t *)root->lv;
    g_mesure_l = b ? lv_obj_get_width(b) : 300;
    /* 2e passe : à cette largeur, avec la pleine largeur des colonnes, la
     * hauteur réelle (un enfant élargi peut changer de hauteur). */
    lv_obj_clean(lv_screen_active());
    oublier(root);
    g_mesure = 2;
    preparer_ecran(root);
    lv_obj_update_layout(lv_screen_active());
    b = (lv_obj_t *)root->lv;
    *l = g_mesure_l + pad;
    *h = (b ? lv_obj_get_height(b) : 200) + pad;
    g_mesure = 0;
    oublier(root);
    lv_display_delete(d);
    if (*l < 200) *l = 200;
    if (*h < 80)  *h = 80;
    if (a_un_dialogue(root)) {
        if (*l < 380) *l = 380;
        if (*h < 340) *h = 340;
    }
    if (*l > 1600) *l = 1600;
    if (*h > 1000) *h = 1000;
}

static int veut_wayland(void)
{
    const char *w = getenv("WAYLAND_DISPLAY"), *x = getenv("SERMO_LVGL_X11");
    return w && *w && !(x && *x && strcmp(x, "0") != 0);
}

/* evdev « auto » : chaque périphérique trouvé (souris, clavier, toucher)
 * rejoint l'écran ; un clavier, le groupe des champs. */
static void evdev_trouve(lv_indev_t *i, lv_evdev_type_t type, void *ud)
{
    (void)type; (void)ud;
    if (lv_indev_get_type(i) == LV_INDEV_TYPE_KEYPAD) lv_indev_set_group(i, lv_group_get_default());
}

static int run_fbdev(WidgetNode *root, const char *fb)
{
    lv_init();
    lv_display_t *d = lv_linux_fbdev_create();
    if (!d || lv_linux_fbdev_set_file(d, fb) != LV_RESULT_OK) {
        fprintf(stderr, "lvglsermo: framebuffer %s inutilisable — dialogue sans écran\n", fb);
        if (d) lv_display_delete(d);
        return run_headless(root);
    }
    const char *ev = getenv("SERMO_LVGL_EVDEV");
    if (ev && strcmp(ev, "auto") == 0) {
        lv_evdev_discovery_start(evdev_trouve, NULL);
    } else if (ev && *ev) {
        lv_indev_t *p = lv_evdev_create(LV_INDEV_TYPE_POINTER, ev);
        if (!p) fprintf(stderr, "lvglsermo: entrée %s inutilisable\n", ev);
    }
    preparer_ecran(root);
    listes_chargees(root);                 /* un « exit:… » peut sortir ici */
    for (;;) {
        uint32_t attente = lv_timer_handler();
        servir_barres(root);
        synchroniser(root);
        if (attente > 20 || attente == LV_NO_TIMER_READY) attente = 20;
        usleep(attente * 1000);
    }
    return 0;   /* jamais atteint : on sort par une action exit:… du cœur */
}

/* ─── Écran X11 : ouvert UNE fois, par le port, puis remis à LVGL ───────────
 *
 * lv_x11_window_create (LVGL embarqué, tiers/ sans retouche) appelle
 * XOpenDisplay sans tester le retour : un écran injoignable plantait dans
 * XDefaultScreen (pile relevée le 2026-10-14 sous ci/bancs.sh). L'ancien
 * garde (6075a35) ouvrait l'écran, le refermait, puis laissait LVGL le
 * rouvrir : la seconde ouverture restait sans garde, et la fermeture d'un
 * client peut faire réinitialiser un serveur X lancé sans -noreset (Xvfb des
 * bancs) — la connexion suivante tombe alors en pleine réinitialisation.
 *
 * Le lien passe -Wl,--wrap=XOpenDisplay (CMakeLists.txt) : tout appel à
 * XOpenDisplay du binaire, LVGL compris, arrive ici. La connexion ouverte par
 * run_window est remise telle quelle au pilote de LVGL (qui la refermera à la
 * destruction de l'écran) ; toute autre ouverture ratée s'arrête proprement,
 * comme GTK. Sans --wrap, __real_XOpenDisplay manque : le lien échoue, le
 * garde ne peut pas disparaître en silence. */
static Display *g_ecran_x11;           /* ouvert par run_window, pas encore remis */
Display *__real_XOpenDisplay(const char *nom);
Display *__wrap_XOpenDisplay(const char *nom);
Display *__wrap_XOpenDisplay(const char *nom)
{
    if (g_ecran_x11) {
        Display *d = g_ecran_x11;
        g_ecran_x11 = NULL;
        return d;
    }
    Display *d = __real_XOpenDisplay(nom);
    if (!d) {
        fprintf(stderr, "lvglsermo: cannot open display: %s\n", XDisplayName(nom));
        exit(EXIT_FAILURE);
    }
    return d;
}

static int run_window(WidgetNode *root, const char *title, int win_w, int win_h)
{
    lv_init();
    {   /* comme run_png : la taille demandée, jamais sous le contenu */
        int l, h;
        mesurer(root, &l, &h);
        if (win_w < l) win_w = l;
        if (win_h < h) win_h = h;
    }
    lv_display_t *d = NULL;
    if (veut_wayland()) {
        char titre[256];
        snprintf(titre, sizeof titre, "%s", title ? title : "lvglsermo");
        d = lv_wayland_window_create((uint32_t)win_w, (uint32_t)win_h, titre, NULL);
        if (!d) fprintf(stderr, "lvglsermo: Wayland injoignable, essai par X11\n");
        /* fermeture par le compositeur : l'écran est détruit, la boucle sort
         * par EXIT="abort" comme sous X11 (le rappel de fermeture est déprécié) */
        else lv_display_add_event_cb(d, ecran_ferme, LV_EVENT_DELETE, NULL);
    }
    if (!d) {
        const char *x = getenv("DISPLAY");
        if (x && *x) {
            /* une seule connexion : ouverte ici (l'échec s'arrête dans
             * __wrap_XOpenDisplay), remise à LVGL par la même enveloppe */
            g_ecran_x11 = XOpenDisplay(NULL);
            d = lv_x11_window_create(title, win_w, win_h);
            if (g_ecran_x11) {             /* pas remise : création ratée avant */
                XCloseDisplay(g_ecran_x11);
                g_ecran_x11 = NULL;
            }
        }
        if (!d) return run_headless(root);
        lv_x11_inputs_create(d, NULL);
        lv_display_add_event_cb(d, ecran_ferme, LV_EVENT_DELETE, NULL);
        fenetre_nommer(d, title);
    }

    preparer_ecran(root);
    listes_chargees(root);                 /* un « exit:… » peut sortir ici */

    while (!g_fermee) {
        clic_bref_tenir();
        uint32_t attente = lv_timer_handler();
        servir_barres(root);
        synchroniser(root);
        if (attente > 20 || attente == LV_NO_TIMER_READY) attente = 20;
        usleep(attente * 1000);
    }
    return export_abort();
}

/* ─── Point d'entrée ───────────────────────────────────────────────────────── */
int render_loop(WidgetNode *root, const char *title, int win_w, int win_h)
{
    if (!root) return 0;
    const char *ecran = getenv("DISPLAY");
    if (option_render_png && *option_render_png)
        return run_png(root, win_w, win_h);
    const char *fb = getenv("SERMO_LVGL_FBDEV");
    if (fb && *fb && !getenv("SERMO_LVGL_BATCH"))
        return run_fbdev(root, fb);
    if (getenv("SERMO_LVGL_BATCH") || ((!ecran || !*ecran) && !veut_wayland()))
        return run_headless(root);
    return run_window(root, title, win_w, win_h);
}
