# Manuel Développeur et Mainteneur — gtk3sermo

**Version :** 1.1.1  
**Date :** Août 2026  
**Audience :** Développeurs C, mainteneurs haplo-dialog  
**Mainteneur :** haplo-dialog <devel@haplo-dialog.fr> | **Dépôt :** https://haplo-dialog.fr

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

gtk3sermo est un programme C de ~35 000 lignes organisé en trois couches distinctes :

```
┌─────────────────────────────────────────────────────────┐
│                    COUCHE D'ENTRÉE                       │
│   gtk3sermo.c  ← arguments CLI, lecture XML             │
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
                         │ Appels GTK3 / GLib
┌────────────────────────▼────────────────────────────────┐
│                    COUCHE WIDGETS                        │
│   widgets.c     ← dispatch, widget_opencommand()        │
│   widget_*.c    ← 43 fichiers pour 52 widgets           │
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
cd gtk3sermo/
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
| `GTK_DEBUG=interactive` | Ouvre l'inspecteur GTK3 (Ctrl+Shift+I dans la fenêtre) |
| `GTK_THEME=Adwaita:dark` | Forcer un thème GTK pour les tests |

Exemple :
```bash
GTK_DEBUG=interactive G_DEBUG=fatal_criticals ./src/gtk3sermo --stdin < examples/button/button
```

---

## 3. Structure des fichiers source

```
src/
├── gtk3sermo.c/.h         # Point d'entrée main(), parsing des options CLI
├── safe_exec.c/.h         # Wrappers sécurisés : safe_system(), safe_popen()
├── macros.h               # Macros de debug : PIP_DEBUG, WARNING, DEBUGMSG
├── config.h.in            # Template de config.h généré par autoconf
│
│   ── MOTEUR ──
├── automaton.c/.h         # Compilateur XML → widgets GTK (cœur du programme)
├── gtkdialog_parser.y     # Grammaire Bison (syntaxe gtk3sermo XML)
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
│   ── WIDGETS (43 fichiers pour 52 widgets — extrait) ──
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
// gtk3sermo.c : main()
1. gtk_init(&argc, &argv)           // Initialise GTK3
2. parse_options(argc, argv)        // Lit --stdin, --program, --file
3. load_program_source()            // Charge le XML dans program_src[]
4. yyparse()                        // Lance le parser Bison
   → appelle automaton_build()      // Construit la liste d'instructions
5. automaton_execute()              // Crée les widgets GTK depuis les instructions
6. gtk_main()                       // Boucle événements GTK
7. variables_export()               // Affiche les valeurs sur stdout
```

### 4.2 Parsing du XML (parser.y + lexer.l)

Le parser n'est **pas** un vrai parser XML : il utilise une grammaire Bison spécifique au format gtk3sermo. Les tokens reconnus par le lexer sont les balises XML gtk3sermo, pas du XML générique.

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

Le cœur d'un widget tient en deux fonctions (prendre `widget_switch.c/.h`
comme modèle) :

```c
// Création du widget GTK
GtkWidget *widget_XXXX_create(AttributeSet *Attr, tag_attr *attr, gint Type);

// Construction de la chaîne exportée (format VAR="valeur")
gchar *widget_XXXX_envvar_construct(GtkWidget *widget);
```

Selon le widget, des fonctions d'appui existent (rafraîchissement depuis
`<input>`, traitement d'actions) — se calquer sur un widget existant proche.
L'instanciation elle-même se fait dans `automaton.c` (voir `CONTRIBUTING.md`,
« Ajouter un widget »).

---

## 5. Modules clés — description détaillée

### 5.1 safe_exec.c — Sécurité des commandes

**Fichiers :** `safe_exec.c`, `safe_exec.h`  
**Dernière refonte :** 2026-05-21 (session 4)

Ce module est la **seule** interface autorisée pour exécuter des commandes externes. Il remplace directement `system()` et `popen()`.

```c
gint  safe_system(const gchar *command);
FILE *safe_popen (const gchar *command);
```

**Mécanisme de safe_system et safe_popen — logique commune :**

```
commande → strpbrk(cmd, "|&;<>(){}$`\"'~*?")
               │
         ┌─────┴──────────────────────────────┐
    NON  │ métacaractères ?               OUI │
         ▼                                    ▼
  g_shell_parse_argv()              argv = {"/bin/sh","-c",cmd,NULL}
  exec direct — sans shell          g_message() « contient de la syntaxe shell »
  (sécurisé contre injection)       (fonctionnel pour pipes/redirections)
