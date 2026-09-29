# Developer and Maintainer Manual — sermo (modular edition)

[Français](MANUEL_DEVELOPPEUR.md)

This document describes the **internal architecture** of sermo and how to
contribute to it: the `libsermocore` core, the rendering backends, the build
variants, adding a new backend, and the regression benches.

## Table of contents

1. Architecture overview
2. The `libsermocore` core — the three build variants
3. The core ↔ backend boundary
4. Anatomy of a backend
5. Adding a new backend
6. Verification benches (the three gates)
7. Building and installing
8. Security — mandatory rules
9. Architectural roadmap (Part 3)
10. Side components

---

## 1. Architecture overview

```
                 XML script
                     │
        ┌────────────▼─────────────┐
        │      libsermocore        │   ← hardened C, NO toolkit
        │  lexer + parser (Flex/   │
        │  Bison), automaton,      │
        │  variables, actions,     │
        │  safe_exec (hardening)   │
        └───────┬──────────┬───────┘
          calls │          │ provides (widget_*_create,
   widget_*_create         │  loop, operation bridges)
                │          │
        ┌───────▼──────────▼───────┐
        │  sermo-backend-<toolkit> │  ← native widgets + the toolkit's
        │   (gtk3/gtk4/qt6/fltk/   │     event loop
        │    efl/sdl3/ncurses)     │
        └──────────────────────────┘
```

The core **parses** the XML and drives the **logic** (variables, actions,
hardened shell execution). The backend **builds and draws** the widgets and owns
the toolkit's **event loop**. The core knows no concrete graphical type: it
handles opaque pointers (`GtkWidget *` = generic pointer).

### Principles

- **One core, several backends.** The grammar (lexer/parser) and the execution
  logic are shared; only rendering differs.
- **The core depends on no toolkit** (neutral variant: `ldd` shows neither GTK
  nor Qt nor SDL). Hardened shell execution is concentrated there.
- **The backend provides the widgets** (`widget_*_create`) and the operations the
  core asks of it (show, hide, add to container, event loop).

## 2. The `libsermocore` core — the three build variants

The core builds from a **largely unified source** in three variants (a CMake
option), because toolkit families do not share an ABI:

| Variant | CMake option | Families | Detail |
|---|---|---|---|
| **neutral** (default) | — | qt6, fltk1, efl1, sdl3, ncurses | Types through a **shim** (`sermocore-shim.h`); toolkit headers replaced by empty **stubs** (`include/_shim/`). No graphical dependency. |
| **glib/gtk3** | `-DSERMOCORE_GLIB=ON` | gtk3 | Real GLib/GTK 3 (native ABI of the `GList`, `GtkWidget`… structs), for the GObject introspection the GTK family uses. |
| **gtk4** | `-DSERMOCORE_GTK4=ON` | gtk4 | Core files **rewritten for GTK 4**, **vendored in `libsermocore/src-gtk4/`** (5 files: `automaton.c`, `variables.c`, `signals.c`, `actions.c`, `gtkdialog.c` plus the `gtk4-compat.c` shim): GTK 4 removed `gtk_main`, `gtk_socket`, and the old event model. No external dependency. See §9. |

The **neutral** and **gtk3** variants today compile **exactly the same** core
sources (`src/*.c`); only the flags and headers differ (shim versus real GLib).
The historical "9/10 files unified" count dates from the modularisation phase and
no longer applies. The only family-specific file on the neutral/gtk3 side is the
**attribute bridge** (`try_set_property` + `widget_set_tag_attributes`), moved out
of the core into the backend (`tag_set_property.c`) because it relies on GObject
introspection, which neutral backends do not have.

Core files: `gtkdialog.c` (orchestration + `main`), `safe_exec.c`, `variables.c`,
`stack.c`, `automaton.c`, `attributes.c`, `signals.c`, `stringman.c`, `actions.c`,
`tag_attributes.c` (helpers), `sermo_icon_theme.c`, plus `gtkdialog_lexer.l` /
`gtkdialog_parser.y` (the grammar, source of truth).

