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
11. Programmes d'aide (`sermo-source/`)

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
        │    efl/sdl3/lvgl/        │
        │    ncurses/html)         │
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
| **neutre** (défaut) | — | qt6, fltk1, efl1, sdl3, lvgl, ncurses, html | Types via un **shim** (`sermocore-shim.h`) ; en-têtes toolkit remplacés par des **stubs** vides (`include/_shim/`). Aucune dépendance graphique. |
| **glib/gtk3** | `-DSERMOCORE_GLIB=ON` | gtk3 | Vraie GLib/GTK 3 (ABI native des structs `GList`, `GtkWidget`…), pour l'introspection GObject utilisée par la famille GTK. |
| **gtk4** | `-DSERMOCORE_GTK4=ON` | gtk4 | Fichiers cœur **réécrits pour GTK 4**, **vendorés dans `libsermocore/src-gtk4/`** (5 fichiers : `automaton.c`, `variables.c`, `signals.c`, `tag_attributes.c`, `gtkdialog.c` + le shim `gtk4-compat.c` ; les autres, `actions.c` compris, viennent de `src/`) : GTK 4 a retiré `gtk_main`, `gtk_socket`, l'ancien modèle d'évènements. Aucune dépendance externe. Voir §9. |

Deux autres options de construction :

| Option CMake | Où | Défaut | Effet |
|---|---|---|---|
| `SERMOCORE_VTE` | `libsermocore` | `ON` | Widget `<terminal>` par VTE dans les variantes glib/gtk3 et gtk4. Le choix part dans `sermocore.pc` (variable `vte`), et les backends gtk3 et gtk4 le suivent. VTE absent arrête la construction ; `-DSERMOCORE_VTE=OFF` construit sans, et `<terminal>` n'affiche qu'un avertissement. |
| `SERMO_LVGL_LEGER` | `sermo-backend-lvgl` | `OFF` | lvgl sans ThorVG ni Lottie, donc sans icônes SVG (paquet `sermo-backend-lvgl-leger`, même binaire `lvglsermo`). |

Les variantes **neutre** et **gtk3** compilent aujourd'hui **le même jeu de
sources** cœur (`src/*.c`), à un fichier près : `sermo_progress.c` (la barre de
progression lue au fil de l'eau) n'entre que dans la variante neutre, gtk3
gardant son propre fil de lecture (`libsermocore/CMakeLists.txt`). Pour le
reste, seuls diffèrent les drapeaux et en-têtes (shim contre vraie GLib). Le décompte historique « 9/10 fichiers unifiés » date de la
phase de modularisation et ne s'applique plus. Le seul fichier « famille-spécifique » côté
neutre/gtk3 est le **bridge d'attributs** (`try_set_property` +
`widget_set_tag_attributes`), sorti du cœur vers le backend (`tag_set_property.c`)
parce qu'il repose sur l'introspection GObject que les backends neutres n'ont pas.

