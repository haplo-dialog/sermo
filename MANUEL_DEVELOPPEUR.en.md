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
11. Helper programs (`sermo-source/`)

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
        │    efl/sdl3/lvgl/        │
        │    ncurses/html)         │
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
| **neutral** (default) | — | qt6, fltk1, efl1, sdl3, lvgl, ncurses, html | Types through a **shim** (`sermocore-shim.h`); toolkit headers replaced by empty **stubs** (`include/_shim/`). No graphical dependency. |
| **glib/gtk3** | `-DSERMOCORE_GLIB=ON` | gtk3 | Real GLib/GTK 3 (native ABI of the `GList`, `GtkWidget`… structs), for the GObject introspection the GTK family uses. |
| **gtk4** | `-DSERMOCORE_GTK4=ON` | gtk4 | Core files **rewritten for GTK 4**, **vendored in `libsermocore/src-gtk4/`** (5 files: `automaton.c`, `variables.c`, `signals.c`, `tag_attributes.c`, `gtkdialog.c` plus the `gtk4-compat.c` shim; the others, `actions.c` included, come from `src/`): GTK 4 removed `gtk_main`, `gtk_socket`, and the old event model. No external dependency. See §9. |

Two other build options:

| CMake option | Where | Default | Effect |
|---|---|---|---|
| `SERMOCORE_VTE` | `libsermocore` | `ON` | `<terminal>` widget through VTE in the glib/gtk3 and gtk4 variants. The choice goes into `sermocore.pc` (variable `vte`), and the gtk3 and gtk4 backends follow it. A missing VTE stops the build; `-DSERMOCORE_VTE=OFF` builds without it, and `<terminal>` only shows a warning. |
| `SERMO_LVGL_LEGER` | `sermo-backend-lvgl` | `OFF` | lvgl without ThorVG or Lottie, hence without SVG icons (package `sermo-backend-lvgl-leger`, same `lvglsermo` binary). |

The **neutral** and **gtk3** variants today compile **the same** core sources
(`src/*.c`), but for one file: `sermo_progress.c` (the progress bar read as it
comes) only goes into the neutral variant, gtk3 keeping its own reading thread
(`libsermocore/CMakeLists.txt`). Otherwise, only the flags and headers differ
(shim versus real GLib).
The historical "9/10 files unified" count dates from the modularisation phase and
no longer applies. The only family-specific file on the neutral/gtk3 side is the
**attribute bridge** (`try_set_property` + `widget_set_tag_attributes`), moved out
of the core into the backend (`tag_set_property.c`) because it relies on GObject
introspection, which neutral backends do not have.

Core files: `gtkdialog.c` (orchestration + `main`), `safe_exec.c`,
`sermo_lanceur.c` (the command launcher, in a single place, and the choice of
shell: `SERMO_SHELL`, otherwise `SERMO_SHELL_DEFAUT`), `variables.c`,
`stack.c`, `automaton.c`, `attributes.c`, `signals.c`, `stringman.c`, `actions.c`,
`tag_attributes.c` (helpers), `sermo_icon_theme.c`, `sermo_input.c` (the
`<input>` cap), `sermo_source.c` (translation of
`<input modbus|opcua|mqtt|mavlink|media>` and `<action mqtt|mavlink|media>` into
a command, see §11), `sermo_focus.c` (which widget takes the focus when a window
opens), `sermo_progress.c` (neutral variant only), `sermo_win32.c` (Windows
only: console, command launching, an object built without the shim), plus
`gtkdialog_lexer.l` / `gtkdialog_parser.y` (the grammar, source of truth).

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
  gtk3/gtk4/efl1/sdl3/lvgl/ncurses/html, `.cpp` for the C++ backends (qt6, fltk1).
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
   (`PKG_CONFIG_PATH=…/libsermocore/_install/lib/pkgconfig`), then pass **the
   three benches** (§6).

Symbols typically to be provided as well (resolved at link time):
`widget_show_list` / `widget_hide_list` (visibility globals), and the glue the
core refers to.

To stay on par with the reference, a `<list>`, `<tree>` or `<table>` asks the
core, when it is created, whether it takes the focus when the window opens
(`sermo_focus_initial()`, `libsermocore/include/sermo_focus.h`): that answer
alone decides whether its first row is chosen without `<default>`.

## 6. Verification benches (the three gates)

No step moves forward on a red bench. The three gates, for a given `<port>`
binary:

```sh
# 1. XML — parses headless (--print-ir), with no display
TIMEOUT=5 bash tests/xml/run_tests.sh <binary>           # expected: 55 PASS

# 2. Behaviour — exported values == those of the gtk3sermo reference
bash tests/comportement/run.sh <binary>                  # expected: 78/78

# 3. Guard — values at risk do not go back through the shell
xvfb-run -a bash tests/garde_echappement_sortie.sh <binary>        # OK — 4 cases
```

