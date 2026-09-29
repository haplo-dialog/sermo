# Manuel Développeur et Mainteneur — sermo (édition modulaire)

[English](MANUEL_DEVELOPPEUR.en.md)

Ce document décrit l'**architecture interne** de sermo et comment y contribuer :
le cœur `libsermocore`, les backends de rendu, les variantes de build, l'ajout
d'un nouveau backend, et les bancs de non-régression.

## Table des matières

1. Vue d'ensemble de l'architecture
2. Le cœur `libsermocore` — les trois variantes de build
3. La frontière cœur ↔ backend
4. Anatomie d'un backend
5. Ajouter un nouveau backend
6. Bancs de vérification (les trois portes)
7. Construire et installer
8. Sécurité — règles obligatoires
9. Feuille de route architecturale (Part 3)
10. Composants annexes

---

## 1. Vue d'ensemble de l'architecture

```
                 script XML
                     │
        ┌────────────▼─────────────┐
        │      libsermocore        │   ← C durci, SANS toolkit
        │  lexer + parser (Flex/   │
        │  Bison), automaton,      │
        │  variables, actions,     │
        │  safe_exec (durcissement)│
        └───────┬──────────┬───────┘
        appelle │          │ fournit (widget_*_create,
   widget_*_create         │  boucle, ponts d'opérations)
                │          │
        ┌───────▼──────────▼───────┐
        │   sermo-backend-<toolkit> │  ← widgets natifs + boucle
        │   (gtk3/gtk4/qt6/fltk/    │     d'évènements du toolkit
        │    efl/sdl3/ncurses)     │
        └──────────────────────────┘
```

Le cœur **analyse** le XML et pilote la **logique** (variables, actions,
exécution shell durcie). Le backend **construit et dessine** les widgets et tient
la **boucle d'évènements** du toolkit. Le cœur ne connaît aucun type graphique
concret : il manipule des pointeurs opaques (`GtkWidget *` = pointeur générique).

### Principes

- **Un seul cœur, plusieurs backends.** La grammaire (lexer/parser) et la logique
  d'exécution sont partagées ; seul le rendu diffère.
- **Le cœur ne dépend d'aucun toolkit** (variante neutre : `ldd` ne montre ni GTK
  ni Qt ni SDL). L'exécution shell durcie y est concentrée.
- **Le backend fournit les widgets** (`widget_*_create`) et les opérations que le
  cœur lui demande (montrer, cacher, ajouter au conteneur, boucle d'évènements).

## 2. Le cœur `libsermocore` — les trois variantes de build

Le cœur se compile depuis **une source largement unifiée** en trois variantes
(option CMake), parce que les familles de toolkits n'ont pas la même ABI :

| Variante | Option CMake | Familles | Détail |
|---|---|---|---|
| **neutre** (défaut) | — | qt6, fltk1, efl1, sdl3, ncurses | Types via un **shim** (`sermocore-shim.h`) ; en-têtes toolkit remplacés par des **stubs** vides (`include/_shim/`). Aucune dépendance graphique. |
| **glib/gtk3** | `-DSERMOCORE_GLIB=ON` | gtk3 | Vraie GLib/GTK 3 (ABI native des structs `GList`, `GtkWidget`…), pour l'introspection GObject utilisée par la famille GTK. |
| **gtk4** | `-DSERMOCORE_GTK4=ON` | gtk4 | Fichiers cœur **réécrits pour GTK 4**, **vendorés dans `libsermocore/src-gtk4/`** (5 fichiers : `automaton.c`, `variables.c`, `signals.c`, `actions.c`, `gtkdialog.c` + le shim `gtk4-compat.c`) : GTK 4 a retiré `gtk_main`, `gtk_socket`, l'ancien modèle d'évènements. Aucune dépendance externe. Voir §9. |

Les variantes **neutre** et **gtk3** compilent aujourd'hui **exactement le même
jeu de sources** cœur (`src/*.c`) ; seuls diffèrent les drapeaux et en-têtes (shim
contre vraie GLib). Le décompte historique « 9/10 fichiers unifiés » date de la
phase de modularisation et ne s'applique plus. Le seul fichier « famille-spécifique » côté
neutre/gtk3 est le **bridge d'attributs** (`try_set_property` +
`widget_set_tag_attributes`), sorti du cœur vers le backend (`tag_set_property.c`)
parce qu'il repose sur l'introspection GObject que les backends neutres n'ont pas.

