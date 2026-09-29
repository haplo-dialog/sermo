# Changelog — sermo (modular edition)

Format inspired by [Keep a Changelog](https://keepachangelog.com/).
Versioning: see [VERSIONING.en.md](VERSIONING.en.md).

**2.7.3 is the first published 2.x release.** The previous versions, summarised
below down to 2.5.0, stayed internal. To move from 1.x to 2.x:
[MIGRATION.en.md](MIGRATION.en.md).

## [2.7.3] — 2026-09-24

PATCH release. What an `<input>` reads had no limit: an endless command (`yes`)
or an endless file (`/dev/zero`) made the dialog grow until memory ran out.
Setting the limit brought three nearby defects to light. All were measured on
the 2.7.2 packages.

### Added

- **A size limit for each `<input>`: 16 MiB.** Command or file, reading stops
  as if at end of file, and **one** warning goes to standard error — never to
  standard output, which an `eval` reads. The `SERMO_INPUT_MAX` environment
  variable changes the limit, in bytes; `0` removes it; an unreadable value is
  reported, and the default limit applies. Set in the core
  (`libsermocore/src/sermo_input.c`), it applies to all seven ports. Measured on
  2026-09-17 on 2.7.2: fed by `yes`, gtk3sermo went past 700 MB in 7 seconds,
  and ncurses ended on a segmentation fault.
- **The progress bar is not capped.** It keeps one line at a time, and must
  follow a long command to the end.

### Fixed

- **gtk3, gtk4 — the progress bar grew without end.** Each line read went to
  the main loop through its own `g_idle_add()`. A fast command (`yes 50`) filled
  that queue faster than it drained: 1,086 MB more in 3 seconds (measured on
  2026-09-24). A single update now waits. Crossings of 100 are still counted:
  the actions run as many times as before.
- **gtk3, gtk4 — a progress bar whose command is refused brought the dialog
  down.** When `SERMO_ALLOWED_CMDS` refused its command, the reader thread still
  started, on a null stream: segmentation fault (measured on 2026-09-24). The
  bar stays empty, the dialog lives on.
- **gtk3, gtk4 — `<edit>` read a file in one go.** It allocated the size given
  by `stat()`: an 8 GB file brought the program down ("failed to allocate",
  measured on 2026-09-24). The buffer was never freed, and a partial read
  displayed bytes that were never written. It now reads like its command, line
  by line, under the limit.
- **fltk1, efl1 — `<input file>` went through `g_file_get_contents()`**, which
  read `/dev/zero` forever. It is read as in the other ports.

### Tests

- `tests/garde_input_sans_fin.sh`, run on the seven ports by `ci/bancs.sh`:
  `yes` and `/dev/zero` stop at the exact limit and say so on standard error;
  an unlimited control run reads everything; the bar on `yes 50` keeps its
  memory; a refused bar lets the dialog live. Run on the 2.7.2 binaries, it
  sees the missing limit and the two progress bar defects.
- `tests/garde_lecture_input.sh`: no file opened for reading escapes the limit,
  apart from a named list of reads that are not `<input>` (the XML script
  itself, the icon theme…). On the 2.7.2 tree, it refuses 72 reads, including
  the `open()` of the gtk3 and gtk4 `<edit>`.
- `tests/unit/test_sermo_input.c`: the exact limit, one byte too many, `0`,
  unreadable values, the unlimited handover. Seven mutations of the code, seven
  failures seen. `test_safe_exec.c` also checks that each copy of
  `safe_exec.c` caps `safe_popen()`.
- `sermoman-mcp/tests/verifie-verite.sh` ties the documented limit (16 MiB) to
  the code.

### Documentation

- `SECURITY.en.md`: the "Size of what an `<input>` reads" section. The former
  known limit "`<input>` has no size limit" becomes: the cap applies to each
  `<input>`, not to the whole dialog.
- `SERMO_INPUT_MAX` in the man pages, the manuals and the documentation served
  by `sermoman-mcp`.

## [2.7.2] — 2026-09-20

PATCH release. What a window actually shows was checked by no bench: they all
read the exported **values**. Three layout defects lived there, and the site's
gallery published an empty image for weeks.

It also brings documentation fixes (the manual, `SECURITY.md`), the window
icon, and ports that did not give their own name.

### Fixed

- **Every port says its own name in `--version`.** The line is printed by the
  core, compiled once: it only knew its own name, and all seven binaries
  answered "sermocore version 2.7.1 sermo/libsermocore". Two **weak** symbols,
  filled in by `contract/sermo_port_id.c`, let each port name itself — a backend
  that does not fill them still links. Word positions do not move: the version
  stays the 3rd, which shipped examples read by rank.
- **sdl3 — off-screen rendering no longer runs away.** Its warm-up loop resized
  the window on every frame, twelve times, where the live loop does it once. A
  dialog asking for 560×420 came out at 560×1530, empty over two thirds.
- **sdl3 — an expanding child leaves room for its siblings.** In an `hbox` the
  first one took the whole width; in a `vbox`, the whole height — the bottom
  band and the button row were drawn below the window edge.
- **qt6 — the stretch factor was recorded nowhere.** `qt6_layout_register()`
  existed but was never called: the table stayed empty, the boxes concluded no
  child was expanding and inserted their right-alignment spring. On
  `system-tools` the content fitted in 258 px instead of 771.
- **qt6 — `--render-png` shows the right window, at the right size.**
  `adjustSize()` overrode `default-width`/`default-height` (500×320 rendered as
  200×142), and the chosen window could be an open drop-down list — a 2×2 pixel
  image, 4 tries out of 6. Replayed six times: 6/6 correct.
- **`examples/system-tools` hard-coded "gtk3sermo"** in its footer, whichever
  port was displaying it.
- **The manual was wrong about `<progressbar>`**: it claimed "between 0.0 and
  1.0" and its example wrote `echo 0.75`, which renders an empty bar on every
  port. The scale is 0 to 100.
- **`examples/c_embedded/example01.c` had no licence header**, and the SPDX
  bench did not look at `examples/`.
- **`examples/system-tools/system-tools.sh` hard-coded `gtk3sermo`** on its last
  line and ignored its first argument, unlike the five `showcase` examples: it
  would not start where another port was installed. It now takes the port as an
  argument, or the first of the seven it finds.
- ⛔ **Windows had no icon.** `debian/rules` installed none: only qt6 shipped
  one, at 32 px, through its own `CMakeLists`. The six graphical ports now ship
  the **HD logo** at the eight hicolor sizes (16 to 256 px), generated from
  `icon/haplo-dialog-hd.svg`. Three files still carried a dead name —
  `fltk1dialog.png`, `efl1dialog.png`, `sdl3dialog.png` — and so named nothing.
- **The ncurses port announced itself as the SDL 3 port.** Derived from it, it
  had kept its names: a window without a title showed `sdl3sermo` in its frame,
  and its warnings carried `[sdl3dialog]`. Measured in a real pseudo-terminal.
  A terminal cannot carry an icon, so the mark is now written there: the top
  frame shows "HD" before the title.
- ⛔ **`--help` sent all seven ports to `info qt6sermo`.** The string was frozen
  in the core; it now names the port that is running, the one whose info page is
  installed.
- **The manual taught six syntaxes the parser refuses** — found by feeding its 54
  XML blocks to `--print-ir` on the shipped binary: `<okbutton/>` and
  `<cancelbutton/>` (it takes `<button ok></button>`), `<width-request>` as a
  sub-element (it is an attribute, or `<width>`), unescaped Pango markup inside
  `<label>`, `<input file="path"></input>` (it takes `<input file>path</input>`),
  `<menu><label>` (the title is the `label=` attribute), and a self-closing
  `<menuitemseparator/>`. Six skeletons did not run either: an empty container is
  refused. **54 blocks out of 54 now pass**; the general rule — every tag is
  written opened then closed — is stated in §4.3.
- **The manual promised the values "when the user closes the window".** Closed by
  the window manager, the whole output is `EXIT="abort"`, with no variable at all.
  The manual now says what happens in both cases.
- **`SECURITY.md` listed only `<linkbutton>` as an escape from the two guards.**
  Two were missing, both measured on the package binary: `<terminal>` opens a
  `/bin/sh` through VTE, outside `SERMO_ALLOWED_CMDS` and
  `SERMO_NO_SHELL_FALLBACK`; and `--include` sends **every** command through
  `/bin/sh -c`, so it runs nothing once a list is set.
- Four `safe_exec.h` (fltk1, efl1, sdl3, ncurses) still claimed "no /bin/sh -c,
  no shell injection" two lines after describing the fallback.
- The `sdl3sermo` binary called itself `sdl3dialog` in its warning messages and in
  its default window title.
- `sermoman-mcp/data/architecture.md` announced **9 packages** where the
  repository builds 13 — contradicting `heritage.md` and `guide.md`, served by the
  same server. Same fix for `sermo-core-dev`, announced as depending on `libc6`
  when its `.deb` carries no `Depends` field at all.

### Changed

- **`VERSION` at the root becomes the single source of the number.** All eight
  CMake projects read it; the ncurses and sdl3 `config.h` are generated instead
  of hand-maintained; manual pages and the Texinfo manual are substituted at
  build time.
- **`install()` rules for the six backends that had none**: binary, manual page
  and icons now install from source too.
- **User and developer manuals translated into English**, and English documents
  linking to French ones now say so.
- Two continuous-integration recipes, one per forge, and a packages job that
  publishes nothing.

### Added

- `tests/garde_taille_fenetre.sh` — the first bench that looks at the
  **window**, not at exported values.
- `tests/garde_version.sh`, `tests/garde_identite_port.sh`,
  `tests/garde_liens_langue.sh`.

## [2.7.1] — 2026-09-17

PATCH release: features the documentation promised, and that 1.x had, were
missing from 2.x without a word. Found while preparing the Debian packages, by
comparing the dependencies of the 1.x and 2.x packages, then measured.

### Fixed
- ⛔ **`<terminal>` no longer existed** on gtk3sermo and gtk4sermo since 2.0.0. The
  setting that compiled VTE only lived in the old `Makefile.am` files; CMake never
  picked it up (gtk4 even forced it to 0). The widget showed a "requires … libvte"
  label and its variable stayed empty. VTE is compiled and linked again: the
  variable returns the PID of the terminal's shell, and the output of the `<input>`
  command is typed into it as if on the keyboard. The core decides (option
  `SERMOCORE_VTE`, on by default), writes it into `sermocore.pc`, and the gtk3 and
  gtk4 backends follow; a missing VTE stops the build instead of producing a binary
  without a terminal. Built without VTE (option turned off), `<terminal>` also says
  so on standard error.
- ⛔ **gtk3sermo's Wayland anchoring was not compiled**: the `layer`, `edge`, `dist`
  and `reserve` attributes of `<window>` were ignored. gtk-layer-shell is linked
  again (option `SERMO_LAYER_SHELL`, on by default): under a compositor that speaks
  wlr-layer-shell, the window becomes a layer surface.
- ⛔ **`<input>` on the ports without GTK.** Depending on the port, widgets sent the
  raw directive to the shell ("Command:echo: not found"), did not read
  `<input file>`, or ran the command twice — a command that writes somewhere did so
  twice:
  - `<text>` read nothing on qt6, fltk1, sdl3 and ncurses; `<entry>` kept every line
    (fltk1, sdl3, ncurses) or glued them together (efl1); `<input file>` read
    nothing on qt6 for `<entry>`, `<text>`, `<spinbutton>`, `<hscale>`, `<vscale>`
    and `<levelbar>`; efl1's `<levelbar>` divided the value read by 100 (0.4 became
    0.004);
  - `<comboboxtext>`, `<list>`, `<tree>` and `<table>` received the raw directive on
    several ports (and `<table>` on gtk4); `<edit>` ran twice on qt6 and fltk1,
    `<list>` and `<combobox>` twice on fltk1; `<text>` lost its final newline on
    efl1 and qt6, `<edit>` on efl1.
  Everywhere, `<input>` goes through a single decoder (command or file) and is read
  once, at the refresh the core requests right after creation. Values follow the
  gtk3sermo reference: `<entry>` keeps the first line; `<text>` and `<edit>` the
  whole text, final newline included; `<comboboxtext>` makes one item per line (an
  empty line too) and selects the first one or its `<default>`; `<combobox>` does
  not implement `<input>`, like gtk3.