```

- `safe_system()` → `g_spawn_sync()` — bloquant, retourne le code de sortie
- `safe_popen()` → `g_spawn_async_with_pipes()` + `fdopen(stdout_fd, "r")` — retourne `FILE*`
  - Flag `G_SPAWN_DO_NOT_REAP_CHILD` + `g_child_watch_add()` → récolte automatique des zombies

> ⚠️ **Règle absolue :** fermer le `FILE*` retourné par `safe_popen()` / `widget_opencommand()`
> **uniquement avec `fclose()`** — jamais avec `pclose()`. Ce `FILE*` vient de `fdopen()`,
> pas de `popen()`. `pclose()` sur un `FILE* fdopen()` est un comportement indéfini (UB).

> 📝 **Note audit :** Quand la commande contient des métacaractères, la bascule vers
> `/bin/sh -c` est loggée via `g_debug()`. Pour voir ces messages :
> `G_MESSAGES_DEBUG=all ./gtk3sermo …`

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

`signals.c` fait le lien entre les signaux GTK (événements bas niveau) et le système d'actions gtk3sermo haut niveau.

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

### 5.5 widget_table.c — Migration GtkCList → GtkTreeView (session 4)

`GtkCList` a été supprimé de GTK3. `widget_table.c` a été intégralement réécrit.

**Architecture interne :**

```
GtkTreeView (var->Widget)
    └── GtkListStore  (modèle — N colonnes G_TYPE_STRING)
         └── GtkTreeViewColumn × N  (une par en-tête de colonne)
              └── GtkCellRendererText
```

L'état propre au widget est stocké via `g_object_set_data()` sur le GtkTreeView :

| Clé | Type | Description |
|-----|------|-------------|
| `"_n_columns"` | GINT_TO_POINTER | Nombre de colonnes de données |
| `"_sort_func"` | GINT_TO_POINTER | 0=GTK défaut, 1=natcmp, 2=natcasecmp |
| `"_auto_sort"` | GINT_TO_POINTER | Ré-trier après chaque ajout de ligne |
| `"_last-column"` | GINT_TO_POINTER(col+1) | Usage +1 : 0 détruirait la donnée |
| `"_initialised"` | GINT_TO_POINTER | Garde anti-double connexion de signaux |

**Points importants pour la maintenance :**

- **Tri naturel :** enregistrer `_natcmp_func` via `gtk_tree_sortable_set_sort_func()` pour chaque colonne. La fonction interroge `gtk_tree_sortable_get_sort_column_id()` pour connaître la colonne active, puis appelle `strnatcmp()` de `stringman.c`.
- **Suppression multiple sûre :** toujours convertir les `GtkTreePath*` en `GtkTreeRowReference*` avant toute suppression — les chemins sont invalidés dès qu'une ligne est supprimée, les références restent valides.
- **Clic d'en-tête :** `gtk_tree_view_column_set_sort_column_id(col, c)` délègue le toggle asc/desc à GTK automatiquement. Plus besoin de callback manuel.
- **Signal sélection :** `"cursor-changed"` → `on_any_widget_cursor_changed_event()` (remplace `"select-row"` de GtkCList).
- **fclose() obligatoire :** `widget_opencommand()` retourne un `FILE* fdopen()` — voir règle safe_exec.c ci-dessus.

### 5.6 glade_support.c — GtkBuilder

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
 * gtk3sermo — fork modernisé de gtk3sermo 0.8.3
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

```c
// CORRECT : export de variable d'environnement
g_setenv("NOM", valeur, TRUE);    // gère sa propre copie, libère l'ancienne

// INTERDIT : putenv() avec chaîne allouée dynamiquement
putenv(g_strdup_printf("NOM=%s", valeur));  // NON — fuite mémoire garantie
// putenv() prend ownership du pointeur passé.
// L'ancienne valeur n'est jamais libérée lors de la mise à jour.
```

```c
// CORRECT : boucle décrémentante sur strlen()
for (count = (gint)strlen(line) - 1; count >= 0; count--)