Fichiers du cœur : `gtkdialog.c` (orchestration + `main`), `safe_exec.c`,
`variables.c`, `stack.c`, `automaton.c`, `attributes.c`, `signals.c`,
`stringman.c`, `actions.c`, `tag_attributes.c` (helpers), `sermo_icon_theme.c`,
+ `gtkdialog_lexer.l` / `gtkdialog_parser.y` (grammaire, source de vérité).

## 3. La frontière cœur ↔ backend

Le contrat est déclaré dans `sermo_backend.h` (installé avec la lib). Le cœur
**appelle**, le backend **fournit** :

- **`sermo_backend_toolkit_init(int *argc, char ***argv, int print_ir)`** — le
  backend initialise son toolkit (`gtk_init`, `QApplication`, `SDL_Init`…), sans
  ouvrir d'affichage en mode `--print-ir`.
- **`widget_*_create(...)`** — un par type de widget (bouton, entrée, liste…).
  Ils renvoient le widget natif sous forme de pointeur opaque.
- **Le pont d'opérations `sermo_be_*`** (variante neutre) : le shim mappe les
  primitives toolkit vers un petit jeu de fonctions que le backend implémente —
  `sermo_be_widget_show`/`hide`/`set_sensitive`/`redraw`, `sermo_be_container_add`,
  `sermo_be_window_move`, `sermo_be_run_loop`, `sermo_be_app_init`/`quit`. C'est le
  contrat de frontière : le cœur ne nomme plus aucun port (le rename
  historique `qt6_*` → `sermo_be_*` est fait).
- **Identité du port** (facultative) : `sermo_port_name` et
  `sermo_port_details`, deux symboles **faibles** déclarés par le cœur. Sans
  eux, `--version` imprime le nom du cœur — et les sept ports s'annonçaient
  « sermocore ». Le plus simple est de compiler `contract/sermo_port_id.c` dans
  le port, en lui passant `SERMO_PORT_NAME` et `SERMO_PORT_VERSION` (les
  `CMakeLists.txt` le font déjà). ⚠️ La ligne de `--version` a une forme figée :
  le 1er mot est le nom, le 3e le numéro — des exemples livrés lisent la version
  à ce rang. `tests/garde_identite_port.sh` vérifie les deux.
- **`main` vit dans le cœur** (dans `gtkdialog.c`), et rappelle
  `sermo_backend_toolkit_init` là où l'ancien code appelait `gtk_init`.

En sens inverse, le backend rappelle `execute_action()` du cœur quand un
évènement survient (clic, timer) pour exécuter l'action associée.

**Note de packaging.** Le cœur appelle `widget_*_create` (fournis par le backend),
donc `libsermocore` a des symboles non résolus : c'est une **bibliothèque
statique** (`.a`), liée dans chaque backend. Une `.so` partagée propre suppose la
frontière IR (§9).

## 4. Anatomie d'un backend

Un backend `sermo-backend-<toolkit>` contient :

- `widget_*.c` **ou** `widget_*.cpp` — un fichier par widget, qui crée le widget
  natif et le connecte. L'extension suit la famille : `.c` pour gtk3/gtk4/efl1/sdl3/ncurses,
  `.cpp` pour les backends C++ (qt6, fltk1).
- `widgets.c`/`widgets.cpp` — la glue de rendu (`widget_show_all`, `widget_get_text_value`…).
- `tag_set_property.c` — le bridge d'attributs, version du toolkit (GObject pour
  gtk, réimplémentation pour les neutres).
- Le **hook d'init** (`sermo_backend_<toolkit>.c`) : `sermo_backend_toolkit_init`
  + les ponts d'opérations pour la famille neutre.
- Son `CMakeLists.txt` : `pkg_check_modules(SERMOCORE REQUIRED sermocore)` +
  les libs du toolkit ; **lien statique** du cœur.

Le backend consomme le cœur **via pkg-config** (`sermocore.pc`). Pour la bonne
variante, on pointe `PKG_CONFIG_PATH` sur le préfixe d'install correspondant
(neutre / gtk3 / gtk4).

## 5. Ajouter un nouveau backend

1. Créer `sermo-backend-<t>/src/` avec les `widget_*.c`/`.cpp` et `widgets.*` pour
   votre toolkit (partez d'un backend voisin de la même famille : `qt6` (C++) ou
   `sdl3` (C) pour un neutre, `gtk3` pour une famille GObject). L'extension des
   fichiers suit le langage du backend (`.cpp` pour qt6/fltk1, `.c` ailleurs).