- ⛔ **`<table>` without `<label>`**: sdl3sermo crashed (missing headers read while
  drawing); gtk4sermo took the first `<item>` as headers. The reference makes it a
  row.
- ⛔ **`<chooser>` aborted the five ports without GTK**: the core built the embedded
  file selector through a GTK call that returns NULL, then registering the variable
  aborted ("ASSERT FAILED: widget != NULL", exit 134). A dialog containing a
  `<chooser>` therefore never opened on qt6, fltk1, efl1, sdl3 and ncurses. There the
  tag now becomes the port's `<filechooser>` — a button that opens the toolkit's
  dialog — with the same variable and the same `<default>`; gtk3 and gtk4 keep the
  in-window selector. The reference and the manual now say so. fltk1's
  `<filechooser>`, found by the same case: it exported its prompt label "(Aucun)"
  instead of an empty value.
- fltk1: a line longer than 1,023 bytes in a drop-down list smashed the stack —
  FLTK 1.4.4 copies the label into a fixed buffer. Lines coming from `<input>` are
  bounded. `clear:` then `refresh:` on a `<tree>` crashed the program.
- sdl3 and ncurses: after `clear:`, `<edit>` kept the capacity of its old buffer; on
  sdl3, typing afterwards could have written past the new one (defect read in the
  code, fixed without having triggered it). efl1: `<edit>` lost its newlines
  ("a\nb" became "ab"); `<list>` and `<tree>` leaked the memory of every row on
  each `clear:` or `refresh:` (under valgrind, after the fix, no row is among the
  losses any more).
