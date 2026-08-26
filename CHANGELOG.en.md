# Changelog - haplo-dialog

All notable changes to this project are documented here.  
Format: [Keep a Changelog](https://keepachangelog.com/en/1.0.0/)  
Versioning: [Semantic Versioning](https://semver.org/) starting from 1.0.0.

---

## [Unreleased] - v1.2.0 (in progress)

*Rien pour l'instant.*

---

## [1.1.0] - 2026-08-26

### Added
- **The info manuals are now shipped (2026-08-26)**: `info_TEXINFOS` was commented out in both `doc/Makefile.am`, and no `.install` carried a manual. Once the package was installed, `info gtk3sermo` found **nothing** — the manuals existed only in the source archive. The reason given in the comment was to avoid the `makeinfo` build dependency: `texinfo` therefore enters `Build-Depends`, the Arch, RPM and Gentoo recipes, and the three pipelines.
  Two defects came out along the way, and they are instructive:
  - **`version.texi` carried variables that automake overwrites.** It held `@set PORT`, `@set BINARY` and `@set TOOLKIT`. But automake **regenerates** that file as soon as `info_TEXINFOS` is active, and puts only `UPDATED`, `UPDATED-MONTH`, `EDITION` and `VERSION` in it. The installed manual therefore displayed, in plain words, `{No value for 'TOOLKIT'}`. Those variables now live in the `.texi`, which automake does not touch.
  - **Without `@dircategory` and `@direntry`, a manual installs but stays unfindable.** It did not appear in the Info index menu: you could only open it by already knowing its filename. Measured: the `dir` file's menu was empty. Both directives are in place, and the manuals show up under "Graphical interfaces".
  Verified by extracting the built packages and running `info` for real: `info gtk3sermo` and `info gtk4sermo` open the manual, node navigation works (`info gtk3sermo Security` does render the Security chapter, accents included), and `lintian` reports nothing about the manual — its only remaining remarks are the English spell checker on French changelog text.
- **Ten facts extracted from the orphan upstream manual (2026-08-25)**: `doc/gtkdialog.texi` is László Pere's manual (2003-2007), present in git but absent from the published archive, and outdated by its own admission. Going through it surfaced ten behaviours the program has had **all along** and that nothing documented. Each was checked in the code **and** replayed in execution, then handed to a verifier tasked with demolishing it: three wordings out of ten were corrected at that stage — including two that claimed "documented nowhere" when it was in fact documented.
  - **The complete action table.** The program recognises **22** prefixes; the manual page documented **7**. The other fifteen are now described: `command:`, `launch:`, `closewindow:`, `activate:`, `grabfocus:`, `presentwindow:`, `save:`, `fileselect:`, `removeselected:`, `break:`, `insert:`, `append:`, `pulse:`, `start:`, `stop:`. With the point that explains the rest: **an unknown prefix is never reported**, the whole string goes to the shell. That is what made `CLOSE:` silent.
  - **The real reach of `closewindow:`**: any named widget, not only windows opened by `launch:` — and on the last window it prints the single line `EXIT="closewindow"`, **without** the widget values, unlike `exit:`.
  - **Where `save:` writes** — the `<output file>` directive — and the **locale trap**: under `fr_FR` numeric values come out with a comma (`0,4`), hence unreadable by `bc` or `awk` without `LC_NUMERIC=C`.
  - **The conditional prefixes `if true ` and `if false `**, used by **11 shipped examples** and documented nowhere. The trailing space is mandatory, and exactly one is needed.
  - **A button's default action**: without `<action>`, it exits printing `EXIT="<its first label>"`; with no label at all, "OK" is imposed on it. This is the mechanism that makes **every** example in the repository work, and no document explained it. `<togglebutton>` is excluded from it.
  - **`<visible>` is a synonym of `<sensitive>`**: it **greys out**, it does not hide — it is the `visible="false"` attribute that removes from display. On an `<entry>`, the value `password` masks the input, an inherited spelling that explains old scripts. **The masking is purely visual**: the value comes back in the clear on standard output.
  - **The self-executable file mode** (`#!/usr/bin/gtk3sermo -f`), with its two measured limits: a single line is dropped, and only when reading a file — with `--stdin` it is not.
  - **Inside a `<menu>`, the `<label>` is declared AFTER the `<menuitem>` elements.** At the head it is a syntax error, in both ports. The manual page claimed the opposite order.
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
- **All documentation moves to GPL-2.0-or-later (2026-08-25)**: there were **three papers for the same files** — `LICENCES.md` announced CC-BY-SA 4.0, both texinfo manuals carried a **GFDL** in their own `@copying`, and `debian/copyright` said GPL. And the GFDL text was nowhere in the repository, although that licence asks for it to be included. A single text now covers the whole thing, the program's.
  The relicensing is legitimate, and this is **measured**, not assumed: the texinfo manuals were written by the project and share **no sentence** with László Pere's original manual, which was under GFDL. Zero six-word sequences in common on the `gtk3sermo` side; on the `gtk4sermo` side, all 31 common sequences came from the "Acknowledgements — AI assistance" section the project had itself added to that manual — so the text flowed from the project into it, not the other way round.
  Attribution is corrected in the same move: the manuals no longer carry Pere's and Thunor's copyright, who wrote the **program** and not these pages. The program itself stays GPL-2.0-or-later, in direct inheritance from their work.
- **The "Haplo-Linux" brand leaves the public repository (2026-08-25)**: 310 occurrences across 262 files. The replacement was not blind, because it could not be — the word had two meanings. Where it named the **author** or the copyright holder, it becomes `haplo-dialog`. Where it named an **operating system** ("installed on", "target distribution", "available on", "Debian / Ubuntu / …"), it becomes `Debian Testing` or disappears: writing that a dialog program is a distribution would have produced false sentences. The operation was done at the **byte** level and not the character level, because a good part of the sources is ISO-8859-1: decoding them as UTF-8 would have broken them, and `grep` in text mode skips them, taking them for binary — that is the trap that had made the count underestimated at 251.
- **Residues of the same brand, out of reach of a literal replacement (2026-08-25)**: "Package from the *haplo-histrio* distribution" in the GTK 4 port's `lintian-overrides` — while the GTK 3 port already said "Third-party package"; "Haplo ships XFCE" in both `widget_window.c`; "Haplo release" and "Haplo repository" in a roadmap; the "Haplo-Dark" theme; the Gentoo overlay `/var/db/repos/haplo/` (now `local/`, the standard overlay); the Slackware tag `_haplo` (now `_sermo`).
- **The specifications published a fingerprint of the build machine (2026-08-25)**: the exact kernel version (`Linux 7.0.4+deb14-amd64`) and desktop (`Xfce4 4.20 / Xfwm4`). Replaced by the port's real prerequisites, which is what the reader was after.
- `LOGO_TMP` uses a fixed name per UID (`/tmp/haplo-logo-UID.png`), avoids the leak in case of `SIGKILL`
- **`ALLOWED_CMDS` broadened (2026-08-24)**, in `examples/system-tools`: `dnf`, `zypper`, `pacman`, `emerge`, `slackpkg`, `xbps-install`, `xbps-query`, `apk` join the `apt` family, so the example's inline help answers off Debian too. The list still refuses everything else (`rm` is still refused, checked).
- Licence **standardised to GPL-2.0-or-later** across the whole repository (source headers, packaging, `LICENCES.md`, `CONTRIBUTING.md`), the GPL-3.0+ attempt has been reverted, in accordance with the "either version 2 … any later version" clause of the sources and with upstream gtkdialog

### Security
- **The output handed to the shell could be executed by `eval` (2026-08-25)**: the program writes `NAME="value"` and `EXIT="value"` lines, and all of the project's documentation — the README's showcase example, both manual pages, six examples in the user manual — invited passing them to `eval`. But inside a double-quoted context, the POSIX shell expands **four** characters: `\`, `"`, `$` and the backtick. `shell_escape_value()` escaped only two of them, while claiming in its own comment that its output was *"safely eval'd"*. And `gtk4sermo` did not have the function at all — it escaped nothing.
  What makes the defect real is **where the value comes from**. It is not the author of the XML script — they already have the right to run commands, that is the assumed trust model. It is the person who **uses** the dialog, who is not necessarily the same one. Measured before the fix, without ever touching the script: open a window with a plain field, **type on the keyboard** `$(touch /tmp/proof)`, click, then evaluate the returned line — the file was created. Identical in both ports.
  The four characters are now escaped, at **both** emission points (`variables.c` for variables, `actions.c` for `EXIT`) and in **both** ports. The value is not damaged for all that: "Ada Lovelace" travels through intact, and the payload comes out as literal text.
  Locked down by `tests/garde_echappement_sortie.sh`, wired into the three pipelines. That bench opens a **real window**, clicks, and passes the line actually produced to `eval` under the watch of a witness file — because `--print-ir` does not print those lines. A first version fell back on a fabricated string when the binary returned nothing: it was therefore validating itself on its own noise. The fallback was removed, and a **sensitivity witness** checks that an unescaped line does trigger detection. Proven by two sabotages: reverting to the original two characters (all four cases fail), and removing the call from the `EXIT` path only (only that case fails).
  **The safe route remains `--do`**: values arrive through the environment and are never re-read as code. Verified by replaying the same payload: it is not executed. `README`, `SECURITY.md` and the manual pages now say so, and the first example in the user manual has moved to `--do`.

### Removed
- **The orphan upstream manual (2026-08-25)**: `doc/gtkdialog.texi`, its generated form `doc/gtkdialog.info`, and two artefacts the GTK 3 port did not have — `gtk4sermo.info` and `stamp-vti`. Both ports' `doc/` directories now have the same shape. The GTK 3 port had already deprecated that file at patch 13; only the GTK 4 port had kept a copy. The ten facts worth keeping had been extracted from it just before. `make dist` and the build pass, and the produced archive no longer contains it.

### Fixed
- **A generated file was tracked by git (2026-08-26)**: `gtk4sermo/gtkdialog.spec` is produced by `configure` from `gtkdialog.spec.in`, and was nonetheless committed — the GTK 3 port tracks only its template. Removed and ignored; both ports now have the same shape. Its template still named the package **`gtk3d`**, pointed at a 2004 FTP URL and gave upstream as the packager; the real RPM recipe and the `slack-desc` also described a "gtk3d" that exists nowhere. Corrected, respecting the `slack-desc`'s calibrated line lengths.
- **A changelog line exceeded the Debian limit (2026-08-26)**: `lintian` reported it (`debian-changelog-line-too-long`). Refolded. The only remaining remarks are the English spell checker on French text.
- **The XML manual page said the `<window>` root was mandatory (2026-08-25)**: "Every dialog must have a single `<window>` root element". False, and proven false in both ports with a counter-case: a description with no root — a lone `<vbox>`, or even a bare widget — is wrapped automatically by the grammar (the start rule of `gtkdialog_parser.y`), and a real window opens. That is the shape of most dialogs written for the gtkdialog ancestor, hence of the compatibility the project claims. Corrected by also saying what an explicit `<window>` is still good for: the title, the size, the decorations, the Wayland anchoring — and the fact that a window carries **only one** child, the following widgets being lost without warning.
- **`--include` was described backwards in all three manual pages (2026-08-25)**: "Include and execute FILE **before running** the main GUI description". In reality the file is not executed once at startup — every action command is turned into `. FILE ; command`, so the file is re-read in the subshell of **each** action. With one measured exception nobody would have guessed: `--do` is **not** prefixed, and therefore does not see the included file.
- **The example `examples/miscellaneous/standalone_file` could not run (2026-08-25)**: it still carried the upstream shebang `#! /usr/local/bin/gtkdialog`, a path no package installs (`gtk3sermo.install` installs `usr/bin/gtk3sermo`, `gtksermo.install` installs `usr/bin/gtkdialog`). Run directly, it failed with "bad interpreter". Fixed per port, and the shebang mode is now documented — with its limits, replayed.
- **Clicking a `<switch>` killed the program (2026-08-25)**: SIGSEGV on the first click, measured **6 times out of 6**, in both ports. The stack points at our code — `attributeset_get_first()` called from `widget_signal_executor()` on the "changed" signal. The cause: `GObject::notify` passes **three** arguments — `(object, pspec, data)` — and `on_any_widget_changed_event()` expects only two, `(widget, Attr)`. The `GParamSpec` therefore arrived in place of the attribute set, and was dereferenced. This is **exactly** the `GtkListBox::row-selected` fault fixed three days earlier: a signal whose arity does not match the connected callback. Signature adapter added, on the same pattern.
  What this defect teaches above all is that `tests/run_examples.sh` **opens** windows but never **clicks**: the 55 and 58 examples were counted OK and the pipeline stayed green while the switch killed the program. Hence `tests/garde_clic_widgets.sh`: one minimal dialog per interactive widget, two clicks, and the process must survive. 17 widgets, both ports, in the three pipelines. A widget that opens no window is a failure there, not an ignored case. Full sweep of the 17: `<switch>` was the only one to fall. Proven by sabotage — wiring the callback back directly brings `MORT rc=139` out again.
- **The `CLOSE:` action does not exist (2026-08-25)**: both `MANUEL_UTILISATEUR.md` documented it in a reference table **and** in a complete example to copy. The real prefix is `closewindow:`. The point that makes the error silent: an unknown prefix is not reported, it falls back to an ordinary shell command — `CLOSE:MYWINDOW` therefore closed nothing, and nobody saw an error. Verified with a harness compiled against `stringman.c`: `command_get_prefix("CLOSE:test")` yields `prefix=command, command=CLOSE:test`, while `closewindow:test` yields `prefix=closewindow, command=test`. Found by going through the orphan upstream manual, the only document in the repository giving the spelling that works.
- **`packaging/arch/.SRCINFO` had diverged from its `PKGBUILD` (2026-08-25)**: the 2026-08-24 fix on the four non-Debian recipes had only touched the `PKGBUILD`. The `.SRCINFO`, which Arch tools read first, still pointed at a URL that does not exist (`https://haplo-dialog.fr/pub/…`) and declared fewer dependencies (`gtk-layer-shell` missing on the GTK 3 side). Both files are regenerated from the `PKGBUILD` and checked field by field; the URL answers **HTTP 200**.
- **The GTK 4 port's info manual was a stub (2026-08-25)**: `doc/gtk4sermo.info` was 442 bytes and announced "produced by makeinfo" without having been. Regenerated for real: 8,372 bytes. `doc/gtkdialog.info` regenerated too, its 17 table references checked one by one.
- **The version bump had left seven packaging recipes at 1.0.0 (2026-08-26)**: renaming directories and bumping `AC_INIT` was not enough. An adversarial audit of the diff — five independent readings, then one refuter per finding tasked with demolishing it — confirmed twelve, nine of which genuinely **break**. The two Gentoo ebuilds were the perfect example of the invisible defect: renamed to `-1.1.0.ebuild`, hence `PV=1.1.0`, they still downloaded the `v1.0.0` tag archive while looking for `${WORKDIR}/sermo-v1.1.0/<port>/<port>_1.1.0` — a path that archive does not contain. `emerge` would have failed at unpack. Verified by actually unpacking the published archive: its root really is `sermo-v<tag>/`, and its port directory `<port>_1.0.0`. Both `SRC_URI` lines are now written with `${PV}`: the next `git mv` will be enough. Same treatment for both `Manifest` files, both `README.gentoo.md` and both `README.slackware.md`, which cited deleted files.
  The instructive part lies elsewhere: I had indeed bumped both `.spec` files — except those are **generated**, and git-ignored. The real source, `<port>.spec.in`, had stayed at `%define version 1.0.0`, and `configure` rewrote the `.spec` back to 1.0.0 on every run. The template now says `%define version @VERSION@`, which `configure` fills from `AC_INIT`: it follows on its own, and that trap is shut for good. The release checklist in `VERSIONING.md`, which had let these seven locations through, now carries six more entries.
- **The GTK 4 port had almost no `.gitignore` (2026-08-26)**: 3 lines against 60 for the GTK 3 port. Its compiled binary (1.5 MB), its generated `.info` manual and its object files were protected by nothing — a `git add -A` would have swept them in. The GTK 3 port's file was transposed onto it. Along the way, one pattern that bit in neither port: `debian/` is a **symlink** to `packaging/debian/`, and git applies ignore rules to the real path only — so debhelper's staging directories stayed visible. Both paths are now covered, and the tree comes back clean after a full build.
- **`ci/build.sh` did not know the GTK 4 port (2026-08-26)**: `VERSIONING.md` prescribes `./ci/build.sh gtk4sermo --test` as the first publishing step, and the command answered "unknown argument" — then silently built `gtk3sermo`, because an unknown argument was only a warning. The two missing branches are in, `all` now builds both ports, and an unknown argument **stops** the script instead of quietly doing something else. The test report no longer lies either: `make check` walks the subdirectories and returns 0 even with not a single case, so the script announced "Tests gtk4sermo" for zero tests run. It now counts the cases and says explicitly when a port has none wired in — which is the GTK 4 port, whose benches live at the repository root.
- **Both ports died as soon as you used them (2026-08-22)**: a window holding a `<list>` died on open (segfault) — the `row-selected` callback expected two of the three parameters GTK passes; and any decimal number killed the program under a French locale (abort) — `atof()` reads `0.5` as `0` under `fr_FR`. 52 `atof()` calls replaced by `g_ascii_strtod`. The pass was incomplete — see the 2026-08-25 entry below. Measured under `fr_FR`: the examples go from 26/54 to 55/55 for gtk3sermo, and 58/58 for gtk4sermo.
- **The progress bar drove GTK from its reader thread (2026-08-23)**: `gtk_main_iteration_do()` called off the main thread, under a `gdk_threads_enter()` that has protected nothing since GTK 3.6. Segfault or abort roughly one run in ten. The thread now only reads its pipe; everything touching GTK goes back to the main loop through `g_idle_add()`.
- **`make check` did not compile under `--enable-unit-tests` (2026-08-25)**: the option is documented in `COMPILE.md` and in the release checklist, and it failed to link with about thirty "undefined reference" errors. `test_stringman.c` was written against some twenty `command_is_*()` / `command_get_*()` functions that exist in neither binary: their definitions are sealed inside a `/* Redundant: Not being used */` block in `stringman.c`, inherited from upstream gtkdialog, opened at line 76 and closed at line 303. The test file even said so in writing — "these functions exist in stringman.c" — without it being true. It now targets the API that replaced them (the prefix table, read by `command_get_prefix()` and `command_prefix_get_type()`), plus `strnatcmp`, `input_is_shell_command`, `linecutter` and `str_default_name`: 15 assertions, including the alignment of the table with the `CommandType` enum and the case of a shell command containing a colon. Proved by two sabotages, both caught and named. **CI now replays `make check` on every push**: its absence is exactly what let the breakage go unnoticed.
- **`linecutter()` mixed two allocators (2026-08-25)**: fields after the first were allocated with `strdup()` (malloc) and freed with `g_free()`. Harmless in practice on glibc, but formally undefined. Switched to `g_strdup()`. Along the way, the original comment claimed the function "does not destroy the original string, rather it makes duplicates of it": false on both counts — it writes `\0` bytes into it, and `line[0]` **is** the original string, which `list_t_free()` later frees. The real ownership contract is now written above the function; getting it wrong means passing a literal and freeing an address GLib never allocated.
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

