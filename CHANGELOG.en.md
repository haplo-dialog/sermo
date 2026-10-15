# Changelog — sermo (modular edition)

Format inspired by [Keep a Changelog](https://keepachangelog.com/).
Versioning: see [VERSIONING.en.md](VERSIONING.en.md).

**2.7.3 is the first published 2.x release**; 2.7.4, 2.7.5, 2.7.7 and 2.8.2 follow it (2.7.6, 2.8.0 and 2.8.1 were only delivered as packages; their content is published with the next release). Versions 2.0.0 to 2.7.2 stayed
internal; they are described below to keep track of what changed since 1.x. To
move from 1.x to 2.x: [MIGRATION.en.md](MIGRATION.en.md).

## [3.0.0] — 2026-10-15

It joins two lines of work. The ncurses port leaves Linux: Windows 10/11,
Android (Termux), FreeBSD and OpenBSD; commands go through a single launcher,
whose shell is set by `SERMO_SHELL`. And sermo reads a PLC from the XML,
read-only: `<input modbus>`, `<input opcua>`. Neither the XML language nor the
contract between the core and the ports is broken (three optional functions
are added to the contract): moving to 3 is the maintainer's choice of number,
see [VERSIONING.en.md](VERSIONING.en.md).

### Added

- **`<input modbus>` and `<input opcua>`: read a PLC from the XML, read-only.**
  `<input modbus>tcp://host:502?registre=1&diviseur=10</input>`,
  `<input opcua>opc.tcp://host:4840?noeud=ns=1;s=name</input>` (certificates:
  `mode=chiffre&certificat=…&cle=…&confiance=…`). The core turns the tag into a
  command that runs a helper (`/usr/libexec/sermo`, new packages
  `sermo-source-modbus` and `sermo-source-opcua`), the address encoded as a
  single argument with no metacharacter: never `/bin/sh`, the
  `SERMO_ALLOWED_CMDS` allowlist applies, and the nine ports read it unchanged.
  Unknown parameter refused; 5 s pause after a failure; **the window never
  freezes**: a value under 10 s old (`perime=`) is shown at once and reread in
  the background, only the very first read of an address waits for the device
  (measured, device that never answers: 10 clicks on "Fermer", 10 closes in
  103 to 135 ms; before, 1 in 10 at 1.2 s); shared cache (`cache=`, 900 ms) and one read
  at a time per device: six widgets on the same address, one connection
  (measured through a counting relay). Benches: `garde_input_source.sh` (nine ports),
  `garde_sermo_source.sh` (helpers against a fake PLC and a fake server), unit
  test `test_sermo_source.c`.
- **`sermo-backend-lvgl-leger`**: the same `lvglsermo` built without ThorVG
  (option `SERMO_LVGL_LEGER`): 1,663 KiB instead of 2,399 (−31 %), same values
  (behaviour bench at parity), without SVG icons from themes. It replaces
  `sermo-backend-lvgl`: install one or the other.
- **Quality benches**: `garde_poids.sh` (weight ceiling per port,
  `tests/poids-plafonds.txt`) in `ci/bancs.sh`; `garde_sanitizers.sh` (nine
  ports under ASan + UBSan, fatal errors, a faulty witness that must stop), on
  request: `SERMO_BANC_SANITIZERS=1`.
- **Modbus and OPC UA examples shipped in `sermo-doc`** (sources only:
  `/usr/share/doc/sermo-doc/examples/modbus`, `…/opcua`).
- **Two industrial examples, read-only**: `examples/modbus/`
  (`modbus-lecture.sh` reads a Modbus/TCP PLC's registers through `mbpoll`)
  and `examples/opcua/` (`opcua-lecture.sh` and `opcua-lire`, a small open62541
  C client with no write service at all). Gauges and values refresh every
  second; a fake PLC (`faux-automate.py`, python3-pymodbus) and a fake server
  (`faux-serveur-opcua`) let you try them without hardware. Reading runs in the
  background: an unreachable device does not freeze the window.
- **OPC UA with certificates**: `opcua-lire -m sign|chiffre -c … -k … -T …`
  (and `OPCUA_MODE`, `OPCUA_CERTIFICAT`, `OPCUA_CLE`, `OPCUA_CONFIANCE` in the
  window). The client only trusts the certificates given with `-T`: there is no
  "accept everything" mode. `faux-serveur-opcua PORT --securise` and
  `certificats-essai.sh` let you try it.

- **Windows 10/11, native** (ncurses port): a single `ncursessermo.exe`,
  cross-compiled from Debian (mingw-w64, UCRT) and statically linked. UTF-8 text
  and console; commands through `CreateProcess` (default shell: cmd.exe, or
  busybox-w32 named by `SERMO_SHELL`); links through `ShellExecute`, for
  `http`, `https` and `mailto` only. The `<input>` limit (`SERMO_INPUT_MAX`)
  does not exist on Windows yet. Details:
  [packaging/autres-os/README.md](packaging/autres-os/README.md) (French only).
- **Termux (Android)**: a recipe for the ncurses port in termux-packages
  format. The graphical ports build there by hand, eight out of nine (not efl1:
  EFL does not exist in Termux); the steps are in the same README.
- **FreeBSD and OpenBSD**: a system port for the ncurses port.
- **`SERMO_SHELL`**: the shell that runs a command with shell syntax, until now
  a hard-coded `/bin/sh`. Used when it names an executable program by an
  absolute path; otherwise the one chosen at build time (`SERMO_SHELL_DEFAUT`,
  `/bin/sh` by default). A rejected value is reported once on stderr. Applies
  to all nine ports; the code that runs commands now lives in one place
  (`libsermocore/src/sermo_lanceur.c`).

### Security

- **Control-flow protection chosen by trial**: CET on x86, BTI/PAC on ARM64.
  Hard-coded, the x86 option stopped every ARM64 build. Objects compiled
  separately (`neutral_glist.c`, `image_load.c`, ImGui) are now hardened like
  the binary: on x86, the CET marking was set on a binary whose objects lacked
  the instructions; on ARM64, they made the whole binary lose the BTI/PAC
  marking.
- **sermoia-mcp: FORTIFY had no effect.** With no build type, CMake passed no
  optimisation and `_FORTIFY_SOURCE` did nothing. Release by default, like the
  ports.

### Fixed

- **`refresh` replays the `<input>` on all nine ports, like the reference.** On
  qt6, fltk1, efl1, sdl3, lvgl, html and ncurses, most widgets read their
  `<input>` only at creation (`<levelbar>`, `<hscale>`, `<vscale>`,
  `<spinbutton>`) or never (`<checkbox>`, `<switch>`, `<radiobutton>`,
  `<togglebutton>`, `<calendar>`, `<colorbutton>`, `<fontbutton>`,
  `<searchentry>`, `<statusbar>`, `<infobar>`, `<linkbutton>`, `<expander>`,
  `<notebook>`): a gauge driven by a `<timer>` stayed frozen while the
  neighbouring `<text>` widgets followed (seen on the Modbus example). Measured
  on 2026-10-13: 0 or 1 run of the command instead of 3. Reading now lives in
  `refresh`, which the core calls right after creation and on each `refresh:`;
  the measured rules of the gtk3sermo reference are followed: first line of the
  output; on the first pass a `<default>` wins over the `<input>` (calendar,
  colorbutton, fontbutton, searchentry and **every `<entry>`**), `<label>` for
  statusbar, `expanded=` for expander; toggle widgets set their state without
  playing actions on the first pass, then toggle like a click (actions
  included, the unchecked radio sibling first); a radio button is never
  unchecked by its `<input>`. Benches: behaviour cases 73 to 78, green on all
  nine ports. Remaining gaps: colour names ("blue") not read outside gtk;
  "2021-02-31" becomes 2021-02-28 on qt6; several `<input>` on one widget: only
  the first is read outside gtk.
- **qt6sermo no longer crashes on exit when a plain `<menuitem>` carries a
  `<variable>`** (segmentation fault on every `exit`); sdl3, lvgl, html and
  ncurses exported `false` for that menu item, the reference gives an empty value.
- **A `<progressbar>`'s value reaches the picture and the page.** Found by the
  website gallery ("Progress (should show 65 %)": empty bar). htmlsermo wrote
  the page before the bar had read its command: it now rewrites it once the bars
  are done. With `--render-png`, gtk3sermo, gtk4sermo, qt6sermo and efl1sermo
  captured before the first line; they wait for it, one second at most
  (`sermo_progress_en_attente()` in the core, and the same rule in the gtk3 and
  gtk4 bars). gtk4sermo did not even write a picture when a bar was present.
- **htmlsermo: numbers in the page are written with a dot.** Under a
  comma-decimal locale (fr_FR), `value="0,65"`, `step="0,5"`: browsers rejected
  them (empty bar, slider at the start).
- Two tests: `garde_html_valeurs_page.sh` (bar and slider in the page, under
  fr_FR) and `garde_render_barre.sh` (the 65 % bar does not look like the 0 %
  bar, on the seven graphical ports); each is red on the 2.8.2 binaries.
- **ncursessermo: `<edit>` is a multi-line area.** It fitted on one line
  ("[text]") and a multi-line text only showed its beginning. It is now a box of
  at least three lines, ten at most, that takes all its room when its column
  stretches it, like the reference's white area. Typing is still one line at
  the bottom of the screen.
- **Slackware 15.0: gtk3 and lvgl build.** Without gtk-layer-shell (missing from
  Slackware 15.0), the SlackBuild builds gtk3 without the Wayland anchoring and
  says so; lvgl takes `linux-dmabuf` from `unstable/` when wayland-protocols is
  old, and links `librt` when `shm_open` is not in libc (glibc 2.33). Man pages
  are no longer duplicated in the packages (`/usr/man` and `/usr/share/man`).
  Built, installed and run in a container.

- **Widgets in reverse with clang.** `push(_sum(pop(), pop()))` left the order
  of the two `pop()` calls to the compiler, which C does not fix. Built by clang
  (Termux, BSD, macOS…), every container laid out its children in reverse:
  initial focus on the last widget, notebook opened on the last tab, `<list>`
  never selected. The old `#ifdef __arm__` workaround is removed.
- **GTK 4: a read-only property** was never refused (`!flags &
  G_PARAM_WRITABLE` is always 0); it now is, as with GTK 3.
- **efl1 and lvgl with no reachable display**: abort (efl1), segmentation
  fault (lvgl). Now "cannot open display" and exit code 1, like GTK. lvgl
  opens the display only once: its connection is handed to LVGL's X11 driver
  (the first guard opened, closed, then LVGL reopened unchecked — occasional
  crash under the test benches).
- **lvgl on Android**: `shm_open`/`shm_unlink`, missing from bionic, are
  provided through `memfd_create` to LVGL's Wayland driver, without touching
  its code.
- **fltk1**: FLTK 1.4 required at configure time (with FLTK 1.3 the failure
  only came at compile time); `<terminal>` found with a separately installed
  FLTK; an `-include` coming from `fltk-config` no longer breaks the build.
- **Decimal numbers on FreeBSD and OpenBSD**: `strtod_l` was not declared
  (FreeBSD) or does not exist (OpenBSD); every decimal number in an XML would
  have been wrong.
- **Build without a single warning**, with gcc and with clang, for the core,
  the nine ports and sermoia-mcp. `ci/construire.sh` counts warnings whatever
  the language (bison's, in French, counted as zero). lvgl requires
  `wayland-protocols` at configure time.
- **Test benches**: `test_safe_exec` no longer assumes `/tmp` or `fopencookie`;
  the unreachable virtual display of GTK 4 and SDL 3 is recognised and the run
  retried. Its cause is found: Xvfb, started without `-noreset`, reset itself
  when its last client left and refused connections meanwhile; the benches now
  start it with `-noreset`.

### Measured

- Debian (gcc), `ci/bancs.sh` (14/10): 208 benches; on the joined line, 207
  green, then 208 out of 208 at 3.0.0. The first run's red one comes back now
  and then: lvglsermo stops on a segmentation fault when the X display does
  not open (LVGL does not check the opening); a defect older than this
  release, being fixed.
- ASan and UBSan (15/10): the nine ports built under both, behaviour bench
  run with fatal errors: **no error**, parity on all nine (the lvgl UBSan
  error, an integer overflow in a grid, seen on 14/10, is fixed). Leaks,
  measured on ncurses and html: about 31 bytes per widget refresh (variable
  export), being fixed.
- Weight (binary without symbols, as in the package): from 343 KiB (efl1) to
  1,167 KiB (sdl3) and 2,399 KiB (lvgl); one ceiling per port, checked by a
  bench.
- Android 15 arm64 (Termux, clang 21): nine targets out of ten built;
  behaviour bench 61/61 for gtk3, gtk4, qt6, sdl3, lvgl, ncurses, html and
  fltk1, under Xvfb.
- FreeBSD 15.1 and OpenBSD 7.9, in a virtual machine: ncurses 61/61; job
  scripts and toolboxes, 187 dialogs identical to gtk3sermo.
- Windows: behaviour bench 61/61 under Wine 10. On a real Windows 11: display,
  accents and output to a file. On a real Windows 10: commands (`cmd.exe`, and
  busybox-w32 through `SERMO_SHELL`), accents and opening an `https` link; a
  `calc.exe` is refused.

## [2.8.2] — 2026-10-11

PATCH release. Sixty job scripts and four toolboxes, now the acceptance test of
the ports, found gaps between the ports and the gtk3sermo reference: they are
fixed. Both MCP servers become packages.

### Security

- **sermoia-mcp: `sermo_render` ran the XML's commands.** An `<input>` ran
  during rendering: asking for a preview was enough to launch a command.
  Rendering now sets `SERMO_ALLOWED_CMDS` to a command that does not exist,
  which forbids everything (`sh -c` fallback included); the test checks it with
  a witness file. Only `sermo_run` runs commands, and says so.

### Measured

- `ci/bancs.sh`: 177 tests, 174 green on the first run, the three red ones
  explained and settled; `examples/metiers/verifie.sh`: 104 scripts, 187
  dialogs, nine ports, 0 gap.
  Details: [BILAN_SANTE.md](BILAN_SANTE.md) (French only).

### Added

- **Sixty job scripts** (`examples/metiers/`): five per job, for twelve jobs
  (system administration, development, support, integration, kiosks, workshop,
  small business and non-profits, teaching, multimedia, security, headless
  server, local web). Ready to run, safely: by default the command is printed
  instead of being run (`SERMO_A_BLANC=0` to act). They are also the **ports'
  acceptance test**: `examples/metiers/verifie.sh` plays each dialog on the
  nine ports and compares the values with gtk3sermo. Public domain (CC0-1.0).
- **Four toolboxes** (`examples/boites-a-outils/`): system (display, sound, Wi-Fi,
  time, keyboard…), server (nginx, certificates, SSH, fail2ban, log rotation,
  shares…), administrator (groups, permissions, packages, fstab, formatting,
  sudo…) and user (search, archives, password, reminder, printing…). Each: a
  launcher and ten tools. Same rules as the job scripts (dry run by default,
  confirmation before a destructive action, no `sudo` or `pkexec`, checked
  input); reviewed for security; played by the same bench on the nine ports.
  Public domain (CC0-1.0).
- **Signed checksums**: `SHA256SUMS.asc`, with the haplo-dialog key
  (`devel@haplo-dialog.fr`, fingerprint `66C7 F4AD 7A58 A037 967F 8339 3799 9F99
  7570 F1ED`, in the README); the public key is on the website.
- **`sermoia-mcp` and `sermoman-mcp` packages**: both MCP servers install
  (`/usr/bin/sermoia-mcp`, `/usr/bin/sermoman-mcp`). sermoia finds its
  installed catalogue, job scripts and toolboxes included; sermoman reports its version once
  installed. Job scripts and toolboxes ship in `sermo-doc`.
- **sermoman-mcp: `sermo_scripts`**, the inventory of the 104 scripts (need,
  wrapped command, suggested port, path), generated from their headers and
  included in the search; the "truth" test fails when it is out of date.
  **sermoia-mcp**: the catalogue has 16 topics (12 jobs, 4 boxes) and 187
  dialogs, and its test fails when it is out of date.

### Improved

- **sermoia-mcp**, behaviour:
  - timeouts on every tool, killing the whole process group (`xvfb-run`
    included); `sermo_run` takes a `timeout` argument;
  - `sermo_render` showed text instead of the picture of a small dialog (PNGs
    under ~800 bytes were discarded): the picture is judged by its header;
  - `sermo_run` without a display reports the error instead of "no value";
  - `sermo_check` returns "OK" or "ERREUR" with the parser's message, without
    the graphics libraries' noise;
  - `sermo_examples` searches by words ("contact form") in names and contents,
    ranks by relevance then vote, and gives the catalogue map without argument;
  - `sermo_vote` refuses a topic outside the catalogue;
  - protocol: `ping`, correct JSON-RPC error codes, versions 2025-06-18 and
    2025-03-26, `instructions` field, annotated tools (read-only, destructive);
  - temporary files in a private directory.
- **sermoia-mcp**, documentation: README rewritten (each tool with its
  arguments, answers and errors; recommended loop; security; timeouts;
  declaring it in Claude Code) and an English `README.en.md`. Test extended to
  five checks (picture, witness command, timeouts); four code mutations each
  make it fail.

### Fixed

- **ncursessermo: the output variables were wiped.** Run on a terminal without
  capture, it printed `NAME="…"` and `EXIT="…"` while the alternate screen
  was still open: they vanished with it. The contract gains an optional hook,
  `sermo_be_before_export()`, called before the export; the ncurses port
  restores the screen there. The other ports have nothing to provide.
- **`--do` receives `EXIT`.** The manual shows it (`[ "$EXIT" = "valide" ]`), but
  only the widgets' variables were exported: `EXIT` arrived empty, and a script
  could not tell which button had been clicked. It is now exported on every exit
  (button, close, abort), in the core and in the ports that print `EXIT`
  themselves. Found by the first job script.
- **efl1: a field's value came out as markup.** `<entry>`, `<password>` and
  `<searchentry>` returned "&laquo;" for "«", "&amp;" for "&"; a `<` in a default
  value was read as a tag. Plain text on write and read, as `<edit>` does.
- **XML entities decoded** in text and attributes: `&lt;` `&gt;` `&amp;`
  `&quot;` `&apos;`, `&#NN;` and `&#xHH;`. `<default>a &amp; b</default>` gave
  "a &amp; b"; it now gives "a & b", as any XML reader does. An unknown entity is
  left as is; a quoted string is not touched. A script that escapes its values
  (`&` → `&amp;`) now gets the original value back.
- **A value written on several lines keeps all its lines** (`<default>`,
  `<label>`…): only the last line was kept.
- **First row chosen as in the reference, on all nine ports.** `<list>`, `<tree>`
  and `<table>` choose their first row only when they are the first widget to
  take the focus in their window (visible and active); otherwise nothing is
  chosen. `<default>` chooses the wanted row everywhere, gtk3 and gtk4 included
  (gtk3 ignored it on `<list>`).
- **Radio button groups as in the reference, on all nine ports.** The first
  button of a group is checked when none has `<default>` (gtk4, qt6, fltk1,
  sdl3, lvgl, ncurses and html checked none); efl1 put every button of the
  window in a single group. A new container starts a new group.
- **Boxes laid out like the reference, on all nine ports.** `homogeneous="true"`
  gives equal cells over the whole length, a child that does not fill its cell
  being centred in it; `space-expand` / `space-fill` set on a `<vbox>` or an
  `<hbox>` apply to its children, with gtk3's exact rule (carried from one child
  to the next, defaults by type); an `<hbox>` packs whatever does not stretch to
  the right. Before, most ports ignored these settings: columns squeezed to one
  side, OK/Cancel buttons misplaced, frames not stretched.
- **Icon button: the icon and the label form one centred group** in the button
  (gtk3 pushed it right, gtk4 left); `image-position` is honoured everywhere.
- **Timers**: ncurses fired every `<timer>` on every frame (not at its
  interval); without a display, ncurses and html fired them in document order
  instead of shortest first. `milliseconds="true"` is read as a boolean on the
  four neutral ports.
- **fltk1: `<spinbutton digits>`** shared one format buffer between all fields:
  the last one created imposed its decimals on the others.
- **htmlsermo, `serve` mode**: `--do` receives the values sent by the browser (it
  received the defaults); `EXIT` is the button's exit (`exit:X` → "X"), no longer
  its label; a button whose actions do not close the dialog no longer submits
  the form.
- **gtk3: `<table>` is no longer sorted on its own.** Its rows were ordered
  by the first column on opening, even without `auto-sort`: the document order
  was lost, and the first chosen row differed from the other ports. As in
  gtkdialog, sorting happens only with `auto-sort` or on a header click.
- **`<text>` returns the text shown, on the nine ports**, as gtkdialog does: its
  label ("text" when it has none), then what `<input>` read. The neutral ports
  returned an empty value, and qt6, efl1, sdl3, lvgl, ncurses and html showed a
  `<default>`, which `<text>` does not have.
- **`<tree exported-column="N">` on the nine ports.** Only gtk3 and gtk4
  returned the requested column; the seven others always returned the first.
  Found by the "user" toolbox (the folder of a file found).
- **fltk1: an `<item>` containing "/" became a submenu** (`<comboboxtext>`,
  `<combobox>`): "Europe/Paris" returned "Paris". Items are added as they
  are, as `<input>` lines already were. Found by the "system" toolbox (time
  zone).
- **htmlsermo: the page has the shape of the window.** The window takes its
  width (`default-width`, `width-request`) instead of a fixed 640 px; a
  horizontal box no longer wraps and packs its children from the end, as
  gtkdialog does; what stretches (`space-expand`, and by default `<entry>`,
  `<edit>`, `<frame>`) takes the free space; tabs follow `tab-pos` (left, right,
  bottom); an image takes the requested size (`icon-size`, `<width>`/`<height>`)
  and keeps its ratio; a text keeps its line breaks and is centred as in GTK; a
  missing icon leaves an empty space, as gtk3 does. Reported on the website's
  "A complete system tool" picture.
- **Theme icons: looked up as GTK does, on the neutral ports.** The person's
  theme, then Adwaita, then hicolor; for each name, the `-symbolic` variant, then
  the generic name (`network-wired` → `network`). With no theme set (SSH, empty
  session), most icons were missing: Adwaita only has the symbolic variant of
  many of them. htmlsermo tints a symbolic icon with the text colour, as GTK does.
- **Icons: ports that could show them and did not** (found on the website's
  gallery): gtk4 showed a broken image in a button (it first asked the theme
  whether it had the icon, and the answer was always no); qt6 looked up an
  `<image icon-name>` only through Qt (one in five found) and now uses the core
  lookup, like its buttons; efl1 recolours symbolic icons in a light colour on
  its dark theme, where they were nearly invisible; lvgl draws SVG icons
  (ThorVG, shipped inside LVGL — the port now contains C++) and button icons,
  and tints a symbolic icon with the text colour. The core gains
  `sermo_icon_symbolique_teinte()`.
- **lvgl: tabs and stretched items.** A tab bar on the side takes the width of
  the longest label (36 px cut "Systeme" into "ystem") and its tabs keep their
  height, at the top; `tab-pos` right and bottom are read. An item with
  `space-expand` keeps at least its natural size: in a box sized to its
  content, LVGL squashed it to 0 px (the "Actions rapides" buttons and the
  "dmesg" frame of system-tools disappeared). As in gtkdialog, a row packs
  what does not stretch to the right, `<entry>`, `<edit>` and `<frame>`
  stretch by default, a column stretching in a row gives its width to its
  children, and the frames of a tab page fill its height; a bar, a slider or a
  field keeps its own height.
- **sermoia-mcp: `sermo_examples` ranks each example** (topic name, file name,
  content), no longer the whole topic: in a toolbox, "fail2ban" first showed
  the nginx site and the certificates. The test checks it.
- **fltk1: a row of columns (icon and label) overflowed** under the next
  frame: its height was read after the row had reset its children to its own
  height. Seen on the website's gallery ("Buttons and icons").
- **gtk3, gtk4: a label that is not valid Pango markup** is shown as plain text,
  without a warning.

## [2.8.1] — 2026-10-10

PATCH release. A review of the website screenshots, port by port against the
gtk3sermo reference, found widgets drawn wrong or not at all that the tests
did not see: they read values, not pictures.

### Security

- **lvgl: `<searchentry>` wrote to the wrong place.** Typed text went to
  `state.entry.buf`, which shares its memory with `state.list.items` (a
  pointer): it overwrote it. It now goes to `state.list.filter`, as on the
  other ports.

### Fixed

- **`<tree>` in columns, under its headers, on every neutral port** (qt6,
  fltk1, efl1, sdl3, lvgl, ncurses, html). The raw "a|b|c" line was shown;
  fltk1 even read the "/" of a path as a tree level. The exported variable is
  still the first column, as in the reference.
- **`<statusbar>`**: `<default>` wins over `<label>`, as in the reference. The
  bar stayed empty on the seven neutral ports; on qt6 the text was then drawn
  twice on top of itself.
- **`<text>`** shows what `<input>` read (lvgl, html, sdl3, ncurses), without
  the trailing newline (fltk1 showed "^J"); the variable keeps the whole text.
- **`<expander label="…">`**: the attribute is read by the seven neutral
  ports; sdl3 honours the open state; ncurses draws the title, which Enter
  opens and closes.
- **`<levelbar>`**: `range-min` / `range-max` are read (lvgl, html, ncurses,
  sdl3).
- **`--render-png` exits with code 0** on fltk1, qt6 and efl1. The picture was
  written, but the parser then started again and ended on "syntax error near
  `<window>`", code 1.
- **lvgl**, now drawn or redrawn: `<edit>` (invisible), `<calendar>`,
  `<pixmap>` and `<image>` (PNG), `<togglebutton>` (a pressed button, no longer
  a check box), `<linkbutton>` (underlined link), `<menubutton>` (drop-down),
  `<stack>` and `<wizard>` (one page at a time, wizard buttons), `<grid>`
  (rows), `<overlay>` (stacked layers), `<vscale>` (vertical), `<infobar>`
  (tinted by `message-type`), `<aspectframe label>`, vertical separator in a
  row, round and exclusive radio button, a single chosen row in `<list>`. A
  box's `width-request` is a minimum, and a requested window size no longer
  shrinks below its content: the 480×420 form was cut under its buttons.
- **html**: vertical `<vscale>`, `<infobar>` tint, `<aspectframe label>`.

### Measured

- `ci/bancs.sh`: 176 tests, 175 green on the first run; the red one (a guard
  that did not find its window in time, a test defect that also hits 2.8.0)
  is green when replayed alone. Behaviour 61/61 at parity on all nine ports.
  Details: [BILAN_SANTE.md](BILAN_SANTE.md) (French only).

## [2.8.0] — 2026-10-10

MINOR release. Two more ports — nine in all —, a second MCP server, package
recipes for six other systems, and the history of gtkdialog.

### Added

- **`htmlsermo`, the HTML port.** It writes the dialog into a standalone HTML
  page (embedded CSS, no external file; no JavaScript, except the `interactive`
  mode, which composes the result inside the page) instead of opening a
  window: `SERMO_HTML_OUT` picks the file (default `sermo-output.html`).
  `SERMO_HTML_MODE=serve` serves the page on the loopback and returns the entered
  values, like a real dialog window. No graphics library: it installs on a
  machine without a display. Package `sermo-backend-html`; `sermo` alternative at
  priority 1, it only takes it when installed alone.
- **`lvglsermo`, the LVGL port.** LVGL v9.6.0, embedded (not in Debian), MIT.
  Wayland or X11; framebuffer and evdev input on request only
  (`SERMO_LVGL_FBDEV`, `SERMO_LVGL_EVDEV`). Package `sermo-backend-lvgl`;
  priority 8.
- **`sermoia-mcp`**, a second MCP server, next to `sermoman-mcp` which serves the
  manual: it lets an AI write, check and see a dialog (`sermo_schema`,
  `sermo_check`, `sermo_run`, `sermo_render`, `sermo_examples`, `sermo_vote`).
  Its schema is checked against the parser's tags; it is not packaged. Test:
  `sermoia-mcp/tests/verifie.sh`.
- **Recipes beyond Debian** (`packaging/autres-os/`): Arch, RPM, Gentoo,
  Slackware, Alpine and NixOS, on a single build sequence (`construire-os.sh`).
- **`HISTOIRE.en.md`**: the history of gtkdialog, from László Pere to today,
  with sources.

### Fixed

- **Attributions**: `gtk3sermo`'s Wayland anchoring was written in gtkdialog at
  woof-CE (Puppy Linux), not at BunsenLabs; Thunor maintained gtkdialog from 2011
  to 2013; 0.8.3 dates from January 2013.
- **SECURITY.en.md** said `safe_exec.c` came in two copies: there is only one.

### Measured

- `htmlsermo` and `lvglsermo`: behaviour test 61/61, XML test 55/55. The guards
  that click or measure a window do not apply to html.

## [2.7.7] — 2026-10-04

PATCH release. An out-of-bounds read in the progress bar, and three fixes the
GTK4 core did not have yet.

### Security

- **Progress bar (gtk3, gtk4): out-of-bounds read.** The thread reading the
  `<input>` of a `<progressbar>` stripped the trailing newline without checking
  that the line was not empty. A line starting with a NUL byte made it read one
  byte before the buffer, and write it if that byte was a newline. This content
  comes from a command, hence from data the script author does not always
  control. Measured under AddressSanitizer on both ports
  (`stack-buffer-overflow`, `READ of size 1`); a guard closes the defect.

### Fixed

- **GTK4: two windows open at the same time with the same variable.** The second
  one took over the first one's variable and silently overwrote it: on exit, the
  variable carried the value of the wrong window. It is now stored as
  `NAME__W<id>`, with a warning — which gtk3 already did.
- **GTK4: a widget's `<default>` value was not visible to a neighbour's first
  `<input>`.** An `<input>` command reading `$A` found an empty variable where
  gtk3 found the default value of `A`.
- **GTK4: `g_setenv()` replaces `putenv()`** when exporting a variable (memory
  leak, and an environment entry pointing to a heap string).

These three fixes existed in `libsermocore/src`; they were missing from the
`libsermocore/src-gtk4` copy.

### Tests

- `tests/garde_progressbar_ligne_vide.sh`: static check with a witness (on an
  ordinary binary, reading one stack byte goes unnoticed).
- `tests/garde_fenetres_homonymes.sh` (gtk3, gtk4) and behaviour case
  `54-defaut-vu-par-input`, green on all seven ports. All three are red on the
  old code.

## [2.7.6] — 2026-10-03

Security PATCH release. Bypass of the `SERMO_ALLOWED_CMDS` allowlist through the
environment.

### Security

- **A widget variable named `LD_PRELOAD` got past the allowlist.** A script could
  name a variable `LD_PRELOAD` (or `LD_LIBRARY_PATH`, `LD_AUDIT`…); its value was
  exported, then inherited by the allowed command, which then loaded arbitrary
  code. When the allowlist is active, every `LD_*` variable is now removed from
  the command's environment and `PATH` is pinned to `/usr/bin:/bin`. Without an
  allowlist, nothing changes.

### Internal

- **Single source, first step.** The `WIDGET_*` enumeration lives in one header
  (`libsermocore/include/widget_types.h`) instead of being copied into every
  port, and five core files (`safe_exec`, `stack`, `attributes`, `stringman`,
  `actions`) no longer have a GTK4 copy: the GTK4 variant builds them from
  `libsermocore/src`. No behaviour change expected.

## [2.7.5] — 2026-10-01

Security PATCH release. Heap buffer overflow in `linecutter()`, inherited from
gtkdialog.

### Security

- **`linecutter()` overflowed beyond 127 separators (CWE-787).** This function
  splits `|`-separated values: `<table>` and `<tree>` columns, `<colorbutton>`,
  among others. It allocated 128 slots and never checked that limit; a value with
  127 separators or more wrote past the array. That content can come from a
  command or a file, hence from data the script author does not always control —
  which is what makes it a security risk, not merely a defect. The array now
  grows as needed. Fixed in **both** copies of the core (`libsermocore/src` and
  `libsermocore/src-gtk4`).

### Tests

- Regression unit test (`linecutter` over 200 fields), verified **red on the old
  code** under AddressSanitizer, then green on the fix.
- `run_unit_tests.sh` now exercises `stringman` against both copies of the core,
  as it already does for `safe_exec`.

## [2.7.4] — 2026-09-30

PATCH release. A gtk4 `<calendar>` could set the wrong month, depending on the
day the dialog was opened.

### Fixed

- **gtk4 — a `<calendar>` set the wrong month on days 29 to 31.** A `<default>`
  of 2000-02-29 gave 2000-09-29 on 30 September. The calendar starts from today,
  then receives the year, the month and the day, one at a time: starting from
  30 September, it went through 30 February 2000, which GTK silently refuses, and
  the month stayed September. Depending on the current day and the requested
  date, month ends and 29 February were affected; so was the reset to today. Day
  1, which exists in every month, is now set first. The six other ports were
  right. Seen on 2026-09-30 by the GitLab CI: the behaviour test was green the
  day before, on the 29th.

### Added

- **A calendar guard** (`tests/garde_calendrier_date_du_jour.sh`), run on the
  seven ports: it sets the binary's current date (`faketime`) to 31 January,
  30 September and, as a control, 15 June. On 2.7.3, gtk4 fails on both trap
  days.
- **CI: three missing packages** in `ci/dependances.txt`: `imagemagick` (the
  window-size guard reads the capture with `identify`), `libevas1-engines-x`
  (only "recommended": without it, EFL creates no window) and `faketime`. The
  workstation had them; the CI, never run before publication, failed 13 tests.

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

## [2.7.1] — 2026-09-17 — The terminal, Wayland anchoring and `<input>` keep their word

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

## [2.7.0] — 2026-09-16 — Glade is back, and the progress bar moves everywhere

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

## [2.6.8] — 2026-09-16 — Benches that no longer say green by mistake

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

## [2.6.7] — 2026-09-16 — Promised protections, made real

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

## [2.6.6] — 2026-09-15 — Five hundred and eighty-six dated headers

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

## [2.6.5] — 2026-09-15 — Three defects stacked under one click

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

## [2.6.4] — 2026-09-15 — The title the click erased

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

## [2.6.3] — 2026-09-15 — The expander born on the wrong side

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

## [2.6.2] — 2026-09-15 — The frame that kept its title quiet

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

## [2.6.1] — 2026-09-15 — What the changelog had forgotten

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
  ⚠️ **The fix has been in the tree since the 14th**, before the 2.6.0 entry
  itself — it had simply never been written down. It is written here: a fix that
  cannot be found in the changelog does not exist for whoever reads it later.

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

## [2.6.0] — 2026-09-14 — The flow, the overlay and the curtain

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

## [2.5.0] — 2026-09-14 — Four more tags

MINOR release: **four new tags** — `<toolbar>`, `<stack>`, `<wizard>`,
`<menubutton>` — on the seven ports.

### Added
- **`<toolbar>`**: a row that calls itself a toolbar. ⚠️ No `GtkToolbar`: GTK 4
  **removed** it and GTK 3 deprecates it — a `GtkBox` with the CSS class
  “toolbar” gives the same rendering, the same code on both sides, and no
  condemned API. No `elm_toolbar` either: it arranges ITEMS, not arbitrary
  widgets.
- **`<stack>`**: N pages, one visible — `<notebook>` without the tabs. `page=`
  picks the initial step, `switcher="true"` adds a row of buttons. Export: the
  index of the visible page.
- **`<wizard>`**: a sequence of steps with Previous / Next / Finish.
  ⚠️ **Composed, not borrowed**: GTK 4 removed `GtkAssistant`, and `QWizard` is
  a **window**, not a container. Each port assembles it with ITS own page
  mechanism. “Finish” plays the tag's `<action>` and closes nothing by itself.
- **`<menubutton>`**: a menu WHERE YOU ARE, not only at the top of the window.
  Its children are ordinary `<menuitem>`. Export: the label of the last item
  chosen.
- **Four behaviour bench cases** (33 to 36): corpus 32 → **36 cases**.

### Fixed
- ⛔ **Name collision with the gtk4 port.** It ALREADY had a `widget_stack.c` —
  a native `GtkStack` exporting the page NAME — and the new tag overwrote it
  before anyone noticed. Restored; the common tag is now called
  `widget_stackpages_*` in all seven ports.
- ⛔ **Two `<stack>` rules in the lexer.** The old one was shadowed by
  the new one — two rules for the same pattern, the first one wins. Removed,
  with the reason written on the spot.
- **`<menubutton>` did not show on gtk3**: `gtk_container_add()` on a
  `GtkMenuButton` adds nothing (it already has its child, the arrow).
  `set_label` is the right door.
- **gtk3's `<wizard>` stayed invisible**: a `GtkStack` only picks a page among
  the children that are **shown**, and the core only shows the tree at the end.
  Found on capture, not at the bench — the export itself was correct.

### What this brought to light
- ⚠️ **`<stack>` already existed**… but only worked on gtk4: on the six other
  ports the neutral core had no `case` at all — the tag parsed and produced
  **nothing**. `<flowbox>`, `<overlay>` and `<revealer>` were found to be in
  that same state (see 2.6.0).

## [2.4.0] — 2026-09-14 — One number, one spelling

MINOR release: the **export behaviour changes** for four tags — `<hscale>`,
`<vscale>`, `<spinbutton>` and `<progressbar>` — on the seven ports. No new tag.
It is the direct sequel to 2.3.1: the same disease, the rest of the family.

### Changed — what a script used to receive, and what it receives now
- **The number is written with a DOT, everywhere and always.** The reference
  port wrote through `g_snprintf("%.2f")`, which follows the locale: under
  `fr_FR` an `<hscale digits="2">` rendered “2,50”, under `LC_ALL=C` “2.50”.
  Same binary, same script, two outputs. Now `g_ascii_formatd` — verified under
  `fr_FR`, `C` and `de_DE`.
- **`digits=` decides the decimals, on all seven ports.** The reference already
  honoured it; the six others ignored it. Measured before the fix on
  `<hscale digits="2"><default>2.5</default>`: the reference rendered “2,50”;
  `qt6`, `efl1`, `sdl3` and `ncurses` rendered “2”; `fltk1` rendered “2,5”.
- **`<progressbar>` exports NOTHING**, like the reference — it is a display, not
  an input. Five ports rendered “50”.
- **`<spinbutton>` without `digits=` renders an integer.** `qt6` started from TWO
  decimals by default (“4.25” instead of “4”); the neutral ports rendered “%g”,
  which ignores the attribute.

### Fixed
- **`qt6` lost the decimal part at creation**: `QSlider` only knows integers, and
  the port called `setValue((int)value)`. The axis is now scaled by 10^digits —
  `digits=` finally has an effect on screen too. ⚠️ Rounding follows
  `nearbyint`, not “+0.5”: `printf` rounds to even (“%.0f” of 2.5 gives 2), and
  the reference does the same.
- ⛔ **`fltk1`: two distinct `SermoScale` types.** The core routes `<vscale>` to
  `widget_hscale_create` but its export to `widget_vscale_envvar_construct`: the
  object is created in one translation unit and read back in another. With a
  class defined in the anonymous namespace of EACH file, the `dynamic_cast`
  failed **silently** and the decimals were lost (`<vscale digits="1">` rendered
  “4” for 3.5). The type now lives in a shared header, `sermo_scale.h`.
- **`efl1`** stored its value without decimals: `digits` now travels inside the
  Evas object's data — it must follow the OBJECT, since creation and export live
  in two different files.

### Added
- **One behaviour bench case** (32-echelles-decimales): corpus 31 → **32 cases**.
  It pins the six values together, with and without `digits=`.

## [2.3.1] — 2026-09-14 — The value that depended on the machine

PATCH release: no new tag. `<levelbar>` and `<drawingarea>` rendered a different
value from one port to the next — and the reference port itself changed its
answer depending on the machine's LOCALE.

### Fixed
- **The reference exported a locale-dependent number.**
  `g_strdup_printf("%g", 0.5)` writes “0,5” under `fr_FR` and “0.5” under
  `LC_ALL=C`: same binary, same script, two outputs. A shell script comparing
  “0.5” no longer recognised anything depending on the machine. `g_ascii_formatd`
  everywhere — the rule of `g_ascii_strtod` applied to **writing**. Verified
  under `fr_FR`, `C` and `de_DE`: the output no longer moves.
- **`<levelbar>` was mute on four ports** (`qt6`, `efl1`, `sdl3`, `ncurses`): the
  widgets were ported, but the export dispatcher never called them — they fell
  into the `default:`. `efl1` even carried a note saying it was intentional; that
  was honest, and wrong as a policy.
- **`<levelbar>` rendered anything at all elsewhere**: `fltk1` truncated to the
  integer (“0” for 0.5) *and* read the value with `atof`, which returns ZERO
  under a French locale; `qt6` exported its bar's internal **fraction** (0.005)
  instead of the value written in the script; `efl1` rendered “0.500” where the
  reference renders “0.5”.
- **`<drawingarea>` was chatty**: `gtk4` rendered its dimensions (“40:20”),
  `sdl3` and `ncurses` rendered “canvas”. The reference renders an empty string.
- ⛔ **`<progressbar>` made the fltk1 port ABORT** when it had no tag attribute
  at all: `get_tag_attribute()` asserts that `attr` is not NULL. The most
  ordinary tag there is, and the program stopped.
- **Sixteen more `atof` in fltk1** (`hscale`, `vscale`, `spinbutton`,
  `progressbar`, `timer`): the same locale trap, replaced by `g_ascii_strtod`.

### Changed
- ⚠️ **The forbidden-functions guard was not looking at C++.** It walked `*.c`
  and `*.h` only — yet `qt6` and `fltk1` have their ENTIRE widget layer in
  `.cpp`. It announced “no forbidden call” without having opened the files. It
  now takes `*.cpp`, `*.cc` and `*.hpp` — and it immediately found the sixteen
  `atof` above.
- **One behaviour bench case** (31-levelbar-drawingarea): corpus 30 → **31
  cases**.

## [2.3.0] — 2026-09-14 — The handle

MINOR release: **one new tag**, `<paned>`, backward compatible. Both
planned containers are delivered.

### Added
- **`<paned>`, on the seven ports.** Two areas and a handle the **user** drags:
  `<hbox>` freezes the split when the script is written, `<paned>` lets it move.
  Attributes: `orientation` (horizontal by default | vertical), `position`
  (pixels or `%`), `resizable`.
  ⚠️ **Exactly two children**; a third is **refused with a message**, never
  swallowed in silence — that was `<eventbox>`'s defect.
- Per port: `GtkPaned` (gtk3, gtk4), `QSplitter` (qt6), `Fl_Tile` (fltk1),
  `elm_panes` (efl1), a draggable handle built from an `InvisibleButton` (sdl3),
  an arrow-key movable boundary (ncurses).
- **One behaviour bench case** (30-paned): corpus 29 → **30 cases**.

### Fixed
- ⛔ **The content of an `<edit>` was read at the wrong place in the union**, on
  the `ncurses` port: `state.text.content` is a HEAP POINTER, `state.entry.buf` a
  1,024-byte array **at the same location**. Reading `entry.buf` returned the
  pointer's bytes then read past them until the first zero — a **read
  overflow**. Measured: `value="\xef\xbf\xbdjU"` instead of the text. Worse,
  typing WROTE into `entry.buf`, overwriting the pointer (a leak, then a
  `free()` on a fabricated pointer). Reading and writing now both go through
  `state.text`.

## [2.2.0] — 2026-09-13 — A grid, at last

MINOR release: **one new tag**, `<grid>`, backward compatible — no existing
script changes its rendering.

### Added
- **`<grid columns="N">`, on the seven ports.** Children are laid out in flow
  (document order, wrapping every N) and the columns are **aligned from one row
  to the next**. Until now the language had 62 widget types and **two** layout
  containers: a form was written by stacking `<hbox>` inside a `<vbox>`, and each
  row sized itself on its own. Attributes: `columns`, `row-spacing`,
  `column-spacing`, `homogeneous`; `space-expand`/`space-fill` per child.
  ⚠️ `<grid>` is not `<table>`: `<table>` is the column list of data inherited
  from gtkdialog.
- Per port: `gtk_grid_attach` (gtk3, gtk4), `QGridLayout` (qt6), `Fl_Grid`
  (fltk1, new in FLTK 1.4), hand-aligned boxes (efl1), `ImGui::BeginTable`
  (sdl3), equal-width columns (ncurses).
- **Two behaviour bench cases** (28-grid, 29-grid-flot): corpus 27 → **29
  cases**. Written BEFORE the code, with `.attendu` files produced by the gtk3
  reference.

### Fixed
- **fltk1 swapped `<levelbar>` and `<drawingarea>`.** Its local copy of
  `automaton.h` gave 0xBE to DRAWINGAREA and 0xBF to LEVELBAR — the opposite of
  the core, which creates widgets with ITS values. Measured: the reference
  renders `LB="0,5" DA=""`, fltk1 rendered `LB="" DA="0"`, both values crossed.
  Same family as the two-enum bug of the GTK4 variant (fixed on 2026-09-09).
  ⛔ A local copy of `automaton.h` NEVER renumbers.
- **Installing the GTK4 variant's headers silently skipped files.**
  `file(INSTALL)` compares TIMESTAMPS, not content: since `include/` was
  installed just before with the same names, an older `automaton.h` from
  `src-gtk4/` was ignored — and the gtk4 backend compiled against the
  reference's enum. Observed symptoms: `<filechooser>` rendered a date,
  `<password>` rendered “1”, `<calendar>` rendered empty. The core's
  `CMakeLists.txt` now deletes by name the headers that `src-gtk4/` is going to
  provide.

## [2.1.1] — 2026-09-13 — The tags that were pretending

PATCH release: no new tag, no XML language change — what was advertised is now
**made true**. Three backends carried a `widget_stubs.c` file: the tag was
accepted, printed a warning, and did nothing. Two more (`qt6`, `fltk1`) had no
stub file but the same hole, **invisible for lack of a bench case**. Three test
cases were added (25-eventbox, 26-linkbutton, 27-pulse): they measured the gap
before the fix, and they guard it after.

> Note: this English changelog has no 2.1.0 entry — the mirror lags behind the
> French one, which is the master. See [CHANGELOG.md](CHANGELOG.md).

### Fixed
- **`<eventbox>` dropped its content.** On `efl1`, `sdl3` and `ncurses` the
  stub never popped the stack: the wrapped child simply vanished. On `fltk1`
  only the FIRST child was kept, crushed to 100 x 30 — a wrapped `<frame>` went
  off screen. All four ports now wrap their content like a `<vbox>`.
- **`<linkbutton>` did not export its URI.** Five ports out of seven returned
  `LB=""` where the gtk3 reference returns the address: three through stubs,
  `qt6` by exporting the displayed LABEL instead of the URI (and reading it
  from `<input>` instead of `<default>`), `fltk1` by never reading the URI and
  doing nothing on click.
- **`<pulse>` did not export `pulse`.** Same count: five ports returned an empty
  string. `qt6` routed its export to the `<spinner>` one; `fltk1` only drew a
  label, with no bar.
- **`<image>` exported an empty string** on `efl1`, `sdl3` and `ncurses`: the
  export dispatcher short-circuited the real widget, which existed. It now
  returns the **icon name**, like the gtk3 reference.

### Added
- **`<filechooser>` really picks a file.** `sdl3` opens the **system native
  picker** through `SDL_ShowOpenFileDialog` / `SDL_ShowSaveFileDialog` /
  `SDL_ShowOpenFolderDialog` (XDG portal under Wayland) — SDL 3.2 API, no added
  dependency. `efl1` opens `elm_fileselector_button`. `ncurses` opens a real
  **in-terminal file browser** (arrows, `..`, `h` for hidden files, Esc to give
  up). Tag attribute `action`: `open` (default), `save`, `select-folder`.
- **`<pulse>` animates** everywhere: `elm_progressbar` pulse mode (efl1),
  `ImGui::ProgressBar` with a negative fraction (sdl3), a hand-drawn block going
  back and forth (fltk1, ncurses).
- **`<linkbutton>` really opens the link**: `SDL_OpenURL` (sdl3), `xdg-open`
  spawned **without a shell** (fork + execlp, URI as `argv[1]`) for `ncurses`,
  `efl1` and `fltk1` —
  a query URL commonly contains `&`, and going through `/bin/sh` would reopen an
  injection. A URI starting with `-` is refused.
- **Three behaviour bench cases** (25 to 27): the corpus grows from 24 to
  **27 cases**, replayed on all seven ports.

### Removed
- The `widget_stubs.c` files of the three backends that had one. The `<menu>`
  creator, which lived there without being a stub, moves to `widget_menubar.c`.

## [2.1.0] — 2026-09-12 — The house put in order

MINOR release: backward-compatible additions, no change to the core↔backend
contract nor to the XML language.

### Added
- `contract/`: the `sermo-contract.h` header (MIT) is **in the repository**.

### Fixed
- **A clone did not build.** Six backends included `sermo-contract.h` from
  `../../sermo-contract`, a neighbouring repository absent from any clone. Fixed
  by bringing the header in; its MIT licence is reproduced in
  `contract/LICENSE.MIT`.
- **`qt6` relieved of 9,198 dead lines**: nine upstream engine sources sitting in
  its `src/` and **never compiled** (the core comes from `libsermocore`). It now
  matches the level of its neighbours.
- **Three NUL bytes** in `dialog_state.h` files made those headers binary as far
  as git was concerned — no readable diff. Removed.
- File headers: 216 introduced themselves under a name that was not theirs
  (`sdl3dialog 1.0.0`, `SDL3/ImGui` inside the ncurses port…). A trace of the
  path they were built along, corrected.

### Ownership and provenance
- **602 files** carried a licence without saying who they belonged to. They now
  carry a copyright. **Zero orphans** across the five non-GTK ports.
- **Provenance measured**, no longer merely asserted: a file-by-file comparison
  against the upstream `gtkdialog 0.8.3` archive (GTK2 only, which is why the
  rendering layer of the five non-GTK ports is necessarily new). Table in
  `LICENCES.md`.
- `gtk3` has a **second upstream** on record: the Wayland `layer-shell` anchoring
  comes from the lineage taken up after Pere — copyrights Dima Krasner and Mick
  Amadio, preserved.

## [2.0.0] — 2026-09-09 — Modular edition

First version of the modular edition (not published). It is meant to replace the
monolithic 1.x lineage. The MAJOR bump reflects the **change of the core↔backend
boundary contract** (`sermo_backend.h`): a backend from the monolithic lineage no
longer links against the new core. The **XML language stays backward-compatible**:
gtkdialog / sermo 1.x scripts stay valid.

### Added
- **7th backend `ncurses`** (terminal, the lightest): TUI, headless path,
  behaviour parity 24/24 + XML 55/55; `SERMO_NCURSES_THEME`
  (light|dark|blue|grey) setting and image/icon suppression.
- **Light/dark theme per backend**, driven by `SERMO_DARK`
  (`sermo_desktop_is_dark()` in the core).
- `tests/run_examples.sh` bench: actually opens the 210 examples against a given
  binary (beyond parsing and the 24 behaviour cases).

### Changed
- **Phase 3**: the core↔backend contract is renamed `qt6_*` →
  **`sermo_be_*`** (the core no longer names any toolkit; loop =
  `sermo_be_run_loop`).
- **Version unified to 2.0.0** for the core and the seven backends shipped
  together.

## [1.1.1] — Modularisation (Phase 1)

The modular edition splits monolithic sermo into a single **core**
(`libsermocore`) and autonomous **rendering backends**, one per toolkit. The XML
language and observable behaviour stay identical to sermo 1.1.x.

### Added

- **`libsermocore`**: syntactic core (Flex/Bison lexer/parser) + hardened
  execution (`safe_system`/`safe_popen`), variables, actions, automaton — with
  **no graphical dependency** in the neutral variant.
- **Three core build variants** from a largely unified source: neutral (shim +
  stubs, for qt6/fltk1/efl1/sdl3/ncurses), `SERMOCORE_GLIB` (real GLib/GTK 3),
  `SERMOCORE_GTK4` (gtk4sermo's rewritten core files).
- **Seven backends** consuming the core via pkg-config (`sermocore.pc`).
- **Frontier contract** `sermo_backend.h`: hook `sermo_backend_toolkit_init()`
  replacing the hard-coded `gtk_init`.
- **Separate packaging**: `sermo-core-dev` (static + headers + `.pc`, depends on
  libc6 only) plus one `sermo-backend-<t>` per toolkit with **disjoint**
  dependencies (derived from `ldd` → `dpkg -S`). `sermo` alias via
  `update-alternatives`; the `gtkdialog` command ships in a separate
  `sermo-gtkdialog` package (link → gtk3sermo; Conflicts/Replaces/Provides
  `gtkdialog, gtk3dialog, haplo-dialog` to avoid a file clash with the original
  gtkdialog and BunsenLabs' `gtk3dialog`).

### Changed

- The GObject attribute bridge moves out of the core into each backend
  (`tag_set_property.c`).
- `main()` now lives in the core and calls the backend's init hook.

### Fixed

- **efl1**: footer clipping (fixed window) → default width/height treated as
  **minimums**, the window grows to content.
- **efl1**: `calendar` returned the date off by one day (broken `elm_calendar`
  `time_t` round-trip across DST) → canonical date kept outside that round-trip.
  efl1 back to 24/24.
- **gtk4**: 3 value discrepancies (`05-filechooser`→date, `11-infobar`→type,
  `13-password`→`1`) caused by **two distinct `WIDGET_*` enums** in the
  `SERMOCORE_GTK4` variant (the generated parser saw the reference `automaton.h`
  while gtk4sermo's core files saw their own; the 0xB4–0xC3 block is reordered).
  `GTK4SRC` put first on the include path → a single enum. gtk4 back to 24/24.
- **qt6**: missing button icons under bare Xvfb → `sermo_icon_lookup` fallback.
- **System theme**: shared `sermo_desktop_is_dark()` detection; each port follows
  the current system theme. efl1 stays dark by default.

### Port status (Phase 1)

All seven ports at full parity (24/24) — six in GUI, ncurses in a terminal
(behaviour bench replayed headless in batch mode). gtk4 runs on the dedicated
`SERMOCORE_GTK4` variant (GTK 4 dropped `gtk_main`/`gtk_socket`/the old event
model). See [BILAN_SANTE.md](BILAN_SANTE.md) (French only).

### Coming

- **Part 3**: generalise the `qt6_*` bridges to `sermo_be_*`, move the
  event loop into the backend. Enables reuniting gtk4 on the unified source, a
  clean shared `.so`, then process isolation (IPC).

---

## Upstream history

This modular edition continues the **sermo** lineage (itself from **gtkdialog**,
GPL-2.0-or-later). The monolithic sermo version history is kept in the original
sermo repositories. This modular edition restarts from functional level 1.1.1.