- Manual: the `<terminal>` example (`echo "ls -la" | bash`) would have typed the
  OUTPUT of `ls` into the terminal; the `<timer>` one (`milliseconds="1000"`) took a
  boolean for a duration.

### Removed
- The dead autotools files: `Makefile.am` and `Makefile.in` of gtk3, gtk4, fltk1 and
  efl1, the `Makefile.am` of qt6's data, and a dead duplicate of `gtk4-compat.c`.
  That is where the lost settings lived; nothing read them any more.

### Debian packages
- `debian/` (debhelper) produces 13 packages: `sermo-core-dev`, the seven
  `sermo-backend-*`, `sermo-gtkdialog`, and `gtk3sermo`, `gtk4sermo`, `qt6sermo`,
  `gtksermo` turned into transitional packages from 1.x. Dependencies computed at
  build time; `sermo` alternative with its man page and Texinfo manual;
  `debian/copyright` generated from the git-tracked tree.
- `packaging/construire-paquets.sh` replaces `build-debs.sh`: build in a pristine
  Debian testing, lintian, install, checks and purge, upgrade from 1.x through a
  local apt repository.
- `sermo-backend-gtk3` now depends on VTE and gtk-layer-shell, `sermo-backend-gtk4`
  on VTE.
- `libsermocore`: adjustable install directory (`SERMOCORE_LIBDIR`); the headers
  generated by bison and flex are no longer installed (they carried the path of the
  build directory).
- `debian/copyright` declares qt6's AppStream metadata under FSFAP and Dear ImGui's
  kanji table under CC BY 4.0 (also in LICENCES); the upstream tarball no longer
  carries `debian/`.

### Benches
- Behaviour bench: **53 cases**. 53: `<chooser>` on the seven ports. 43: `<entry>` and `<text>` from a file and from a
  command, run once; 44: `<comboboxtext>`, `<combobox>`, `<edit>`; 45 to 50:
  `<list>`, `<tree>` and `<table>`, from a command and from a file, each alone in its
  window (on the reference, their first row only comes out with the focus); 51: an
  empty line and numbers read from a file; 52: a table without `<label>`. Each case
  runs in a fresh working directory stocked with `cas/donnees/`.
- The differences from the reference measured along the way but left out of this
  release are listed in the TODO (focus-dependent selection, several `<input>`,
  mixed `<item>` and `<input>`, `auto-refresh` outside GTK…).
- `tests/garde_terminal.sh` (seven ports) and `tests/garde_layer_shell.sh` (gtk3: the
  link, then, when sway is present and not running as root, a real layer surface
  under a headless sway, against a control), run by `ci/bancs.sh`.
- `tests/run_examples.sh` waits up to 10 s for a window instead of 3: under a load
  next door, a different example turned red on each pass, with no defect.
- `packaging/bancs-sur-paquets.sh`: `ci/bancs.sh` as is, but on the binaries
  extracted from the `.deb` packages (SHA-256 sums compared); each binary must find
  its debug symbols in its `-dbgsym` package.

### Compatibility
- Building gtk3 or gtk4 requires `libvte-2.91-dev` / `libvte-2.91-gtk4-dev`, and gtk3
  `libgtk-layer-shell-dev` — or the explicit opt-out option.
- `<combobox>` filled by `<input>`: no longer loaded on qt6, fltk1 and efl1, like on
  gtk3, which never did. Use `<comboboxtext>`.
- `<text>` and `<edit>` filled by `<input>` keep the final newline everywhere.

### Verified, not assumed
- Fresh clone of 2.7.1 (`087ca97`), limited to two cores: `ci/construire.sh` builds
  the ten projects in 110 s (11 warnings, the same ones, file by file, as in
  2.7.0), `--version` returns 2.7.1 on the seven binaries and "Glade, VTE" on gtk3
  and gtk4; `ci/bancs.sh` runs **108 benches in 18 min 52 s**: 104 green in one go
  and 4 red — the click and real-examples benches of gtk4 and fltk1, each time a
  window not yet open within the delay —, which fell during loads started alongside
  on the same cores. Run again alone on the same clone: click 17/17 and examples
  55/55 (gtk4), click 17/17 and examples 54/54 (fltk1).
- Behaviour **53/53** on the seven backends; XML 55/55; `garde_terminal.sh` green on
  all seven (a real terminal on gtk3 and gtk4: PID exported, witness file written by
  the typed line); `garde_layer_shell.sh` green, level 2 run under a headless sway
  (control: an ordinary window; `layer="top"`: a layer surface).
- Red on the 2.7.0 binaries: case 43 on qt6, fltk1, efl1, sdl3 and ncurses; cases 49
  and 50 on gtk4; `garde_terminal.sh` on gtk3 and gtk4; `garde_layer_shell.sh` (not
  linked, ordinary window); the table without `<label>` crashed sdl3 and returned
  "3" on gtk4.
- Built without VTE or layer-shell (options off): the build succeeds, with CMake
  warnings, nothing linked, and `<terminal>` warns on standard error.
