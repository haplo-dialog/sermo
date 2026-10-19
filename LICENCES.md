# Licences — sermo

[English](LICENCES.en.md)

## Licence du projet

**GPL-2.0-or-later** — sermo descend de **gtkdialog**, placé sous
GNU General Public License version 2 « ou toute version ultérieure ». Le cœur
`libsermocore`, les neuf backends, `sermoman-mcp`, `sermoia-mcp`, les
programmes d'aide de `sermo-source/` et la documentation (manuels, pages de manuel, Texinfo) sont sous cette même licence.
Aucune exception propre au projet.

Les fichiers C et C++ portent un identifiant SPDX :

```
/* SPDX-License-Identifier: GPL-2.0-or-later */
```

Un fichier sans identifiant (script, page de manuel, fichier de construction)
relève de la licence du projet, sauf s'il figure dans le tableau ci-dessous.

## Fichiers sous une autre licence

| Chemin | Licence | Titulaires |
|---|---|---|
| `contract/` — `sermo-contract.h` (la frontière cœur ↔ backend) et `sermo_port_id.c` | MIT (texte dans l'en-tête et dans `contract/LICENSE.MIT`) | S. Cage |
| `sermo-backend-sdl3/src/imgui/` — **Dear ImGui 1.92.9 WIP**, compilé dans `sdl3sermo` | MIT (`imgui/LICENSE.txt`) | Omar Cornut |
| `imgui/imstb_rectpack.h`, `imstb_textedit.h`, `imstb_truetype.h` — bibliothèques stb modifiées par ImGui | MIT ou domaine public, au choix (fin de chaque fichier) | Sean Barrett |
| Polices ProggyClean et ProggyForever, compilées dans `imgui/imgui_draw.cpp` | MIT | Tristan Grimmer ; Disco Hello |
| Table des kanji Joyo et Jinmeiyo (points de code), compilée dans `imgui/imgui_draw.cpp` | CC-BY-4.0 (attribution dans le fichier) | Agence pour les affaires culturelles et ministère de la Justice du Japon (listes officielles) |
| `imgui/backends/imgui_impl_opengl3_loader.h` — chargeur OpenGL | domaine public et MIT (en-têtes Khronos) | The Khronos Group |
| `sermo-backend-lvgl/tiers/lvgl/` — **LVGL v9.6.0**, compilé dans `lvglsermo` (provenance : `tiers/lvgl/PROVENANCE`) | MIT (`tiers/lvgl/LICENCE.txt`) ; parties tierces : `tiers/lvgl/COPYRIGHTS.md` | LVGL Kft |
| `sermo-backend-lvgl/lv_conf.h` — réglage de LVGL, tiré du modèle d'origine | MIT | LVGL Kft ; S. Cage |
| `tiers/lvgl/src/libs/thorvg/` — **ThorVG**, moteur SVG embarqué dans LVGL, compilé dans `lvglsermo` depuis la 2.8.2 (`LV_USE_THORVG` dans `sermo-backend-lvgl/lv_conf.h`) ; pas dans `sermo-backend-lvgl-leger` | MIT (en-tête de chaque fichier, `thorvg/LICENSE.txt`) | le projet ThorVG ; Samsung Electronics |
| `thorvg/tvgSvgLoader.cpp`, `thorvg/tvgSvgPath.cpp` — l'analyseur SVG de ThorVG, repris en partie des EFL | MIT et BSD-2 (notice EFL dans le fichier) | le projet ThorVG ; les développeurs des EFL |
| `thorvg/tvgLottieInterpolator.cpp` — interpolation de courbes des animations Lottie | MIT et MPL-2.0 (pour une partie du fichier) | le projet ThorVG ; Mozilla Foundation |
| `thorvg/rapidjson/` — **rapidjson**, lecteur JSON des animations Lottie, compilé | MIT | THL A29 Limited (Tencent) et Milo Yip |
| `tiers/lvgl/src/libs/tiny_ttf/` — lecteur de police TrueType (stb_truetype), compilé dans `lvglsermo` | MIT ou Unlicense, au choix | Sean Barrett ; codewitch-honey-crisis (tiny_ttf) |
| Polices Montserrat et symboles Font Awesome 5 Free de LVGL (`tiers/lvgl/src/font/lv_font_montserrat_*.c`) ; seule `lv_font_montserrat_14.c` est compilée (police par défaut) | OFL-1.1 | Julieta Ulanovsky ; Fonticons, Inc. |
| `sermo-source/tiers/mavlink/` — en-têtes C de **MAVLink** (c_library_v2, provenance : `tiers/mavlink/LISEZMOI`), compilés dans `sermo-source-mavlink` et `sermo-cible-mavlink` | LGPL-3 avec l'exception MAVLink : sous licence MIT une fois intégrés à un binaire (`tiers/mavlink/COPYING.generateur`) | les auteurs de MAVLink |
| `sermo-source/tiers/mavlink/mavlink_sha256.h` — SHA-256 de Heimdal (signature des messages) | BSD-3 (texte en tête du fichier) | Kungliga Tekniska Högskolan (KTH) |
| `sermo-backend-qt6/data/fr.haplo_dialog.qt6sermo.metainfo.xml` — métadonnées AppStream (AppStream demande une licence permissive) | FSFAP | S. Cage |
| `tests/xml/*.xml`, `sermo-backend-qt6/examples/*/demo.sh`, et les exemples écrits pour sermo qui le déclarent dans leur en-tête (`examples/showcase/`, `examples/system-tools/`), les scripts métier (`examples/metiers/`), les boîtes à outils (`examples/boites-a-outils/`), les exemples Modbus et OPC UA (`examples/modbus/`, `examples/opcua/`), les scripts `examples/mavlink/drone-lecture.sh` et `drone-commande.sh`, les quinze exemples de la 3.1.0 (`examples/approbation/`, `bluetooth-scan/`, `bus-can/`, `camera-apercu/`, `certificat-tls/`, `dns-testeur/`, `domotique-mqtt/`, `gnss-moniteur/`, `heure-ntp/`, `journal-direct/`, `lecteur-media/`, `liaison-serie/`, `reseau-testeur/`, `usb-moniteur/`, `wifi-scan/`), et `contrib/trousses/` | CC0-1.0 | — |
| Icônes des exemples `button`, `togglebutton`, `pfeme` (héritées de gtkdialog) | GPL-2 ou LGPL-2.1 : voir le fichier `COPYING-*-icons` posé à côté | projets elementary, fast-forward, nuvola |

## Provenance — de quoi sermo descend exactement

sermo part de **gtkdialog 0.8.3** (`PACKAGE_VERSION='0.8.3'`, déclaré par le
paquet), écrit par **Pere László** et repris par **Thunor** à partir de 2011.
Cette archive, publiée le 6 janvier 2013, demande `gtk+-2.0` et **uniquement**
cela : elle n'a pas d'autre couche de rendu que GTK2 (le GTK 3 expérimental de
Thunor, ajouté ensuite dans svn, n'a jamais été publié en archive). L'histoire
complète de gtkdialog et de ses descendants est dans [HISTOIRE.md](HISTOIRE.md).

Ce point n'est pas anecdotique. Il détermine ce qui, dans ce dépôt, vient de
l'amont et ce qui a été écrit ici — parce qu'on ne transpose pas du code GTK2
vers Qt6, SDL3, EFL, FLTK ou un terminal : les appels n'existent pas.

Comparaison fichier par fichier contre l'archive d'amont, mesurée le 2026-10-10
sur la 2.8.0 (lignes significatives : ni vides, ni accolades, ni commentaires, ni
directives du préprocesseur ; « hérité » = au moins la moitié des lignes du
fichier se retrouvent à l'identique dans le fichier de même nom de gtkdialog
0.8.3). La mesure se rejoue : `tests/mesure_provenance.py`, qui donne aussi
l'empreinte de l'archive.

| Backend | Hérités | Réécrits | Neufs | Total |
|---|---|---|---|---|
| `gtk3` | 72 | 2 | 52 | 126 |
| `gtk4` | 64 | 4 | 61 | 129 |
| `qt6` | 9 | 63 | 60 | 132 |
| `sdl3` | 10 | 65 | 65 | 140 |
| `lvgl` | 9 | 64 | 63 | 136 |
| `ncurses` | 9 | 64 | 63 | 136 |
| `efl1` | 8 | 62 | 64 | 134 |
| `fltk1` | 8 | 63 | 67 | 138 |
| `html` | 9 | 64 | 65 | 138 |

- **`gtk3` et `gtk4` sont des portages** des widgets GTK2 de l'amont. Leurs
  en-têtes le disent : `(GTK3 port, security)`, `(GTK4 port)`.
- **`gtk3` a un SECOND amont.** L'ancrage Wayland (`layer-shell`) ne vient pas de
  gtkdialog 0.8.3, publié en janvier 2013 : il a été écrit en 2021 dans la
  lignée reprise par Puppy Linux (woof-CE), puis repris dans gtk3dialog
  (01micko), que BunsenLabs empaquette — c'est de là que sermo l'a porté. Une trentaine de lignes dans
  `sermo-backend-gtk3/src/widget_window.c` et les attributs XML `layer`, `edge`,
  `dist`, `reserve`. Copyrights conservés :
  **Dima Krasner** (2021) et **Mick Amadio** (2021-2024). La bibliothèque
  `gtk-layer-shell` elle-même est une dépendance système, pas du code embarqué.
  **Les huit autres ports n'ont pas cette fonction** et n'ont donc pas ce second
  amont.
- **Les sept autres portent une couche de rendu neuve.** Ce qu'ils héritent encore,
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

- `libsermocore` (cœur), `sermo-backend-*` (backends), `sermoman-mcp`, `sermoia-mcp`,
  `sermo-source` (programmes d'aide) : GPL-2.0-or-later, sauf les fichiers du
  tableau ci-dessus.
- Le lexer/parser générés (Flex/Bison) suivent la licence du projet ; les
  squelettes Flex/Bison bénéficient de leurs exceptions habituelles.

## Bibliothèques liées

Chaque backend se lie à sa boîte à outils (GTK, Qt, FLTK, EFL, SDL, ncurses ;
LVGL est compilé dans `lvglsermo`, html n'en a pas) et
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
- MIT : `contract/LICENSE.MIT`, `sermo-backend-sdl3/src/imgui/LICENSE.txt`,
  `sermo-backend-lvgl/tiers/lvgl/LICENCE.txt` et
  `sermo-backend-lvgl/tiers/lvgl/src/libs/thorvg/LICENSE.txt`.
- Exception MAVLink : `sermo-source/tiers/mavlink/COPYING.generateur`.
- MPL-2.0 : <https://www.mozilla.org/MPL/2.0/>.
- Le texte complet de chaque licence, fichier par fichier, est aussi dans
  `debian/copyright`.
- OFL-1.1 : <https://openfontlicense.org>.
- CC BY 4.0 : <https://creativecommons.org/licenses/by/4.0/legalcode>.
