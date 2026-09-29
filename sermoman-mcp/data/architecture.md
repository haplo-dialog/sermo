# Architecture modulaire de sermo

sermo est une boîte à dialogues pilotée par XML, de la lignée de **gtkdialog
0.8.3** (László Pere, GPL-2.0+). Un même script XML décrit une fenêtre ; sermo la
lit, l'affiche et renvoie les valeurs saisies sur la sortie standard.

Aujourd'hui, sermo n'est **plus** un binaire unique par toolkit. Il est découpé
en deux morceaux :

- **un cœur unique**, `libsermocore`, qui lit le XML et pilote la logique ;
- **sept backends de rendu**, un par boîte à outils graphique, qui dessinent les
  widgets.

Le même XML rend le **même comportement** sur les sept backends (parité mesurée
contre l'étalon `gtk3sermo`).

```
                 script XML
                     │
        ┌────────────▼─────────────┐
        │      libsermocore        │   ← C durci, SANS toolkit
        │  lexer + parser Flex/    │
        │  Bison, automate, variables,
        │  actions, exécution durcie│
        └───────┬──────────┬───────┘
        appelle │          │ fournit (widgets, boucle,
   widget_*_create         │  ponts d'opérations)
                │          │
        ┌───────▼──────────▼───────┐
        │  sermo-backend-<toolkit> │  ← widgets natifs + boucle
        │  gtk3 gtk4 qt6 fltk1     │     d'évènements du toolkit
        │  efl1 sdl3 ncurses       │
        └──────────────────────────┘
```

---

## 1. Le cœur — libsermocore

Le cœur fait deux choses, et **rien** de graphique :

1. **Analyse le XML** — un seul lexer/parser Flex/Bison
   (`gtkdialog_lexer.l` + `gtkdialog_parser.y`), source de vérité de la grammaire.
   Il n'y a **qu'une** grammaire dans tout le dépôt.
2. **Pilote la logique** — l'automate, les variables, les actions et
   l'**exécution shell durcie** (`safe_exec.c` : `safe_system` / `safe_popen`).

La **variante neutre** du cœur ne tire **aucune** boîte à outils : `ldd` n'y
montre ni GTK, ni Qt, ni SDL. Les types graphiques passent par un *shim*
(`sermocore-shim.h`) et des en-têtes toolkit remplacés par des stubs vides.

Le cœur ne connaît **aucun** type graphique concret : il manipule des pointeurs
opaques. Les widgets, eux, sont fabriqués par le backend.

Fichiers du cœur : `gtkdialog.c` (orchestration + `main`), `safe_exec.c`,
`variables.c`, `stack.c`, `automaton.c`, `attributes.c`, `signals.c`,
`stringman.c`, `actions.c`, `tag_attributes.c`, `sermo_icon_theme.c`, plus la
grammaire Flex/Bison.

---

## 2. Les sept backends de rendu

Chaque backend construit les widgets natifs de sa boîte à outils et tient sa
boucle d'évènements. Un backend ne dépend **que** de sa propre toolkit.

| Backend | Binaire | Toolkit | Particularité |
|---|---|---|---|
| `sermo-backend-gtk3` | `gtk3sermo` | GTK 3 (C) | Port de référence (l'étalon). Widget `terminal` via VTE ; ancrage Wayland layer-shell. Cœur variante `SERMOCORE_GLIB`. |
| `sermo-backend-gtk4` | `gtk4sermo` | GTK 4 (C) | Cœur variante dédiée `SERMOCORE_GTK4` (GTK 4 a retiré `gtk_main`/`gtk_socket`/l'ancien modèle d'évènements). |
| `sermo-backend-qt6` | `qt6sermo` | Qt 6 (C++) | Widgets en C++. `QApplication` reçoit un argv filtré (sermo garde sa propre CLI). Cœur neutre. |
| `sermo-backend-fltk1` | `fltk1sermo` | FLTK 1.4 (C++) | Installation légère : ne tire ni GTK ni Qt. Cœur neutre. |
| `sermo-backend-efl1` | `efl1sermo` | EFL / Elementary (C) | **Thème sombre par défaut** (sinon suit le thème système). `default-width/height` traités comme minimums. Cœur neutre. |
| `sermo-backend-sdl3` | `sdl3sermo` | SDL 3 + Dear ImGui (C) | Rendu immédiat via Dear ImGui (vendorisé). Cœur neutre. |
| `sermo-backend-ncurses` | `ncursessermo` | ncursesw — **terminal** (C) | Le plus léger. Rendu en mode texte : **images et icônes occultées** (pas de rendu graphique). Cœur neutre. |

**Familles.** gtk3 et gtk4 sont des familles GObject (introspection GLib). Les
cinq autres (qt6, fltk1, efl1, sdl3, ncurses) sont **neutres** : ils
réimplémentent le pont d'attributs sans GObject. Le parsing numérique des backends neutres est
forcé en **locale-C** (`g_ascii_strtod` réel via `strtod_l`) pour ne pas dépendre
de la virgule décimale locale.

---

## 3. Le contrat cœur ↔ backend (`sermo_be_*`)

La frontière est déclarée dans `sermo_backend.h`, installé avec la lib. Le cœur
**appelle**, le backend **fournit**. Le cœur ne nomme **aucun** toolkit.

- **`sermo_backend_toolkit_init(int *argc, char ***argv, int print_ir)`** — le
  backend initialise sa toolkit (`gtk_init`, `QApplication`, `SDL_Init`…), sans
  ouvrir d'affichage en mode `--print-ir`.
- **`widget_*_create(...)`** — un par type de widget ; renvoie le widget natif
  sous forme de pointeur opaque.
- **Le pont d'opérations** (backends neutres) — un petit jeu de fonctions que le
  backend implémente : montrer, cacher, sensibilité, ajout au conteneur,
  déplacement, et la **boucle d'évènements**.
- **`main` vit dans le cœur** (`gtkdialog.c`) et rappelle
  `sermo_backend_toolkit_init` là où l'ancien code appelait `gtk_init`.

En sens inverse, le backend rappelle `execute_action()` du cœur quand un
évènement survient (clic, timer).

La boucle d'évènements côté backend est exposée par **`sermo_be_run_loop`** : le
cœur lance la boucle sans savoir de quel toolkit il s'agit.

⚠️ Certains ponts portent encore un préfixe historique `qt6_*` ; leur
généralisation en `sermo_be_*` est un chantier ouvert (Part 3).

---

## 4. Les trois variantes de build du cœur

Les familles de toolkits n'ont pas la même ABI. Le cœur se compile donc en trois
variantes, depuis une source largement unifiée (option CMake) :

| Variante | Option CMake | Sert | Détail |
|---|---|---|---|
| **neutre** (défaut) | — | qt6, fltk1, efl1, sdl3, ncurses | Types via *shim*, en-têtes toolkit remplacés par des stubs vides. Aucune dépendance graphique. |
| **glib / gtk3** | `-DSERMOCORE_GLIB=ON` | gtk3 | Vraie GLib/GTK 3 (ABI native des structs), pour l'introspection GObject de la famille GTK. |
| **gtk4** | `-DSERMOCORE_GTK4=ON` | gtk4 | Fichiers cœur réécrits pour GTK 4 (plus de `gtk_main`/`gtk_socket`). |

Neuf fichiers cœur sur dix sont identiques entre neutre et gtk3. La variante gtk4
reste dédiée le temps de la Part 3 (inversion de la boucle d'évènements).

Le backend consomme la bonne variante **via pkg-config** (`sermocore.pc`), en
pointant `PKG_CONFIG_PATH` sur le préfixe d'install voulu (neutre / gtk3 / gtk4).

---

## 5. Packaging — 13 paquets .deb

sermo produit des paquets **séparés** : un cœur de développement, un backend par
toolkit, et un paquet de compatibilité.

| Paquet | Contenu | Dépend de |
|---|---|---|
| `sermo-core-dev` | `libsermocore.a` + en-têtes + `sermocore.pc` | rien (aucun champ `Depends`) |
| `sermo-backend-gtk3` | binaire `gtk3sermo` | sa toolkit GTK 3 |
| `sermo-backend-gtk4` | binaire `gtk4sermo` | sa toolkit GTK 4 |
| `sermo-backend-qt6` | binaire `qt6sermo` | sa toolkit Qt 6 |
| `sermo-backend-fltk1` | binaire `fltk1sermo` | sa toolkit FLTK |
| `sermo-backend-efl1` | binaire `efl1sermo` | Enlightenment / Elementary |
| `sermo-backend-sdl3` | binaire `sdl3sermo` | SDL 3 |
| `sermo-backend-ncurses` | binaire `ncursessermo` | ncursesw (terminal) |
| `sermo-gtkdialog` | lien `/usr/bin/gtkdialog → gtk3sermo` | `sermo-backend-gtk3` |
| `gtk3sermo` | vide — transition depuis la 1.x | `sermo-backend-gtk3` |
| `gtk4sermo` | vide — transition depuis la 1.x | `sermo-backend-gtk4` |
| `qt6sermo` | vide — transition depuis la 1.x | `sermo-backend-qt6` |
| `gtksermo` | vide — transition depuis la 1.x | `sermo-gtkdialog` |

Soit **13 paquets** : `sermo-core-dev` + 7 backends + `sermo-gtkdialog` + 4
paquets de transition depuis la 1.x, plus un `-dbgsym` par backend.

**Le cœur est lié statiquement** dans chaque backend (`.a`). `ldd` d'un backend ne
montre donc **aucun** `libsermocore` : le moteur est embarqué. Un backend ne
dépend **pas** de `sermo-core-dev`, qui est un paquet de **développement**
uniquement (pour compiler un nouveau backend). Les dépendances runtime sont
**dérivées** du binaire (`ldd` → `dpkg -S`), pas devinées.

**Alias `sermo`.** Chaque backend s'enregistre dans `update-alternatives` pour
`/usr/bin/sermo` (priorités gtk3=50, gtk4=45, qt6=40, fltk1=30, efl1=20, sdl3=10,
ncurses=5) : gtk3 est le défaut.

**Pont `gtkdialog`.** La commande `gtkdialog` (scripts historiques) vit dans le
paquet séparé `sermo-gtkdialog` : `Provides`/`Conflicts`/`Replaces: gtkdialog`
pour ne pas entrer en conflit de fichier avec un autre `/usr/bin/gtkdialog` déjà
présent.

---

## 6. Les bancs de test

Aucune étape n'avance avec un banc rouge. Trois portes gardent chaque backend
(binaire `<port>`) :

- **XML** — `tests/xml/run_tests.sh` : le XML parse en headless (`--print-ir`),
  sans affichage. Attendu : **55 PASS**.
- **Comportement** — `tests/comportement/` (aussi via `run_all.sh`) : les valeurs
  exportées (`VAR="valeur"`) doivent être **identiques** à celles de l'étalon
  `gtk3sermo`. Attendu : **53/53**. Chaque cas se ferme seul, par un `<timer>`
  déclenchant `exit:fin` ou (cas 41-42) par une barre de progression arrivée à 100 ; ncurses joue ce banc en mode **batch headless**.
- **Garde** — jeu de scripts `tests/garde_*.sh` : durcissement du binaire,
  échappement de la sortie (les valeurs à risque ne repassent pas par le shell),
  fonctions interdites, allowlist de commandes, absence d'en-tête de poste, etc.

`tests/run_examples.sh` rejoue en plus les exemples de démonstration. Le corpus
de comportement vit dans `sermo-backend-qt6/tests/comportement/cas/` ; le lanceur
générique `tests/comportement/run.sh <binaire>` le joue sur **n'importe quel**
backend, et `ci/bancs.sh` enchaîne tous les bancs sur les sept.

---

## Ce qu'il faut retenir pour écrire un dialogue

- Le **langage XML est unique** et identique sur les sept backends : un script
  correct rend partout, aux nuances de rendu près.
- Le **ncurses** est en terminal : pas d'image ni d'icône — prévoir un dialogue
  qui reste lisible en texte pur.
- L'**efl1** est sombre par défaut.
- Vérifier une syntaxe sans ouvrir de fenêtre : `--print-ir` (code de retour 0 =
  la syntaxe passe).

Contact : `devel@haplo-dialog.fr`.