- efl1 under valgrind, list and tree reloaded in a loop: no invalid read or write,
  no row among the losses.
- FLTK: "stack smashing detected" from 1,100 characters with the five-argument
  `Fl_Choice::add`; 1,023 escaped "/" or "\\" (2,046 input bytes) go through — the
  bound applies to the unescaped label.
- Packages, in pristine `debian:testing` containers: source and binary build, build
  tests green; lintian with no error or warning (2 informational, 3 pedantic tags);
  the 13 packages installed, `--version` 2.7.1 on the seven binaries,
  `sermo-backend-gtk3` depends on libvte-2.91-0 and libgtk-layer-shell0; purge
  leaves nothing; upgrade 1.x → 2.7.1-1 through `apt upgrade`.
- On the DELIVERED binaries (`packaging/bancs-sur-paquets.sh`, packages 2.7.1-1,
  clone of `087ca97`, two cores): the seven package binaries put in place (sums
  compared), debug symbols found for all seven; `ci/bancs.sh`: 108 benches, **108
  green** on the first pass. Two more passes, run to exercise the script after two
  fixes (translated `readelf` output, `grep -q` under `pipefail`), gave 107 then 106
  green: each red was a different example whose window was not open yet after 3 s
  (gtk4 `pfontview`; qt6 `list`, efl1 `togglebutton`), green in the other passes.

## [2.7.0] — 2026-09-16

MINOR release: one capability (`--glade-xml` on gtk3 and gtk4) and four defects
fixed, all found by the benches repaired in 2.6.8.

### New
- **`--glade-xml` loads a GtkBuilder interface file** (the format Glade saves) on
  **gtk3sermo** and **gtk4sermo**, each in the format of its GTK version. The
  window is the object `--program` names (`MAIN_WINDOW` by default); every widget
  with an id becomes a variable of that name; a signal handler is a sermo action
  (command, `exit:`…) under the same rules as `<action>`; on `realize`, its output
  fills the widget. The option relied on libglade, which exists for neither GTK 3
  nor GTK 4: it was compiled as "ignored" everywhere, and any script using it
  aborted. The GtkBuilder code existed, never compiled, with three defects, now
  fixed: every other handler never connected (missing braces), variables named
  after the widget's TYPE, an abort on an unreadable file (status 1 and a message
  now). On GTK 4, handlers go through a `GtkBuilderScope`.
- `examples/glade/`: GTK 4 versions of both examples (`*-gtk4.ui`); the scripts
  pick their file from `GTKDIALOG`.

### Fixed
- ⛔ **`<progressbar>` only lived on gtk3 and gtk4.** qt6 read the first line of the
  `<input>` command and closed the pipe; fltk1, efl1, sdl3 and ncurses read all of
  it before opening the window (3.5 s of empty screen on `examples/progressbar`).
  None of the five moved the bar, and the action set for 100 never ran: a dialog
  relying on it to close stayed open. The reading, written once in the core
  (`sermo_progress`), follows the command line by line without blocking the port's
  loop, like the reference: a number moves the bar, a text becomes its label, the
  line that reads 100 runs the actions.
- ⛔ **`--include` with a relative name loaded nothing.** Each command becomes
  ". FILE; command", and under `/bin/sh` (dash on Debian) "." looks a name without
  "/" up in `PATH`, never in the current directory: an `<input>` calling a function
  from the file returned nothing, silently, on all seven ports. The path is made
  absolute at start-up, and quoted for the shell: a space or an apostrophe no
  longer break it.
- **A `<frame>` only showed its first child** on fltk1, efl1, sdl3 and ncurses; the
  others were created but never drawn (efl1 left a piece of one outside the frame).
  They are all there now, as with the reference, which packs them in a vertical
  box.
- **Ports without GTK refuse `--glade-xml`** and say so (status 1), instead of
  ignoring it and then aborting.

### Benches
- Behaviour bench: **42 cases**. Case 41, a progress bar that closes the dialog at
  100; case 42, a frame with several children. Both fail on the 2.6.8 binaries;
  case 42 also on a sabotage of the frame fix alone.
- `tests/garde_glade.sh` and `tests/garde_include.sh` (new), run by `ci/bancs.sh`
  on all seven ports; red on the 2.6.8 binaries.
- `tests/run_examples.sh`: an example meant for some ports says so in a `PORTS`
  file; elsewhere it is shown as "reserved", visibly, and counted neither as
  passed nor as failed. The bench passes `GTKDIALOG` to the examples.

### Compatibility
- `--glade-xml` on qt6, fltk1, efl1, sdl3 and ncurses: refused (status 1). It never
  worked there.
- `--include`: a relative path is resolved against the start-up directory.

### Verified, not assumed
- Fresh clone of 2.7.0, limited to two cores: `ci/construire.sh` builds the ten
  projects in 98 s (11 warnings — the glade_support.c `#endif` one is gone),
  `--version` returns 2.7.0 on the seven binaries; `ci/bancs.sh` runs **100
  benches in 17 min 33 s, none failing**.
- XML 55/55 and behaviour **42/42** on the seven backends; real examples 55/55 on
  gtk3 and gtk4, 54/54 on qt6, fltk1, efl1 and sdl3 (`glade` reserved);
  `garde_glade.sh` and `garde_include.sh` green on all seven; hardening, command
  list, `--do`, widget limit, escaping, click and masked password green; core
  FORTIFY 13/14, 6/13 and 8/14 objects; unit tests 3/3; sermoman-mcp: 6/6 against
  the code, 392 runs without failure.
- Cases 41 and 42 fail on the 2.6.8 binaries; case 42 also when the frame fix
  alone is sabotaged (ncurses). `garde_glade.sh` and `garde_include.sh` fail on
  the seven 2.6.8 binaries, for the expected reasons.
- On `examples/progressbar`, which counts to 100 in 3.3 s: window open in 150 ms
  and closing on its own in 3.0 to 3.1 s, `EXIT="Ready"`, on qt6, fltk1, efl1,
  sdl3 and ncurses. PNG rendering of a frame with three children on fltk1 and
  efl1: one child before, three after.

## [2.6.8] — 2026-09-16

PATCH release: a click defect, benches that said OK without checking, and the
automatic verification at the root of the repository.

### Fixed
- **sdl3, ncurses: clicking a `<fontbutton>` closed the dialog**, with `EXIT` set
  to the font name. There, the font chooser is built as a button, and a button
  without an action closes the dialog. On ncurses, the 2.6.7 fix (a bare button
  finally exits) had opened that door.