2. Fournir `tag_set_property.c` (bridge d'attributs) et, pour un backend neutre,
   les ponts d'opérations (`sermo_backend_<t>.c`).
3. Implémenter `sermo_backend_toolkit_init` (init du toolkit).
4. Écrire le `CMakeLists.txt` : `pkg_check_modules(SERMOCORE REQUIRED sermocore)`,
   les libs du toolkit, `-D_FORTIFY_SOURCE=3 -fstack-protector-strong
   -fcf-protection=full`.
5. Construire contre la lib installée (`PKG_CONFIG_PATH=…/libsermocore/_install`)
   puis faire passer **les trois bancs** (§6).

Symboles typiquement à fournir en plus (résolus au lien) : `widget_show_list` /
`widget_hide_list` (globales de visibilité), et la glue référencée par le cœur.

## 6. Bancs de vérification (les trois portes)

Aucune étape n'avance avec un banc rouge. Les trois portes, pour un binaire
`<port>` donné :

```sh
# 1. XML — parse en headless (--print-ir), sans affichage
TIMEOUT=5 bash tests/xml/run_tests.sh <binaire>          # attendu : 55 PASS

# 2. Comportement — les valeurs exportées == celles de l'étalon gtk3sermo
bash tests/comportement/run.sh <binaire>                 # attendu : 53/53

# 3. Garde — les valeurs à risque ne repassent pas par le shell
xvfb-run -a bash tests/garde_echappement_sortie.sh <binaire>        # OK — 4 cas
```

Le banc de comportement compare les **exports** (`VAR="valeur"`) à ceux de
l'étalon gtk3sermo. Chaque cas se ferme seul : par un `<timer>` déclenchant
`exit:fin`, ou (cas 41 et 42) par l'action d'une barre de progression arrivée à 100.

### Tous les bancs d'un coup — la CI

`ci/construire.sh` construit les variantes utiles du cœur puis les backends
demandés (défaut : les sept) ; `ci/bancs.sh` enchaîne **tous** les bancs : tests
unitaires du cœur, gardes sur les sources (SPDX, fonctions interdites, thread de
la barre de progression, en-têtes de poste, FORTIFY du cœur), puis pour chaque
backend le XML, le durcissement, `SERMO_ALLOWED_CMDS`, les sources sans fin
(`garde_input_sans_fin.sh`), `--do`, la limite de widgets, l'échappement, le clic et l'ouverture réelle des exemples de
`examples/` (le mot de passe masqué pour ncurses), le comportement sur les sept,
et les deux bancs de `sermoman-mcp`. La CI
(`.gitlab-ci.yml`) n'appelle que ces deux scripts, dans une Debian testing munie
de `ci/dependances.txt` : ce qu'elle voit se rejoue à l'identique.

Règles tenues par ces scripts, parce que chacune a déjà menti une fois :

- **tout jouer, puis conclure** : un banc rouge n'arrête pas les suivants, le
  code retour vaut 1 s'il y en a un seul ;
- **un banc qui n'a rien vérifié est un échec** (code 77, « IGNORÉ ») ;
- **un banc figé est arrêté** (`SERMO_BANC_DELAI`, 1800 s par défaut) et compté
  en échec ;
- **aucun affichage** : `DISPLAY` est retiré, chaque garde graphique démarre son
  propre Xvfb — un poste de travail mesure ce que mesure la CI.

### Contrôle de façade (avant publication)

`tests/verifie-facade.sh` cherche ce qui ne doit pas sortir : en-têtes
d'horodatage de poste, renvois à des documents de décision internes, marques
d'attribution dans les messages de commit, chemins personnels. Il fouille
l'arbre suivi, ou tout l'historique (`--historique REF` : messages, auteur et
committer, chaque version de chaque fichier, étiquettes annotées), ou des
produits (`--fichiers`, `.deb` dépaquetés). Un **témoin** (un petit dépôt
fabriqué, porteur de neuf défauts) doit être vu en entier avant toute
conclusion.

- **La CI et les contributeurs** : `--sans-identite`, sur l'arbre. Une
  contribution signée garde le nom de son auteur.
- **Le mainteneur, avant de publier** : `SERMO_BANC_PUBLICATION=1 bash ci/bancs.sh`,
  qui ajoute `--strict`, l'identité de chaque commit et tout l'historique. Les
  motifs propres au poste du mainteneur ne sont **jamais** versionnés : ils vivent
  dans `$SERMO_FACADE_MOTIFS` ou `<git-common-dir>/facade-motifs`, et `--strict`
  refuse de conclure sans eux.

## 7. Construire et installer

```sh
# Cœur — variante neutre (défaut)
cmake -S libsermocore -B libsermocore/_build -DCMAKE_INSTALL_PREFIX=$PWD/libsermocore/_install
make -C libsermocore/_build && make -C libsermocore/_build install

# Cœur — variante gtk3
cmake -S libsermocore -B libsermocore/_build_gtk -DSERMOCORE_GLIB=ON \
      -DCMAKE_INSTALL_PREFIX=$PWD/libsermocore/_install_gtk
make -C libsermocore/_build_gtk && make -C libsermocore/_build_gtk install

# Un backend (ex. qt6, variante neutre)
export PKG_CONFIG_PATH=$PWD/libsermocore/_install/lib/pkgconfig
cmake -S sermo-backend-qt6 -B sermo-backend-qt6/_build
make -C sermo-backend-qt6/_build

# Paquets .deb séparés, construits et éprouvés dans une Debian testing vierge
bash packaging/construire-paquets.sh
```

## 8. Sécurité — règles obligatoires

Héritées de sermo, **non négociables** :

- **Jamais** `system`/`popen`/`strcpy`/`strcat`/`sprintf`. Utiliser `safe_system`
  / `safe_popen` (dans le cœur) et les équivalents GLib bornés.
- `safe_system` n'ouvre un shell **que** si la commande contient des
  métacaractères ; sinon exécution directe par `argv`. Le repli `/bin/sh -c` est
  **journalisé** (« injection risk »).
- Flags de durcissement **à conserver** sur chaque cible : `-D_FORTIFY_SOURCE=3`,
  `-fstack-protector-strong`, `-fstack-clash-protection`, `-fcf-protection=full`,
  PIE/RELRO/BIND_NOW/NX. Vérifiés par `tests/garde_durcissement.sh`.
- La frontière de confiance est **l'auteur local du script** (documenté), pas une
  liste blanche. Voir [SECURITY.md](SECURITY.md).
- **Toute lecture d'un `<input>` est plafonnée** (`libsermocore/include/sermo_input.h`,
  16 Mio par défaut) : une commande passe par `widget_opencommand()` →
  `safe_popen()`, un fichier par `sermo_fopen_input()`. Un lecteur qui ouvrirait
  un fichier par `fopen`, `open` ou `g_file_get_contents` échapperait à la
  limite. Seule la barre de progression, qui ne garde qu'une ligne, la retire
  (`sermo_input_sans_limite()`). Vérifié par `tests/garde_input_sans_fin.sh`.

