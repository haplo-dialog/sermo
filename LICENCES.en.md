# Licenses — sermo

## Project license

**GPL-2.0-or-later** — sermo descends from **gtkdialog**, under the GNU General
Public License version 2 "or any later version". The `libsermocore` core, the
seven backends, `sermoman-mcp` and the documentation (manuals, manual pages,
Texinfo) are under this same license. No project-specific exception.

C and C++ files carry an SPDX identifier:

```
/* SPDX-License-Identifier: GPL-2.0-or-later */
```

A file without an identifier (script, manual page, build file) falls under the
project license, unless it is listed in the table below.

## Files under another license

| Path | License | Holders |
|---|---|---|
| `contract/sermo-contract.h` — the core ↔ backend boundary | MIT (text in the header and in `contract/LICENSE.MIT`) | S. Cage |
| `sermo-backend-sdl3/src/imgui/` — **Dear ImGui 1.92.9 WIP**, compiled into `sdl3sermo` | MIT (`imgui/LICENSE.txt`) | Omar Cornut |
| `imgui/imstb_rectpack.h`, `imstb_textedit.h`, `imstb_truetype.h` — stb libraries modified by ImGui | MIT or public domain, at your choice (end of each file) | Sean Barrett |
| ProggyClean and ProggyForever fonts, compiled into `imgui/imgui_draw.cpp` | MIT | Tristan Grimmer; Disco Hello |
| Joyo and Jinmeiyo kanji table (code points), compiled into `imgui/imgui_draw.cpp` | CC-BY-4.0 (attribution in the file) | Agency for Cultural Affairs and Ministry of Justice of Japan (official lists) |
| `imgui/backends/imgui_impl_opengl3_loader.h` — OpenGL loader | public domain and MIT (Khronos headers) | The Khronos Group |
| `sermo-backend-qt6/data/fr.haplo_dialog.qt6sermo.metainfo.xml` — AppStream metadata (AppStream requires a permissive license) | FSFAP | S. Cage |
| `tests/xml/*.xml`, `sermo-backend-qt6/examples/*/demo.sh`, and the examples written for sermo that say so in their header (`examples/showcase/`, `examples/system-tools/`) | CC0-1.0 | — |
| Icons of the `button`, `togglebutton`, `pfeme` examples (inherited from gtkdialog) | GPL-2 or LGPL-2.1: see the `COPYING-*-icons` file next to them | elementary, fast-forward, nuvola projects |

## Provenance — what exactly sermo descends from

sermo starts from **gtkdialog 0.8.3** (`PACKAGE_VERSION='0.8.3'`, as declared by
the package), written by **Pere László** and taken over by **Thunor** from 2011.
Upstream requires `gtk+-2.0` and **only** that: it never had any rendering layer
other than GTK2.

This is not a detail. It determines what, in this repository, comes from
upstream and what was written here — because GTK2 code cannot be carried over to
Qt6, SDL3, EFL, FLTK or a terminal: the calls do not exist there.

File-by-file comparison against the upstream archive (significant lines: no
blank lines, no braces, no comments, no `#include`; "inherited" = at least half
of the file's lines are found identical in gtkdialog 0.8.3):

| Backend | Inherited | Rewritten | New | Total |
|---|---|---|---|---|
| `gtk3` | 72 | 2 | 34 | 108 |
| `gtk4` | 64 | 4 | 50 | 118 |
| `qt6` | 9 | 63 | 42 | 114 |
| `sdl3` | 10 | 64 | 45 | 119 |
| `ncurses` | 9 | 63 | 42 | 114 |
| `efl1` | 8 | 61 | 43 | 112 |
| `fltk1` | 8 | 63 | 46 | 117 |

- **`gtk3` and `gtk4` are ports** of upstream's GTK2 widgets. Their headers say
  so: `(GTK3 port, security)`, `(GTK4 port)`.
- **`gtk3` has a SECOND upstream.** Wayland anchoring (`layer-shell`) does not
  come from gtkdialog 0.8.3, which dates from 2012: it comes from the later
  lineage (BunsenLabs / Puppy Linux). About thirty lines in
  `sermo-backend-gtk3/src/widget_window.c` and the XML attributes `layer`,
  `edge`, `dist`, `reserve`. Copyrights kept: **Dima Krasner** (2021) and
  **Mick Amadio** (2021-2024). The `gtk-layer-shell` library itself is a system
  dependency, not embedded code. **The six other ports do not have this
  feature** and therefore do not have this second upstream.
- **The five others carry a new rendering layer.** What they still inherit are
  the core headers — `actions.h`, `attributes.h`, `automaton.h`, `signals.h`,
  `stack.h`, `stringman.h`, `tag_attributes.h`, `variables.h`, `widgets.h`,
  `macros.h`, `gtkdialog.h`.
- **The core** keeps upstream's parser: the lexer (`gtkdialog_lexer.l`) and the
  parser (`gtkdialog_parser.y`) keep its notice (Pere László 2003-2007, Thunor
  2011-2012) and a large share of its lines.
- **`examples/`**: 188 of the 213 files exist in gtkdialog 0.8.3 (73 identical).
  They stay under the upstream license. Examples written for sermo say so in
  their header.
- `qt6` also kept nine upstream engine sources in its `src/` — never compiled
  (the core comes from `libsermocore`), so dead weight. Removed on 2026-09-12: it
  is now on par with its neighbours.

The original copyright is kept in every file concerned. Nothing is removed,
nothing is reassigned: the GPL requires attribution to survive, and it is the
least one owes to the work this one builds on.

## Scope

- `libsermocore` (core), `sermo-backend-*` (backends), `sermoman-mcp`:
  GPL-2.0-or-later, except the files in the table above.
- The generated lexer/parser (Flex/Bison) follow the project license; the
  Flex/Bison skeletons keep their usual exceptions.

## Linked libraries

Each backend links against its toolkit (GTK, Qt, FLTK, EFL, SDL, ncurses) and
that toolkit's dependencies, under their own licenses: LGPL, MIT/X11, BSD,
zlib… Qt 6 is offered under LGPL-3 or GPL-2.

Some indirectly loaded libraries are under LGPL-3 or Apache-2.0 (for example
through GTK or EFL). A program combined with such code can only be distributed
under GPL-3: this is possible because sermo is under GPL-2 "or any later
version". This reading follows the FSF's usual compatibility table; it is not
legal advice.

## Full texts

- GPL-2: the `COPYING` file at the repository root, or
  <https://www.gnu.org/licenses/old-licenses/gpl-2.0.html>.
- MIT: `contract/LICENSE.MIT` and `sermo-backend-sdl3/src/imgui/LICENSE.txt`.
- CC BY 4.0: <https://creativecommons.org/licenses/by/4.0/legalcode>.
