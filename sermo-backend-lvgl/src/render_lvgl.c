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

#include <X11/Xlib.h>
#include <X11/Xatom.h>
#include <X11/Xutil.h>
#include <X11/Xlibint.h>   /* resource_base/resource_mask : reconnaître NOTRE fenêtre */
#include <png.h>
#include <dirent.h>
#include <limits.h>
#include <sys/stat.h>
#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
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
    if (actif == n->state.toggle.active) return;
    n->state.toggle.active = actif;
    if (n->action && *n->action) declencher(n, "toggled", "clicked");
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
    } else {
        snprintf(n->state.entry.buf, sizeof(n->state.entry.buf), "%s", t ? t : "");
    }
}

static void liste_clic(lv_event_t *e)
{
    WidgetNode *n = (WidgetNode *)lv_event_get_user_data(e);
    lv_obj_t   *b = lv_event_get_target_obj(e);
    int         i = (int)(intptr_t)lv_obj_get_user_data(b);
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

static void construire_enfants(WidgetNode *n, lv_obj_t *o)
{
    for (int i = 0; i < n->child_count; i++) construire(n->children[i], o);
}

/* <list> : une colonne de boutons cochables qui défile (lv_list est
 * déprécié en 9.6 au profit de ce montage). */
static void construire_liste(WidgetNode *n, lv_obj_t *parent)
{
    lv_obj_t *l = lv_obj_create(parent);
    lv_obj_set_size(l, n->width > 0 ? n->width : 220, n->height > 0 ? n->height : 140);
    /* Largeur naturelle : en pour-cent (largeur ou largeur minimale), ce
     * conteneur défilant tombait à 0 px ou n'en tenait pas compte (mesuré). */
    lv_obj_set_flex_flow(l, LV_FLEX_FLOW_COLUMN);
    lv_obj_set_style_pad_gap(l, 2, 0);
    n->lv = l;
    for (int i = 0; i < n->state.list.item_count; i++) {
        lv_obj_t *b = lv_button_create(l);
        lv_obj_set_width(b, lv_pct(100));
        lv_label_set_text(lv_label_create(b), n->state.list.items[i]);
        lv_obj_set_user_data(b, (void *)(intptr_t)i);
        lv_obj_set_checkable(b, true);
        /* Ligne choisie : bleu nuit #1A3A5C (le thème mettait du rouge, que
         * rien ne distingue du bleu des autres lignes pour un daltonien) —
         * elle se lit par la clarté, pas par la teinte. */
        lv_obj_set_style_bg_color(b, lv_color_hex(0x1A3A5C), LV_STATE_CHECKED);
        if (i == n->state.list.selected_index) lv_obj_add_state(b, LV_STATE_CHECKED);
        lv_obj_add_event_cb(b, liste_clic, LV_EVENT_CLICKED, n);
    }
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
    lv_tabview_set_tab_bar_size(tv, barre);
    if (n->state.notebook.side) lv_tabview_set_tab_bar_position(tv, LV_DIR_LEFT);
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

static void construire(WidgetNode *n, lv_obj_t *parent)
{
    lv_obj_t *o = NULL;
    const char *lbl = n->label ? n->label : "";

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
    case WT_VBOX: case WT_EXPANDER:
    case WT_EVENTBOX: case WT_STACK: case WT_WIZARD: case WT_REVEALER:
    case WT_OVERLAY: case WT_ASPECTFRAME: case WT_PANED:
        o = boite(parent, LV_FLEX_FLOW_COLUMN);
        construire_enfants(n, o);
        break;
    case WT_HBOX: case WT_TOOLBAR: case WT_FLOWBOX: case WT_GRID:
        o = boite(parent, n->type == WT_FLOWBOX || n->type == WT_GRID
                          ? LV_FLEX_FLOW_ROW_WRAP : LV_FLEX_FLOW_ROW);
        /* Une étiquette se centre sur la hauteur de son champ, comme chez l'étalon. */
        lv_obj_set_flex_align(o, LV_FLEX_ALIGN_START, LV_FLEX_ALIGN_CENTER, LV_FLEX_ALIGN_CENTER);
        construire_enfants(n, o);
        break;
    case WT_FRAME: {
        o = lv_obj_create(parent);
        lv_obj_set_size(o, LV_SIZE_CONTENT, LV_SIZE_CONTENT);
        lv_obj_set_flex_flow(o, LV_FLEX_FLOW_COLUMN);
        lv_obj_set_scrollable(o, false);
        if (lbl[0]) lv_label_set_text(lv_label_create(o), lbl);
        construire_enfants(n, o);
        break;
    }
    case WT_TEXT: case WT_STATUSBAR: case WT_INFOBAR:
        o = lv_label_create(parent);
        lv_label_set_text(o, lbl);
        break;
    case WT_BUTTON: case WT_LINKBUTTON: case WT_MENUITEM: case WT_FONTBUTTON:
    case WT_COLORBUTTON: case WT_FILECHOOSER: {
        o = lv_button_create(parent);
        lv_label_set_text(lv_label_create(o), lbl[0] ? lbl : "OK");
        lv_obj_add_event_cb(o, bouton_clic, LV_EVENT_CLICKED, n);
        if (n->type == WT_COLORBUTTON) {
            /* le bouton montre la couleur retenue ; texte clair ou foncé selon
             * la clarté du fond, pour rester lisible */
            lv_color_t c = lv_color_make((uint8_t)(n->state.color.r * 255), (uint8_t)(n->state.color.g * 255),
                                         (uint8_t)(n->state.color.b * 255));
            lv_obj_set_style_bg_color(o, c, 0);
            lv_obj_set_style_text_color(o, lv_color_luminance(c) > 140 ? lv_color_black() : lv_color_white(), 0);
        }
        break;
    }
    case WT_CHECKBOX: case WT_RADIOBUTTON: case WT_TOGGLEBUTTON:
        o = lv_checkbox_create(parent);
        lv_checkbox_set_text(o, lbl);
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
        lv_textarea_set_text(o, n->state.entry.buf);
        lv_obj_add_event_cb(o, champ_change, LV_EVENT_VALUE_CHANGED, n);
        break;
    case WT_EDIT:
        o = lv_textarea_create(parent);
        lv_obj_set_size(o, n->width > 0 ? n->width : 300, n->height > 0 ? n->height : 120);
        lv_textarea_set_text(o, n->state.text.content ? n->state.text.content : "");
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
        lv_obj_set_width(o, 200);
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
        break;
    case WT_SPINNER:
        o = lv_spinner_create(parent);
        lv_obj_set_size(o, 32, 32);
        break;
    case WT_SEPARATOR:
        o = lv_obj_create(parent);
        lv_obj_set_size(o, lv_pct(100), 2);
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
        construire_enfants(n, o);
        break;
    }

    finir(n, o);
}

static void finir(WidgetNode *n, lv_obj_t *o)
{
    n->lv = o;
    if (o && n->type == WT_TEXT) lv_obj_set_style_text_align(o, LV_TEXT_ALIGN_CENTER, 0);
    /* Pleine largeur dans une colonne — pas pendant la mesure : une largeur en
     * pour-cent d'une boîte taillée sur son contenu ne se mesure pas. */
    if (o && g_mesure != 1 && colonne(n->parent) && !(n->width > 0) && n->type != WT_TABLE)
        lv_obj_set_width(o, lv_pct(100));   /* lv_table : une largeur en pour-cent la met à 0 px (mesuré) */
    /* space-expand="true" : l'enfant prend la place restante dans sa boîte,
     * en largeur dans un hbox, en hauteur dans un vbox — et remplit l'autre
     * sens, comme chez l'étalon (space-fill). */
    if (o && n->expand > 0 && g_mesure != 1) {
        lv_obj_set_flex_grow(o, 1);
        if (n->parent && (n->parent->type == WT_HBOX || n->parent->type == WT_TOOLBAR)) {
            /* Remplir la hauteur d'une rangée n'a de sens que si la rangée en a
             * une : taillée sur son contenu, un 100 % devient circulaire et LVGL
             * écrase le champ (texte coupé, mesuré sur 01-formulaire). */
            if (n->parent->height > 0) lv_obj_set_height(o, lv_pct(100));
        } else
            lv_obj_set_width(o, lv_pct(100));
    }
    if (o && !n->visible) lv_obj_set_hidden(o, true);
    if (o && !n->sensitive) lv_obj_add_state(o, LV_STATE_DISABLED);
}

/* ─── Synchronisation : ce que le cœur a changé revient à l'écran ─────────── */
static void synchroniser(WidgetNode *n)
{
    lv_obj_t *o = (lv_obj_t *)n->lv;
    if (o) {
        lv_obj_set_hidden(o, !n->visible);
        if (n->sensitive) lv_obj_remove_state(o, LV_STATE_DISABLED);
        else              lv_obj_add_state(o, LV_STATE_DISABLED);

        switch (n->type) {
        case WT_TEXT: case WT_STATUSBAR: case WT_INFOBAR: {
            const char *t = n->label ? n->label : "";
            if (strcmp(lv_label_get_text(o), t) != 0) lv_label_set_text(o, t);
            break;
        }
        case WT_ENTRY: case WT_PASSWORD: case WT_SEARCHENTRY:
            if (strcmp(lv_textarea_get_text(o), n->state.entry.buf) != 0)
                lv_textarea_set_text(o, n->state.entry.buf);
            break;
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
        case WT_PROGRESSBAR:
            /* le cœur pose une fraction 0..1 (widget_progressbar.c) */
            lv_bar_set_value(o, (int32_t)(n->state.progress.value * 100.0 + 0.5), LV_ANIM_OFF);
            break;
        case WT_LIST:
            if ((int)lv_obj_get_child_count(o) != n->state.list.item_count) {
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

static int run_png(WidgetNode *root, int win_w, int win_h)
{
    lv_init();
    g_image_l = win_w > 0 ? win_w : 640;
    g_image_h = win_h > 0 ? win_h : 480;
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
}

/* ─── Taille de la fenêtre : celle du contenu, comme chez l'étalon ──────────
 * Le dialogue est construit une fois sur un écran hors champ, mesuré, puis
 * défait ; la fenêtre X11 s'ouvre à cette taille (bornée), sauf taille
 * demandée par default-width/height. */
static void oublier(WidgetNode *n)
{
    n->lv = NULL;
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

static int run_window(WidgetNode *root, const char *title, int win_w, int win_h)
{
    lv_init();
    if (win_w <= 0 || win_h <= 0) {
        int l, h;
        mesurer(root, &l, &h);
        if (win_w <= 0) win_w = l;
        if (win_h <= 0) win_h = h;
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
        if (x && *x) d = lv_x11_window_create(title, win_w, win_h);
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