Fichiers du cœur : `gtkdialog.c` (orchestration + `main`), `safe_exec.c`,
`sermo_lanceur.c` (le lanceur de commandes, en un seul endroit, et le choix du
shell : `SERMO_SHELL`, sinon `SERMO_SHELL_DEFAUT`),
`variables.c`, `stack.c`, `automaton.c`, `attributes.c`, `signals.c`,
`stringman.c`, `actions.c`, `tag_attributes.c` (helpers), `sermo_icon_theme.c`,
`sermo_input.c` (plafond des `<input>`), `sermo_source.c` (traduction de
`<input modbus|opcua|mqtt|mavlink|media>` et `<action mqtt|mavlink|media>` en
commande, voir §11), `sermo_focus.c` (quel widget prend le
focus à l'ouverture), `sermo_progress.c` (variante neutre seule),
`sermo_win32.c` (Windows seulement : console, lancement des commandes, objet
compilé sans le shim),
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
  natif et le connecte. L'extension suit la famille : `.c` pour gtk3/gtk4/efl1/sdl3/lvgl/ncurses/html,
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
5. Construire contre la lib installée (`PKG_CONFIG_PATH=…/libsermocore/_install/lib/pkgconfig`)
   puis faire passer **les trois bancs** (§6).

Symboles typiquement à fournir en plus (résolus au lien) : `widget_show_list` /
`widget_hide_list` (globales de visibilité), et la glue référencée par le cœur.

Pour rester à parité avec l'étalon, une `<list>`, un `<tree>` ou une `<table>`
demande au cœur, à sa création, s'il prend le focus à l'ouverture
(`sermo_focus_initial()`, `libsermocore/include/sermo_focus.h`) : seule cette
réponse décide si sa 1re ligne est choisie sans `<default>`.

## 6. Bancs de vérification (les trois portes)

Aucune étape n'avance avec un banc rouge. Les trois portes, pour un binaire
`<port>` donné :

```sh
# 1. XML — parse en headless (--print-ir), sans affichage
TIMEOUT=5 bash tests/xml/run_tests.sh <binaire>          # attendu : 55 PASS

# 2. Comportement — les valeurs exportées == celles de l'étalon gtk3sermo
bash tests/comportement/run.sh <binaire>                 # attendu : 78/78

# 3. Garde — les valeurs à risque ne repassent pas par le shell
xvfb-run -a bash tests/garde_echappement_sortie.sh <binaire>        # OK — 4 cas
```

Le banc de comportement compare les **exports** (`VAR="valeur"`) à ceux de
l'étalon gtk3sermo. Chaque cas se ferme seul : par un `<timer>` déclenchant
`exit:fin`, ou (cas 41 et 42) par l'action d'une barre de progression arrivée à 100.

Au-delà des trois portes, le test de réussite d'un port est
`bash examples/metiers/verifie.sh --ports "<port>"` : il joue chaque dialogue des
scripts métier et des boîtes à outils (104 scripts, 187 dialogues) et compare les
valeurs à gtk3sermo.

### Tous les bancs d'un coup — la CI

`ci/construire.sh` construit les variantes utiles du cœur, les backends
demandés (défaut : les neuf), puis `sermoia-mcp`, le port lvgl léger
(`-DSERMO_LVGL_LEGER=ON`, quand lvgl est demandé) et les huit programmes d'aide
de `sermo-source/` (`-DSERMO_SOURCE_EXIGER=ON` : une bibliothèque absente est
un échec). `ci/bancs.sh` enchaîne **tous** les bancs : tests
unitaires du cœur, version unique, liens de langue, gardes sur les sources
(SPDX, licences des scripts, fonctions interdites, thread de la barre de
progression, en-têtes de poste, lectures d'`<input>` plafonnées), les
programmes d'aide (Modbus/OPC UA, MQTT en lecture et en écriture, MAVLink en
lecture et en écriture contre un faux drone, lecteur son et vidéo) et leur
durcissement binaire par binaire, les exemples outillés (approbation, réseau,
GNSS, série, domotique, CAN, DNS, TLS, journal, USB, NTP, Wi-Fi, Bluetooth,
caméra), le comportement du lvgl léger, le poids par port, le contrôle de
façade, le FORTIFY du cœur, puis pour chaque backend le XML, l'identité, le
durcissement, `SERMO_ALLOWED_CMDS`, les sources sans fin
(`garde_input_sans_fin.sh`), `<input modbus|opcua|mqtt|mavlink>`, `<action
mqtt|mavlink>` au clic seul, `--do`, `SERMO_SHELL`, la limite de widgets,
`--include`, Glade, `<terminal>`, l'échappement, le clic et l'ouverture réelle des exemples de
`examples/` (le mot de passe masqué pour ncurses ; ni clic ni exemples pour html,
qui n'ouvre pas de fenêtre), le comportement sur tous les ports construits,
les deux bancs de `sermoman-mcp`, les scripts métier et celui de
`sermoia-mcp`. La CI (`.gitlab-ci.yml`, et son jumeau
`.github/workflows/bancs.yml`) n'appelle, pour construire et éprouver, que ces
deux scripts, dans une Debian testing munie de `ci/dependances.txt` (le reste
de la recette installe les dépendances et la locale française) : ce qu'elle
voit se rejoue à l'identique.

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
cmake -S sermo-backend-qt6 -B sermo-backend-qt6/_build -DCMAKE_INSTALL_PREFIX=$PWD/_install_qt6
make -C sermo-backend-qt6/_build && make -C sermo-backend-qt6/_build install

# Les programmes d'aide (§11)
cmake -S sermo-source -B sermo-source/_build -DSERMO_SOURCE_EXIGER=ON
cmake --build sermo-source/_build

# Paquets .deb séparés, construits et éprouvés dans une Debian testing vierge
bash packaging/construire-paquets.sh
```

## 8. Sécurité — règles obligatoires

Héritées de sermo, **non négociables** :

- **Jamais** `system`/`popen`/`strcpy`/`strcat`/`sprintf`. Utiliser `safe_system`
  / `safe_popen` (dans le cœur) et les équivalents GLib bornés.
- `safe_system` n'ouvre un shell **que** si la commande contient des
  métacaractères ; sinon exécution directe par `argv`. Le repli par le shell
  (`/bin/sh -c`, ou `SERMO_SHELL`) est **journalisé**, comme un fait et non une
  alarme : `safe_system: '<commande>' contient de la syntaxe shell -- execution
  via /bin/sh -c` (`libsermocore/src/safe_exec.c`). L'ancien message « injection
  risk » a été retiré : dit à chaque `--do` ordinaire, il ne s'entendait plus.
  Avec `SERMO_NO_SHELL_FALLBACK` ou `SERMO_ALLOWED_CMDS`, le repli est
  **refusé**, et ce refus reste une alarme (`g_critical`).
- Flags de durcissement **à conserver** sur chaque cible : `-D_FORTIFY_SOURCE=3`,
  `-fstack-protector-strong`, `-fstack-clash-protection`, `-fcf-protection=full`,
  PIE/RELRO/BIND_NOW/NX. `tests/garde_durcissement.sh` les mesure sur le
  **binaire produit**, pas sur les drapeaux (`readelf`) : PIE, RELRO, BIND_NOW,
  pile non exécutable, CET IBT, CET SHSTK, protecteur de pile
  (`__stack_chk_fail`) et FORTIFY (appels `__*_chk`, `__stack_chk_*` exclus),
  soit huit garanties. Un **témoin** passe d'abord : deux petits binaires,
  fortifié et nu, que la mesure FORTIFY doit distinguer. `-fstack-clash-protection`
  ne laisse pas de trace que ce banc lit : il n'est pas vérifié par lui.
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

Deux outils accompagnent le dépôt sans faire partie de la chaîne de rendu ; tous
deux sont empaquetés depuis la 2.8.2 :

- **`sermoman-mcp/`** — un serveur **MCP** de **documentation** (lecture seule) qui
  expose la référence sermo à une IA : outils `sermo_reference`, `sermo_guide`,
  `sermo_architecture`, `sermo_heritage`, `sermo_ecosystem`, `sermo_scripts`,
  `sermo_search`, `sermo_example`, `sermo_how_to_report`. Code écrit pour sermo,
  GPL-2.0-or-later ; une partie des exemples qu'il sert vient de gtkdialog.
  `sermo_scripts` sert l'inventaire des scripts métier et des boîtes à outils,
  engendré par `sermoman-mcp/outils/inventaire-scripts.py`.
- **`sermoia-mcp/`** — un serveur **MCP** (en C) qui fait **écrire, vérifier et
  voir** un dialogue : `sermo_schema`, `sermo_check`, `sermo_run`, `sermo_render`,
  `sermo_examples`, `sermo_vote`. Son catalogue comprend les dialogues des scripts
  métier et des boîtes à outils (`sermoia-mcp/outils/catalogue-metiers.sh`). Voir
  [sermoia-mcp/README.md](sermoia-mcp/README.md).

La vérification et le rendu hors écran restent aussi disponibles en ligne de
commande (`--print-ir`, `--render-png`).

## 11. Programmes d'aide (`sermo-source/`)

Les balises `<input modbus|opcua|mqtt|mavlink|media>` et `<action
mqtt|mavlink|media>` ne font rien elles-mêmes. Le cœur (`sermo_source.c`) les
traduit en une commande qui lance un **programme d'aide**, installé dans
`SERMO_SOURCE_DIR` (défaut `/usr/libexec/sermo`). Les neuf ports n'y voient
qu'une commande de plus : aucun port ne dépend de libmodbus, open62541,
libmosquitto ni MAVLink.

**Le contrat** est `libsermocore/include/sermo_source.h` :

- l'adresse part en **un seul argument**, encodée « pour cent » (tout octet hors
  de `[A-Za-z0-9._:/=,+@-]` devient `%XX`) : la commande n'a aucun
  métacaractère, elle part en `argv`, jamais par le shell, même avec
  `SERMO_NO_SHELL_FALLBACK` ou `SERMO_ALLOWED_CMDS` ;
- refusé à l'analyse du XML : protocole inconnu, adresse vide, plus de 4096
  octets (`SERMO_SOURCE_ADRESSE_MAX`), caractère de contrôle, `SERMO_SOURCE_DIR`
  relatif ou porteur d'un caractère hors de l'ensemble sûr ;
- le programme lancé est `<dossier>/sermo-source-<protocole>` (lecture) ou
  `<dossier>/sermo-cible-<protocole>` (écriture). Le cœur n'accepte une
  `<action>` d'écriture que dans un `<button>` ou un `<menuitem>`, et refuse le
  nom `sermo-cible-` écrit à la main dans un `<input>` ou une `<action>`.

**Les huit programmes**, un paquet chacun :

| Programme | Rôle | Bibliothèque |
|---|---|---|
| `sermo-source-modbus` | lit des registres Modbus (TCP, RTU) | libmodbus |
| `sermo-source-opcua` | lit une variable OPC UA | open62541 |
| `sermo-source-mqtt` | lit un sujet MQTT (jamais de publication) | libmosquitto |
| `sermo-cible-mqtt` | publie une valeur, d'après une liste blanche | libmosquitto |
| `sermo-source-mavlink` | lit la télémétrie d'un autopilote (n'envoie aucun octet) | en-têtes MAVLink embarqués |
| `sermo-cible-mavlink` | règle un paramètre, donne un ordre de sécurité, envoie une mission | en-têtes MAVLink embarqués |
| `sermo-source-media` | lit l'état d'un lecteur mpv | aucune |
| `sermo-cible-media` | lance et commande mpv (en cage `bwrap` si présent) | aucune (mpv est un programme) |

**Les fichiers partagés** :

- `commun.c` / `commun.h` — à tous : lecture de l'adresse (décodage, 32
  paramètres au plus, clés inconnues refusées, entiers bornés), pause de 5 s
  après un échec, cache commun sous `$XDG_RUNTIME_DIR/sermo-source/`, verrou par
  équipement, et `source_lire()`, qui ne fige jamais la fenêtre (valeur
  fraîche, valeur périmée relue en fond, ou case vide) ;
- `mqtt-commun.c` / `.h` — lecture et écriture MQTT : adresse `mqtt[s]://`,
  chiffrement (certificat et nom vérifiés, jamais « insecure »), identifiants
  (refusés sur `mqtt://` sans `clair=oui`), connexion bornée par un délai ;
- `mavlink-commun.c` / `.h` — lecture et écriture MAVLink : adresse du lien
  (`udp://`, `tcp://`, `serie://`), débits admis, ouverture ;
- `media-commun.c` / `.h` — les deux programmes media : nom du lecteur, dossier
  privé `$XDG_RUNTIME_DIR/sermo-media/NOM/` (700), échange avec le socket de
  mpv (seules des requêtes écrites par le programme y partent) ;
- `mission-wpl.c` / `.h` — lecture d'un fichier de mission « QGC WPL 110 »,
  réduite aux points de passage (100 au plus, 256 Kio au plus) ;
- `tiers/mavlink/` — les en-têtes C de MAVLink (protocole v2), copiés sans
  modification ; origine, empreinte et licence dans son `LISEZMOI`.

**Construire** :

```sh
cmake -S sermo-source -B sermo-source/_build -DSERMO_SOURCE_EXIGER=ON
cmake --build sermo-source/_build
```

Les programmes media et MAVLink sont toujours construits. Les autres le sont si
leur bibliothèque est là (`libmodbus-dev`, `libopen62541-1.4-dev`,
`libmosquitto-dev`) ; sans `SERMO_SOURCE_EXIGER`, une bibliothèque absente
saute son programme avec un avertissement. Avec `-DSERMO_SOURCE_EXIGER=ON`
(CI, paquets), elle arrête la construction. Chaque programme reçoit le même
durcissement que les ports ; `ci/bancs.sh` le mesure sur chacun avec
`tests/garde_durcissement.sh`. Pour essayer sans installer, poser
`SERMO_SOURCE_DIR` sur `sermo-source/_build` (chemin absolu).