The behaviour bench compares the **exports** (`VAR="value"`) with those of the
gtk3sermo reference. Each case closes by itself: through a `<timer>` firing
`exit:fin`, or (cases 41 and 42) through the action of a progress bar reaching
100.

Beyond the three gates, the pass test for a port is
`bash examples/metiers/verifie.sh --ports "<port>"`: it plays every dialog of the
job scripts and toolboxes (104 scripts, 187 dialogs) and compares the values
with gtk3sermo.

### Every bench at once — the CI

`ci/construire.sh` builds the useful core variants, the requested backends
(default: all nine), then `sermoia-mcp`, the light lvgl port
(`-DSERMO_LVGL_LEGER=ON`, when lvgl is requested) and the eight helper programs
of `sermo-source/` (`-DSERMO_SOURCE_EXIGER=ON`: a missing library is a
failure). `ci/bancs.sh` chains **every** bench: core unit tests, single
version, language links, source guards (SPDX, script licences, forbidden
functions, progress-bar thread, workstation headers, capped `<input>` reads),
the helper programs (Modbus/OPC UA, MQTT read and write, MAVLink read and write
against a fake drone, sound and video player) and their hardening, binary by
binary, the tooled examples (approval, network, GNSS, serial, home automation,
CAN, DNS, TLS, journal, USB, NTP, Wi-Fi, Bluetooth, camera), the behaviour of
the light lvgl, the weight per port, the facade check, the core FORTIFY, then
for each backend the XML, the identity, the hardening, `SERMO_ALLOWED_CMDS`,
endless sources (`garde_input_sans_fin.sh`), `<input
modbus|opcua|mqtt|mavlink>`, `<action mqtt|mavlink>` on click only, `--do`,
`SERMO_SHELL`, the widget limit, `--include`, Glade, `<terminal>`, escaping,
clicking and the real opening of the `examples/` scripts (the masked password
for ncurses; neither clicks nor examples for html, which opens no window), the
behaviour on every built port, the two `sermoman-mcp` benches, the job scripts
and the `sermoia-mcp` one. The CI (`.gitlab-ci.yml`, and its twin
`.github/workflows/bancs.yml`) calls, to build and test, nothing but these two
scripts, in a Debian testing fitted with `ci/dependances.txt` (the rest of the
recipe installs the dependencies and the French locale): what it sees replays
identically.

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

# The helper programs (§11)
cmake -S sermo-source -B sermo-source/_build -DSERMO_SOURCE_EXIGER=ON
cmake --build sermo-source/_build

