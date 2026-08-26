# Manuel Développeur et Mainteneur — gtk4sermo (gtk4sermo)

**Version :** 1.0.0-gtk4  
**Date :** Juin 2026 (mis à jour 2026-06-01)  
**Audience :** Développeurs C, mainteneurs haplo-dialog  
**Mainteneur :** haplo-dialog <devel@haplo-dialog.fr> | **Dépôt :** https://haplo-dialog.fr

> **État de compilation (2026-06-01) :** `gtk4sermo` compile et lie sans erreur
> (binaire ~1,45 Mo). Les warnings ont été réduits de 1549 à 128 (~92 %).
> Le port GTK4 des API GTK2/3 supprimées est complet (voir `src/gtk4-compat.h`).
> Les 128 warnings résiduels sont des idiomes hérités de l'amont gtkdialog 0.8.3
> (assignation en condition, casts entier↔pointeur) et ne sont pas bloquants.
> `src/Makefile.am` filtre les dépréciations GTK 4.22 et le bruit hérité via
> `-Wno-deprecated-declarations -Wno-missing-prototypes -Wno-unused-variable`.
> Voir `BILAN_SANTE.md` (audit #4) et `NEWS` pour le détail des correctifs.

---

## Table des matières

1. [Vue d'ensemble de l'architecture](#1-vue-densemble-de-larchitecture)
2. [Environnement de développement](#2-environnement-de-développement)
3. [Structure des fichiers source](#3-structure-des-fichiers-source)
4. [Flux d'exécution détaillé](#4-flux-dexécution-détaillé)
5. [Modules clés — description détaillée](#5-modules-clés--description-détaillée)
6. [Conventions de code](#6-conventions-de-code)
7. [Sécurité — règles obligatoires](#7-sécurité--règles-obligatoires)
8. [Ajouter un nouveau widget](#8-ajouter-un-nouveau-widget)
9. [Système de build](#9-système-de-build)
10. [Tests et débogage](#10-tests-et-débogage)
11. [Processus de contribution](#11-processus-de-contribution)
12. [Tâches de maintenance récurrentes](#12-tâches-de-maintenance-récurrentes)
13. [Roadmap technique](#13-roadmap-technique)
14. [Références et ressources](#14-références-et-ressources)

---

## 1. Vue d'ensemble de l'architecture

gtk4sermo est un programme C de ~35 000 lignes organisé en trois couches distinctes :

```
┌─────────────────────────────────────────────────────────┐
│                    COUCHE D'ENTRÉE                       │
│   gtkdialog.c ← arguments CLI, lecture XML             │
│   gtkdialog_lexer.l + gtkdialog_parser.y  ← parsing     │
└────────────────────────┬────────────────────────────────┘
                         │ Arbre d'instructions (AST)
┌────────────────────────▼────────────────────────────────┐
│                    COUCHE MOTEUR                         │
│   automaton.c   ← construction widgets depuis l'AST     │
│   variables.c   ← état des widgets, export stdout       │
│   signals.c     ← connexion signaux GTK ↔ actions       │
│   actions.c     ← traitement EXIT, REFRESH, LAUNCH…     │
│   safe_exec.c   ← exécution sécurisée de commandes      │
└────────────────────────┬────────────────────────────────┘
                         │ Appels GTK4 / GLib
┌────────────────────────▼────────────────────────────────┐
│                    COUCHE WIDGETS                        │
│   widgets.c     ← dispatch, widget_opencommand()        │
│   widget_*.c    ← 30 implémentations de widgets         │
│   glade_support.c ← support GtkBuilder (.glade/.ui)     │
└─────────────────────────────────────────────────────────┘
```

### 1.1 Principes de conception

- **Séparation parsing / rendu** : le parser produit une liste d'instructions (`instruction[]` dans `automaton.c`), puis l'automate les exécute pour créer les widgets GTK. Ces deux phases sont indépendantes.
- **Un fichier par widget** : chaque widget GTK est encapsulé dans sa propre paire `.c/.h`. Ce cloisonnement facilite l'ajout, la modification et le test de widgets individuellement.
- **État centralisé** : `variables.c` est le point unique de vérité pour l'état des widgets et l'export des valeurs.
- **Sécurité par couche** : toute exécution de commande externe passe obligatoirement par `safe_exec.c`.

---

## 2. Environnement de développement

### 2.1 Installation des dépendances

```bash
# Debian / Ubuntu / Parrot OS
sudo apt-get install \
  libgtk-3-dev libvte-2.91-dev \
  flex bison autoconf automake pkg-config \
  gcc gdb valgrind \
  clang-format cppcheck \
  git

# Arch Linux
sudo pacman -S gtk3 vte3 flex bison autoconf automake pkgconf gcc gdb valgrind
```

### 2.2 Compilation en mode développement

```bash
cd gtk4sermo/
./autogen.sh

# Mode debug avec sanitizers (détection mémoire et comportement indéfini)
./configure CFLAGS="-g -O0 -fsanitize=address,undefined -fno-omit-frame-pointer"
make -j$(nproc)

# Mode release (production)
./configure CFLAGS="-O2 -D_FORTIFY_SOURCE=3 -fstack-protector-strong -fPIE"
make -j$(nproc)
```

### 2.3 Variables d'environnement utiles

| Variable | Description |
|----------|-------------|
| `G_DEBUG=fatal_criticals` | Arrête le programme sur tout avertissement GLib critique |
| `G_MESSAGES_DEBUG=all` | Affiche tous les messages de debug GLib |
| `GTK_DEBUG=interactive` | Ouvre l'inspecteur GTK4 (Ctrl+Shift+I dans la fenêtre) |
| `GTK_THEME=Adwaita:dark` | Forcer un thème GTK pour les tests |

Exemple :
```bash
GTK_DEBUG=interactive G_DEBUG=fatal_criticals ./src/gtk4sermo --stdin < examples/button/button
```

---

## 3. Structure des fichiers source

```
src/
├── gtkdialog.c/.h         # Point d'entrée main(), parsing des options CLI
├── safe_exec.c/.h         # Wrappers sécurisés : safe_system(), safe_popen()
├── macros.h               # Macros de debug : PIP_DEBUG, WARNING, DEBUGMSG
├── config.h.in            # Template de config.h généré par autoconf
│
│   ── MOTEUR ──
├── automaton.c/.h         # Compilateur XML → widgets GTK (cœur du programme)
├── gtkdialog_parser.y     # Grammaire Bison (syntaxe gtkdialog XML)
├── gtkdialog_lexer.l      # Lexer Flex (tokenisation XML)
├── gtkdialog_parser.c/.h  # Fichiers générés par Bison (ne pas éditer)
├── gtkdialog_lexer.c      # Fichier généré par Flex (ne pas éditer)
├── variables.c/.h         # Gestion de l'état et export des variables
├── signals.c/.h           # Connexion des signaux GTK aux actions
├── actions.c/.h           # Implémentation des actions (EXIT, REFRESH, etc.)
├── attributes.c/.h        # Lecture des attributs XML des tags
├── tag_attributes.c/.h    # Stockage des attributs tag par widget
├── stack.c/.h             # Pile utilisée par l'automate
├── stringman.c/.h         # Manipulation de chaînes (recherche de fichiers, etc.)
├── printing.c/.h          # Support impression GTK
├── glade_support.c/.h     # Support GtkBuilder / fichiers .ui
│
│   ── WIDGETS (30 fichiers) ──
├── widgets.c/.h           # Dispatch, widget_opencommand(), widget_get_text_value()
├── widget_button.c/.h
├── widget_checkbox.c/.h
├── widget_colorbutton.c/.h
├── widget_combobox.c/.h
├── widget_comboboxtext.c/.h
├── widget_edit.c/.h
├── widget_entry.c/.h
├── widget_eventbox.c/.h
├── widget_expander.c/.h
├── widget_fontbutton.c/.h
├── widget_frame.c/.h
├── widget_hbox.c/.h
├── widget_hscale.c/.h
├── widget_hseparator.c/.h
├── widget_list.c/.h
├── widget_menubar.c/.h
├── widget_menuitem.c/.h
├── widget_notebook.c/.h
├── widget_pixmap.c/.h
├── widget_progressbar.c/.h
├── widget_radiobutton.c/.h
├── widget_spinbutton.c/.h
├── widget_statusbar.c/.h
├── widget_table.c/.h
├── widget_terminal.c/.h
├── widget_text.c/.h
├── widget_timer.c/.h
├── widget_tree.c/.h
├── widget_vbox.c/.h
└── widget_window.c/.h
```

---

## 4. Flux d'exécution détaillé

### 4.1 Démarrage

```c
// gtkdialog.c : main()
1. gtk_init()                          // Initialise GTK4 (sans argc/argv)
2. parse_options(argc, argv)        // Lit --stdin, --program, --file
3. load_program_source()            // Charge le XML dans program_src[]
4. yyparse()                        // Lance le parser Bison
   → appelle automaton_build()      // Construit la liste d'instructions
5. automaton_execute()              // Crée les widgets GTK depuis les instructions
6. gtk_main()                       // Boucle événements GTK
7. variables_export()               // Affiche les valeurs sur stdout
```

### 4.2 Parsing du XML (parser.y + lexer.l)

Le parser n'est **pas** un vrai parser XML : il utilise une grammaire Bison spécifique au format gtkdialog. Les tokens reconnus par le lexer sont les balises XML gtkdialog, pas du XML générique.

```
<window>    → TOKEN_WINDOW
<button>    → TOKEN_BUTTON
<label>     → TOKEN_LABEL
texte       → TOKEN_STRING
</window>   → TOKEN_END_WINDOW
```

Le parser produit une liste plate d'`instruction` :
```c
typedef struct {
    token  command;     // type de token (WIDGET_BUTTON, ATTR_LABEL, etc.)
    char  *argument;    // valeur associée (texte, attribut, commande)
    tag_attr *attributes; // attributs XML supplémentaires
} instruction;
```

### 4.3 Construction des widgets (automaton.c)

`automaton.c` est le cœur du programme. La fonction `widget_create()` reçoit un type de widget et retourne un `GtkWidget*` en appelant la fonction de création correspondante dans `widget_*.c`.

```c
// Exemple simplifié de l'automate
GtkWidget *widget_create(token type, tag_attr *attr) {
    switch (type) {
        case WIDGET_BUTTON:    return widget_button_create(attr);
        case WIDGET_ENTRY:     return widget_entry_create(attr);
        case WIDGET_CHECKBOX:  return widget_checkbox_create(attr);
        // ...
    }
}
```

### 4.4 Cycle de vie d'un widget

```
widget_*_create()        ← Création GTK, connexion des signaux de base
    ↓
widget_*_refresh()       ← Appel de l'<input> pour peupler le widget
    ↓
[événement utilisateur]
    ↓
signal GTK → signals.c → actions.c
    ↓
[EXIT / REFRESH / autre action]
    ↓
widget_*_envvar_construct()  ← Construit la string "VAR=valeur"
    ↓
variables_export()           ← Affiche sur stdout
```

### 4.5 Structure d'un fichier widget_*.c

Chaque widget expose **obligatoirement** ces 4 fonctions :

```c
// Création du widget GTK
GtkWidget *widget_XXXX_create(variable *var, tag_attr *attr);

// Peuplement depuis <input> (appel widget_opencommand si nécessaire)
void widget_XXXX_refresh(variable *var);

// Construction de la string pour l'export
gchar *widget_XXXX_envvar_construct(GtkWidget *widget);

// Traitement des actions spécifiques à ce widget
void widget_XXXX_action(GtkWidget *widget, gchar *action);
```

---

## 5. Modules clés — description détaillée

### 5.1 safe_exec.c — Sécurité des commandes

**Fichiers :** `safe_exec.c`, `safe_exec.h`

Ce module est la **seule** interface autorisée pour exécuter des commandes externes. Il remplace directement `system()` et `popen()` avec des équivalents GLib qui ne passent pas par `/bin/sh -c`.

```c
gint  safe_system(const gchar *command);
FILE *safe_popen (const gchar *command);
```

**Mécanisme de safe_system :**
1. `g_shell_parse_argv()` décompose la commande en tableau `argv[]`
2. `g_spawn_sync()` exécute `argv[0]` directement avec `execvp()`
3. Aucun shell intermédiaire → pas d'injection via `; rm -rf /` etc.

**Mécanisme de safe_popen :**
1. Même décomposition via `g_shell_parse_argv()`
2. `g_spawn_async_with_pipes()` crée un processus enfant avec un pipe sur stdout
3. `fdopen()` enveloppe le file descriptor en `FILE*` pour compatibilité avec les appelants existants

**Limitation connue :** La décomposition par `g_shell_parse_argv()` casse les commandes utilisant la redirection shell (`>`, `|`, `&&`). Ces fonctionnalités étaient disponibles via `system()`. Pour les cas où le pipe shell est nécessaire, il faut passer explicitement par `sh -c "commande"` en construisant manuellement l'argv : `{"/bin/sh", "-c", command, NULL}`.

### 5.2 variables.c — Gestion de l'état

`variables.c` maintient la liste chainée des `variable*` — une structure par widget nommé.

```c
typedef struct _variable {
    gchar      *Name;       // Nom de la variable (<variable>NOM</variable>)
    GtkWidget  *Widget;     // Pointeur vers le widget GTK
    gint        Type;       // WIDGET_BUTTON, WIDGET_ENTRY, etc.
    gchar      *Value;      // Valeur courante (cache)
    struct _variable *next;
} variable;
```

**Fonctions principales :**
- `variables_new()` — alloue et enregistre une nouvelle variable
- `variables_find(name)` — cherche par nom
- `variables_export()` — itère sur toutes les variables et affiche `NOM="valeur"` sur stdout
- `variables_refresh(name)` — force le rechargement via `<input>`

### 5.3 automaton.c — Le compilateur

`automaton.c` contient l'état de compilation : la liste `instruction[]`, la pile de conteneurs GTK (`stack.c`), et les fonctions de création et d'exécution.

**Structures clés :**
```c
typedef struct {
    token     command;
    gchar    *argument;
    tag_attr *attributes;
} instruction;

instruction *program;      // tableau d'instructions (heap alloué)
gint         program_size; // taille courante
```

**Points d'attention :**
- Le tableau `program` est réalloué par blocs via `g_realloc()` — attention aux pointeurs invalidés
- La pile de conteneurs gère l'imbrication des `<vbox>/<hbox>/<frame>/<notebook>`
- `automaton.c` est le seul module qui appelle directement `gtk_container_add()` et `gtk_box_pack_start()`

### 5.4 signals.c — Pont signaux / actions

`signals.c` fait le lien entre les signaux GTK (événements bas niveau) et le système d'actions du moteur haut niveau.

La fonction principale est `widget_signal_executor()` qui reçoit le nom du signal émis et cherche dans les attributs du widget l'action correspondante.

```c
// Exemple : connexion d'un signal dans widget_button.c
g_signal_connect(G_OBJECT(widget), "clicked",
    G_CALLBACK(on_button_clicked), var);

// Dans on_button_clicked :
widget_signal_executor(widget, var->attributes, "clicked");
// → cherche <action signal="clicked">...</action>
// → appelle execute_action()
```

### 5.5 glade_support.c — GtkBuilder

Ce module a été migré de libglade vers GtkBuilder. Les différences importantes pour la maintenance :

| libglade (ancienne API) | GtkBuilder (nouvelle API) |
|-------------------------|--------------------------|
| `glade_xml_new(file, NULL, PKG)` | `gtk_builder_new_from_file(file)` |
| `glade_xml_get_widget(xml, name)` | `GTK_WIDGET(gtk_builder_get_object(builder, name))` |
| `glade_xml_get_widget_prefix(xml, "")` | `gtk_builder_get_objects(builder)` retourne `GSList*` |
| `GladeXMLConnectFunc` | `GtkBuilderConnectFunc` |

**Point d'attention :** `gtk_builder_connect_signals_full()` a une signature de callback légèrement différente de `glade_xml_signal_autoconnect_full()`. Si des erreurs de compilation persistent dans `glade_support.c`, vérifier la signature du callback `signal_handler_connector`.

---

## 6. Conventions de code

### 6.1 Style général

- **Indentation :** tabulations (taille 4), jamais d'espaces
- **Longueur de ligne :** maximum 100 caractères
- **Nommage des fonctions :** `snake_case`, préfixe `widget_XXXX_` pour les fonctions d'un widget
- **Nommage des types :** `PascalCase` pour les typedef structs
- **Nommage des constantes :** `MAJUSCULES_AVEC_TIRETS`

### 6.2 En-tête obligatoire pour tout nouveau fichier

```c
/*
 * widget_monwidget.c: Description courte.
 * gtk4sermo — port GTK4 du fork gtk3sermo, lui-même dérivé de gtkdialog 0.8.3
 * Copyright (C) 2026  [Votre Nom] <email>
 *
 * Ce programme est un logiciel libre ; vous pouvez le redistribuer
 * et/ou le modifier selon les termes de la Licence Publique Générale
 * GNU telle que publiée par la Free Software Foundation, version 2
 * ou toute version ultérieure.
 */
```

### 6.3 Règles d'utilisation de la mémoire

```c
// CORRECT : allocation GLib (aborte proprement si OOM)
gchar *str = g_strdup("texte");
gchar *fmt = g_strdup_printf("valeur=%d", n);
gchar *buf = g_new(gchar, taille);    // au lieu de malloc()
g_free(str);                          // libération GLib

// INTERDIT : malloc/free directs
char *str = malloc(100);  // NON — utiliser g_malloc() ou g_new()
free(str);                // NON — utiliser g_free()

// CORRECT : copie de chaîne
g_strlcpy(dest, src, sizeof(dest));  // au lieu de strcpy()
g_strlcat(dest, src, sizeof(dest));  // au lieu de strcat()

// CORRECT : formatage de chaîne
g_snprintf(buf, sizeof(buf), fmt, args);    // borné
gchar *s = g_strdup_printf(fmt, args);      // alloue automatiquement
```

### 6.4 Macros de debug

```c
// macros.h définit :
PIP_DEBUG("message %s", variable);   // Debug verbeux (désactivé en production)
WARNING("avertissement %s", texte);  // Avertissement non fatal
// Ces macros sont désactivées par défaut via :
#undef DEBUG
#undef WARNING
// Pour les activer dans un fichier, commenter les #undef avant l'include
```

### 6.5 Gestion des erreurs

```c
// Pour les opérations GLib/GTK qui peuvent échouer :
GError *error = NULL;
if (!g_spawn_sync(..., &error)) {
    g_warning("%s(): spawn failed: %s", __func__, error->message);
    g_error_free(error);
    return FALSE;  // ou valeur de retour d'erreur appropriée
}

// Ne jamais appeler g_error() (fatal) sauf pour les bugs de programmation
// Utiliser g_warning() pour les erreurs récupérables
// Utiliser g_critical() pour les erreurs sérieuses mais non fatales
```

---

## 7. Sécurité — règles obligatoires

Ces règles **ne peuvent pas être contournées** même pour des raisons de praticité.

### 7.1 Exécution de commandes — OBLIGATOIRE

```c
// INTERDIT dans tout le codebase :
system("commande");              // NON
popen("commande", "r");         // NON
execlp("/bin/sh", "sh", "-c", commande, NULL);  // NON (sauf via safe_exec)

// OBLIGATOIRE :
safe_system("commande");        // OUI
FILE *f = safe_popen("cmd");    // OUI
```

**Vérification :** `grep -rn "^[^/]*\bsystem\s*(\|^[^/]*\bpopen\s*(" src/ | grep -v safe_`

### 7.2 Manipulation de chaînes — OBLIGATOIRE

```c
// INTERDIT :
strcpy(dest, src);               // NON
strcat(dest, src);               // NON
sprintf(buf, fmt, ...);          // NON
gets(buf);                       // NON (retiré en C11, ne jamais utiliser)

// OBLIGATOIRE :
g_strlcpy(dest, src, sizeof(dest));
g_strlcat(dest, src, sizeof(dest));
g_snprintf(buf, sizeof(buf), fmt, ...);
gchar *s = g_strdup_printf(fmt, ...);  // Pour allocation dynamique
```

### 7.3 Flags de compilation — NE PAS SUPPRIMER

Ces flags dans `src/Makefile.am` ne doivent jamais être retirés :

```makefile
AM_CFLAGS = ... \
  -D_FORTIFY_SOURCE=3 \          # Buffer overflow detection runtime
  -fstack-protector-strong \     # Stack canaries
  -fPIE                          # Position Independent Executable

AM_LDFLAGS = -pie \
  -Wl,-z,relro \                 # Read-only relocations
  -Wl,-z,now                     # Immediate binding (full RELRO)
```

### 7.4 Validation des entrées utilisateur

Toute valeur venant d'un widget d'entrée utilisateur et destinée à être passée à une commande doit être validée :

```c
// Exemple pour un nom de fichier :
if (filename && *filename && !strpbrk(filename, ";&|`$\\")) {
    // Fichier semble sûr
} else {
    g_warning("Nom de fichier suspect: %s", filename);
    return;
}
```

Pour des cas complexes, utiliser `g_shell_quote()` de GLib qui échappe correctement pour le shell.

---

## 8. Ajouter un nouveau widget

Voici la procédure complète pour ajouter un widget hypothétique `<slider>`.

### Étape 1 : Créer `src/widget_slider.h`

```c
#ifndef WIDGET_SLIDER_H
#define WIDGET_SLIDER_H

#include <gtk/gtk.h>
#include "variables.h"
#include "tag_attributes.h"

GtkWidget *widget_slider_create(variable *var, tag_attr *attr);
void       widget_slider_refresh(variable *var);
gchar     *widget_slider_envvar_construct(GtkWidget *widget);
void       widget_slider_action(GtkWidget *widget, gchar *action,
                                 tag_attr *attr, gchar *type);

#endif
```

### Étape 2 : Créer `src/widget_slider.c`

```c
#include "config.h"
#include <gtk/gtk.h>
#include "gtkdialog.h"
#include "widget_slider.h"
#include "tag_attributes.h"
#include "macros.h"

#undef DEBUG
#undef WARNING

GtkWidget *widget_slider_create(variable *var, tag_attr *attr)
{
    GtkWidget *widget;
    gdouble    min   = atof(tag_attribute_find(attr, "range-min") ?: "0");
    gdouble    max   = atof(tag_attribute_find(attr, "range-max") ?: "100");
    gdouble    step  = atof(tag_attribute_find(attr, "range-step") ?: "1");
    
    widget = gtk_scale_new_with_range(GTK_ORIENTATION_HORIZONTAL, min, max, step);
    gtk_scale_set_draw_value(GTK_SCALE(widget), TRUE);
    
    /* Valeur initiale depuis <default> */
    const gchar *defval = tag_attribute_find(attr, "default");
    if (defval)
        gtk_range_set_value(GTK_RANGE(widget), atof(defval));
    
    return widget;
}

gchar *widget_slider_envvar_construct(GtkWidget *widget)
{
    gdouble val = gtk_range_get_value(GTK_RANGE(widget));
    return g_strdup_printf("%.2f", val);
}

void widget_slider_refresh(variable *var)
{
    /* Relire depuis <input> si défini */
    FILE *infile;
    gchar line[64];
    const gchar *cmd = tag_attribute_find(var->attributes, "input");
    if (cmd) {
        GPid pid;
        if ((infile = widget_opencommand(cmd))) {
            if (fgets(line, sizeof(line), infile)) {
                line[sizeof(line) - 1] = '\0';
                gtk_range_set_value(GTK_RANGE(var->Widget), atof(line));
            }
            fclose(infile);
        }
    }
}

void widget_slider_action(GtkWidget *widget, gchar *action,
                           tag_attr *attr, gchar *type)
{
    /* Déléguer les actions génériques */
}
```

### Étape 3 : Enregistrer dans `automaton.c`

Ajouter le case dans `widget_create()` :
```c
case WIDGET_SLIDER:
    widget = widget_slider_create(var, attr);
    break;
```

### Étape 4 : Enregistrer dans `gtkdialog_lexer.l`

```flex
"<slider"    { return TOKEN_SLIDER; }
"</slider>"  { return TOKEN_END_SLIDER; }
```

### Étape 5 : Enregistrer dans `gtkdialog_parser.y`

Ajouter le token et la règle de grammaire correspondante.

### Étape 6 : Enregistrer dans `widgets.c`

Ajouter le case dans `widget_get_text_value()` et `widget_refresh()`.

### Étape 7 : Déclarer dans `widgets.h`

Ajouter `WIDGET_SLIDER` dans l'enum des types de widgets.

### Étape 8 : Ajouter dans `src/Makefile.am`

```makefile
gtkdialog_SOURCES = \
    ...
    widget_slider.c widget_slider.h \
    ...
```

### Étape 9 : Écrire un exemple dans `examples/slider/`

---

## 9. Système de build

### 9.1 Vue d'ensemble Autotools

```
configure.in  →  [autoconf]  →  configure
Makefile.am   →  [automake]  →  Makefile.in
configure + Makefile.in  →  [./configure]  →  Makefile
Makefile  →  [make]  →  binaire gtk4sermo
```

### 9.2 Fichiers de build

| Fichier | Rôle | À éditer ? |
|---------|------|------------|
| `configure.in` | Détection des dépendances, options | OUI — pour ajouter des dépendances |
| `src/Makefile.am` | Sources, flags de compilation | OUI — pour ajouter des fichiers source |
| `Makefile.am` (racine) | Build global, install | OUI — pour ajouter des répertoires |
| `configure` | Script généré par autoconf | JAMAIS — regénérer avec `autoconf` |
| `Makefile` | Généré par `./configure` | JAMAIS — regénérer avec `./configure` |
| `aclocal.m4` | Macros m4 autoconf | JAMAIS directement — utiliser `aclocal` |

### 9.3 Regénérer le système de build

```bash
# Après modification de configure.in ou Makefile.am :
autoreconf --install --force
./configure
make
```

### 9.4 Ajouter une dépendance

Dans `configure.in` :
```m4
PKG_CHECK_MODULES([MALIB], [malib >= 1.0],
    AC_SUBST(HAVE_MALIB, 1),
    AC_SUBST(HAVE_MALIB, 0))
AC_SUBST(MALIB_CFLAGS)
AC_SUBST(MALIB_LIBS)
```

Dans `src/Makefile.am` :
```makefile
AM_CFLAGS = ... $(MALIB_CFLAGS) -DHAVE_MALIB=$(HAVE_MALIB)
LIBS = ... $(MALIB_LIBS)
```

### 9.5 Cibles make utiles

```bash
make              # Compilation
make install      # Installation dans /usr/local/bin
make uninstall    # Désinstallation
make clean        # Nettoyer les objets
make distclean    # Nettoyer + Makefile générés
make dist         # Créer l'archive de distribution .tar.gz
```

---

## 10. Tests et débogage

### 10.1 Test rapide fonctionnel

```bash
# Test de chaque widget principal
for example in examples/*/; do
    name=$(basename "$example")
    echo "=== Test $name ===" 
    timeout 2 ./src/gtk4sermo --file="$example/$(ls $example | head -1)" \
        2>/dev/null && echo "OK" || echo "ÉCHEC ou timeout"
done
```

### 10.2 Détection de fuites mémoire (Valgrind)

```bash
valgrind \
  --leak-check=full \
  --show-leak-kinds=definite,indirect \
  --track-origins=yes \
  --suppressions=/usr/share/glib-2.0/valgrind/glib.supp \
  ./src/gtk4sermo --stdin << 'EOF'
<window><vbox>
  <entry><variable>TEST</variable></entry>
  <button><label>OK</label><action>EXIT:ok</action></button>
</vbox></window>
EOF
```

### 10.3 Détection de vulnérabilités (AddressSanitizer)

```bash
# Recompiler avec ASAN
./configure CFLAGS="-fsanitize=address,undefined -g -O1"
make

# Tester
ASAN_OPTIONS=detect_leaks=1 ./src/gtk4sermo --stdin < examples/button/button
```

### 10.4 Vérification statique (cppcheck)

```bash
cppcheck --enable=all --inconclusive --std=c17 \
  --suppress=missingIncludeSystem \
  -I src/ src/*.c 2>&1 | grep -v "^$"
```

### 10.5 Vérification des protections (checksec)

```bash
# Après compilation en mode release
checksec --file=src/gtk4sermo
# Résultat attendu :
# RELRO: Full | STACK CANARY: Canary found | NX: NX enabled | PIE: PIE enabled
```

### 10.6 Débogage avec GDB

```bash
gdb ./src/gtk4sermo
(gdb) run --stdin < examples/tree/tree
(gdb) bt          # backtrace en cas de crash
(gdb) info locals # variables locales
```

### 10.7 Inspecteur GTK4

```bash
GTK_DEBUG=interactive ./src/gtk4sermo --stdin < examples/notebook/notebook
# Puis Ctrl+Shift+I dans la fenêtre pour ouvrir l'inspecteur
```

---

## 11. Processus de contribution

### 11.1 Avant de commencer

1. Vérifier que le bug/feature n'est pas déjà en cours via les issues
2. Tester que la compilation passe sans erreur en mode debug et release
3. Lire les conventions de code (§6) et les règles de sécurité (§7)

### 11.2 Workflow de développement

```bash
# 1. Créer une branche
git checkout -b feature/nom-du-widget

# 2. Développer en suivant les conventions
# ... modifications ...

# 3. Vérifier la compilation sans avertissements
make clean && make CFLAGS="-Wall -Wextra -Werror" 2>&1 | head -20

# 4. Vérifier les règles de sécurité
grep -rn "\bsystem\s*(\|\bpopen\s*(\|\bstrcpy\b\|\bstrcat\b\|\bsprintf\b" src/ \
  | grep -v "safe_\|//\|/\*\|parser\|lexer"
# Ce grep doit retourner 0 résultat

# 5. Tester avec valgrind
valgrind ./src/gtk4sermo --stdin < examples/button/button

# 6. Commit avec message clair
git commit -m "feat(widget_slider): ajouter le widget curseur GTK4

- Implémente widget_slider_create/refresh/envvar_construct
- Attributs: range-min, range-max, range-step, digits
- Ajout exemple examples/slider/slider_basic
- Compatible avec les scripts 0.8.3 utilisant hscale"
```

### 11.3 Checklist avant une Pull Request

- [ ] Compilation sans erreur avec `-Wall -Wextra`
- [ ] Aucun `system()`, `popen()`, `strcpy()`, `strcat()`, `sprintf()` nu dans le code ajouté
- [ ] Aucune fuite mémoire détectée par Valgrind
- [ ] En-tête de licence ajouté aux nouveaux fichiers
- [ ] Au moins un exemple dans `examples/` pour tout nouveau widget
- [ ] Mise à jour de la documentation (manuel utilisateur §5 si nouveau widget)

---

## 12. Tâches de maintenance récurrentes

### 12.1 Mise à jour des dépendances GTK4

Lors d'une mise à jour de GTK4 :
1. Consulter la liste des dépréciations : https://docs.gtk.org/gtk3/migrating.html
2. Rechercher dans le code les APIs dépréciées : `grep -rn "gtk_XXX_deprecated" src/`
3. Appliquer les remplacements en suivant la documentation GTK

### 12.2 Migration future vers GTK4

La migration GTK3 -> GTK4 est **réalisée** dans ce port (voir GTK4_MIGRATION.md). Points d'attention résiduels :

- `GtkContainer` supprimé → utiliser `gtk_widget_set_parent()`
- `GdkEvent` redesigné → révision complète de `signals.c`
- `GTK_DIALOG_USE_HEADER_BAR` obligatoire sur certaines plateformes
- Le rendu passe par Cairo/OpenGL → `widget_pixmap.c` à réviser
- `GtkTreeView` toujours présent mais `GtkListView` préféré
- VTE 0.70+ avec API mise à jour pour GTK4

Outil de migration officiel : `gtk4-builder-tool simplify`

### 12.3 Vérification périodique de sécurité

À effectuer à chaque release :

```bash
# 1. Grep sécurité
grep -rn "\bsystem\b\|\bpopen\b\|\bstrcpy\b\|\bstrcat\b\|\bgets\b\|\bsprintf\b" src/ \
  | grep -v "safe_\|//\|parser\|lexer\|\.h:" | wc -l
# Doit être 0

# 2. Checksec
checksec --file=src/gtk4sermo | grep -E "Full|enabled"

# 3. Analyse ASAN
make CFLAGS="-fsanitize=address,undefined -g" && \
  for ex in examples/*/; do
    echo -n "$ex: "
    ASAN_OPTIONS=detect_leaks=0 timeout 2 ./src/gtk4sermo \
      --stdin < "$ex/$(ls $ex | head -1)" 2>&1 | grep -c "ERROR" && echo "ERREUR" || echo "OK"
  done
```

### 12.4 Création d'une release

```bash
# 1. Mettre à jour la version dans configure.in
sed -i 's/AC_INIT([gtk4sermo], \[0\.9\..*\]/AC_INIT([gtk4sermo], [0.9.1]/' configure.in

# 2. Mettre à jour ChangeLog
echo "$(date +%Y-%m-%d)  Mainteneur <email>" >> ChangeLog
echo "    * Corrections de bugs, améliorations de sécurité" >> ChangeLog

# 3. Générer l'archive
autoreconf --install
./configure
make dist
# → gtk4sermo-0.9.1.tar.gz

# 4. Tagger dans git
git tag -a v0.9.1 -m "Release 0.9.1"
git push origin v0.9.1
```

---

## 13. Roadmap technique

### Version 0.9.x — Stabilisation GTK4 (en cours)

- [x] Migration GTK2 → GTK3 (45 fichiers)
- [x] Correctifs de sécurité (safe_exec, strcpy, malloc)
- [x] Mise à jour du build system
- [ ] Compilation propre sur Debian 12 / Ubuntu 22.04
- [ ] Tests fonctionnels sur les 30 widgets
- [ ] Correction des erreurs résiduelles glade_support.c
- [ ] Paquet .deb modernisé

### Version 0.9.5 — Améliorations qualité

- [ ] Réduction des avertissements gcc à < 5 avec `-Wall -Wextra`
- [ ] Ajout de tests unitaires avec `cmocka` ou `check`
- [ ] Intégration CI/CD (GitHub Actions ou GitLab CI)
- [ ] Passage au standard C17 (`-std=c17`)
- [ ] Amélioration `safe_exec.c` pour supporter les pipes shell via `execvp()`

### Version 1.0 — Migration GTK4

- [x] Migration GTK3 → GTK4 (réalisée)
- [ ] Refactoring `signals.c` pour la nouvelle API GdkEvent
- [ ] Remplacement GtkTreeView par GtkColumnView (optionnel)
- [ ] Support Wayland natif (sans XWayland)
- [ ] Passage à Meson/Ninja (remplacement Autotools)

### Vision long terme

- [ ] Remplacement du parser Yacc/Lex par GLib GMarkup (meilleure gestion d'erreurs XML)
- [ ] Mode serveur (socket Unix) pour interfaces persistantes
- [ ] Bindings Python (via GObject Introspection)

---

## 14. Références et ressources

### Documentation officielle

- **GTK4** : https://docs.gtk.org/gtk4/
- **GLib** : https://docs.gtk.org/glib/
- **GIO** : https://docs.gtk.org/gio/
- **VTE 2.91** : https://gnome.pages.gitlab.gnome.org/vte/
- **Migration GTK2→GTK3** : https://docs.gtk.org/gtk3/migrating-2to3.html
- **Migration GTK3→GTK4** : https://docs.gtk.org/gtk4/migrating-3to4.html

### Outils

- **GTK Inspector** : `GTK_DEBUG=interactive` — inspection des widgets en temps réel
- **Glade** : Éditeur d'interface GTK (génère des .ui pour GtkBuilder)
- **Valgrind** : Détection de fuites et erreurs mémoire
- **AddressSanitizer** : Détection rapide de corruption mémoire (LLVM/GCC)
- **cppcheck** : Analyse statique C/C++
- **checksec** : Vérification des protections binaires

### Fichiers importants dans ce dépôt

| Fichier | Description |
|---------|-------------|
| `CAHIER_DES_CHARGES.md` | Spécifications complètes du projet |
| `MANUEL_UTILISATEUR.md` | Guide pour les utilisateurs finaux |
| `MANUEL_DEVELOPPEUR.md` | Ce document |
| `COMPILE.md` | Instructions rapides de compilation |
| `GUIDE_MODERNISATION_GTKD.md` | Analyse et plan de migration original |
| `src/safe_exec.c` | Module de sécurité (wrappers commandes) |
| `configure.in` | Configuration du build (dépendances) |
| `src/Makefile.am` | Sources et flags de compilation |
| `TODO` | Liste des bugs connus (originaux gtk3d 0.8.3) |


---

## 11. Architecture GTK4 — gtk4sermo

### 11.1 Compat layer `gtk4-compat.h`

Le fichier `src/gtk4-compat.h` est force-inclus dans **chaque unité de traduction** via `-include` dans `AM_CFLAGS`. Il contient 65+ shims couvrant les APIs GTK3 supprimées en GTK4 :

```makefile
AM_CFLAGS = $(GTK_CFLAGS) ... -include $(top_srcdir)/src/gtk4-compat.h
```

| Technique | Exemples |
|-----------|----------|
| `#define` simple | `gtk_entry_get_text` → `gtk_editable_get_text` |
| `static inline` | `_compat_calendar_get_date()`, `_compat_container_add()` |
| No-op macros | `gtk_widget_override_color()`, `gdk_threads_enter()` |
| Shims complexes | `GtkInfoBar` → `GtkBox`, `GtkAspectFrame` constructeur |

### 11.2 Constantes WIDGET_* pour les nouveaux widgets

Définis dans `src/automaton.h` à partir de `0x00B30000` :

```c
/* GTK3 widgets portés en GTK4 */
#define WIDGET_SWITCH            0x00B30000
#define WIDGET_CALENDAR          0x00B40000
#define WIDGET_FILECHOOSER       0x00B50000
#define WIDGET_IMAGE             0x00B60000
#define WIDGET_INFOBAR           0x00B70000
#define WIDGET_LINKBUTTON        0x00B80000
#define WIDGET_PASSWORD          0x00B90000
#define WIDGET_PULSE             0x00BA0000
#define WIDGET_SEARCHENTRY       0x00BB0000
#define WIDGET_SPINNER           0x00BC0000
#define WIDGET_ASPECTFRAME       0x00BD0000
/* Widgets GTK4-natifs */
#define WIDGET_LEVELBAR          0x00BE0000
#define WIDGET_REVEALER          0x00BF0000
#define WIDGET_STACK             0x00C00000
#define WIDGET_FLOWBOX           0x00C10000
#define WIDGET_OVERLAY           0x00C20000
#define WIDGET_DRAWINGAREA       0x00C30000
```

### 11.3 Ajouter un nouveau widget à gtk4sermo

Pour ajouter un widget `<mywidget>`, 6 étapes sont nécessaires :

**Étape 1 — Constante** (`automaton.h`) :
```c
#define WIDGET_MYWIDGET  0x00C40000
```

**Étape 2 — Header** (`widget_mywidget.h`) :
```c
#ifndef WIDGET_MYWIDGET_H
#define WIDGET_MYWIDGET_H
#include <gtk/gtk.h>
#include "variables.h"
void    widget_mywidget_clear(variable *var);
GtkWidget *widget_mywidget_create(AttributeSet *Attr, tag_attr *attr, gint Type);
gchar  *widget_mywidget_envvar_construct(GtkWidget *widget);
gchar  *widget_mywidget_envvar_all_construct(variable *var);
void    widget_mywidget_fileselect(variable *var, const char *name, const char *value);
void    widget_mywidget_refresh(variable *var);
void    widget_mywidget_removeselected(variable *var);
void    widget_mywidget_save(variable *var);
#endif
```

**Étape 3 — Implémentation** (`widget_mywidget.c`) :
```c
#define _GNU_SOURCE
#include <gtk/gtk.h>
#include "config.h"
#include "gtkdialog.h"
#include "attributes.h"
#include "automaton.h"
#include "widgets.h"
#include "signals.h"
#include "tag_attributes.h"
#include "widget_mywidget.h"

GtkWidget *widget_mywidget_create(AttributeSet *Attr, tag_attr *attr, gint Type)
{
    GtkWidget *widget = gtk_my_widget_new();
    /* ... configuration depuis attr ... */
    return widget;
}

gchar *widget_mywidget_envvar_construct(GtkWidget *widget)
{
    return g_strdup("valeur_exportée");
}
/* ... autres fonctions obligatoires ... */
```

**Étape 4 — Dispatch** (`widgets.c`) :
```c
// Dans widget_get_text_value() :
case WIDGET_MYWIDGET:
    return widget_mywidget_envvar_construct(widget);

// Dans widgets_to_str() :
case WIDGET_MYWIDGET:
    type = "MYWIDGET"; break;
```

**Étape 5 — Parser** (`gtkdialog_lexer.l` + `gtkdialog_parser.y`) :
```
/* lexer.l */
"mywidget"   { return MYWIDGET; }

/* parser.y */
%token MYWIDGET
mywidget: MYWIDGET attributes children END_MYWIDGET
    { token_store_with_argument_attr(WIDGET_MYWIDGET, NULL, $2); }
```

**Étape 6 — Build** (`Makefile.am`) :
```makefile
gtk4sermo_SOURCES = ... \
    widget_mywidget.c widget_mywidget.h
```

### 11.4 Shims gtk4-compat.h — ajouter un nouveau shim

Pour couvrir une API GTK3 supprimée :

```c
/* Shim simple (macro) */
#define gtk_old_function(args...) gtk_new_function(args)

/* Shim avec logique (inline) */
static inline ReturnType
_compat_old_function(Type arg1, Type arg2)
{
    /* Implémentation GTK4 */
    return gtk_new_function(arg1, arg2);
}
#define gtk_old_function(a, b) _compat_old_function((a), (b))
```

**Règles :**
- Les fonctions inline sont préfixées `_compat_`
- Les macros no-op sont documentées avec `/* GTK4_TODO: ... */`
- Toujours tester après ajout (`-Wimplicit-function-declaration` = 0)

### 11.5 Stubs GTK4 — comportement

Les widgets non portables (`<menubar>`, `<table>`, `<gvim>`) retournent
un `GtkLabel` d'avertissement visible :

```c
#ifdef GTK4_NO_MENU
GtkWidget *widget_menubar_create(...)
{
    fprintf(stderr, "GTK4_TODO: menubar stub - use GMenuModel instead
");
    pop();  /* vider la pile pour éviter les fuites */
    return gtk_label_new("[GTK4: menubar non disponible]");
}
#endif
```

### 11.6 GtkDrawingArea — rendu Cairo

En GTK4, le signal `"draw"` est supprimé. Utiliser `set_draw_func()` :

```c
static void _draw_func(GtkDrawingArea *area, cairo_t *cr,
                       int width, int height, gpointer user_data)
{
    DrawingAreaData *d = (DrawingAreaData *)user_data;
    cairo_set_source_rgba(cr, d->r, d->g, d->b, d->a);
    cairo_paint(cr);
}

gtk_drawing_area_set_draw_func(GTK_DRAWING_AREA(widget),
                               _draw_func, data,
                               (GDestroyNotify)g_free);
```

### 11.7 Vérification rapide d'un widget

```bash
# Vérifier la syntaxe C d'un widget sans compiler tout le projet
gcc -fsyntax-only -std=c11 \
    $(pkg-config --cflags gtk4) \
    -include src/gtk4-compat.h \
    -I src/ \
    src/widget_mywidget.c

# Vérifier que le widget est bien dispatch dans widgets.c
grep "WIDGET_MYWIDGET" src/widgets.c src/automaton.h
```

---

*Document horodaté — dernière mise à jour : 2026-06-07 (audit #4 — compilation complète gtk4sermo).*
