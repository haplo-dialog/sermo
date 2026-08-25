# Changelog - haplo-dialog

All notable changes to this project are documented here.  
Format: [Keep a Changelog](https://keepachangelog.com/en/1.0.0/)  
Versioning: [Semantic Versioning](https://semver.org/) starting from 1.0.0.

---

## [Unreleased] - v1.1.0 (in progress)

### Added
- **`--do=CMD` (2026-08-24)**: the option had been in the installed manual pages and the info manual all along, without existing in any binary — copying the official example got you an abort. It runs the command after the dialog closes, widget values exported into the environment, through `safe_system()`: direct `exec()` when there is no shell metacharacter, a logged `/bin/sh -c` fallback otherwise, and a clean refusal under `HAPLO_NO_SHELL_FALLBACK=1`. Wired through `atexit()`, because two direct exits never reach the end of `main()`.
- **`HAPLO_ALLOWED_CMDS` (2026-08-24)**: an optional bound on which commands may run (`HAPLO_ALLOWED_CMDS=ls,cat,date`). **Unset by default**, deliberately: the language exists to run commands and 14 of the shipped examples call `bash` or `sh`. It targets whoever *deploys* a dialog into a less-trusted context. The name compared is the base name, and while it is set the shell fallback is refused — otherwise `sh -c` would walk through it.
- **Regression benches (2026-08-22 → 25)**: `tests/run_examples.sh` actually runs the examples under Xvfb instead of checking that they exist; `garde_progressbar_thread.sh`, `garde_maxwidgets.sh`, `garde_option_do.sh`, `garde_allowed_cmds.sh`, `garde_durcissement.sh`, `garde_spdx.sh` and `garde_fonctions_interdites.sh` each hold one promise the repository makes. All wired into CI, on both ports.
- **`garde_fonctions_interdites.sh` (2026-08-25)**: `SECURITY.md` states that no forbidden function is called in `src/`. A statement like that is true the day it is written and nobody re-checks it afterwards — which is how three `strtod()` calls survived four months. The bench replays it on both `src/` trees at every push, for `strcpy`, `strcat`, `sprintf`, `vsprintf`, `gets`, `system`, `popen`, `atof` and `strtod`. Two traps handled: **comments** are stripped before searching (`safe_exec.c` legitimately quotes `system()` and `popen()` to say it replaces them), and a word boundary is required **on the left**, otherwise `safe_system` would count as `system` and `g_ascii_strtod` as `strtod`. Proved by six sabotages, including a real call, a call inside a single-line comment, one inside a multi-line comment, and an empty directory that must fail rather than pass silently.
- **`gtk4sermo` port published (2026-08-21)**: a second backend, same grammar, four extra tags (`flowbox`, `overlay`, `revealer`, `stack`). CI now builds and exercises it like the other.
- **Wayland anchoring for `<window>` (2026-08-20)**: four attributes, `layer` (`background`/`bottom`/`top`/`overlay`), `edge` (the 4 edges, the 4 corners, and 6 "stride" forms anchoring two opposite edges), `dist` (margin 0-200 px, 20 by default) and `reserve` (`yes`/`no`, `no` by default: the surface floats over ordinary windows, or the compositor reserves its space and lays the others out beside it), turn the dialog into a *wlr-layer-shell* surface: a bar, a dock, a desktop widget. Ported from the BunsenLabs gtk3dialog fork (GPL-2.0+), with two corrections to the original: the margin, parsed into an unsigned type and then checked with a `< 0` test that can never fire, is now parsed with `strtol()`, rejects trailing garbage and is clamped to [0, 200]; and the Wayland detection, which read the `GDK_BACKEND` environment variable and proceeded whenever it was unset (the normal case under X11), now asks the library itself (`gtk_layer_is_supported()`). Off Wayland, or on a compositor without the protocol (GNOME), the four attributes are ignored and an ordinary window opens. Optional dependency `gtk-layer-shell >= 0.8.0`; `./configure --without-layer-shell` builds without it. Documented in `haplo-dialog-xml(5)`, example in `examples/layer-shell`. **The anchoring is exercised under sway 1.12 (wlroots 0.20) and measured to the pixel**: a `topstride` bar flush across the full width, a `bottom` `dist="24"` dock exactly 24 px off the edge, a `background` strip entirely covered by an ordinary window, and `dist="0"` versus `dist="60"` separated by exactly 60 px. `reserve="yes"` measured through `swaymsg`: a 48 px bar pushes the ordinary window down to `y=48` with `dist="0"`, and to `y=88` with `dist="20"`, the margin counting on both sides; `reserve="no"` and an absent attribute leave the window filling the whole output. Not covered: multi-output, real hardware, and compositors outside wlroots such as Hyprland.
- **gtkdialog backwards compatibility (2026-06-06)** : `make install` installs a **`gtkdialog` → `gtk3sermo` symlink** (and `gtkdialog.1` → `gtk3sermo.1`) through the autotools hook; a legacy gtkdialog script (`export MAIN_DIALOG='<window …>'; gtkdialog --program=MAIN_DIALOG`) parses, runs and returns its output in the historical format (`VAR="value"`). The consistency of the symlink is carried through into every packaging recipe (Debian `.links`, RPM `%files`, etc.).
- **`detect_terminal()` / `detect_editor()` (2026-08-24)**, in `examples/system-tools`: auto-detection of the installed graphical terminal and editor (`x-terminal-emulator` first, then xfce4-terminal, gnome-terminal, konsole, mate-terminal, lxterminal, xterm; mousepad, gedit, kate, pluma, gnome-text-editor, xed, leafpad). The detection already existed as an inline loop; this entry named it after functions that did not exist — they do now, and they return the first tool found.
- XML suite extended to **55 test cases** (Wayland anchoring, empty labels and default values, searchentry, levelbar, drawingarea, colorbutton, fontbutton, aspectframe, tree, table, menubar, statusbar, togglebutton, timer, edit, list, separators, infobar types, 3-page notebook, REFRESH/ENABLE/DISABLE/SHOW/HIDE/CLEAR actions, complex form)
- `AUTHORS` and `NEWS` at the root (GNU standard)

### Changed
- `LOGO_TMP` uses a fixed name per UID (`/tmp/haplo-logo-UID.png`), avoids the leak in case of `SIGKILL`
- **`ALLOWED_CMDS` broadened (2026-08-24)**, in `examples/system-tools`: `dnf`, `zypper`, `pacman`, `emerge`, `slackpkg`, `xbps-install`, `xbps-query`, `apk` join the `apt` family, so the example's inline help answers off Debian too. The list still refuses everything else (`rm` is still refused, checked).
- Licence **standardised to GPL-2.0-or-later** across the whole repository (source headers, packaging, `LICENCES.md`, `CONTRIBUTING.md`), the GPL-3.0+ attempt has been reverted, in accordance with the "either version 2 … any later version" clause of the sources and with upstream gtkdialog

### Fixed
- **Both ports died as soon as you used them (2026-08-22)**: a window holding a `<list>` died on open (segfault) — the `row-selected` callback expected two of the three parameters GTK passes; and any decimal number killed the program under a French locale (abort) — `atof()` reads `0.5` as `0` under `fr_FR`. 52 `atof()` calls replaced by `g_ascii_strtod`. The pass was incomplete — see the 2026-08-25 entry below. Measured under `fr_FR`: the examples go from 26/54 to 55/55 for gtk3sermo, and 58/58 for gtk4sermo.
- **The progress bar drove GTK from its reader thread (2026-08-23)**: `gtk_main_iteration_do()` called off the main thread, under a `gdk_threads_enter()` that has protected nothing since GTK 3.6. Segfault or abort roughly one run in ten. The thread now only reads its pipe; everything touching GTK goes back to the main loop through `g_idle_add()`.
- **Three `strtod()` calls had survived the locale pass (2026-08-25)**: the 2026-08-22 campaign did remove the 52 `atof()` calls, but left three `strtod()` per port, which suffer from exactly the same illness — the locale decides where the decimal separator is. Measured under `fr_FR.UTF-8`: `strtod("0.9")` returns **0.000**, while `g_ascii_strtod("0.9")` returns **0.900**. The three sites: a floating-point XML attribute in `tag_attributes.c` (for instance `xalign="0.9"`), and two numeric `<tree>` columns in `widget_tree.c`. In other words, a number written with a dot in an XML file was worth zero **silently** on every French machine. Fixed in both ports, and now forbidden by a bench.
- **Memory safety of the GTK 4 port (2026-08-24)**: `strncpy` into a non-zeroed buffer (a variable name of 512 characters or more was left unterminated), a container's widget copy without the `MAXWIDGETS` bound (300 direct children were silently accepted), and `action_append()` copying its first parameter from the start of the string. The GTK 3 port already had all three guards; the two ports are now aligned.
- **Child environment filtering, GTK 4 port (2026-08-24)**: `_build_child_env()` was missing entirely — 60 lines apart from the GTK 3 port — while `SECURITY.md` advertised the filtering for the product.
- **Empty text-content element (2026-08-20)**: `<label></label>`, or a label made only of whitespace, broke parsing with a "syntax error" pointing at the closing tag, even though writing `<text><label>   </label></text>` is the natural way to place a spacer. `<label>` and `<default>` now accept an empty body and evaluate to `""` (as `<item>` already did); `<sensitive>`, `<width>`, `<height>`, `<input>`, `<output>`, `<variable>` and `<action>` remain an error, but with a message naming the cause: "the <variable> element is empty; it requires a name."
- **Truncation of auto-generated widget names (2026-06-06)**: `g_snprintf(name, sizeof(name), …)` where `name` is a `char*`, `sizeof` therefore evaluated to 8 bytes, truncating the names, fixed by passing 64 as the buffer size; restores the original gtkdialog behaviour
- **Read of an uninitialised variable (2026-06-06)**: `instruction inst;` whose `inst.ival` was being read → `instruction inst = {0};`
- **Clean build (2026-06-06)**: 0 errors; the residual warnings stem from upstream gtkdialog idioms and from 2 untouchable bison shift/reduce conflicts

---

## Packaging revisions of 1.0.0

The **software** stays at version 1.0.0. Revisions -2 and -3 change only the
Debian package and ship an identical binary; from -4 onwards they change the
binary too.

Each port has its own `debian/changelog`, which is authoritative for it, and their
revisions may diverge: a fix does not always touch both. As of 2026-08-24:
**gtk3sermo 1.0.0-10**, **gtk4sermo 1.0.0-11**.

### [1.0.0-11] - 2026-08-22 — gtk4sermo only
- Manual page: the SEE ALSO section pointed at `fltk1d`, `efl1d`, `qt6d` and other
  ports that were never published.

### [1.0.0-10] - 2026-08-22 — both ports
- GTK 4 menus: `<menuitem>` rendered a standalone `GtkPopoverMenu`, stacked into
  the window instead of being attached to its bar.

### [1.0.0-9] - 2026-08-21 — both ports
- **Rename.** The package was called `gtk3dialog`, a name already carried by a
  BunsenLabs package, then `haplo-dialog` — which is the name of the product, not
  of a port. The ports are now `gtk3sermo` and `gtk4sermo`, and the `gtkdialog`
  compatibility link lives in a separate package, `gtksermo`, in deliberate
  conflict. Both ports therefore install alongside any other implementation.

### [1.0.0-5] - 2026-08-20
- **Parser**: an empty body is accepted where emptiness is a value (`<label>`,
  `<default>`, and the attributed form of `<item>`); elsewhere the error finally
  names its cause instead of pointing at the closing tag. See the 1.1.0 section
  above.
- **Wayland anchoring verified** under sway 1.12 (wlroots 0.20) on a headless
  1280x720 output and measured to the pixel: a `topstride` bar flush across the
  full width, a `bottom` dock exactly `dist` px off the edge, a `background`
  strip entirely covered by an ordinary window, and `dist="0"` against
  `dist="60"` separated by 60 px. The -4 package predates that check. Not
  covered: multi-output, real hardware, and compositors outside wlroots such as
  Hyprland.
- A bar does **not** reserve screen space: the exclusive zone is left at zero,
  so an anchored surface floats over ordinary windows rather than pushing them
  aside.
- Like -4, this revision changes the binary.

### [1.0.0-4] - 2026-08-20
- First package carrying the **Wayland anchoring** (`layer`/`edge`/`dist` on
  `<window>`, see the 1.1.0 section above). Unlike the two previous revisions,
  the binary is **not** identical to its predecessor.
- New optional dependency `libgtk-layer-shell0` (build:
  `libgtk-layer-shell-dev >= 0.8.0`).
- Built before the anchoring was exercised on a real compositor; -5 is the
  revision that brings that verification.

### [1.0.0-3] - 2026-08-16
- Package maintainer identity: the `Maintainer` field carried a personal name
  and a private domain, visible to every user through `apt show gtk3sermo`. It
  now carries the project's identity.
- `Vcs-Git` and `Vcs-Browser` point to the real repository; they previously
  designated a forge that never existed.
- No functional change.

### [1.0.0-2] - 2026-08-11
- First clean build of the package, after the pre-publication security pass
  (see 1.0.0 below): `lintian` reporting nothing, PIE / RELRO /
  stack-protector / FORTIFY hardening verified.

---

## [1.0.0] - 2026-05-29

First stable public version, complete rework of gtkdialog 0.8.3.

### Added
- **gtk3sermo** : reference port (GTK 3 backend), providing the backwards-compatible `gtkdialog` alias
- **43 widgets** implemented
- New widgets: `<switch>`, `<password>`, `<searchentry>`, `<calendar>`, `<infobar>`, `<levelbar>`, `<spinner>`, `<aspectframe>`, `<drawingarea>`
- `safe_system()` / `safe_popen()`, secure replacement for `system()` / `popen()`
- Hardening: `FORTIFY_SOURCE=3`, PIE, Full RELRO, NX stack, stack canary (`-fstack-protector-strong`), CFI (`-fcf-protection`)
- roff manpage `gtk3sermo(1)`
- `haplo-dialog-xml(5)`, reference manpage for the XML syntax
- Texinfo documentation (`.texi` → `.info`)
- Documentation website (static HTML)
- Packaging recipes: `.deb` (Debian, the only one built by the project), `.rpm` (Fedora/SUSE), `PKGBUILD` (Arch), `.ebuild` (Gentoo), `.SlackBuild` (Slackware)
- Example scripts covering every widget (`examples/`, 53 directories)
- XML regression suite (`tests/xml/`)
- `SECURITY.md`, `.clang-format`, `.editorconfig`
- `CONTRIBUTING.md`, `CODE_OF_CONDUCT.md`, `ROADMAP.md`

### Changed
- Binary renamed: `gtkdialog` → `gtk3sermo`
- `gtkdialog_parser.y` / `gtkdialog_lexer.l`, preserved
- Elevation through `pkexec` (PolicyKit), GUI `sudo` removed
- `fclose()` on any `FILE*` returned by `safe_popen()`, `pclose()` banned

### Removed
- Residual GTK2 dependencies
- Direct `system()` and `popen()` in the widgets
- Dead code and stray comments

### Security
- CVE-like: no known vulnerability as of 2026-05-29
- `safe_exec.c` hardening, shell-free execution where possible, `/bin/sh -c` fallback that is logged and can be disabled (`HAPLO_NO_SHELL_FALLBACK`)
- Variables exported through the environment, never evaluated by the tool

---

## [0.9.0-haplo1] - 2026-03-01 (internal)

Initial working version, port from gtkdialog 0.8.3.

### Added
- Initial structure of the gtk3sermo port
- Migration of the core (XML parser, variables, actions, signals, stack)
- Initial implementation of the widgets
- Build system: autotools

---

## [gtkdialog-0.8.3] - 2013-xx-xx (original upstream)

Last release of the original upstream by László Pere and Thunor.  
Reference: https://github.com/01micko/gtkdialog

---

*haplo-dialog is a fork of gtkdialog 0.8.3 (GPL-2.0+),  
modernised and extended by the haplo-dialog project (GPL-2.0-or-later).*

---

