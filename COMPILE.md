# Compilation depuis les sources — sermo

[English](COMPILE.en.md)

sermo se compile en **deux temps** : d'abord le cœur `libsermocore` (dans la
variante qui convient à votre backend), puis le backend lui-même contre ce cœur.

## 1. Dépendances de compilation

Communes (le cœur) : `flex`, `bison`, `cmake` (≥ 3.16), `gcc`, `pkg-config`.
Les ports qt6, fltk1, sdl3 et lvgl demandent aussi un compilateur C++ (`g++`) :
lvgl depuis la 2.8.2, pour ThorVG (le SVG de LVGL).

Par backend, les `-dev` de sa toolkit — voir [DEPENDENCIES.md](DEPENDENCIES.md).
Exemples : `libgtk-3-dev libvte-2.91-dev libgtk-layer-shell-dev` (gtk3),
`libgtk-4-dev libvte-2.91-gtk4-dev` (gtk4), `qt6-base-dev` (qt6),
`libfltk1.4-dev` (fltk1), `libefl-all-dev` (efl1), `libsdl3-dev` (sdl3),
`libx11-dev libpng-dev libwayland-dev libxkbcommon-dev wayland-protocols` (lvgl,
dont la bibliothèque LVGL est embarquée), `libncurses-dev` (ncurses) ; html n'en
demande aucune. Sans VTE, les variantes GTK du cœur refusent de se
construire (le widget `<terminal>` manquerait sans bruit) ; `-DSERMOCORE_VTE=OFF`
l'accepte explicitement. De même pour l'ancrage Wayland de gtk3 :
`-DSERMO_LAYER_SHELL=OFF`.

```bash
sudo apt install flex bison cmake gcc pkg-config
```

Liste complète, pour construire les neuf backends **et** jouer tous les bancs,
telle que la CI l'installe : [`ci/dependances.txt`](ci/dependances.txt).

## 2. Compiler le cœur

Choisissez **une** variante selon le backend visé (voir
[MANUEL_DEVELOPPEUR.md](MANUEL_DEVELOPPEUR.md) §2) :

```bash
cd libsermocore

# Variante neutre (qt6 / fltk1 / efl1 / sdl3 / lvgl / ncurses / html) — défaut, sans toolkit
cmake -S . -B _build -DCMAKE_INSTALL_PREFIX=$PWD/_install
make -C _build && make -C _build install

# Variante GTK 3
cmake -S . -B _build_gtk -DSERMOCORE_GLIB=ON -DCMAKE_INSTALL_PREFIX=$PWD/_install_gtk
make -C _build_gtk && make -C _build_gtk install

# Variante GTK 4
cmake -S . -B _build_gtk4 -DSERMOCORE_GTK4=ON -DCMAKE_INSTALL_PREFIX=$PWD/_install_gtk4
make -C _build_gtk4 && make -C _build_gtk4 install
```

## 3. Compiler un backend

Le backend trouve le cœur par pkg-config. Pointez `PKG_CONFIG_PATH` sur le
préfixe d'install de la **bonne variante** :

```bash
# Exemple : qt6 (variante neutre)
export PKG_CONFIG_PATH=$PWD/libsermocore/_install/lib/pkgconfig
cmake -S sermo-backend-qt6 -B sermo-backend-qt6/_build
make -C sermo-backend-qt6/_build
# → binaire : sermo-backend-qt6/_build/qt6sermo

# Exemple : gtk3 (variante GLib)
export PKG_CONFIG_PATH=$PWD/libsermocore/_install_gtk/lib/pkgconfig
cmake -S sermo-backend-gtk3 -B sermo-backend-gtk3/_build
make -C sermo-backend-gtk3/_build

# Exemple : gtk4 (variante GTK4)
export PKG_CONFIG_PATH=$PWD/libsermocore/_install_gtk4/lib/pkgconfig
cmake -S sermo-backend-gtk4 -B sermo-backend-gtk4/_build
make -C sermo-backend-gtk4/_build
```

Correspondance variante ↔ backend :

