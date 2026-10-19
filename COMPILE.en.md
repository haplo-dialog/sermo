# Compiling from source — sermo

[Français](COMPILE.md)

sermo is compiled in **two steps**: first the `libsermocore` core (in the variant
that suits your backend), then the backend itself against that core.

## 1. Build dependencies

Shared (the core): `flex`, `bison`, `cmake` (≥ 3.16), `gcc`, `pkg-config`.
The qt6, fltk1, sdl3 and lvgl ports also need a C++ compiler (`g++`): lvgl
since 2.8.2, for ThorVG (LVGL's SVG support).

Per backend, the `-dev` packages of its toolkit — see
[DEPENDENCIES.en.md](DEPENDENCIES.en.md). Examples:
`libgtk-3-dev libvte-2.91-dev libgtk-layer-shell-dev` (gtk3),
`libgtk-4-dev libvte-2.91-gtk4-dev` (gtk4), `qt6-base-dev` (qt6),
`libfltk1.4-dev` (fltk1), `libefl-all-dev` (efl1), `libsdl3-dev` (sdl3),
`libx11-dev libpng-dev libwayland-dev libxkbcommon-dev wayland-protocols` (lvgl,
whose LVGL library is embedded), `libncurses-dev` (ncurses); html needs none.
Without VTE, the GTK variants of the core refuse to
build (the `<terminal>` widget would be missing without a word);
`-DSERMOCORE_VTE=OFF` accepts that explicitly. Likewise for gtk3's Wayland
anchoring: `-DSERMO_LAYER_SHELL=OFF`.

```bash
sudo apt install flex bison cmake gcc pkg-config
```

The complete list, to build the nine backends **and** run every bench, as the
CI installs it: [`ci/dependances.txt`](ci/dependances.txt).

## 2. Compiling the core

Pick **one** variant for the target backend (see
[MANUEL_DEVELOPPEUR.en.md](MANUEL_DEVELOPPEUR.en.md) §2, French):

```bash
cd libsermocore

# Neutral variant (qt6 / fltk1 / efl1 / sdl3 / lvgl / ncurses / html) — default, no toolkit
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
| qt6, fltk1, efl1, sdl3, lvgl, ncurses, html | neutral | `_install` |
| gtk3 | `SERMOCORE_GLIB` | `_install_gtk` |
| gtk4 | `SERMOCORE_GTK4` | `_install_gtk4` |

### Installing the backend

Each backend now carries its own install rules: the binary, its manual page
substituted with the right version, and the application icon at all eight
hicolor theme sizes. gtk3 and gtk4 also install their section 5 page (the XML
format); ncurses has no icon — a terminal cannot carry one — and lvgl and html
do not ship one either.

```bash
cmake -S sermo-backend-gtk3 -B sermo-backend-gtk3/_build \
      -DCMAKE_INSTALL_PREFIX=$PWD/_install_gtk3
make -C sermo-backend-gtk3/_build && make -C sermo-backend-gtk3/_build install
```

Ports without a page of their own (fltk1, efl1, sdl3, lvgl, ncurses, html) get the generic
`doc/sermo.1.in` under their own name, exactly as `debian/rules` does: both
paths ship the same thing.

### Helper programs (`sermo-source/`)

`<input modbus>`, `<input opcua>`, `<input mqtt>`, `<input mavlink>`,
`<input media>` and `<action mqtt>`, `<action mavlink>`, `<action media>` go
through eight small separate programs. They are built on their own, without
the core:

```bash
cmake -S sermo-source -B sermo-source/_build
cmake --build sermo-source/_build
# → sermo-source/_build/sermo-source-modbus, … , sermo-cible-media
```

Each one is built only if its library is there: `libmodbus-dev`
(`sermo-source-modbus`), `libopen62541-1.4-dev` (`sermo-source-opcua`),
`libmosquitto-dev` (`sermo-source-mqtt` and `sermo-cible-mqtt`). Otherwise
CMake warns and skips that program. With `-DSERMO_SOURCE_EXIGER=ON`, a missing
library stops the build: the CI and the packages set it. MAVLink needs nothing
installed: its C headers are bundled (`sermo-source/tiers/mavlink`). The two
media programs have no library: at run time, `sermo-cible-media` starts the
`mpv` program (in a `bwrap` sandbox when it is there), and `sermo-source-media`
queries it.

`cmake --install` puts them in `<prefix>/libexec/sermo`. The core looks for
them in `/usr/libexec/sermo`; the `SERMO_SOURCE_DIR` variable (an absolute
path) points to another directory, for instance `sermo-source/_build` for a
test.

## 4. Hardening

The core and the backends compile with `-D_FORTIFY_SOURCE=3`,
`-fstack-protector-strong`, `-fstack-clash-protection` and control-flow
protection, and link as PIE/RELRO/BIND_NOW. CMake picks the control-flow
protection after a test: `-fcf-protection=full` (CET) on x86,
`-mbranch-protection=standard` (BTI/PAC) on ARM64; `-fstack-clash-protection` is
kept only if the compiler actually uses it. Do not remove these flags: the
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
bash tests/comportement/run.sh "$BIN"                             # 78 green
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
`dpkg-buildpackage -us -uc -b`. Twenty-seven packages: `sermo-core-dev`, the
ten `sermo-backend-*` (including `sermo-backend-lvgl-leger`), `sermo-doc`,
`sermo-gtkdialog`, the two MCP servers (`sermoia-mcp`, `sermoman-mcp`), four
transitional packages from 1.x, and the eight helper programs
(`sermo-source-modbus`, `-opcua`, `-mqtt`, `-mavlink`, `-media`;
`sermo-cible-mqtt`, `-mavlink`, `-media`). See
[PACKAGING.en.md](PACKAGING.en.md).

## 7. Other systems (ncurses port)

Outside Linux, only the ncurses port is built. One script serves everywhere,
`packaging/autres-os/construire-os.sh`: the core, then the port. CMake tests what
the system provides and chooses on its own: `funopen` when the C library has no
`fopencookie`, a fallback when `strtod_l`, `pipe2` or `execvpe` are missing, the
right control-flow protection.

**Android (Termux)**, inside Termux:

```bash
pkg install clang cmake make flex bison pkg-config ncurses
CMAKE_EXTRA="-DSERMO_SHELL_DEFAUT=$PREFIX/bin/sh" \
    sh packaging/autres-os/construire-os.sh construire . _os ncurses
```

`SERMO_SHELL_DEFAUT`: Android's `/bin/sh` is not Termux's shell. The recipe
`packaging/autres-os/termux/build.sh` does the same in termux-packages format.

**FreeBSD** and **OpenBSD** (curses from the base system):

```bash
pkg install cmake bison pkgconf gmake          # FreeBSD
pkg_add cmake bison gmake                      # OpenBSD
sh packaging/autres-os/construire-os.sh construire . _os ncurses
```

The ports `packaging/autres-os/freebsd/devel/sermo/` and `openbsd/devel/sermo/`
call that same script.

**macOS** (ncurses, bison and flex from Homebrew; macOS's own are too old):

```bash
brew install cmake bison flex ncurses pkgconf
export PATH="$(brew --prefix bison)/bin:$(brew --prefix flex)/bin:$PATH"
export PKG_CONFIG_PATH="$(brew --prefix ncurses)/lib/pkgconfig"
sh packaging/autres-os/construire-os.sh construire . _os ncurses
```

The formula `packaging/autres-os/homebrew/sermo.rb` does the same; no tap is
published.

**Windows**: cross-compiled from Debian (`gcc-mingw-w64-ucrt64`), with the
toolchain file `packaging/autres-os/windows/mingw-w64.cmake` and an ncurses built
for the same target. The commands and the ncurses settings are in
[packaging/autres-os/README.md](packaging/autres-os/README.md) (French only),
Windows section.

In every case the binary is `_os/ncurses/ncursessermo`, and
`bash tests/comportement/run.sh "$PWD/_os/ncurses/ncursessermo"` checks it.