## 9. Feuille de route architecturale (Part 3)

Le contrat de frontière `sermo_be_*` est **en place** : le rename
`qt6_*` → `sermo_be_*` est fait et la boucle passe par `sermo_be_run_loop()` — le
cœur ne nomme plus aucun port. Un seul chantier reste pour une architecture
pleinement propre :

1. **Source unique gtk3+gtk4** — la dépendance externe à l'ancienne édition monolithique est levée
   (le cœur GTK4 est **vendoré** dans `libsermocore/src-gtk4/`), mais la variante
   `SERMOCORE_GTK4` reste un jeu de sources séparé. GTK 4 ayant retiré
   `gtk_main`/`gtk_socket`/l'ancien modèle d'évènements, cinq fichiers cœur
   divergent : les réconcilier en UNE source (retirer `SERMOCORE_GTK4`) est le
   vrai travail restant.
2. **Formaliser une IR** consommée par le backend → une **`.so` partagée** propre
   (le cœur ne rappellerait plus `widget_*_create`), puis à terme un
   **cloisonnement de processus** (IPC) confinant le shell au cœur.

Ces étapes sont incrémentales et gardées par les trois bancs.

## 10. Composants annexes

Un outil accompagne le dépôt sans faire partie de la chaîne de rendu :

- **`sermoman-mcp/`** — un serveur **MCP** de **documentation** (lecture seule) qui
  expose la référence sermo à une IA : outils `sermo_reference`, `sermo_guide`,
  `sermo_architecture`, `sermo_heritage`, `sermo_ecosystem`, `sermo_search`,
  `sermo_example`, `sermo_how_to_report`. Code écrit pour sermo, GPL-2.0-or-later ;
  une partie des exemples qu'il sert vient de gtkdialog.

Le serveur MCP de **génération** de fenêtres par une IA (boucle vérifier / rendre
/ voter sur un catalogue d'exemples), qui vivait ici, **a été retiré de sermo**.
Le dépôt garde le MCP de **manuel** ci-dessus, en lecture seule. La vérification
et le rendu hors écran restent disponibles en ligne de commande
(`--print-ir`, `--render-png`).
