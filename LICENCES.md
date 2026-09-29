# Licences — sermo

## Licence du projet

**GPL-2.0-or-later** — sermo descend de **gtkdialog**, placé sous
GNU General Public License version 2 « ou toute version ultérieure ». Le cœur
`libsermocore`, les sept backends, `sermoman-mcp` et la documentation (manuels,
pages de manuel, Texinfo) sont sous cette même licence. Aucune exception propre
au projet.

Les fichiers C et C++ portent un identifiant SPDX :

```
/* SPDX-License-Identifier: GPL-2.0-or-later */
```

Un fichier sans identifiant (script, page de manuel, fichier de construction)
relève de la licence du projet, sauf s'il figure dans le tableau ci-dessous.

## Fichiers sous une autre licence

| Chemin | Licence | Titulaires |
|---|---|---|
| `contract/sermo-contract.h` — la frontière cœur ↔ backend | MIT (texte dans l'en-tête et dans `contract/LICENSE.MIT`) | S. Cage |
| `sermo-backend-sdl3/src/imgui/` — **Dear ImGui 1.92.9 WIP**, compilé dans `sdl3sermo` | MIT (`imgui/LICENSE.txt`) | Omar Cornut |
| `imgui/imstb_rectpack.h`, `imstb_textedit.h`, `imstb_truetype.h` — bibliothèques stb modifiées par ImGui | MIT ou domaine public, au choix (fin de chaque fichier) | Sean Barrett |
| Polices ProggyClean et ProggyForever, compilées dans `imgui/imgui_draw.cpp` | MIT | Tristan Grimmer ; Disco Hello |
| Table des kanji Joyo et Jinmeiyo (points de code), compilée dans `imgui/imgui_draw.cpp` | CC-BY-4.0 (attribution dans le fichier) | Agence pour les affaires culturelles et ministère de la Justice du Japon (listes officielles) |
| `imgui/backends/imgui_impl_opengl3_loader.h` — chargeur OpenGL | domaine public et MIT (en-têtes Khronos) | The Khronos Group |
| `sermo-backend-qt6/data/fr.haplo_dialog.qt6sermo.metainfo.xml` — métadonnées AppStream (AppStream demande une licence permissive) | FSFAP | S. Cage |
| `tests/xml/*.xml`, `sermo-backend-qt6/examples/*/demo.sh`, et les exemples écrits pour sermo qui le déclarent dans leur en-tête (`examples/showcase/`, `examples/system-tools/`) | CC0-1.0 | — |
| Icônes des exemples `button`, `togglebutton`, `pfeme` (héritées de gtkdialog) | GPL-2 ou LGPL-2.1 : voir le fichier `COPYING-*-icons` posé à côté | projets elementary, fast-forward, nuvola |

## Provenance — de quoi sermo descend exactement

sermo part de **gtkdialog 0.8.3** (`PACKAGE_VERSION='0.8.3'`, déclaré par le
paquet), écrit par **Pere László** et repris par **Thunor** à partir de 2011.
L'amont demande `gtk+-2.0` et **uniquement** cela : il n'a jamais eu d'autre
couche de rendu que GTK2.

Ce point n'est pas anecdotique. Il détermine ce qui, dans ce dépôt, vient de
l'amont et ce qui a été écrit ici — parce qu'on ne transpose pas du code GTK2
vers Qt6, SDL3, EFL, FLTK ou un terminal : les appels n'existent pas.

Comparaison fichier par fichier contre l'archive d'amont (lignes significatives :
ni vides, ni accolades, ni commentaires, ni `#include` ; « hérité » = au moins la
moitié des lignes du fichier se retrouvent à l'identique dans gtkdialog 0.8.3) :

| Backend | Hérités | Réécrits | Neufs | Total |
|---|---|---|---|---|
| `gtk3` | 72 | 2 | 34 | 108 |
| `gtk4` | 64 | 4 | 50 | 118 |
| `qt6` | 9 | 63 | 42 | 114 |
| `sdl3` | 10 | 64 | 45 | 119 |
| `ncurses` | 9 | 63 | 42 | 114 |
| `efl1` | 8 | 61 | 43 | 112 |
| `fltk1` | 8 | 63 | 46 | 117 |

- **`gtk3` et `gtk4` sont des portages** des widgets GTK2 de l'amont. Leurs
  en-têtes le disent : `(GTK3 port, security)`, `(GTK4 port)`.
- **`gtk3` a un SECOND amont.** L'ancrage Wayland (`layer-shell`) ne vient pas de
  gtkdialog 0.8.3, qui date de 2012 : il vient de la lignée reprise plus tard
  (BunsenLabs / Puppy Linux). Une trentaine de lignes dans
  `sermo-backend-gtk3/src/widget_window.c` et les attributs XML `layer`, `edge`,
  `dist`, `reserve`. Copyrights conservés :
  **Dima Krasner** (2021) et **Mick Amadio** (2021-2024). La bibliothèque
  `gtk-layer-shell` elle-même est une dépendance système, pas du code embarqué.
  **Les six autres ports n'ont pas cette fonction** et n'ont donc pas ce second
  amont.
- **Les cinq autres portent une couche de rendu neuve.** Ce qu'ils héritent encore,
  ce sont les en-têtes du cœur — `actions.h`, `attributes.h`, `automaton.h`,
  `signals.h`, `stack.h`, `stringman.h`, `tag_attributes.h`, `variables.h`,
  `widgets.h`, `macros.h`, `gtkdialog.h`.
- **Le cœur** garde l'analyseur de l'amont : le lexer (`gtkdialog_lexer.l`) et le
  parser (`gtkdialog_parser.y`) en conservent la notice (Pere László 2003-2007,
  Thunor 2011-2012) et une large part des lignes.
- **`examples/`** : 188 des 213 fichiers existent dans gtkdialog 0.8.3 (73 à
  l'identique). Ils restent sous la licence de l'amont. Les exemples écrits pour
  sermo le déclarent dans leur en-tête.
- `qt6` conservait en plus neuf sources du moteur d'amont dans son `src/` — jamais
  compilées (le cœur vient de `libsermocore`), donc du poids mort. Retirées le
  2026-09-12 : il rejoint le niveau de ses voisins.

Le copyright d'origine est conservé dans chaque fichier concerné. Rien n'est
retiré, rien n'est réattribué : la GPL exige que l'attribution survive, et c'est
la moindre des choses envers un travail sur lequel celui-ci s'appuie.

## Portée

- `libsermocore` (cœur), `sermo-backend-*` (backends), `sermoman-mcp` :
  GPL-2.0-or-later, sauf les fichiers du tableau ci-dessus.
- Le lexer/parser générés (Flex/Bison) suivent la licence du projet ; les
  squelettes Flex/Bison bénéficient de leurs exceptions habituelles.

## Bibliothèques liées

Chaque backend se lie à sa boîte à outils (GTK, Qt, FLTK, EFL, SDL, ncurses) et
aux dépendances de celle-ci, sous leurs propres licences : LGPL, MIT/X11, BSD,
zlib… Qt 6 est proposé sous LGPL-3 ou GPL-2.

Certaines bibliothèques chargées indirectement sont sous LGPL-3 ou Apache-2.0
(par exemple par l'intermédiaire de GTK ou d'EFL). Un programme combiné avec ce
code ne peut circuler que sous GPL-3 : c'est possible parce que sermo est sous
GPL-2 « ou toute version ultérieure ». Cette lecture suit la table de
compatibilité usuelle de la FSF ; ce n'est pas un avis juridique.

## Textes complets

- GPL-2 : fichier `COPYING` à la racine du dépôt, ou
  <https://www.gnu.org/licenses/old-licenses/gpl-2.0.html>.
- MIT : `contract/LICENSE.MIT` et `sermo-backend-sdl3/src/imgui/LICENSE.txt`.
- CC BY 4.0 : <https://creativecommons.org/licenses/by/4.0/legalcode>.