// INCORRECT : strlen() retourne size_t (non signé), soustraire 1 sur 0 = SIZE_MAX
for (count = strlen(line) - 1; count >= 0; count--)  // NON — UB si len == 0
```

### 6.4 Macros de debug et journalisation GLib

```c
// macros.h définit :
PIP_DEBUG("message %s", variable);   // Debug verbeux (désactivé en production)
WARNING("avertissement %s", texte);  // Avertissement non fatal
// Ces macros sont désactivées par défaut via :
#undef DEBUG
#undef WARNING
// Pour les activer dans un fichier, commenter les #undef avant l'include
```

**Règle de journalisation GLib (appliquée à partir de la session 6) :**

À partir du patch 6, les `fprintf(stderr, …)` non conditionnels ont été remplacés par
les fonctions GLib appropriées. La règle est la suivante :

```c
// Erreur récupérable — toujours visible dans les logs système :
g_warning("widget_foo: failed to get value for '%s'", key);

// Trace de débogage — silencieuse par défaut :
g_debug("widget_foo: refreshing row %d with '%s'", row, val);
```

Pour activer les messages `g_debug()` : `G_MESSAGES_DEBUG=all ./gtk3sermo …`

Les blocs `#ifdef DEBUG_TRANSITS`, `#ifdef DEBUG_CONTENT`, `#ifdef PIP_DEBUG`, etc.
existants ont été conservés tels quels — ils restent le mécanisme de debug verbeux
hérité pour les développeurs qui compilent avec `-DDEBUG_TRANSITS` ou équivalent.