| Backend | Variante du cœur | `PKG_CONFIG_PATH` |
|---|---|---|
| qt6, fltk1, efl1, sdl3, lvgl, ncurses, html | neutre | `_install` |
| gtk3 | `SERMOCORE_GLIB` | `_install_gtk` |
| gtk4 | `SERMOCORE_GTK4` | `_install_gtk4` |

### Installer le backend

Chaque backend a ses règles d'installation : le binaire, sa page de manuel
substituée à la bonne version, et l'icône d'application aux huit tailles du
thème hicolor. gtk3 et gtk4 posent en plus leur page de section 5 (le format
XML) ; ncurses n'a pas d'icône — un terminal n'en porte pas —, et lvgl et html
n'en livrent pas non plus.

```bash
cmake -S sermo-backend-gtk3 -B sermo-backend-gtk3/_build \
      -DCMAKE_INSTALL_PREFIX=$PWD/_install_gtk3
make -C sermo-backend-gtk3/_build && make -C sermo-backend-gtk3/_build install
```

Les ports sans page propre (fltk1, efl1, sdl3, lvgl, ncurses, html) reçoivent la page
générique `doc/sermo.1.in` sous leur propre nom, exactement comme le fait
`debian/rules` : les deux chemins livrent la même chose.

### Programmes d'aide (`sermo-source/`)

`<input modbus>`, `<input opcua>`, `<input mqtt>`, `<input mavlink>`,
`<input media>` et `<action mqtt>`, `<action mavlink>`, `<action media>`
passent par huit petits programmes séparés. Ils se construisent à part, sans
le cœur :

```bash
cmake -S sermo-source -B sermo-source/_build
cmake --build sermo-source/_build
# → sermo-source/_build/sermo-source-modbus, … , sermo-cible-media
```

Chacun n'est construit que si sa bibliothèque est là : `libmodbus-dev`
(`sermo-source-modbus`), `libopen62541-1.4-dev` (`sermo-source-opcua`),
`libmosquitto-dev` (`sermo-source-mqtt` et `sermo-cible-mqtt`). Sinon CMake
prévient et saute ce programme. Avec `-DSERMO_SOURCE_EXIGER=ON`, une
bibliothèque absente arrête la construction : la CI et les paquets le posent.
MAVLink n'a rien à installer : ses en-têtes C sont embarqués
(`sermo-source/tiers/mavlink`). Les deux programmes media n'ont pas de
bibliothèque : à l'exécution, `sermo-cible-media` lance le programme `mpv`
(dans une cage `bwrap` quand elle est là), et `sermo-source-media` l'interroge.

`cmake --install` les pose dans `<préfixe>/libexec/sermo`. Le cœur les cherche
dans `/usr/libexec/sermo` ; la variable `SERMO_SOURCE_DIR` (un chemin absolu)
désigne un autre dossier, par exemple `sermo-source/_build` pour un essai.

## 4. Durcissement

Le cœur et les backends compilent avec `-D_FORTIFY_SOURCE=3`,
`-fstack-protector-strong`, `-fstack-clash-protection` et une protection du flot
de contrôle, et lient en PIE/RELRO/BIND_NOW. La protection du flot de contrôle est
choisie par CMake après essai : `-fcf-protection=full` (CET) sur x86,
`-mbranch-protection=standard` (BTI/PAC) sur ARM64 ; `-fstack-clash-protection`
n'est gardé que si le compilateur s'en sert. Ne retirez pas ces drapeaux : le banc
`tests/garde_durcissement.sh` les vérifie. Voir [SECURITY.md](SECURITY.md).

