# Compiling from source — sermo

[Français](COMPILE.md)

sermo is compiled in **two steps**: first the `libsermocore` core (in the variant
that suits your backend), then the backend itself against that core.

## 1. Build dependencies

Shared (the core): `flex`, `bison`, `cmake` (≥ 3.16), `gcc`, `pkg-config`.

Per backend, the `-dev` packages of its toolkit — see
[DEPENDENCIES.en.md](DEPENDENCIES.en.md). Examples:
`libgtk-3-dev libvte-2.91-dev libgtk-layer-shell-dev` (gtk3),
`libgtk-4-dev libvte-2.91-gtk4-dev` (gtk4), `qt6-base-dev` (qt6),
`libfltk1.4-dev` (fltk1), `libefl-all-dev` (efl1), `libsdl3-dev` (sdl3),
`libncurses-dev` (ncurses). Without VTE, the GTK variants of the core refuse to
build (the `<terminal>` widget would be missing without a word);
`-DSERMOCORE_VTE=OFF` accepts that explicitly. Likewise for gtk3's Wayland
anchoring: `-DSERMO_LAYER_SHELL=OFF`.

```bash
sudo apt install flex bison cmake gcc pkg-config
```

The complete list, to build the seven backends **and** run every bench, as the
CI installs it: [`ci/dependances.txt`](ci/dependances.txt).

## 2. Compiling the core

Pick **one** variant for the target backend (see
[MANUEL_DEVELOPPEUR.en.md](MANUEL_DEVELOPPEUR.en.md) §2, French):

```bash
cd libsermocore

# Neutral variant (qt6 / fltk1 / efl1 / sdl3 / ncurses) — default, no toolkit
cmake -S . -B _build -DCMAKE_INSTALL_PREFIX=$PWD/_install
make -C _build && make -C _build install

# GTK 3 variant
cmake -S . -B _build_gtk -DSERMOCORE_GLIB=ON -DCMAKE_INSTALL_PREFIX=$PWD/_install_gtk
make -C _build_gtk && make -C _build_gtk install

# GTK 4 variant
cmake -S . -B _build_gtk4 -DSERMOCORE_GTK4=ON -DCMAKE_INSTALL_PREFIX=$PWD/_install_gtk4
make -C _build_gtk4 && make -C _build_gtk4 install
```

## 3. Compiling a backend

The backend finds the core through pkg-config. Point `PKG_CONFIG_PATH` at the
install prefix of the **right variant**:

```bash
# Example: qt6 (neutral variant)
export PKG_CONFIG_PATH=$PWD/libsermocore/_install/lib/pkgconfig
cmake -S sermo-backend-qt6 -B sermo-backend-qt6/_build
make -C sermo-backend-qt6/_build
# → binary: sermo-backend-qt6/_build/qt6sermo

# Example: gtk3 (GLib variant)
export PKG_CONFIG_PATH=$PWD/libsermocore/_install_gtk/lib/pkgconfig
cmake -S sermo-backend-gtk3 -B sermo-backend-gtk3/_build
make -C sermo-backend-gtk3/_build

# Example: gtk4 (GTK 4 variant)
export PKG_CONFIG_PATH=$PWD/libsermocore/_install_gtk4/lib/pkgconfig
cmake -S sermo-backend-gtk4 -B sermo-backend-gtk4/_build
make -C sermo-backend-gtk4/_build
```

Variant ↔ backend mapping:

| Backend | Core variant | `PKG_CONFIG_PATH` |
|---|---|---|
| qt6, fltk1, efl1, sdl3, ncurses | neutral | `_install` |
| gtk3 | `SERMOCORE_GLIB` | `_install_gtk` |
| gtk4 | `SERMOCORE_GTK4` | `_install_gtk4` |

### Installing the backend

Each backend now carries its own install rules: the binary, its manual page
substituted with the right version, and the application icon at all eight
hicolor theme sizes. gtk3 and gtk4 also install their section 5 page (the XML
format); ncurses has no icon — a terminal cannot carry one.

```bash
cmake -S sermo-backend-gtk3 -B sermo-backend-gtk3/_build \
      -DCMAKE_INSTALL_PREFIX=$PWD/_install_gtk3
make -C sermo-backend-gtk3/_build && make -C sermo-backend-gtk3/_build install
```

Ports without a page of their own (fltk1, efl1, sdl3, ncurses) get the generic
`doc/sermo.1.in` under their own name, exactly as `debian/rules` does: both
paths ship the same thing.

## 4. Hardening

The core and the backends compile with `-D_FORTIFY_SOURCE=3`,
`-fstack-protector-strong`, `-fstack-clash-protection`, `-fcf-protection=full`,
and link as PIE/RELRO/BIND_NOW. Do not remove these flags: the
`tests/garde_durcissement.sh` bench checks them. See
[SECURITY.en.md](SECURITY.en.md).

Without `-DCMAKE_BUILD_TYPE`, every project builds as `Release`: that is what
makes `_FORTIFY_SOURCE` effective (it only works with optimisation). A `Debug`
build loses this protection. For the core:

```bash
bash tests/garde_fortify_coeur.sh libsermocore/_build/libsermocore.a
```

## 5. Checking

Pass **the three gates** (see [MANUEL_DEVELOPPEUR.en.md](MANUEL_DEVELOPPEUR.en.md) §6,
French):

```bash
BIN=sermo-backend-qt6/_build/qt6sermo
TIMEOUT=5 bash tests/xml/run_tests.sh "$BIN"                       # 55 PASS
bash tests/comportement/run.sh "$BIN"                             # 53 green
xvfb-run -a bash tests/garde_echappement_sortie.sh "$BIN"          # OK — 4 cases
```

Or build everything and check everything, exactly like the CI:

```bash
bash ci/construire.sh && bash ci/bancs.sh
```

## 6. `.deb` packages

```bash
bash packaging/construire-paquets.sh    # in a clean Debian testing system (Docker) → packaging/sortie/
```

Or directly on Debian testing: `sudo apt build-dep ./` then
`dpkg-buildpackage -us -uc -b`. Thirteen packages: `sermo-core-dev`, the seven
`sermo-backend-*`, `sermo-gtkdialog` and four transitional packages from 1.x. See
[PACKAGING.en.md](PACKAGING.en.md).