## 3. The core ↔ backend boundary

The contract is declared in `sermo_backend.h` (installed with the library). The
core **calls**, the backend **provides**:

- **`sermo_backend_toolkit_init(int *argc, char ***argv, int print_ir)`** — the
  backend initialises its toolkit (`gtk_init`, `QApplication`, `SDL_Init`…),
  without opening a display in `--print-ir` mode.
- **`widget_*_create(...)`** — one per widget type (button, entry, list…). They
  return the native widget as an opaque pointer.
- **The `sermo_be_*` operation bridge** (neutral variant): the shim maps toolkit
  primitives onto a small set of functions the backend implements —
  `sermo_be_widget_show`/`hide`/`set_sensitive`/`redraw`, `sermo_be_container_add`,
  `sermo_be_window_move`, `sermo_be_run_loop`, `sermo_be_app_init`/`quit`. This is
  the boundary contract: the core no longer names any port (the historical
  `qt6_*` → `sermo_be_*` rename is done).
- **Port identity** (optional): `sermo_port_name` and `sermo_port_details`, two
  **weak** symbols declared by the core. Without them, `--version` prints the
  core's name — which is why all seven ports used to announce themselves as
  "sermocore". The simplest route is to compile `contract/sermo_port_id.c` into
  the port, passing it `SERMO_PORT_NAME` and `SERMO_PORT_VERSION` (the
  `CMakeLists.txt` files already do). ⚠️ The `--version` line has a fixed shape:
  the 1st word is the name, the 3rd the number — shipped examples read the
  version at that rank. `tests/garde_identite_port.sh` checks both.
- **`main` lives in the core** (in `gtkdialog.c`) and calls back into
  `sermo_backend_toolkit_init` where the old code called `gtk_init`.

The other way round, the backend calls the core's `execute_action()` when an
event occurs (click, timer) to run the associated action.

**Packaging note.** The core calls `widget_*_create` (provided by the backend),
so `libsermocore` has unresolved symbols: it is a **static library** (`.a`),
linked into each backend. A clean shared `.so` presupposes the IR boundary (§9).

## 4. Anatomy of a backend

A `sermo-backend-<toolkit>` contains:

- `widget_*.c` **or** `widget_*.cpp` — one file per widget, creating the native
  widget and connecting it. The extension follows the family: `.c` for
  gtk3/gtk4/efl1/sdl3/ncurses, `.cpp` for the C++ backends (qt6, fltk1).
- `widgets.c`/`widgets.cpp` — the rendering glue (`widget_show_all`,
  `widget_get_text_value`…).
- `tag_set_property.c` — the attribute bridge, in the toolkit's own terms
  (GObject for gtk, a reimplementation for the neutral ones).
- The **init hook** (`sermo_backend_<toolkit>.c`): `sermo_backend_toolkit_init`
  plus the operation bridges for the neutral family.
- Its `CMakeLists.txt`: `pkg_check_modules(SERMOCORE REQUIRED sermocore)` plus the
  toolkit libraries; the core is **statically linked**.

The backend consumes the core **through pkg-config** (`sermocore.pc`). For the
right variant, point `PKG_CONFIG_PATH` at the matching install prefix (neutral /
gtk3 / gtk4).

## 5. Adding a new backend

1. Create `sermo-backend-<t>/src/` with the `widget_*.c`/`.cpp` and `widgets.*`
   files for your toolkit (start from a neighbour in the same family: `qt6` (C++)
   or `sdl3` (C) for a neutral one, `gtk3` for a GObject family). The file
   extension follows the backend's language (`.cpp` for qt6/fltk1, `.c`
   elsewhere).
2. Provide `tag_set_property.c` (the attribute bridge) and, for a neutral
   backend, the operation bridges (`sermo_backend_<t>.c`).