- 2.6.7 blamed the failure of `garde_clic_widgets.sh` on sdl3 on the lack of
  OpenGL under Xvfb. **That was wrong**: the guard looked for the window by a
  title that xdotool cannot read in UTF-8. Once repaired, it measured sdl3 for
  the first time, and found the defect above.

### Benches
- ⛔ **`run_unit_tests.sh` printed "OK" without running anything**: it targeted
  1.x directories. It now tests both copies of the core's `safe_exec.c`, and
  `stringman.c`; zero tests run is a failure.
- ⛔ **`tests/xml/run_tests.sh` could test the installed 1.x** instead of the
  built binary, and counted a timeout as a success. A name is only looked up in
  the build tree; a malformed XML file serves as a witness.
- `run_all.sh` and the `all` mode of the XML bench fail when a port is missing,
  unless `SERMO_PORTS_OPTIONNELS=1`.
- The qt6 test exercised a copy of `safe_exec.c` that nothing linked into the
  binary: it now exercises the core's, and the copy leaves the repository.
- `garde_spdx.sh` descends into subdirectories and also reads `.cpp .hpp .l .y`;
  witnesses added to `garde_spdx`, `garde_fonctions_interdites` and
  `garde_progressbar_thread`. `run_fuzz.sh` also targeted the old layout.
- `tests/run_examples.sh`, which really opens every example, gave false
  verdicts: sdl3 "no window" on 54 examples (a UTF-8 title xdotool cannot read),
  qt6 "syntax error" on 50 (the program, left running, died when the X server
  went away and printed that false message). Search by class too, stop the whole
  process group, judge the output recorded before stopping: 54 examples out of
  55 on both ports.

### New
- **Automatic verification, at the root.** `ci/construire.sh` builds the core and
  the seven backends, `ci/bancs.sh` runs every bench, and `.gitlab-ci.yml` only
  calls these two scripts, in a Debian testing with `ci/dependances.txt`, under a
  French locale. What the CI sees can be replayed at home, identically. A bench
  that is red, hung, or verified nothing fails the whole run. The `sermoman-mcp`
  recipe, which GitLab never read (it was not at the root), is removed.
- **`tests/verifie-facade.sh`** checks that no workstation timestamp header,
  reference to an internal working document, attribution trailer or personal
  path goes out: in the tree, in the whole history (identities and tags
  included) or in packages. A fabricated witness must be seen in full. The CI
  runs it on the tree without checking identity: a contribution keeps its
  author's name. Before publishing, the maintainer runs
  `SERMO_BANC_PUBLICATION=1 bash ci/bancs.sh`.

### Documentation
- The EFL package is `libefl-all-dev`: `libefl-dev`, written in three documents,
  does not exist in Debian.
- The gtk3sermo and gtk4sermo man pages promised that `--glade-xml` loads a Glade
  file. The option is accepted and **has no effect**: this edition has no Glade
  library (libglade exists for neither GTK 3 nor GTK 4), as `--help` already
  said.
- `CONTRIBUTING`, `COMPILE` and the developer manual explain how to replay
  everything like the CI; the behaviour bench has 40 cases, not 24 or 22.

### Verified, not assumed
- Fresh clone, limited to two cores (the size of a shared CI runner):
  `ci/construire.sh` builds the ten projects in 99 s (12 warnings, no error),
  `--version` returns 2.6.8 on the seven binaries; `ci/bancs.sh` runs 86 benches
  in 17 minutes.
- XML 55/55 and behaviour 40/40 on the seven backends; hardening, command list,
  `--do` and widget limit green on all seven; escaping and click (17 widgets)
  green on the six graphical ports, sdl3 included; masked password green on
  ncurses; core FORTIFY 13/13, 6/13 and 8/14 objects; unit tests 3/3;
  sermoman-mcp: 6 checks out of 6 against the code, 392 runs without failure.
- `ci/bancs.sh` tested on four fabricated benches: green, red, "verified
  nothing" (code 77) and hung are classified correctly, and the last three fail
  the run.
- Still red, for the reasons given above: real examples, 54 of 55 on gtk3, gtk4,
  qt6 and sdl3 (`glade`), 53 of 55 on fltk1 and efl1 (`glade`, `progressbar`).
- Found along the way, not fixed (see TODO.md): with `--program`, efl1 stopped by
  SIGTERM (it then exits with code 1) and qt6 losing its X server print a false
  "syntax error"; CET notes are set at link time, but neither glibc nor the
  toolkits of Debian testing carry them.

## [2.6.7] — 2026-09-16

PATCH release: four security defects, two terminal defects, and the clean-up
that comes before publication.

### Security
- ⛔ **FORTIFY_SOURCE did not protect the core.** `-D_FORTIFY_SOURCE=3` was passed,
  but the core, gtk3, gtk4, fltk1 and efl1 compiled without any `-O`: without
  optimisation, FORTIFY does nothing. Measured: zero `__*_chk` call in the three
  variants of `libsermocore.a`. They now pick the `Release` build type by
  default, like qt6, sdl3 and ncurses. `-w`, which silenced the glibc warning,
  leaves our code.
- ⛔ **`garde_durcissement.sh` wrongly answered OK**: it took `__stack_chk_fail`
  as proof of FORTIFY. It now excludes it, and carries a witness.
  `garde_fortify_coeur.sh` (new) checks the core library, which the optimised
  backend of qt6, sdl3 and ncurses was hiding.
- ⛔ **`SERMO_ALLOWED_CMDS` could be bypassed by a namesake.** Only the base name
  was compared: `/tmp/x/ls` passed as soon as `ls` was listed. The comparison is
  now on the command as it will be run: a bare name, the path where `PATH` finds
  it, or a path listed as such.
- **1.x settings are no longer lost.** `HAPLO_ALLOWED_CMDS` and
  `HAPLO_NO_SHELL_FALLBACK` are read again when the `SERMO_…` name is not set,
  with a warning.
- **ncurses no longer shows the password** while it is typed: one star per
  character. `garde_ncurses_mot_de_passe.sh` (new) checks it in a real
  pseudo-terminal.

### Fixed
- **ncurses: no text field could be filled in a terminal.** Input gave up after
  100 ms, before any keystroke, and a "q" typed afterwards quit the program. A
  bare button such as `<button ok>` did not close the dialog either.