# Separate .deb packages, built and tested in a pristine Debian testing
bash packaging/construire-paquets.sh
```

## 8. Security — mandatory rules

Inherited from sermo, **non-negotiable**:

- **Never** `system`/`popen`/`strcpy`/`strcat`/`sprintf`. Use `safe_system` /
  `safe_popen` (in the core) and the bounded GLib equivalents.
- `safe_system` opens a shell **only** when the command carries metacharacters;
  otherwise it executes directly through `argv`. The fallback through the shell
  (`/bin/sh -c`, or `SERMO_SHELL`) is **logged**, as a fact, not an alarm:
  `safe_system: '<command>' contient de la syntaxe shell -- execution via
  /bin/sh -c` (`libsermocore/src/safe_exec.c`). The old "injection risk"
  message was removed: said on every ordinary `--do`, it was no longer heard.
  With `SERMO_NO_SHELL_FALLBACK` or `SERMO_ALLOWED_CMDS`, the fallback is
  **refused**, and that refusal stays an alarm (`g_critical`).
- Hardening flags **to be kept** on every target: `-D_FORTIFY_SOURCE=3`,
  `-fstack-protector-strong`, `-fstack-clash-protection`, `-fcf-protection=full`,
  PIE/RELRO/BIND_NOW/NX. `tests/garde_durcissement.sh` measures them on the
  **produced binary**, not on the flags (`readelf`): PIE, RELRO, BIND_NOW,
  non-executable stack, CET IBT, CET SHSTK, stack protector
  (`__stack_chk_fail`) and FORTIFY (`__*_chk` calls, `__stack_chk_*` excluded),
  eight guarantees in all. A **witness** runs first: two small binaries,
  fortified and bare, that the FORTIFY measure must tell apart.
  `-fstack-clash-protection` leaves no trace this bench reads: it does not
  check it.
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

Two tools ship alongside the repository without being part of the rendering
chain; both are packaged since 2.8.2:

- **`sermoman-mcp/`** — an **MCP** **documentation** server (read-only) exposing
  the sermo reference to an AI: tools `sermo_reference`, `sermo_guide`,
  `sermo_architecture`, `sermo_heritage`, `sermo_ecosystem`, `sermo_scripts`,
  `sermo_search`, `sermo_example`, `sermo_how_to_report`. Code written for sermo,
  GPL-2.0-or-later; some of the examples it serves come from gtkdialog.
  `sermo_scripts` serves the inventory of the job scripts and toolboxes,
  generated by `sermoman-mcp/outils/inventaire-scripts.py`.
- **`sermoia-mcp/`** — an **MCP** server (in C) that has a dialog **written,
  checked and seen**: `sermo_schema`, `sermo_check`, `sermo_run`, `sermo_render`,
  `sermo_examples`, `sermo_vote`. Its catalogue includes the dialogs of the job
  scripts and toolboxes (`sermoia-mcp/outils/catalogue-metiers.sh`). See
  [sermoia-mcp/README.en.md](sermoia-mcp/README.en.md).

Checking and off-screen rendering also remain available on the command line
(`--print-ir`, `--render-png`).

## 11. Helper programs (`sermo-source/`)

The `<input modbus|opcua|mqtt|mavlink|media>` and `<action
mqtt|mavlink|media>` tags do nothing themselves. The core (`sermo_source.c`)
translates them into a command that starts a **helper program**, installed in
`SERMO_SOURCE_DIR` (default `/usr/libexec/sermo`). The nine ports only see one
more command: no port depends on libmodbus, open62541, libmosquitto or MAVLink.

**The contract** is `libsermocore/include/sermo_source.h`:

- the address goes as **a single argument**, percent-encoded (every byte outside
  `[A-Za-z0-9._:/=,+@-]` becomes `%XX`): the command has no metacharacter, it
  goes through `argv`, never through the shell, even with
  `SERMO_NO_SHELL_FALLBACK` or `SERMO_ALLOWED_CMDS`;
- refused when the XML is parsed: unknown protocol, empty address, more than
  4096 bytes (`SERMO_SOURCE_ADRESSE_MAX`), control character, a relative
  `SERMO_SOURCE_DIR` or one carrying a character outside the safe set;
- the program started is `<folder>/sermo-source-<protocol>` (reading) or
  `<folder>/sermo-cible-<protocol>` (writing). The core only accepts a writing
  `<action>` inside a `<button>` or a `<menuitem>`, and refuses the name
  `sermo-cible-` written by hand in an `<input>` or an `<action>`.

**The eight programs**, one package each:

| Program | Role | Library |
|---|---|---|
| `sermo-source-modbus` | reads Modbus registers (TCP, RTU) | libmodbus |
| `sermo-source-opcua` | reads an OPC UA variable | open62541 |
| `sermo-source-mqtt` | reads an MQTT topic (never publishes) | libmosquitto |
| `sermo-cible-mqtt` | publishes a value, according to an allow-list | libmosquitto |
| `sermo-source-mavlink` | reads an autopilot's telemetry (sends no byte) | embedded MAVLink headers |
| `sermo-cible-mavlink` | sets a parameter, gives a safety order, sends a mission | embedded MAVLink headers |
| `sermo-source-media` | reads the state of an mpv player | none |
| `sermo-cible-media` | starts and commands mpv (in a `bwrap` cage if present) | none (mpv is a program) |

**The shared files**:

- `commun.c` / `commun.h` — for all: address reading (decoding, 32 parameters
  at most, unknown keys refused, bounded integers), 5 s pause after a failure,
  shared cache under `$XDG_RUNTIME_DIR/sermo-source/`, a lock per device, and
  `source_lire()`, which never freezes the window (fresh value, stale value
  reread in the background, or an empty field);
- `mqtt-commun.c` / `.h` — MQTT reading and writing: `mqtt[s]://` address,
  encryption (certificate and name checked, never "insecure"), credentials
  (refused on `mqtt://` without `clair=oui`), connection bounded by a timeout;
- `mavlink-commun.c` / `.h` — MAVLink reading and writing: link address
  (`udp://`, `tcp://`, `serie://`), accepted speeds, opening;
- `media-commun.c` / `.h` — the two media programs: player name, private folder
  `$XDG_RUNTIME_DIR/sermo-media/NAME/` (700), exchange with mpv's socket (only
  requests written by the program go there);
- `mission-wpl.c` / `.h` — reading a "QGC WPL 110" mission file, reduced to
  waypoints (100 at most, 256 KiB at most);
- `tiers/mavlink/` — MAVLink's C headers (protocol v2), copied unmodified;
  origin, checksum and licence in its `LISEZMOI` (French only).

**Building**:

```sh
cmake -S sermo-source -B sermo-source/_build -DSERMO_SOURCE_EXIGER=ON
cmake --build sermo-source/_build
```

The media and MAVLink programs are always built. The others are built if their
library is there (`libmodbus-dev`, `libopen62541-1.4-dev`, `libmosquitto-dev`);
without `SERMO_SOURCE_EXIGER`, a missing library skips its program with a
warning. With `-DSERMO_SOURCE_EXIGER=ON` (CI, packages), it stops the build.
Each program gets the same hardening as the ports; `ci/bancs.sh` measures it on
each one with `tests/garde_durcissement.sh`. To try without installing, set
`SERMO_SOURCE_DIR` to `sermo-source/_build` (absolute path).