3. Implement `sermo_backend_toolkit_init` (toolkit init).
4. Write the `CMakeLists.txt`: `pkg_check_modules(SERMOCORE REQUIRED sermocore)`,
   the toolkit libraries, `-D_FORTIFY_SOURCE=3 -fstack-protector-strong
   -fcf-protection=full`.
5. Build against the installed library
   (`PKG_CONFIG_PATH=…/libsermocore/_install`), then pass **the three benches**
   (§6).

Symbols typically to be provided as well (resolved at link time):
`widget_show_list` / `widget_hide_list` (visibility globals), and the glue the
core refers to.

## 6. Verification benches (the three gates)

No step moves forward on a red bench. The three gates, for a given `<port>`
binary:

```sh
# 1. XML — parses headless (--print-ir), with no display
TIMEOUT=5 bash tests/xml/run_tests.sh <binary>           # expected: 55 PASS

# 2. Behaviour — exported values == those of the gtk3sermo reference
bash tests/comportement/run.sh <binary>                  # expected: 53/53

# 3. Guard — values at risk do not go back through the shell
xvfb-run -a bash tests/garde_echappement_sortie.sh <binary>        # OK — 4 cases
```

The behaviour bench compares the **exports** (`VAR="value"`) with those of the
gtk3sermo reference. Each case closes by itself: through a `<timer>` firing
`exit:fin`, or (cases 41 and 42) through the action of a progress bar reaching
100.

### Every bench at once — the CI

`ci/construire.sh` builds the useful core variants then the requested backends
(default: all seven); `ci/bancs.sh` chains **every** bench: core unit tests,
source guards (SPDX, forbidden functions, progress-bar thread, workstation
headers, core FORTIFY), then for each backend the XML, the hardening,
`SERMO_ALLOWED_CMDS`, endless sources (`garde_input_sans_fin.sh`), `--do`, the
widget limit, escaping, clicking and the real
opening of the `examples/` scripts (the masked password for ncurses), the
behaviour on all seven, and the two `sermoman-mcp` benches. The CI
(`.gitlab-ci.yml`, and its twin `.github/workflows/bancs.yml`) calls nothing but
these two scripts, in a Debian testing fitted with `ci/dependances.txt`: what it
sees replays identically.

Rules these scripts enforce, because each one has already lied once:

- **run everything, then conclude**: a red bench does not stop the following
  ones, the exit code is 1 if there is a single one;
- **a bench that verified nothing is a failure** (code 77, "SKIPPED");
- **a frozen bench is stopped** (`SERMO_BANC_DELAI`, 1800 s by default) and
  counted as failed;
- **no display**: `DISPLAY` is removed, each graphical guard starts its own Xvfb —
  a workstation measures what the CI measures.

### Facade check (before publishing)

`tests/verifie-facade.sh` looks for what must not ship: workstation timestamp
headers, references to internal decision documents, attribution marks in commit
messages, personal paths. It searches the tracked tree, or the whole history
(`--historique REF`: messages, author and committer, every version of every file,
annotated tags), or products (`--fichiers`, unpacked `.deb`). A **witness** (a
small fabricated repository carrying nine defects) must be seen in full before
any conclusion.

- **The CI and contributors**: `--sans-identite`, on the tree. A signed
  contribution keeps its author's name.
- **The maintainer, before publishing**: `SERMO_BANC_PUBLICATION=1 bash
  ci/bancs.sh`, which adds `--strict`, the identity of every commit and the whole
  history. Patterns specific to the maintainer's own workstation are **never**
  versioned: they live in `$SERMO_FACADE_MOTIFS` or
  `<git-common-dir>/facade-motifs`, and `--strict` refuses to conclude without
  them.

## 7. Building and installing

