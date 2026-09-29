# Compilation depuis les sources — sermo

[English](COMPILE.en.md)

sermo se compile en **deux temps** : d'abord le cœur `libsermocore` (dans la
variante qui convient à votre backend), puis le backend lui-même contre ce cœur.

## 1. Dépendances de compilation

Communes (le cœur) : `flex`, `bison`, `cmake` (≥ 3.16), `gcc`, `pkg-config`.

Par backend, les `-dev` de sa toolkit — voir [DEPENDENCIES.md](DEPENDENCIES.md).
Exemples : `libgtk-3-dev libvte-2.91-dev libgtk-layer-shell-dev` (gtk3),
`libgtk-4-dev libvte-2.91-gtk4-dev` (gtk4), `qt6-base-dev` (qt6),
`libfltk1.4-dev` (fltk1), `libefl-all-dev` (efl1), `libsdl3-dev` (sdl3),
`libncurses-dev` (ncurses). Sans VTE, les variantes GTK du cœur refusent de se
construire (le widget `<terminal>` manquerait sans bruit) ; `-DSERMOCORE_VTE=OFF`
l'accepte explicitement. De même pour l'ancrage Wayland de gtk3 :
`-DSERMO_LAYER_SHELL=OFF`.

```bash
sudo apt install flex bison cmake gcc pkg-config
```

Liste complète, pour construire les sept backends **et** jouer tous les bancs,
telle que la CI l'installe : [`ci/dependances.txt`](ci/dependances.txt).

## 2. Compiler le cœur

Choisissez **une** variante selon le backend visé (voir
[MANUEL_DEVELOPPEUR.md](MANUEL_DEVELOPPEUR.md) §2) :

```bash
cd libsermocore

# Variante neutre (qt6 / fltk1 / efl1 / sdl3 / ncurses) — défaut, sans toolkit
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
| qt6, fltk1, efl1, sdl3, ncurses | neutre | `_install` |
| gtk3 | `SERMOCORE_GLIB` | `_install_gtk` |
| gtk4 | `SERMOCORE_GTK4` | `_install_gtk4` |

### Installer le backend

Chaque backend a ses règles d'installation : le binaire, sa page de manuel
substituée à la bonne version, et l'icône d'application aux huit tailles du
thème hicolor. gtk3 et gtk4 posent en plus leur page de section 5 (le format
XML) ; ncurses n'a pas d'icône — un terminal n'en porte pas.

```bash
cmake -S sermo-backend-gtk3 -B sermo-backend-gtk3/_build \
      -DCMAKE_INSTALL_PREFIX=$PWD/_install_gtk3
make -C sermo-backend-gtk3/_build && make -C sermo-backend-gtk3/_build install
```

Les ports sans page propre (fltk1, efl1, sdl3, ncurses) reçoivent la page
générique `doc/sermo.1.in` sous leur propre nom, exactement comme le fait
`debian/rules` : les deux chemins livrent la même chose.

## 4. Durcissement

Le cœur et les backends compilent avec `-D_FORTIFY_SOURCE=3`,
`-fstack-protector-strong`, `-fstack-clash-protection`, `-fcf-protection=full`,
et lient en PIE/RELRO/BIND_NOW. Ne retirez pas ces drapeaux : le banc
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
bash tests/comportement/run.sh "$BIN"                             # 53 au vert
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
`dpkg-buildpackage -us -uc -b`. Treize paquets : `sermo-core-dev`, les sept
`sermo-backend-*`, `sermo-gtkdialog` et quatre paquets de transition depuis la
1.x. Voir [PACKAGING.md](PACKAGING.md).