- **`--version` announced 2.0.0** on the seven ports: the number now comes from
  the core's `CMakeLists.txt`.
- The documentation writes `eval "$(sermo …)"`: without quotes, the shell
  expanded wildcards in the output.

### Documentation and licenses
- `LICENCES.md` states what the repository really contains: the MIT contract,
  Dear ImGui (MIT) embedded in sdl3, new tests and examples under CC0, and the
  fact that 188 of the 210 files in `examples/` come from gtkdialog. The original
  author's name, damaged in 88 headers, is restored; `AUTHORS` credits the whole
  upstream.
- References to unpublished working documents have left the repository, as has
  the `for-claude/` skill: `sermoman-mcp` remains the only tool for AI.

### Compatibility
- ⚠️ **`SERMO_ALLOWED_CMDS` is stricter.** A command written with a path is only
  accepted if that path is listed, or if it is where `PATH` finds a listed name.
  On a system where `PATH` finds `/usr/bin/echo`, listing `echo` no longer allows
  `/bin/echo`: list `/bin/echo`, or write `echo`.
- A `Debug` build has no FORTIFY: it only works with optimisation.

### Verified, not assumed
- Fresh clone, no build type forced: the ten projects (core ×3, seven backends)
  build as `Release`, 12 warnings in total, no error.
- `--version`: `sermocore version 2.6.7` on the seven binaries.
- `garde_fortify_coeur.sh`: 13 fortified objects out of 13 (neutral core), 6 out
  of 13 (GLib), 8 out of 14 (GTK 4); before: 0 in all three.
- XML suite 55/55 and behaviour suite 40/40 on the seven backends.
- `garde_durcissement.sh` and `garde_allowed_cmds.sh` green on the seven;
  `garde_ncurses_mot_de_passe.sh` green. Each one fails on the previous binary or
  on a deliberate sabotage: they measure something.
- sermoman-mcp suites: documentation matches the code (6/6), 56 examples on the
  seven backends (392 runs, 0 failure).
- Still red, as they were before: `garde_clic_widgets.sh` on sdl3 (no window under
  Xvfb without OpenGL) and `garde_fonctions_interdites.sh` run outside its scope on
  `libsermocore/include` (deliberate `strtod` in the compatibility layer).

## [2.6.6] — 2026-09-15

PATCH release: no code change. **586 source files** stop announcing a version in
their header.

### Fixed
- ⛔ **A source header carrying a version number was, by construction, lying.**
  586 files introduced themselves under fifteen different labels, nearly all
  dead: `sermo 2.0.0` (138), `qt6sermo 1.0.0` (102), `fltk1dialog 1.0.0` (82),
  `efl1dialog 1.0.0` (77), `sermo 2.5.0` (65), `gtk4sermo 1.0.0` (24)… Four of
  them even carried the product's FORMER name, from before the rename.
  The number is not updated, it is **removed**: the line now reads
  `sermo — haplo-dialog <devel@haplo-dialog.fr>`, with no digit. A source file
  has no reason to carry a version: that is exactly how it goes stale without
  anyone seeing it — twice today, once in an example, once here.
  ⚠️ The 2.1.0 clean-up handled 216 of them and only looked at NAMES; the numbers
  kept drifting behind it.

### What did not move
- The **copyright** lines and the **SPDX** identifiers: untouched, file by file.
  Provenance and ownership do not depend on a version number.
- The code: the pass touches one comment line per file.

### Verified, not assumed
- The three core variants and the seven backends rebuild; behaviour **7 ports ×
  40 cases** green; `garde_spdx` green on the nine source directories.

## [2.6.5] — 2026-09-15

PATCH release: on `fltk1`, opening an `<expander>` scrambled the whole window.
Three stacked defects, all invisible to the bench.

### Fixed
- ⛔ **Neighbours were never repositioned.** The toggle callback grew the
  expander's group and stopped there. But this port **pins** every child of a box
  to its height (`Fl_Flex::fixed()`) at construction time: a child that grows
  AFTERWARDS overlaps its neighbours instead of pushing them. Measured: the label
  below climbed on top of the title. The toggle now walks up the chain of boxes —
  pin reset to the real height, box grown by the same delta (otherwise `layout()`
  takes the space from the neighbours instead of adding it), layout redone — and
  the window follows.
- ⛔ **The group was stretching its own children.** An `Fl_Group` whose
  `resizable` is itself resizes ALL its children proportionally: the header
  button grew with the group (title floating in the middle of a tall band) and
  the body landed on top of the next widget. The inner geometry is no longer
  inferred, it is **written**: header at `HEADER_H`, body right below, at creation
  as on every toggle.
- ⛔ **`@v` is not an FLTK symbol.** The port used it for the open-state arrow, so
  FLTK drew **nothing**. Rotation is written with the numeric-keypad digit —
  `@2>` points down. An open expander has its arrow back.

### Verified, not assumed
- Real click under Xvfb, three captures: collapsed, open, closed again. The title
  stays, the content fits in its frame, the neighbour is pushed down then comes
  back, and the window grows then returns to its size.
- Behaviour **7 ports × 40 cases** green; XML 55/55 on `fltk1`;
  `garde_clic_widgets` 17 widgets with no crash; hardening, SPDX and forbidden
  functions green.

### What this brings to light
- ⚠️ **159 source files** of `fltk1` (82) and `efl1` (77) still introduce
  themselves in their header as “fltk1dialog 1.0.0” / “efl1dialog 1.0.0”: the
  product's former name and a dead version. The 2.1.0 clean-up handled 216 and
  left those behind.

## [2.6.4] — 2026-09-15

PATCH release: two debts settled on `<expander>` — the title a click erased on
`fltk1`, and documentation that did not exist.

### Fixed
- ⛔ **`fltk1` erased the expander's title on the first click.** The toggle
  callback rewrote the header button's label with the ARROW ALONE
  (`btn->label("@v  ")`): an expander named “Details” became anonymous as soon as
  it was opened, and stayed that way. The title is now kept in the widget's state
  and rewritten on every toggle, by the same function that sets it at creation —
  one single way to compose the header, so no divergence.
  ⚠️ No bench could see it: the behaviour bench compares VALUES, not the screen.
  Measured with a real click, under Xvfb, with captures.

