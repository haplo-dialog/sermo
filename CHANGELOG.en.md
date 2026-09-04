# Changelog - haplo-dialog

All notable changes to this project are documented here.  
Format: [Keep a Changelog](https://keepachangelog.com/en/1.0.0/)  
Versioning: [Semantic Versioning](https://semver.org/) starting from 1.0.0.

---

## [1.1.4] - 2026-09-04

> Ports: gtk3sermo/gtk4sermo **1.1.4**, qt6sermo **1.0.1** (versioned
> independently), gtksermo 1.1.4 (alias). The family tag follows the GTK ports.

### Fixed

- **qt6sermo — fidelity to the reference port** (measured widget by widget
  against the gtk3sermo oracle): bare `<frame Title>` form accepted by the
  lexer; `<text>` centered like GtkLabel; complete `<pixmap>` (`<input file>`,
  `icon=`, `stock=`, icon theme resolved like GTK, `-symbolic` fallback, icon
  scaled to the requested size); `border-width` honored; no more 200x100 floor
  on small windows; decimal `<timer interval>` correct under French locale.
- **qt6sermo — the compatibility shim now honors the real GLib contracts**:
  `g_ascii_strtod`/`g_strtod` (locale), `g_ascii_str[n]casecmp` (pure ASCII),
  `g_strcmp0`, `g_strndup`, `g_strsplit`, the `g_utf8_*` family (real
  decoding), `g_shell_parse_argv` (`#` comments), `g_spawn_*` (exec failure
  reported), `gdk_keyval_to_unicode`, `g_malloc` (aborts like GLib).
- **gtk4sermo — end-of-box alignment**: the `pack_end` spring is no longer
  recreated after removal; composite headers align as in GTK 3.
- **Hardening**: the ELF CET note (IBT/SHSTK) is now FORCED at link time
  (`-Wl,-z,ibt -Wl,-z,shstk`) — on current Debian testing it was silently
  lost at link; measured on every shipped binary.

### Added

- Wider benches: shared XML suite on all THREE ports (55 cases each),
  `geometrie.sh` (layout: border-width, icon sizes) and real execution of the
  32 qt6 examples under the French locale, wired into CI.

### Added — the qt6sermo port joins the family release

- **`qt6sermo` — third port, Qt 6.** The same XML syntax as gtk3sermo and
  gtk4sermo, rendered with Qt 6 (Qt Widgets). "Write once, run on GTK 3, GTK 4
  or Qt 6." Version 1.0.0 (the youngest port; versioned independently). Its value
  parity with the reference GTK 3 port is checked by a 24-case behaviour bench
  shipped in its tree (`qt6sermo_1.0.0/tests/comportement/`). A `.deb` recipe is
  provided. Output to the shell goes through the same hardening as the other
  ports: full escaping (`\ " $ \``), opt-in `HAPLO_ALLOWED_CMDS` allowlist,
  `--do`.

### Fixed

- **Qt 6 port — `<edit>` fed by `<input>`:** the command was run with its internal
  prefix still attached (`Command:bash …`), the shell answered
  `Command:bash: not found`, and **every `<edit>` fed by a command stayed empty,
  silently**. The GTK 3 port's dispatch is restored, together with the
  `File:<path>` form that was missing too, and the loop over every `<input>`
  instead of the first one only. A bench case now covers this form (the bench
  goes from 23 to 24 cases): both existing `<edit>` cases used `<default>` only,
  hence the blind spot.
- **Qt 6 port — rendering:** five visual gaps against the reference GTK 3 port,
  found by running the `system-tools` example under both.
  (1) A `<frame label="…">` showed no title — the label was only read from the
  attribute set, never from the tag attribute.
  (2) `<notebook>` tab labels were rotated 90°; they are horizontal again, as in
  GTK, with position and current index unchanged.
  (3) `image-position="right"` was ignored on `<button>`.
  (4) `<edit>` lines were wrapped, whereas the reference does not wrap them.
  (5) An `<edit space-expand="true">` did not fill its frame: scrolled widgets are
  stacked inside their scroll container, but the expand hint was only registered
  on the inner widget; and a scroll area's child was reparented instead of being
  handed to its viewport, which produced two nested sets of scrollbars. None of
  these fixes affects an exported value.
- **GTK 4 port — `<tree>`:** setting the `stock-id` property on the cell renderer
  emitted a `GLib-GObject-CRITICAL` per row — that property was removed from
  `GtkCellRendererPixbuf` in GTK 4. A `stock-id` is now read as an `icon-name`.
  The value `<tree>` exported was already correct; the warnings are gone.
- **GTK 4 port — `<table>`:** two defects. (1) The header was read from the first
  `<item>`, which consumed the only data row — it is now read from `<label>`,
  like the GTK 3 port, and **every** `<item>` becomes a row. (2) Cells were split
  on tab, so `<item>col1|col2</item>` stayed a single cell. Splitting is now on
  `|` for `<item>`/`<label>` (tab is kept for command input). A `<table>` finally
  returns its column value.

Both defects were found by comparing, widget by widget, the value the GTK 4 port
returns to the shell against the reference GTK 3 port.

---

## [1.1.3] - 2026-08-29

### Fixed

- ⚠️ **A crash affecting BOTH ports.** Clicking an `<infobar>`'s close button
  killed the program with SIGSEGV. `widget_infobar.c` connected
  `on_any_widget_changed_event(GtkWidget *, AttributeSet *)` to the `response`
  signal, whose real arity is `(GtkInfoBar *, gint, gpointer)` —
  `g_signal_query` returns `n_params=1, param[0]=gint`. The `response_id`
  therefore arrived **where the `AttributeSet` pointer was expected**.
  Reproduced with a compiled probe: passing `0x55aa55aa0000` to
  `g_signal_connect`, the handler receives `0xfffffff9`, i.e. `-7`,
  `GTK_RESPONSE_CLOSE`. A trampoline restores the arity. **Every earlier version
  carries this defect**, gtk3sermo included.
- **A `.ui` file containing a `<menu>` crashed the GTK 4 port** (SIGABRT). In
  GTK 4, `gtk_builder_get_objects()` also returns objects that are not widgets;
  `gtk_widget_get_name()` returned `NULL` there, and an assertion failed.
- `<calendar>` returned **today's date** instead of the requested one: the
  macros that set it were `((void)0)`. In GTK 4 month and day cannot be set
  separately — `set_month` on a 31 December returns
  `assertion 'date != NULL' failed` and changes nothing. A complete, valid date
  is now set in a single call.
- `<infobar>` was a disguised `GtkBox`: the message type was ignored.
  `GtkInfoBar` still exists in GTK 4.22, deprecated but present; only
  `get_content_area()` is gone.
- `<pixmap>` **never** loaded a theme icon: `gtk_icon_theme_load_icon` was a stub
  returning `NULL`.
- The window icon **reported success while doing nothing**: the shim was
  `(TRUE)`. Measured with `xprop _NET_WM_ICON` — the reference returns
  `Icon (64 x 64)`, the port returned a blank `Icon (32 x 32)`.
- `--geometry=+X+Y` was silently swallowed. Implemented on X11 via
  `XMoveWindow`; on Wayland the protocol forbids it, and a warning says so
  instead of pretending the option was honoured.
- `disable:` on a whole `<menu>` had no effect.
- Menus in a `.ui` file were not recognised: five `GTK_IS_MENU_*` macros were
  `(FALSE)`.

### Removed

- Accelerators, **removed rather than implemented**: the `accel_groups` list was
  never populated. This was not an incomplete port, it was unreachable code — a
  shim guarding dead code creates the illusion of work in progress.

### Added

- **`tests/comportement/`**: a bench that **launches** the dialogue under Xvfb
  and compares the variables it emits, with the GTK 3 port as oracle in `--diff`
  mode. The `tests/xml/` suite runs `--print-ir`: it **parses** the XML without
  building a single widget, and therefore says nothing about behaviour. That
  misunderstanding is what left twelve dead functions in place. Plus
  `geometrie.sh`, for what no variable reveals (`border-width`, theme icon size).

---

## [1.1.2] - 2026-08-29

### Fixed

- ⚠️ **Data loss.** `<filechooser>` with `<output file:…>` called `fopen("w")` —
  which **truncates** — then wrote nothing, the value always being empty.
  Measured: a 26-byte file dropped to **0**.
- **`<checkbox>` and `<radiobutton>` silently returned a WRONG value.** GTK 4
  broke the `GtkCheckButton` → `GtkToggleButton` inheritance: measured,
  `gtk_check_button_new()` gives `GTK_IS_TOGGLE_BUTTON=0`. Calling
  `gtk_toggle_button_get_active()` on it **always** returned FALSE.
  `<checkbox><default>true</default>` emitted `CB="false"`: a script testing
  `[ "$CB" = "true" ]` systematically took the wrong branch.
- **`border-width` was swallowed without any warning.** The attribute does not
  go through the compatibility macro but through the generic GObject property
  path; in GTK 4, `GtkContainer` is gone and the property no longer exists.
  47 of the shipped examples use it. Measured: the same window goes from 56×16
  whatever the value, to 56×16 / 96×56 / 136×96 for 0 / 20 / 40 — **exactly +2N**.
- **Checkable menu entries did not exist.** GTK 4 removed `GtkCheckMenuItem`;
  the state now lives in a stateful `GSimpleAction`. Radios within one `<menu>`
  exclude each other, and `toggled` is emitted only on those that **actually
  change**.
- **`<filechooser>` was a plain button.**
  `gtk_file_chooser_button_new(title, action)` was
  `gtk_button_new_with_label(title)`: the `action="select-folder"` attribute was
  discarded, and a `GtkButton` has no `"file-set"` signal. Reimplemented on
  `GtkFileDialog`, asynchronous.

---

## [1.1.1-3] - 2026-08-28

### Fixed

- `glade_support.c`: a `return TRUE;` sat **outside the `if`**, despite its
  indentation. The function returned true on the very first iteration
  regardless of the comparison: only `signals[0]` could ever be connected, and a
  failed connection was reported as success. **Both ports.**
- `widget_tree.c`: in `widget_tree_save()`, `text` is only assigned inside the
  column loop. If that loop does not run, `text` was read by `fprintf` then
  passed to `g_free()` **having never been initialised**. **Both ports.**
- GTK 4 port: `if (!paramspec->flags & G_PARAM_WRITABLE)` — `!` binds tighter
  than `&`, so the expression was `(!flags) & 2`, i.e. **always 0**. The
  "property is not writable" guard never fired. The GTK 3 port carried this fix;
  the GTK 4 port had never received it.
- GTK 4 port: compiler warnings down from 36 to 17 — four dangling `else`
  braced without changing meaning, a parameter shadowing a global (renamed
  **along with its three uses**, otherwise the comparison deciding which widgets
  get destroyed would have switched from the parameter to the global), six
  sign/unsigned comparisons, and `%option noinput` in the lexer.

---

## [1.1.1-2] - 2026-08-28

### Fixed

- **`_FORTIFY_SOURCE` was defined twice at compile time**: `=2` by the Debian
  flags in `CPPFLAGS`, then `=3` by `src/Makefile.am`. GCC did keep the second —
  the advertised hardening was the applied one, verified by a probe and by the
  binaries — but it warned on **every compiled file**: 125 lines of noise across
  both ports, drowning the 60 real code warnings. `-U_FORTIFY_SOURCE` disarms
  the first.
- **`HARDENING_CFLAGS` and `HARDENING_LDFLAGS` were declared then ignored.**
  `configure.ac` defined them, no `Makefile.am` referenced them: hardening came
  from hard-copied lists, and the two lists had **diverged** — `-fPIE` missing
  from the first, `-Wl,-z,ibt -Wl,-z,shstk` from the second. Wiring them as-is
  would therefore have **weakened** the binaries; they were completed first.
  Proof that nothing moved: the compile and link flag sets, extracted and sorted
  before and after, are identical.

### Changed

- Documentation: the GTK 4 developer manual claimed **30 widgets**, the port has
  **56**; the GTK 3 one said "43 files, one per widget", while 43 files carry
  **52 widgets**. Both manuals presented themselves as 1.0.0 inside 1.1.1
  directories.

---

## [1.1.1] - 2026-08-27

### Removed
- **The packages from release `v1.0.0` are withdrawn (2026-08-27)**: the five
  `.deb` files (`gtk3sermo 1.0.0-10`, `gtk4sermo 1.0.0-11`, `gtksermo 1.0.0-10`
  and the two debug-symbol packages) carried the three defects fixed since — the
  output handed to the shell executable by `eval` if whoever uses the dialog types
  `$(command)` into a field, the `<switch>` that kills the program on click, and
  the dotted number that silently reads as zero under a French locale. Leaving
  them downloadable "for the record" was not defensible: a search engine or an old
  bookmark leads there just as readily as to the current version. The release page
  is withdrawn with them.
  What the operation taught: **a release and a registry package are two distinct
  objects.** The files live in the generic registry
  (`packages/generic/sermo/1.0.0/`) and the release only holds links to them —
  deleting the release alone would have left them reachable by direct URL. The
  registry package is therefore deleted **first**, so that a failure part-way
  leaves a visible page with dead links rather than dangerous packages silently
  reachable. Verified afterwards anonymously: all six URLs answer 404, and
  releases `v1.0.0-4` and `v1.1.0` are intact.
  The **git tag `v1.0.0` is kept**: it distributes nothing and dates the first
  public commit. The withdrawn packages are archived outside the repository, with
  their checksums verified, so the operation stays recoverable.

### Fixed
- **The two ports could not be installed together (2026-08-27)**: `gtk3sermo` and `gtk4sermo` both shipped `/usr/share/man/man5/haplo-dialog-xml.5.gz`, with no relation declared between them. `dpkg` refused the second — in both orders, and even when both were given in a single transaction. Yet both package descriptions, the `README` and the release note stated they "install side by side". One single collision across 219 and 214 files, but it hit exactly the promised scenario. The GTK 4 port now ships its own page, `gtk4sermo-xml.5`; the reference port keeps the canonical name. Measured after the fix in a clean `dpkg` root: **0 files in common**, joint unpack `rc=0`, both binaries and both pages present.
- **The manual page promised Wayland anchoring in both ports (2026-08-27)**: it described `layer`/`edge`/`dist`/`reserve` as working, and it was shipped **identically** by both ports. Measured on the delivered binaries: `nm -D` yields **8** `gtk_layer_*` symbols on `gtk3sermo` and **0** on `gtk4sermo`, which links no layer-shell library; `git log -S "gtk_layer_init_for_window"` yields no commit on the GTK 4 side — that code never existed there. What misleads: `libgtk-layer-shell0` *is* installed on the development machine, but that is the **GTK 3** library; the GTK 4 one is a separate package. The GTK 4 port's page now says what it actually does, and the behaviour was verified: the four attributes are silently ignored there and the window opens normally.
- **The origin of the Wayland code was in neither the header nor the manual (2026-08-27)**: the anchoring is ported from the **BunsenLabs gtk3dialog** fork, where it was written by Dima Krasner (2021) and extended by Mick Amadio (2021-2024), under GPL-2.0-or-later. The attribution existed in `AUTHORS`, `LICENCES.md`, `debian/copyright` and in a mid-file comment — but not in the `Copyright` lines of `widget_window.c`, nor in the manual page the user reads. Both are now in place.
- **The manual pages did not follow the version (2026-08-27)**: all six carried "1.0.0" **hard-coded** in their `.TH` line, so the footer rendered by `man` lied at every bump — in a version whose whole argument is "there is only one thing to read". They become `.in` templates filled by `configure`, the same pattern as the `.spec.in` files. Verified on the pages **shipped inside the `.deb`**: all four state the current version.
- **`make install` silently overwrote an existing `gtkdialog` (2026-08-27)**: the autotools hook created the link with `ln -sf … || true` — a silent overwrite of a distribution `gtkdialog`, with the `|| true` hiding even the failure. That is the opposite of what the `README` and the package descriptions promise. The alias now sits behind `--enable-gtkdialog-alias`, **off by default**, and the hook refuses to overwrite a target that is not a link. Measured: 0 `gtkdialog*` files installed by default, 2 with the flag. The `gtksermo` package remains the proper vehicle — it declares the conflict openly to `dpkg` — and becomes a pure symlink package.
- **The shipped examples called commands the packages do not provide (2026-08-27)**: out of 239 examples, **215** called `gtkdialog`, provided only by the `gtksermo` package that nothing makes mandatory — and on the GTK 4 side that therefore launched the **GTK 3** port; **17** called `gtkdialog4`, which **none** of the five packages provides. All move to `GTKDIALOG=${GTKDIALOG:-<port>}`: each calls its own port's binary and stays overridable. Verified by running an example with a `PATH` containing **only** the port's binary: the window opens, no missing command.
- **`<spinbutton>` in the GTK 4 port printed two GTK warnings on every open (2026-08-27)**: `icon-press`/`icon-release` were connected to a `GtkSpinButton`. Under GTK 3 that derives from `GtkEntry` and inherits those signals; under GTK 4 it does not — it is a `GtkWidget` implementing `GtkEditable`. A block copied from the GTK 3 port without rechecking, its comment still reading "GTK3: always available". Measured per widget: `<spinbutton>` **2** warnings, `<entry>` **0**, `<entry password="true">` **0**. After the fix: **0**.
- **`--version` printed the version twice (2026-08-27)**: "gtk3sermo version 1.1.0 gtk3sermo 1.1.0" on one side, "… 1.1.0 1.1.0-gtk4" on the other — two numbers to read, which is precisely what 1.1.0 claimed to have removed. `BUILD_DETAILS` was hard-coded in `configure.ac` and repeated what `PACKAGE_NAME`/`PACKAGE_VERSION` already print.
- **The XML suite returned a silent success (2026-08-27)**: `./tests/xml/run_tests.sh all` exited `rc=0` with **empty output** and zero tests run for anyone who had followed the "clone then build" recipe without installing — the script only looked in the `PATH`. The reader believed they had verified 55/55. It now also looks in the build tree, and exits `rc=2` when it finds nothing. Proven both ways.
- **`make check` stayed green with a completely broken binary (2026-08-27)**: this is the check `CONTRIBUTING.md` requires before merging. Two compounding causes. The generated wrapper ran `run_tests.sh` **with no argument**, so the script looked for the binary in the `PATH`, did not find the built one, and **skipped** the sections that exercise it — a skip automake summarised as `PASS`. And the checks themselves could not fail: each only looked for one word in the output, so a binary printing **nothing** passed them all. Sabotage measured before the fix: a fake binary reduced to `exit 1` gave **49 passed / 0 failed**. The wrapper now passes the built binary, a missing binary is a failure rather than a skip, and the checks demand **positive** evidence. Proven by two sabotages: a mute binary and an over-permissive one, both caught.
- **The `--do` warning cried "injection risk" on its own example (2026-08-27)**: the example given by the manual page triggered `safe_system: shell fallback (injection risk)` on every run, while the release note presents `--do` as the safe route. A `--do` command comes from the **script author**, who already has the right to run commands — that is the assumed trust model. The message becomes factual; **the refusal remains an alarm**, verified under `HAPLO_NO_SHELL_FALLBACK=1`.
- **Packaging (2026-08-27)**: `libgtk-3-0 (>= 3.22)` and `libvte-2.91-0` were hand-written **in duplicate** of `${shlibs:Depends}`, which computes them with the correct `t64` names — the hand-written name no longer exists as a real package in testing, it is satisfied only by a transitional `Provides`. Removed from both ports. The `initial-upload-closes-no-bugs` lintian overrides are dead and reported as unused: deleted. The `.SRCINFO` had diverged from its `PKGBUILD` again (`texinfo` missing), and that is the file Arch tools read first. The manual page pointed at the project's issue tracker, which answers **404**. `src/gtkdialog.1` is deleted: installed by nothing, yet it claimed "This file is installed as /usr/share/man/man1/gtkdialog.1".

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
- **The version bump had left seven packaging recipes at 1.0.0 (2026-08-26)**: renaming directories and bumping `AC_INIT` was not enough. An adversarial audit of the diff — five independent readings, then one refuter per finding tasked with demolishing it — confirmed twelve, nine of which genuinely **break**. The two Gentoo ebuilds were the perfect example of the invisible defect: renamed to `-1.1.0.ebuild`, hence `PV=1.1.0`, they still downloaded the `v1.0.0` tag archive while looking for `${WORKDIR}/sermo-v1.1.0/<port>/<port>_1.1.0` — a path that archive does not contain. `emerge` would have failed at unpack. Verified by actually unpacking the published archive: its root really is `sermo-v<tag>/`, and its port directory `<port>_1.0.0`. Both `SRC_URI` lines are now written with `${PV}`: the next `git mv` will be enough. Same treatment for both `Manifest` files, both `README.gentoo.md` and both `README.slackware.md`, which cited deleted files.
  The instructive part lies elsewhere: I had indeed bumped both `.spec` files — except those are **generated**, and git-ignored. The real source, `<port>.spec.in`, had stayed at `%define version 1.0.0`, and `configure` rewrote the `.spec` back to 1.0.0 on every run. The template now says `%define version @VERSION@`, which `configure` fills from `AC_INIT`: it follows on its own, and that trap is shut for good. The release checklist in `VERSIONING.md`, which had let these seven locations through, now carries six more entries.
- **The GTK 4 port had almost no `.gitignore` (2026-08-26)**: 3 lines against 60 for the GTK 3 port. Its compiled binary (1.5 MB), its generated `.info` manual and its object files were protected by nothing — a `git add -A` would have swept them in. The GTK 3 port's file was transposed onto it. Along the way, one pattern that bit in neither port: `debian/` is a **symlink** to `packaging/debian/`, and git applies ignore rules to the real path only — so debhelper's staging directories stayed visible. Both paths are now covered, and the tree comes back clean after a full build.
- **`ci/build.sh` did not know the GTK 4 port (2026-08-26)**: `VERSIONING.md` prescribes `./ci/build.sh gtk4sermo --test` as the first publishing step, and the command answered "unknown argument" — then silently built `gtk3sermo`, because an unknown argument was only a warning. The two missing branches are in, `all` now builds both ports, and an unknown argument **stops** the script instead of quietly doing something else. The test report no longer lies either: `make check` walks the subdirectories and returns 0 even with not a single case, so the script announced "Tests gtk4sermo" for zero tests run. It now counts the cases and says explicitly when a port has none wired in — which is the GTK 4 port, whose benches live at the repository root.
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