Ne pas utiliser `fprintf(stderr, …)` dans le nouveau code. Utiliser `g_warning()`
pour les conditions anormales et `g_debug()` pour les traces d'exécution.

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
// …utilisation…
fclose(f);                      // OUI — safe_popen() utilise fdopen(), pas popen()
// pclose(f);                   // NON — comportement indéfini sur FILE* fdopen()
```

**Vérification :** `grep -rn "^[^/]*\bsystem\s*(\|^[^/]*\bpopen\s*(" src/ | grep -v safe_`

**Vérification pclose (doit retourner 0) :**
```bash
grep -rn "pclose" src/*.c | grep -v "safe_exec\|widgets\|/\*\|//"
```

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

Le durcissement a **une seule source** : `configure.ac`. Il y était déjà déclaré,
mais n'y était référencé nulle part — les drapeaux étaient recopiés en dur dans
`src/Makefile.am`, et les deux listes avaient divergé (`-fPIE` d'un côté,
`-Wl,-z,ibt -Wl,-z,shstk` de l'autre). Depuis le 2026-08-28 elles sont câblées.

```m4
# configure.ac — la source
AC_SUBST([HARDENING_CFLAGS],
  ["-U_FORTIFY_SOURCE -D_FORTIFY_SOURCE=3 -D_POSIX_C_SOURCE=200809L \
    -fstack-protector-strong -fstack-clash-protection -fcf-protection=full -fPIE"])
AC_SUBST([HARDENING_LDFLAGS],
  ["-pie -Wl,-z,relro -Wl,-z,now -Wl,-z,noexecstack -Wl,-z,ibt -Wl,-z,shstk"])
```

```makefile
# src/Makefile.am — la consommation, rien de plus
gtk3sermo_CFLAGS  = ... @HARDENING_CFLAGS@
gtk3sermo_LDFLAGS = @HARDENING_LDFLAGS@
```

Ce qu'ils apportent : `_FORTIFY_SOURCE=3` (contrôles de dépassement à
l'exécution), canaris de pile, protection du flot de contrôle, exécutable
repositionnable, RELRO complet et liaison immédiate.

`-U_FORTIFY_SOURCE` n'est pas décoratif : les drapeaux Debian posent
`-D_FORTIFY_SOURCE=2` dans `CPPFLAGS`. Sans le `-U`, GCC avertit à **chaque**
fichier compilé — c'était 125 lignes de bruit sur les deux ports réunis.

**Ne jamais remettre ces drapeaux en dur dans `src/Makefile.am`** : c'est
exactement comme ça que les deux listes ont divergé.

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

## 7b. Système de build et packaging

### 7b.1 Régénérer les Makefiles

Après modification de `configure.ac` ou d'un `Makefile.am` :

```bash
./autogen.sh          # autoreconf -fiv + ./configure
# ou :
NOCONFIGURE=1 ./autogen.sh   # seulement autoreconf (pour debian/rules)
```

### 7b.2 Options de configure

| Option | Effet |
|--------|-------|
| `--enable-debug` | `-g -O0 -DDEBUG -DDEBUG_TRANSITS -DDEBUG_CONTENT` |
| `--without-vte` | Compile sans le widget terminal (pas de dépendance libvte) |
| `--prefix=/usr` | Installation système (utilisé par debian/rules) |

### 7b.3 Construire le paquet .deb

```bash
# Méthode rapide (depuis le répertoire racine du projet)
make deb
# → dpkg-buildpackage -us -uc -b -j$(nproc)
# → ../gtk3sermo_1.1.0-1_amd64.deb

# Nettoyage
make deb-clean

# Source package (pour upload vers un PPA)
make deb-src
```

**Dépendances de build :** `dpkg-dev debhelper devscripts` (en plus des dépendances normales).

### 7b.4 Ajouter une dépendance de build

1. Ajouter `PKG_CHECK_MODULES([FOO], [libfoo >= 1.0])` dans `configure.ac`
2. Ajouter `$(FOO_CFLAGS)` à `gtk3sermo_CFLAGS` dans `src/Makefile.am`
3. Ajouter `$(FOO_LIBS)` à `gtk3sermo_LDADD` dans `src/Makefile.am`
4. Ajouter `libfoo-dev` dans `Build-Depends` de `debian/control`
5. Relancer `./autogen.sh`

### 7b.5 Page de manuel

La page de manuel est dans `src/gtk3sermo.1` (format troff). Elle est automatiquement installée par `make install` via `dist_man_MANS` dans `src/Makefile.am`. Pour la consulter sans installer :

```bash
man ./src/gtk3sermo.1
```

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
#include "gtk3d.h"
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
gtk3sermo_SOURCES = \
    ...
    widget_slider.c widget_slider.h \
    ...
```

### Étape 9 : Écrire un exemple dans `examples/slider/`

---

## 8b. Nouveaux widgets GTK3 — session 9 (2026-05-21)

Six widgets natifs GTK3 ont été ajoutés dans la session 9 selon la procédure
de la section 8. Ils constituent des exemples de référence pour les futurs ajouts.

### Tableau récapitulatif

| Tag XML          | Widget GTK3              | Envvar          | Signal par défaut     |
|------------------|--------------------------|-----------------|-----------------------|
| `<switch>`       | GtkSwitch                | `true`/`false`  | `notify::active`      |
| `<filechooser>`  | GtkFileChooserButton     | Chemin absolu   | `file-set`            |
| `<calendar>`     | GtkCalendar              | `YYYY-MM-DD`    | `day-selected`        |
| `<linkbutton>`   | GtkLinkButton            | URI             | `clicked`             |
| `<searchentry>`  | GtkSearchEntry           | Texte courant   | `search-changed`      |
| `<infobar>`      | GtkInfoBar               | Texte message   | `response`            |

### Identifiants WIDGET_*

```c
/* automaton.h — nouvelles constantes */
#define WIDGET_SWITCH            0x00B30000
#define WIDGET_FILECHOOSER       0x00B40000
#define WIDGET_CALENDAR          0x00B50000
#define WIDGET_LINKBUTTON        0x00B60000
#define WIDGET_SEARCHENTRY       0x00B70000
#define WIDGET_INFOBAR           0x00B80000
```

Le prochain widget disponible commence à `0x00B90000`.

### Particularités techniques

**GtkSwitch** — signal `notify::active` (pas `toggled` comme GtkCheckButton).
  Utiliser `gtk_switch_get_active()` / `gtk_switch_set_active()`.

**GtkFileChooserButton** — `gtk_file_chooser_get_file()` (API GTK4-forward-compatible)
  retourne un `GFile*` à libérer avec `g_object_unref()`. Tag attribute
  `action="select-folder"` active le mode sélection de dossier.

**GtkCalendar** — le mois est 0-based en interne (0 = janvier) ; l'envvar
  ajoute 1 pour respecter ISO 8601. Parser `sscanf(str, "%d-%d-%d", &y, &m, &d)`
  puis `gtk_calendar_select_month(cal, m-1, y)`.

**GtkLinkButton** — sous-classe de GtkButton. Dans `glade_support.c`, son
  `GTK_IS_LINK_BUTTON()` doit précéder `GTK_IS_BUTTON()` dans la chaîne.

**GtkSearchEntry** — sous-classe de GtkEntry. Dans `glade_support.c`, son
  `GTK_IS_SEARCH_ENTRY()` doit précéder `GTK_IS_SPIN_BUTTON()` et
  `GTK_IS_ENTRY()`. Signal `search-changed` se déclenche après un délai
  interne (anti-rebond automatique).

**GtkInfoBar** — le label interne est stocké via
  `g_object_set_data(G_OBJECT(bar), "_label", label)` pour pouvoir le
  mettre à jour dynamiquement via `<input>` sans recréer le widget.
  Le type de message (couleur / icône) est fixé à la création.

### Fichiers modifiés (patch 9)

```
src/automaton.h         — 6 nouvelles constantes WIDGET_*
src/automaton.c         — includes + cases print-ir + cases create
src/gtkdialog_lexer.l   — règles lexicales pour 6 nouveaux tags
src/gtkdialog_parser.y  — %token + règles grammaire (4 variantes × 6)
src/widgets.c           — envvar_construct + type name
src/variables.c         — fileselect/save/refresh/clear/removeselected
src/signals.c           — refresh
src/glade_support.c     — détection GTK_IS_* + includes
src/Makefile.am         — 6 paires .c/.h dans gtk3sermo_SOURCES
src/widget_switch.c/.h
src/widget_filechooser.c/.h
src/widget_calendar.c/.h
src/widget_linkbutton.c/.h
src/widget_searchentry.c/.h
src/widget_infobar.c/.h
doc/reference/switch.html
doc/reference/filechooser.html
doc/reference/calendar.html
doc/reference/linkbutton.html
doc/reference/searchentry.html
doc/reference/infobar.html
doc/reference/*.html    — navigation mise à jour (37 pages existantes)
```


---

## 9. Système de build

### 9.1 Vue d'ensemble Autotools

```
configure.ac  →  [autoconf]  →  configure
Makefile.am   →  [automake]  →  Makefile.in
configure + Makefile.in  →  [./configure]  →  Makefile
Makefile  →  [make]  →  binaire gtk3sermo
```

### 9.2 Fichiers de build

| Fichier | Rôle | À éditer ? |
|---------|------|------------|
| `configure.ac` | Détection des dépendances, options | OUI — pour ajouter des dépendances |
| `src/Makefile.am` | Sources, flags de compilation | OUI — pour ajouter des fichiers source |
| `Makefile.am` (racine) | Build global, install | OUI — pour ajouter des répertoires |
| `configure` | Script généré par autoconf | JAMAIS — regénérer avec `autoconf` |
| `Makefile` | Généré par `./configure` | JAMAIS — regénérer avec `./configure` |
| `aclocal.m4` | Macros m4 autoconf | JAMAIS directement — utiliser `aclocal` |

### 9.3 Regénérer le système de build

```bash
# Après modification de configure.ac ou Makefile.am :
autoreconf --install --force
./configure
make
```

### 9.4 Ajouter une dépendance

Dans `configure.ac` :
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
    timeout 2 ./src/gtk3sermo --file="$example/$(ls $example | head -1)" \
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
  ./src/gtk3sermo --stdin << 'EOF'
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
ASAN_OPTIONS=detect_leaks=1 ./src/gtk3sermo --stdin < examples/button/button
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
checksec --file=src/gtk3sermo
# Résultat attendu :
# RELRO: Full | STACK CANARY: Canary found | NX: NX enabled | PIE: PIE enabled
```

### 10.6 Débogage avec GDB

```bash
gdb ./src/gtk3sermo
(gdb) run --stdin < examples/tree/tree
(gdb) bt          # backtrace en cas de crash
(gdb) info locals # variables locales
```

### 10.7 Inspecteur GTK3

```bash
GTK_DEBUG=interactive ./src/gtk3sermo --stdin < examples/notebook/notebook
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
valgrind ./src/gtk3sermo --stdin < examples/button/button

# 6. Commit avec message clair
git commit -m "feat(widget_slider): ajouter le widget curseur GTK3

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

### 12.1 Mise à jour des dépendances GTK3

Lors d'une mise à jour de GTK3 :
1. Consulter la liste des dépréciations : https://docs.gtk.org/gtk3/migrating.html
2. Rechercher dans le code les APIs dépréciées : `grep -rn "gtk_XXX_deprecated" src/`
3. Appliquer les remplacements en suivant la documentation GTK

### 12.2 Vérification périodique de sécurité

À effectuer à chaque release :

```bash
# 1. Grep sécurité — appels directs interdits
grep -rn "\bsystem\b\|\bpopen\b\|\bstrcpy\b\|\bstrcat\b\|\bgets\b\|\bsprintf\b" src/ \
  | grep -v "safe_\|//\|parser\|lexer\|\.h:" | wc -l
# Doit être 0

# 1b. Grep pclose — comportement indéfini sur FILE* fdopen()
grep -rn "pclose" src/*.c | grep -v "safe_exec\|widgets\|/\*\|//"
# Doit être 0

# 2. Checksec
checksec --file=src/gtk3sermo | grep -E "Full|enabled"

# 3. Analyse ASAN
make CFLAGS="-fsanitize=address,undefined -g" && \
  for ex in examples/*/; do
    echo -n "$ex: "
    ASAN_OPTIONS=detect_leaks=0 timeout 2 ./src/gtk3sermo \
      --stdin < "$ex/$(ls $ex | head -1)" 2>&1 | grep -c "ERROR" && echo "ERREUR" || echo "OK"
  done