### Documentation
- **`<expander>` was documented nowhere** in the user manual: neither the tag,
  nor its attribute, nor the value it exports. It is now, with the three things a
  script needs: the exported state (`true`/`false`), `expanded=` which opens it
  at startup (and whose absence **collapses** it), and the two ways of writing
  the title.
- ⚠️ **The reference served by the MCP was wrong on that point.** It claimed that
  `<expander><label>…</label>` “does not pass the parser; only `label="…"`
  works”. Checked against all four forms: `<label>` **AFTER** the content passes,
  `label="…"` passes, `<label>` **before** the content is refused, and
  `<expander Titre>` is refused. The form was never the problem, the PLACE was —
  the grammar is `<expander>` content, then attributes. Corrected.

### What this brings to light
- ⚠️ **`fltk1` scrambles the layout when the expander opens**: the group grows
  (`parent->size()`), but the neighbouring widgets are not repositioned — they
  overlap and the content spills out of the frame. Seen on capture, not fixed
  here: that is a parent-layout question, not a label one.

## [2.6.3] — 2026-09-15

PATCH release: `<expander>` was born in the wrong state on the five neutral
ports — and the two halves were wrong in opposite directions.

### Added
- **One behaviour bench case** (40-expander): corpus 39 → **40 cases**. It plays
  TWO expanders, one open and one collapsed, each with a child that exports a
  real value: the initial state and the child's survival are measured together.

### Fixed
- ⛔ **`expanded=` was read by no neutral port.** The reference reads the TAG
  ATTRIBUTE `expanded=` (“true”, “yes” or 1) and **collapses by default**. The
  five ports shared two opposite mistakes:
  - `qt6` and `fltk1` always collapsed — `<expander expanded="true">` exported
    “false”;
  - `efl1`, `sdl3` and `ncurses` always opened — an expander with no attribute
    exported “true” where the reference exports “false”.
  Those last three read `<default>`, which the reference **ignores** for this
  tag: they answered a syntax that exists nowhere else. They now read
  `expanded=`, with the reference's exact rule.
- `fltk1` now also opens its content area and points the arrow down when the
  expander is born open: the exported state and the state on screen no longer
  diverge.

### Verified, not assumed
- Behaviour **7 ports × 40 cases** green, 0 discrepancy; XML **55/55** on all
  seven; hardening, SPDX and forbidden functions green on the five ports touched.

### What this brings to light
- ⚠️ **`<expander>` is documented nowhere** in the user manual: neither the tag,
  nor its `expanded=` attribute, nor the value it exports. A tag shipped on seven
  ports that the documentation ignores is the same documentation hole in another shape.

## [2.6.2] — 2026-09-15

PATCH release: `<frame>` finally exports its title on the five neutral ports. The
defect surfaced while writing the bench cases that the language's two largest
containers were missing.

### Added
- **Two behaviour bench cases** (38-frame, 39-notebook): corpus 37 → **39
  cases**, played on the seven ports. `<frame>` and `<notebook>` have shipped
  everywhere for a long time and **no case exercised them** — exactly the state
  `<flowbox>`, `<overlay>` and `<revealer>` were in the day before. Both cases
  carry children that export REAL values: a container exports nothing by itself,
  and an empty `.attendu` is a dead case.

### Fixed
- ⛔ **`<frame>` did not export its title on five ports.** The reference exports
  `gtk_frame_get_label()`; `qt6`, `fltk1`, `efl1`, `sdl3` and `ncurses` returned
  an empty string — `qt6` even returned `NULL`. Measured by case 38: the
  reference renders `FR="Options"`, the five rendered `FR=""`. The title was
  there in every port all along (`QGroupBox::title`, `Fl_Widget::label`,
  `elm_object_text_get`, `node->label`) — it was simply never read back.
  ⚠️ `fltk1` carried a comment saying “frames do not export a value”: honest, and
  wrong as a policy. Same pattern as `efl1`'s note about `<levelbar>`, fixed in
  2.3.1 — a port does not decide on its own what the language exports.
- Case 38 ALSO covers the frame **without** a title, which must render an empty
  string. Without it, the fix could have settled for inventing a title.

### Verified, not assumed
- `<notebook>` was already correct on all seven ports: current page exported,
  the children of BOTH tabs preserved. Case 39 pins it so it stays that way.
- Behaviour **7 ports × 39 cases** green, 0 discrepancy; XML **55/55** on all
  seven; hardening, SPDX and forbidden functions green on the five ports touched.

## [2.6.1] — 2026-09-15

PATCH release: nothing new in the language. One faulty declaration repaired in
four ports, a version number that lied in nineteen places, and dead weight taken
out of the repository — one promised backend, seven man pages, one packaging
kit.

### Fixed
- ⛔ **`execute_action` redeclared by hand in six files.** `actions.h` declares
  it as `int execute_action(GtkWidget*, const char*, const char*)`; four
  backends redeclared it on the spot, with a different return type and without
  the `const`. Same symbol, two signatures: an ODR violation, therefore
  undefined behaviour. The compiler only complained on `qt6`; the defect was in
  `widget_wizard` and `widget_menubutton` of `qt6` and `fltk1`, and in
  `widget_wizard` of `gtk3` and `gtk4`. They all include the header now — which
  carries its `extern "C"` guard, so nothing had ever prevented including it.
  `efl1` already did so: its pattern is the one adopted. No call used the return
  value and the arguments passed are compatible: observable behaviour is
  unchanged, the undefined behaviour is gone.

- ⛔ **The version number was wrong in nineteen places**, three of which **ship
  with the package**: `sermocore.pc` announced `Version: 2.0.0` — a backend
  requiring `sermocore>=2.3` would have been turned away by a core that is in
  fact newer than its request — and both `man sermo` and `info sermo` showed
  2.0.0 in the footer. The other sixteen are documentation: VERSIONING,
  PACKAGING (copy-pasteable `.deb` names that did not exist), the user manual,
  the health report, the MCP knowledge base, and the CMake projects of
  `sermocore`, `qt6`, `sdl3` and `ncurses`. The number really only lived in the
  packaging script. **Historical** mentions of 2.0.0 stay: “the 2.x line opened
  with 2.0.0” is true; what was false was announcing it as the current version.

