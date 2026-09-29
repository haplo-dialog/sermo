/* SPDX-License-Identifier: MIT */
/*
 * sermo-contract.h — Frontière cœur <-> backend de rendu de sermo.
 *
 * Copyright (C) 2026 S. Cage
 * haplo-dialog <devel@haplo-dialog.fr>
 *
 * Permission is hereby granted, free of charge, to any person obtaining a copy
 * of this software and associated documentation files (the "Software"), to deal
 * in the Software without restriction, including without limitation the rights
 * to use, copy, modify, merge, publish, distribute, sublicense, and/or sell
 * copies of the Software, and to permit persons to whom the Software is
 * furnished to do so, subject to the following conditions:
 *
 * The above copyright notice and this permission notice shall be included in
 * all copies or substantial portions of the Software.
 *
 * THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR
 * IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY,
 * FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT. IN NO EVENT SHALL THE
 * AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER
 * LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM,
 * OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN THE
 * SOFTWARE.
 *
 * ─────────────────────────────────────────────────────────────────────────────
 * Ce fichier définit UNIQUEMENT le joint entre le cœur et un backend de rendu :
 * des noms et des signatures, aucune implémentation. Un cœur et un backend qui
 * le respectent interopèrent.
 *
 * Aucune inclusion de <glib.h> : les quelques types de base nécessaires sont
 * déclarés plus bas, compatibles à la fois avec la vraie GLib (famille GTK) et
 * avec les shims des backends neutres.
 * ─────────────────────────────────────────────────────────────────────────────
 */
#ifndef SERMO_CONTRACT_H
#define SERMO_CONTRACT_H

/* Types de base — le contrat n'inclut PAS <glib.h>. Il déclare le minimum,
 * compatible ABI à la fois avec la vraie GLib (famille GTK) et avec les shims des
 * backends neutres (qt6/sdl3/ncurses/fltk1/efl1), qui définissent les mêmes.
 * `GList` reste INCOMPLÈTE ici (usage par pointeur uniquement) : GLib ou le shim
 * en fournit la définition. Ces typedefs identiques sont ré-déclarables (gnu11). */
typedef char          gchar;
typedef int           gint;
typedef int           gboolean;
typedef struct _GList GList;

#ifdef __cplusplus
extern "C" {
#endif

/* ── Types du joint ────────────────────────────────────────────────────────
 * Opaques : le contrat n'expose AUCUNE structure interne. Le cœur (libsermocore)
 * en fournit la définition et les accesseurs ; le backend ne les manipule que
 * par pointeur + fonctions d'accès ci-dessous. */
typedef void                 sermo_widget;   /* widget natif du toolkit (handle) */
typedef struct _AttributeSet AttributeSet;   /* jeu d'attributs analysés (opaque) */
typedef struct _tag_attr     tag_attr;       /* attributs d'une balise (opaque)   */

/* ═══ Le BACKEND fournit — le CŒUR appelle ═══════════════════════════════════ */

/* Initialise le toolkit (gtk_init / QApplication / SDL_Init / ncurses…).
 * Si print_ir != 0 : s'initialiser SANS ouvrir d'affichage (analyse headless). */
void          sermo_backend_toolkit_init(int *argc, char ***argv, int print_ir);

/* Pont d'opérations (le cœur ne nomme plus de toolkit). */
void          sermo_be_app_init(int *argc, char ***argv);
int           sermo_be_run_loop(void);
void          sermo_be_app_quit(void);
void          sermo_be_widget_show(sermo_widget *w);
void          sermo_be_widget_hide(sermo_widget *w);
void          sermo_be_widget_set_sensitive(sermo_widget *w, int sensitive);
void          sermo_be_widget_redraw(sermo_widget *w);
void          sermo_be_container_add(sermo_widget *container, sermo_widget *child);
sermo_widget *sermo_be_container_child0(sermo_widget *container);
sermo_widget *sermo_be_scroll_new(int width, int height);
void          sermo_be_window_move(sermo_widget *w, int x, int y);

/* Identité du port, pour « --version ». FACULTATIF : ces deux symboles sont
 * déclarés FAIBLES par le cœur. Un backend qui ne les remplit pas se lie quand
 * même, et « --version » retombe sur le nom du cœur — donc pas de rupture du
 * contrat, et pas de bump majeur.
 *
 * Le plus simple est de compiler contract/sermo_port_id.c dans le port, en lui
 * passant SERMO_PORT_NAME et SERMO_PORT_VERSION (voir les CMakeLists).
 *
 * ⚠️ La ligne de « --version » a une forme FIGÉE : le 1er mot est le nom, le
 * 3e le numéro. Des exemples livrés lisent la version à ce rang. */
extern const char *sermo_port_name;      /* « gtk3sermo », « qt6sermo »…      */
extern const char *sermo_port_details;   /* « gtk3sermo 2.7.1 (haplo-dialog) » */

/* Applique les attributs de balise à un widget natif (spécifique au toolkit :
 * introspection GObject pour GTK, réimplémentation pour les backends neutres). */
gint          widget_set_tag_attributes(sermo_widget *widget, tag_attr *attr);

/* Fabrique de widget : une par type (button, entry, list, tree, table, notebook,
 * calendar, timer, …). Toutes suivent cette signature ; le cœur les appelle par
 * table selon le type analysé. Elles rendent le widget natif (handle opaque). */
typedef sermo_widget *(*sermo_widget_create_fn)(AttributeSet *attr,
                                                tag_attr     *tagattr,
                                                gint          widget_type);

/* ═══ Le CŒUR fournit — le BACKEND appelle ═══════════════════════════════════ */

/* Exécute l'action liée à un évènement (clic, timer, activation…). */
int           execute_action(sermo_widget *w, const char *command, const char *type);

/* Chargement / identité du programme. */
gint          get_program_from_variable(gchar *name);
gchar        *get_program_name(void);

/* — Modèle d'attributs (données ; le backend LIT par ces accesseurs) — */
gchar        *attribute_name(gint attribute);
AttributeSet *attributeset_new(void);
gboolean      attributeset_is_avail(AttributeSet *set, int attribute);
gboolean      attributeset_cmp_left(AttributeSet *set, int attribute, const char *str);
gchar        *attributeset_set_if_unset(AttributeSet *set, gint attribute, const gchar *value);
const char   *attributeset_insert(AttributeSet *set, int attribute, const char *value);
const char   *attributeset_insert_with_tagattrs(AttributeSet *set, int attribute,
                                                const char *s, tag_attr *t_attr);
/* Itérateurs réentrants (l'appelant fournit l'état `element`). */
gchar        *attributeset_get_first(GList **element, AttributeSet *set, gint type);
gchar        *attributeset_get_next(GList **element, AttributeSet *set, gint type);
gchar        *attributeset_get_this_tagattr(GList **element, AttributeSet *set,
                                            gint type, gchar *name);
void          attributeset_set_this_tagattr(GList **element, AttributeSet *set,
                                            gint type, gchar *name, gchar *value);

/* — Attributs de balise (tag_attr) — */
GList        *tag_attributeset_append(GList *list, gchar *name, gchar *value);
char         *get_tag_attribute(tag_attr *attr, const char *name);
tag_attr     *add_tag_attribute(tag_attr *attr, char *name, char *value);
tag_attr     *new_tag_attributeset(char *name, char *value);
void          kill_tag_attribute(tag_attr *attr, const char *name);

#ifdef __cplusplus
}
#endif
#endif /* SERMO_CONTRACT_H */