```

### 12.3 Création d'une release

```bash
# 1. Mettre à jour la version dans configure.ac
sed -i 's/AC_INIT(\[gtk3sermo\], \[0\.9\..*\]/AC_INIT([gtk3sermo], [0.9.1]/' configure.ac

# 2. Mettre à jour ChangeLog
echo "$(date +%Y-%m-%d)  Mainteneur <email>" >> ChangeLog
echo "    * Corrections de bugs, améliorations de sécurité" >> ChangeLog

# 3. Générer l'archive
autoreconf --install
./configure
make dist
# → gtk3sermo-0.9.1.tar.gz

# 4. Tagger dans git
git tag -a v0.9.1 -m "Release 0.9.1"
git push origin v0.9.1
```

---

## 13. Roadmap technique

### Version 1.0.0 — livrée (mai 2026)

- [x] Migration GTK2 → GTK3 (45 fichiers)
- [x] Correctifs de sécurité (safe_exec, strcpy, malloc)
- [x] Mise à jour du build system (autotools)
- [x] Correction UB `pclose()` → `fclose()` (2026-05-21)
- [x] 43 widgets GTK3, validés par 55 tests XML de régression + 9 tests de comportement `safe_exec`
- [x] Paquet `.deb` construit

### Pistes techniques ultérieures

- [ ] Réduction des avertissements gcc restants avec `-Wall -Wextra`
- [ ] Élargissement des tests de comportement au-delà de `safe_exec`
- [ ] Amélioration `safe_exec.c` pour supporter les pipes shell via `execvp()`
- [ ] Remplacement du parser Yacc/Lex par GLib GMarkup (meilleure gestion d'erreurs XML)
- [ ] Mode serveur (socket Unix) pour interfaces persistantes

> La feuille de route **produit** (versions à venir) est tenue dans le `ROADMAP.md`
> à la racine du dépôt.

---

## 14. Références et ressources

### Documentation officielle

- **GTK3** : https://docs.gtk.org/gtk3/
- **GLib** : https://docs.gtk.org/glib/
- **GIO** : https://docs.gtk.org/gio/
- **VTE 2.91** : https://gnome.pages.gitlab.gnome.org/vte/
- **Migration GTK2→GTK3** : https://docs.gtk.org/gtk3/migrating-2to3.html

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
| `MANUEL_UTILISATEUR.md` | Guide pour les utilisateurs finaux |
| `MANUEL_DEVELOPPEUR.md` | Ce document |
| `COMPILE.md` | Instructions rapides de compilation |
| `src/safe_exec.c` | Module de sécurité (wrappers commandes) |
| `configure.ac` | Configuration du build (dépendances) |
| `src/Makefile.am` | Sources et flags de compilation |

---