- ⛔ **The one per-backend man page that is actually INSTALLED was lying.**
  `sermo-backend-qt6/src/qt6sermo.1` is placed in `share/man/man1` by qt6's
  CMakeLists, and it was titled `QT6DIALOG` — the product's former name — while
  announcing `qt6sermo 1.0.0`. Fixed. Along the way, the same CMakeLists'
  `message(STATUS "qt6sermo 1.0.0 …")` now follows `${PROJECT_VERSION}`: that
  number can no longer drift.

### Removed
- **The promise of a “web” backend.** The roadmap (French and English), both
  READMEs and the MCP documentation announced a forthcoming web rendering
  engine. The door stays open — it is the door the `ncurses` port came through —
  but it no longer names an engine: the product announces only the seven ports
  it ships.

- **Seven dead man pages.** Five `.1` (`gtk3sermo`, `gtk4sermo`, `fltk1dialog`,
  `efl1dialog`, `sdl3dialog`) and two `.5` XML references still announced `1.0.0`
  or `1.1.1`, and three carried the product's FORMER name. None of them shipped —
  the modular edition installs `doc/sermo.1` for all seven ports — and four of
  them appear in their `Makefile.am`'s `CLEANFILES`: the build itself treats them
  as products, they should never have been tracked. Their `.in` templates stay.
- **The packaging kit of a product that no longer exists**:
  `sermo-backend-qt6/packaging/` (21 files), arch, rpm, gentoo, slackware and
  debian recipes for a **standalone `qt6sermo 1.0.0`**. The modular edition no
  longer produces that package but `sermo-backend-qt6`, through
  `packaging/build-debs.sh` at the root. No other backend had such a kit, nothing
  referenced it, and it even carried a build log (`qt6sermo.debhelper.log`).

### Documentation
- The lexer comment that follows the `<flowbox>` / `<overlay>` / `<revealer>`
  rules still described those three tags as an open hole on six ports. They have
  been ported since 2.6.0. It now states what happened, and keeps the lesson: a
  token in the grammar without a matching `case` in the core yields a tag that
  pretends, and without a bench case nobody sees it.

### Verified, not assumed
- The three core variants and the seven backends rebuild; behaviour **7 ports ×
  37 cases** green, 0 discrepancy; XML **55/55** on all seven; hardening green on
  the seven binaries.

## [2.6.0] — 2026-09-14

MINOR release: **three more tags** — `<flowbox>`, `<overlay>`, `<revealer>` — on
the seven ports. No change to the core↔backend contract.

### Fixed — three tags that were pretending
These three tags **already had a token** in the grammar, and an implementation on
the **single** gtk4 port. On the six others the core had no `case` at all: the
tag parsed without error and **produced nothing**. A script using them got a
silently truncated window. No bench case exercised them — nobody could see it.

They are now ported everywhere, with a bench case that proves it.

### Added
- **`<flowbox>`**: children fill a line, then wrap to the next. Attributes
  `min-children-per-line`, `max-children-per-line`, `column-spacing`,
  `row-spacing`, `selection-mode`. Variable: the index of the selected child.
  ⚠️ gtk3 and gtk4 genuinely reflow as the window shrinks (GtkFlowBox); the other
  ports lay children out in **N fixed columns** — Qt and FLTK have no flow
  layout, and `elm_gengrid` only arranges items built by a callback class, not
  arbitrary widgets. Only gtk3/gtk4 can **select**: elsewhere the variable stays
  empty.
- **`<overlay>`**: the first child is the background, the following ones float
  above it — the only container in the language that superimposes.
  `QStackedLayout` in `StackAll` mode (qt6), `Fl_Group` with shared geometry
  (fltk1), a single-cell `elm_table` (efl1), cursor reset before each layer
  (sdl3).
  ⚠️ **A terminal does not superimpose**: ncurses draws the background, then the
  layers one below the other, each preceded by a marker. The content stays
  readable and the degradation is visible.
- **`<revealer>`**: a child that appears and disappears. Attributes `transition`,
  `duration`, `reveal`. Variable: `true` or `false`.
  ⚠️ **The animation exists on gtk3 and gtk4 only**; elsewhere the port shows or
  hides. The `transition` attribute is accepted and ignored — said in the manual
  rather than left unsaid.
- **One behaviour bench case** (37): corpus 36 → **37 cases**, played on the
  seven ports.

### Fixed — two defects the bench could not see
A container exports an empty string: the behaviour bench compares VALUES, it does
not look at the screen. These two only surfaced on capture, playing two
`<button>` inside an `<overlay>` on each port.
- ⛔ **`qt6` superimposed the wrong way round.** `QStackedLayout` in `StackAll`
  mode shows everything, but **raises the CURRENT child** — and `currentIndex`
  defaults to 0. The background therefore came out on top of everything: the
  reference showed “ABOVE (second)”, qt6 showed “BACKGROUND (first child)”. The
  last child is now raised, like `GtkOverlay`.
- ⛔ **`sdl3` wrote on STANDARD OUTPUT.** After the stack, the port reset the
  cursor without submitting an item; ImGui refuses to grow the window that way
  and complained **on every frame** — a red banner on screen, and the error text
  on `stdout`, the very channel where sermo writes `VAR="value"`. A `Dummy()`
  closes the stack cleanly.

## [2.5.0] — 2026-09-14

First entry of this line. `sermo` is a **modular** edition: one shared core,
`libsermocore`, which parses the XML and executes, plus **rendering backends**
with disjoint dependencies — installing one pulls in only its own toolkit.

### The language

62 widget types, including the layout containers `<grid>` (column alignment) and
`<paned>` (two areas and a handle), and the structural tags `<toolbar>`,
`<stack>`, `<wizard>`, `<menubutton>`.

The same XML renders an equivalent interface on **seven backends**: `gtk3`,
`gtk4`, `qt6`, `fltk1`, `efl1`, `sdl3` and `ncurses` (terminal). Exported values
(`NAME="value"`) are identical everywhere — that is the product's invariant, and
it is measured, not merely claimed.

### What guards it

Three replayable test benches: XML (55 cases), behaviour (36 cases replayed on
all seven backends against the `gtk3sermo` reference), and hardening guards. No
release ships with a red bench.

### Security

Hardened shell execution (`safe_system`/`safe_popen`, no shell outside
metacharacters, output escaping), binaries built with the available mitigations.
See [SECURITY.en.md](SECURITY.en.md).

### Compatibility

Scripts written for `gtkdialog` keep working; the `sermo-gtkdialog` package
provides the `gtkdialog` command.