Sans `-DCMAKE_BUILD_TYPE`, chaque projet se construit en `Release` : c'est ce qui
rend `_FORTIFY_SOURCE` effectif (il n'agit qu'avec l'optimisation). Un build
`Debug` perd cette protection. Pour le cœur :

```bash
bash tests/garde_fortify_coeur.sh libsermocore/_build/libsermocore.a
```

## 5. Vérifier

Faites passer **les trois portes** (voir [MANUEL_DEVELOPPEUR.md](MANUEL_DEVELOPPEUR.md) §6) :

```bash
BIN=sermo-backend-qt6/_build/qt6sermo
TIMEOUT=5 bash tests/xml/run_tests.sh "$BIN"                       # 55 PASS
bash tests/comportement/run.sh "$BIN"                             # 78 au vert
xvfb-run -a bash tests/garde_echappement_sortie.sh "$BIN"          # OK — 4 cas
```

Ou tout construire et tout vérifier, exactement comme la CI :

```bash
bash ci/construire.sh && bash ci/bancs.sh
```

## 6. Paquets `.deb`

```bash
bash packaging/construire-paquets.sh    # dans une Debian testing vierge (Docker) → packaging/sortie/
```

Ou directement sur une Debian testing : `sudo apt build-dep ./` puis
`dpkg-buildpackage -us -uc -b`. Vingt-sept paquets : `sermo-core-dev`, les dix
`sermo-backend-*` (dont `sermo-backend-lvgl-leger`), `sermo-doc`,
`sermo-gtkdialog`, les deux serveurs MCP (`sermoia-mcp`, `sermoman-mcp`), quatre
paquets de transition depuis la 1.x, et les huit programmes d'aide
(`sermo-source-modbus`, `-opcua`, `-mqtt`, `-mavlink`, `-media` ;
`sermo-cible-mqtt`, `-mavlink`, `-media`). Voir [PACKAGING.md](PACKAGING.md).

## 7. Autres systèmes (port ncurses)

Hors de Linux, seul le port ncurses est construit. Un même script sert partout,
`packaging/autres-os/construire-os.sh` : le cœur, puis le port. CMake essaie ce
que le système fournit et choisit sans rien demander : `funopen` quand la
bibliothèque C n'a pas `fopencookie`, un repli quand `strtod_l`, `pipe2` ou
`execvpe` manquent, la bonne protection du flot de contrôle.

**Android (Termux)**, dans Termux :

```bash
pkg install clang cmake make flex bison pkg-config ncurses
CMAKE_EXTRA="-DSERMO_SHELL_DEFAUT=$PREFIX/bin/sh" \
    sh packaging/autres-os/construire-os.sh construire . _os ncurses
```

`SERMO_SHELL_DEFAUT` : le `/bin/sh` d'Android n'est pas le shell de Termux. La recette
`packaging/autres-os/termux/build.sh` fait la même chose au format termux-packages.

**FreeBSD** et **OpenBSD** (curses du système de base) :

```bash
pkg install cmake bison pkgconf gmake          # FreeBSD
pkg_add cmake bison gmake                      # OpenBSD
sh packaging/autres-os/construire-os.sh construire . _os ncurses
```

Les ports `packaging/autres-os/freebsd/devel/sermo/` et `openbsd/devel/sermo/`
appellent ce même script.

**macOS** (ncurses, bison et flex de Homebrew, ceux de macOS sont trop anciens) :

```bash
brew install cmake bison flex ncurses pkgconf
export PATH="$(brew --prefix bison)/bin:$(brew --prefix flex)/bin:$PATH"
export PKG_CONFIG_PATH="$(brew --prefix ncurses)/lib/pkgconfig"
sh packaging/autres-os/construire-os.sh construire . _os ncurses
```

La formule `packaging/autres-os/homebrew/sermo.rb` fait la même chose ; aucun tap
n'est publié.

**Windows** : compilation croisée depuis Debian (`gcc-mingw-w64-ucrt64`), avec
le fichier de chaîne `packaging/autres-os/windows/mingw-w64.cmake` et un ncurses
construit pour la même cible. Les commandes et les réglages de ncurses sont dans
[packaging/autres-os/README.md](packaging/autres-os/README.md), section Windows.

Dans tous les cas, le binaire est `_os/ncurses/ncursessermo`, et
`bash tests/comportement/run.sh "$PWD/_os/ncurses/ncursessermo"` le vérifie.
