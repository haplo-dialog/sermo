# Dependencies — sermo

[Français](DEPENDENCIES.md)

## Core `libsermocore`

**Build:** `flex`, `bison`, `cmake` (≥ 3.16), `gcc`, `pkg-config`.
The neutral variant has **no** graphical dependency. The `SERMOCORE_GLIB`
variant adds `libgtk-3-dev` (GLib ABI) and `libvte-2.91-dev`; `SERMOCORE_GTK4`
adds `libgtk-4-dev` and `libvte-2.91-gtk4-dev`. VTE is what makes the
`<terminal>` widget exist: without it the build **stops**, unless explicitly
declined (`-DSERMOCORE_VTE=OFF`, and `<terminal>` then only prints a warning).

**Runtime:** the `sermo-core-dev` package has **no** dependency — the `.deb`
carries no `Depends` field (static library, nothing to load at runtime).

## Backends — build `-dev` packages

| Backend | `-dev` packages |
|---|---|
| gtk3 | `libgtk-3-dev`, `libvte-2.91-dev` (`<terminal>`), `libgtk-layer-shell-dev` (Wayland anchoring; without it the build stops, unless `-DSERMO_LAYER_SHELL=OFF`) |
| gtk4 | `libgtk-4-dev`, `libvte-2.91-gtk4-dev` (`<terminal>`) |
| qt6 | `qt6-base-dev` |
| fltk1 | `libfltk1.4-dev` |
| efl1 | `libefl-all-dev` (Enlightenment/Elementary) |
| sdl3 | `libsdl3-dev` |
| ncurses | `libncurses-dev` (provides `ncursesw.pc`) |

## Backends — runtime dependencies

They are **derived from the binary** by `dh_shlibdeps` when the package is built
(see [PACKAGING.en.md](PACKAGING.en.md)), so they are always exact and
**disjoint**: each backend pulls in its own graphical stack only. Main library
per backend:

| Backend | Runtime toolkit (main) |
|---|---|
| gtk3 | `libgtk-3-0t64` (+ pango, cairo, gdk-pixbuf…), `libvte-2.91-0`, `libgtk-layer-shell0` |
| gtk4 | `libgtk-4-1` (+ graphene, gstreamer…), `libvte-2.91-gtk4-0` |
| qt6 | `libqt6core6t64`, `libqt6gui6`, `libqt6widgets6` |
| fltk1 | `libfltk1.4`, `libfltk-images1.4` |
| efl1 | `libelementary1`, `libevas1`, `libecore*`, `libedje1`… |
| sdl3 | `libsdl3-0` |
| ncurses | `libncursesw6`, `libtinfo6` |

All of them include `libc6` and the usual X11/Wayland/fontconfig libraries. The
complete list is recomputed on every package build
(`packaging/construire-paquets.sh`, in a pristine Debian testing).

## Checking / tests

`xvfb` and `xdotool` (graphical guards: clicking, escaping; `run_examples.sh`),
`imagemagick` (`import`, screenshots), `check` (libcheck: unit tests), `python3`
(the ncurses password guard, in a pseudo-terminal). The XML bench itself runs
without a display (`--print-ir`).