```sh
# Core — neutral variant (default)
cmake -S libsermocore -B libsermocore/_build -DCMAKE_INSTALL_PREFIX=$PWD/libsermocore/_install
make -C libsermocore/_build && make -C libsermocore/_build install

# Core — gtk3 variant
cmake -S libsermocore -B libsermocore/_build_gtk -DSERMOCORE_GLIB=ON \
      -DCMAKE_INSTALL_PREFIX=$PWD/libsermocore/_install_gtk
make -C libsermocore/_build_gtk && make -C libsermocore/_build_gtk install

# A backend (e.g. qt6, neutral variant)
export PKG_CONFIG_PATH=$PWD/libsermocore/_install/lib/pkgconfig
cmake -S sermo-backend-qt6 -B sermo-backend-qt6/_build -DCMAKE_INSTALL_PREFIX=$PWD/_install_qt6
make -C sermo-backend-qt6/_build && make -C sermo-backend-qt6/_build install

# Separate .deb packages, built and tested in a pristine Debian testing
bash packaging/construire-paquets.sh
```

## 8. Security — mandatory rules

Inherited from sermo, **non-negotiable**:

- **Never** `system`/`popen`/`strcpy`/`strcat`/`sprintf`. Use `safe_system` /
  `safe_popen` (in the core) and the bounded GLib equivalents.
- `safe_system` opens a shell **only** when the command carries metacharacters;
  otherwise it executes directly through `argv`. The `/bin/sh -c` fallback is
  **logged** ("injection risk").
- Hardening flags **to be kept** on every target: `-D_FORTIFY_SOURCE=3`,
  `-fstack-protector-strong`, `-fstack-clash-protection`, `-fcf-protection=full`,
  PIE/RELRO/BIND_NOW/NX. Checked by `tests/garde_durcissement.sh`.
- The trust boundary is **the local author of the script** (documented), not an
  allowlist. See [SECURITY.en.md](SECURITY.en.md).
- **Every `<input>` read is capped** (`libsermocore/include/sermo_input.h`,
  16 MiB by default): a command goes through `widget_opencommand()` →
  `safe_popen()`, a file through `sermo_fopen_input()`. A reader that opened a
  file with `fopen`, `open` or `g_file_get_contents` would escape the limit. Only
  the progress bar, which keeps a single line, removes it
  (`sermo_input_sans_limite()`). Checked by `tests/garde_input_sans_fin.sh`.

## 9. Architectural roadmap (Part 3)

The `sermo_be_*` boundary contract is **in place**: the `qt6_*` → `sermo_be_*`
rename is done and the loop goes through `sermo_be_run_loop()` — the core no
longer names any port. A single piece of work remains for a fully clean
architecture:

1. **A single gtk3+gtk4 source** — the external dependency on the old monolithic
   edition is lifted (the GTK4 core is **vendored** in `libsermocore/src-gtk4/`),
   but the `SERMOCORE_GTK4` variant remains a separate set of sources. Since GTK 4
   removed `gtk_main`/`gtk_socket`/the old event model, five core files diverge:
   reconciling them into ONE source (dropping `SERMOCORE_GTK4`) is the real
   remaining work.
2. **Formalising an IR** consumed by the backend → a clean **shared `.so`** (the
   core would no longer call back into `widget_*_create`), then in time a
   **process separation** (IPC) confining the shell to the core.

These steps are incremental and guarded by the three benches.

## 10. Side components

One tool ships alongside the repository without being part of the rendering
chain:

- **`sermoman-mcp/`** — an **MCP** **documentation** server (read-only) exposing
  the sermo reference to an AI: tools `sermo_reference`, `sermo_guide`,
  `sermo_architecture`, `sermo_heritage`, `sermo_ecosystem`, `sermo_search`,
  `sermo_example`, `sermo_how_to_report`. Code written for sermo,
  GPL-2.0-or-later; some of the examples it serves come from gtkdialog.

The MCP server for **generating** windows through an AI (a check / render / vote
loop over a catalogue of examples), which used to live here, **has been removed
from sermo**. The repository keeps the **manual** MCP above, read-only. Checking
and off-screen rendering remain available on the command line (`--print-ir`,
`--render-png`).
