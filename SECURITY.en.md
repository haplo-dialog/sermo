# Security — sermo

[Français](SECURITY.md)

sermo runs shell commands described in an XML script. Its security rests on
**where** that execution happens and **how** it is bounded. Most of it happens in
the **core**, `libsermocore`, shared by the nine backends: `safe_exec.c` exists
in a single copy (`libsermocore/src/safe_exec.c`), compiled into all three core
variants.

Two paths escape the core, and that is where the limits are: the `<terminal>` of
gtk3 and gtk4, and the `<linkbutton>` — see
[What these two variables do not reach](#what-these-two-variables-do-not-reach).

## Threat model

The **trust boundary is the local author of the XML script**. sermo is not a
sandbox for running hostile XML: whoever writes the script can already run
commands. sermo's job is to **not turn a user's input into code execution** the
author did not intend.

## Hardened execution (in the core)

- **`safe_system` / `safe_popen`** replace `system`/`popen`. `safe_system` opens
  a shell **only** if the command contains shell metacharacters; otherwise it
  runs directly via `argv[]`, no shell. The `/bin/sh -c` fallback is **logged**.
- **No** `strcpy`/`strcat`/`sprintf`: systematic bounding. The
  `tests/garde_fonctions_interdites.sh` bench runs on the core, the nine ports
  and also on `sermo-source` (the helper programs); the sound and video player
  code has had neither `strcpy` nor `strtod` since 2026-10-18.
- **Output escaping**: exported values (`VAR="…"`) escape the four characters the
  shell expands inside double quotes (`\`, `"`, `$`, backtick), so
  `eval "$(sermo …)"` does not re-execute what a user typed. Verified by
  `tests/garde_echappement_sortie.sh` (the "guard" bench, 4 cases: `$(`, `${`,
  backtick, and the `EXIT=` line). `\` and `"` are not played by this bench.
- **`--do`** is the recommended path when the dialog may be used by someone other
  than the author: values arrive through the environment and are **never** read
  back as code.
- **A Glade file is code** (`--glade-xml`, gtk3sermo and gtk4sermo): its signal
  handlers are commands. They go through the **same path** as `<action>`
  (`safe_system`, `SERMO_ALLOWED_CMDS`, `SERMO_NO_SHELL_FALLBACK`), and an
  interface file deserves the same trust as a dialog script — no more.

## Size of what an `<input>` reads

Since 2.7.3, each `<input>` — command or file — reads **at most 16 MiB**.
Beyond that, reading stops as if at end of file, and **one** warning goes to
standard error, never to standard output (which an `eval` reads):

    <input> « yes » : lecture arrêtée à 16777216 octets (limite SERMO_INPUT_MAX)

A command cut off this way gets `SIGPIPE` on its next write: `yes` stops by
itself. The limit is set in the core (`libsermocore/src/sermo_input.c`) and
applies to all nine backends.

- **`SERMO_INPUT_MAX`** — the limit, in bytes. `0` removes it. A value that is
  not a number of bytes (`16M`, `-1`) is reported, and the default limit
  applies.
- **The progress bar is not capped.** It keeps one line at a time, and must be
  able to follow a long command to the end: a copy that prints every file name
  soon goes past 16 MiB. Its memory stays flat, even on `yes`.

Up to 2.7.2, nothing bounded this read. Fed by `yes`, gtk3sermo went past
700 MB in 7 seconds, and ncurses ended on a segmentation fault (measured on
2026-09-17). Checked by `tests/garde_input_sans_fin.sh` (on every port, with an
unlimited control run) and `tests/unit/test_sermo_input.c`.

## Optional execution bounding (two environment variables)

Two guards, **off by default**, let you deploy a dialog into a less-trusted
context (a kiosk, a guest session) without changing the script. Both are read in
the core (`safe_exec.c`) and therefore apply to all nine
backends:

- **`SERMO_NO_SHELL_FALLBACK`** — when set (to any value), the `/bin/sh -c`
  fallback (used for commands containing shell metacharacters) is **refused**
  instead of merely logged: the program **fails closed** rather than opening a
  shell. Commands without metacharacters still run directly via `exec()`.
  Verified by `tests/garde_option_do.sh`.
- **`SERMO_ALLOWED_CMDS`** — a comma-separated list of allowed commands (e.g.
  `SERMO_ALLOWED_CMDS=ls,cat,/usr/local/bin/tool`). While set, only those
  commands may run, **and** the `/bin/sh -c` fallback is refused (otherwise
  `sh -c '…'` would bypass the list). The comparison is on the command **as it
  will be run**:
  - an entry without `/` (`ls`) allows that name, and the exact path the
    program's `PATH` resolves it to (`/usr/bin/ls`);
  - an entry with `/` allows exactly that path, and the bare name if `PATH`
    resolves it to that path;
  - a program that bears an allowed name but lives elsewhere (`/tmp/x/ls`) is
    **refused**.

  An empty value is the same as an unset variable. Verified by
  `tests/garde_allowed_cmds.sh`.

The 1.x names, **`HAPLO_NO_SHELL_FALLBACK`** and **`HAPLO_ALLOWED_CMDS`**, are
still read when the `SERMO_…` name is not set, with a warning: a deployment
hardened under 1.x stays hardened under 2.x.

### What these two variables do not reach

Three known limits. The first two open an execution path that does not go
through `safe_exec.c`, where neither the allow-list nor the refused fallback
applies.

- **`<terminal>` opens a shell outside both guards** (gtk3sermo and gtk4sermo,
  the only ports that have the widget). It spawns `/bin/sh` through VTE
  (`sermo-backend-gtk3/src/widget_terminal.c`), and the output of its `<input>`
  is **typed into it** as if on a keyboard (`vte_terminal_feed_child`). An
  allowed command that prints a line therefore has that line run by the
  terminal's shell. Measured on 2026-09-19 on the 2.7.1 package binary, then on
  2026-10-17 on gtk3sermo and gtk4sermo from the 3.0.0-1 packages, with
  both `SERMO_ALLOWED_CMDS` and `SERMO_NO_SHELL_FALLBACK` set: a plain
  `<action>` is refused at the very moment the `<terminal>` writes its witness
  file. **A dialog containing a `<terminal>` hands out a shell**: do not deploy
  one where you believe those variables define the boundary.
- **Link buttons (`<linkbutton>`)** open their address through the desktop
  browser, outside this list. They do not go through a shell, though. By port:
  - efl1, fltk1, html and lvgl: a direct `execlp("xdg-open", …)`, the address
    as one argument;
  - ncurses: `xdg-open` started by the core's launcher, as argv (on Windows,
    `ShellExecute`: see [Outside Linux](#outside-linux-what-changes));
  - gtk3 and gtk4: GTK's link button (`gtk_link_button`) opens the address
    itself;
  - qt6: `QDesktopServices::openUrl`; sdl3: `SDL_OpenURL`.
- **`--include` cancels the direct path, and conflicts with the list.** With
  that option every command is rewritten as `. 'file'; command` before it runs:
  the string always carries `;` and `'`, so **everything** goes through
  `/bin/sh -c`, even a command without a single metacharacter. And since the
  fallback is refused as soon as `SERMO_ALLOWED_CMDS` is set, `--include` plus
  an allow-list runs nothing at all. Both measured on 2026-09-19 on the 2.7.1
  package binary, and again on 2026-10-17 on gtk3sermo from the 3.0.0-1 package.
  The prefix also applies to the commands of the helper programs
  (`<input mqtt>`, `<action media>`…): with `--include` they too go through
  `/bin/sh -c` (harmless for the address, which is encoded with no
  metacharacter), and under an allow-list or with `SERMO_NO_SHELL_FALLBACK`
  they are refused like the others (read in the code:
  `sermo-backend-gtk3/src/widgets.c`, `libsermocore/src/actions.c`,
  `safe_exec.c`; not measured on a binary).

## Helper programs: reading from and writing to the outside (3.1.0)

A protocol `<input>` or `<action>` (`modbus`, `opcua`, `mqtt`, `mavlink`,
`media`) does not talk to the network itself. The core turns it into a command
that starts a **helper program**:
`$SERMO_SOURCE_DIR/sermo-<source|cible>-<protocol> <address>`
(`libsermocore/src/sermo_source.c`, `libsermocore/include/sermo_source.h`).

### Common rules

- **As argv, never through the shell.** The address goes as a single argument,
  percent-encoded: every byte outside `[A-Za-z0-9._:/=,+@-]` becomes `%XX`. The
  command therefore has no metacharacter (except with `--include`, see above).
- **`SERMO_SOURCE_DIR`**: the helper programs' directory, `/usr/libexec/sermo`
  by default. A relative path, or one carrying a character outside the set
  above, is refused when the XML is parsed.
- **Under `SERMO_ALLOWED_CMDS`**, the list must name the helper program, like
  any other command.
- **Writing needs a click.** `<action mqtt>`, `<action mavlink>` and
  `<action media>` are only allowed in a `<button>` (or its ok, cancel, help,
  yes, no variants) or a `<menuitem>`; anywhere else (timer, field,
  checkbox…), parsing stops on an error (`libsermocore/src/gtkdialog_parser.y`,
  `cible_verifier`). A `sermo-cible-` name written by hand in an `<input>` or
  an `<action>` is refused too. Modbus and OPC UA have no write program.
- **Each program is a separate package.** Without `sermo-cible-mqtt`,
  `sermo-cible-mavlink` or `sermo-cible-media` installed, sermo can only read.
- **Bounded address.** At most 4096 bytes, no control character (even after
  decoding), at most 32 parameters, and an unknown parameter is refused
  (`sermo-source/commun.c`).
- **Hardening.** The 8 helper programs are built with the same options as the
  ports, and `ci/bancs.sh` runs `tests/garde_durcissement.sh` (8 guarantees)
  on each of them.

### Modbus and OPC UA: read only

- **Modbus** (`sermo-source-modbus`): only read functions 1 to 4 are called
  (coils, discrete inputs, holding and input registers). The protocol has
  neither encryption nor authentication.
- **OPC UA** (`sermo-source-opcua`): reads one value, nothing else.
  `mode=none` by default, so **in clear**. `mode=sign` and `mode=chiffre`
  require a certificate, a key and at least one `confiance=` (an accepted
  server certificate); there is no "accept everything" mode.

### MQTT

- **`sermo-source-mqtt` never publishes** (no call to `mosquitto_publish`).
- **`mqtt://` is in clear.** `mqtts://` requires `confiance=` (the certificate
  of the authority that signed the server); the certificate **and** the
  server name are checked, never in "insecure" mode
  (`sermo-source/mqtt-commun.c`).
- **Credentials**: `identifiants=` names a file of yours, readable by you only
  (`chmod 600`); they never go as an argument.
- **New in 3.1.0: credentials refused on `mqtt://`**, before any connection,
  since they would travel in clear. Only exception: the address carries
  `clair=oui` (trusted local network). `clair=` only accepts `oui`, and only
  applies with `mqtt://`.
- **`sermo-cible-mqtt`** publishes one value on one topic, only if an
  **allow-list** permits it. It is **closed by default**:
  `~/.config/sermo/cibles-mqtt` (yours, readable by you only; the home
  directory comes from the account database, `getpwuid`, not from `$HOME`) and
  `/etc/sermo/cibles-mqtt` (owned by root, writable by nobody else). A badly
  protected file is refused as a whole. The publication is not retained by the
  server (no "retain").

### MAVLink (drones)

- **`sermo-source-mavlink` sends nothing**: it listens to telemetry.
- **`sermo-cible-mavlink`** only acts if its allow-list permits it (closed by
  default, same rules as MQTT: `~/.config/sermo/cibles-mavlink`,
  `/etc/sermo/cibles-mavlink`). It can:
  - **set a parameter**, only if the drone is **disarmed**. The `ARMING_*`,
    `FLTMODE*`, `BRD_SAFETY*`, `FENCE_*` and `SERVOn_FUNCTION` families are
    **always** refused, in the list as in the address;
  - **give a safety order**: `rtl` (return to launch), `land`, motor cut-off.
    The cut-off needs **two presses** to the same drone within 5 seconds, and
    `$XDG_RUNTIME_DIR` to keep the mark of the first press. ⚠️ **These three
    orders are sent in flight too**: that is their purpose;
  - **send a mission** (waypoints) to the disarmed drone, read back and
    compared; it is **never started**.
- **A single sender.** A read request goes first, then the program listens: if
  another ground station is already talking, it refuses.
- **Success only on the drone's proof**: the value read back, or an "accepted"
  acknowledgement. One attempt, never repeated.
- **No MAVLink signing**: nothing authenticates the messages, and another
  device on the same link can talk to the drone.
- **No piloting**: arming, taking off, changing mode or steering the drone is
  not in this version (planned for later), apart from the safety return to
  launch and landing (`rtl`, `land`, above).

### Sound and video (`sermo-cible-media`, `sermo-source-media`)

- **8 commands, closed list**: `lire`, `pause`, `reprendre`, `arreter`,
  `aller`, `volume`, `muet`, `son`. The JSON requests sent to mpv are written
  by the program, with bounded numbers it formats itself; never the string it
  received. mpv's `run` IPC command (which starts a shell) is never sent
  (`sermo-source/sermo-cible-media.c`, `sermo-source/media-commun.c`).
- **Sources**: `file://` (absolute path, readable regular file), `http://`,
  `https://`, `rtsp://`, `rtsps://`, and `v4l2://` for a camera (below) only;
  never credentials in the address
  (`…@`). `edl://`, `av://`, `concat:`… are refused.
- **HTTP(S) credentials**: `identifiants=` names a file of yours, in mode
  exactly `600`. The program writes the header into another `600` file, passed
  to mpv by `--include=`, then deleted. RTSP with a password: refused (mpv
  would only read it from the address, so as an argument).
- **mpv options**: `--no-config`, `--no-terminal`, `--idle=no`, `--ytdl=no`,
  `--load-scripts=no`, `--load-auto-profiles=no`,
  `--input-default-bindings=no`, `--load-unsafe-playlists=no`,
  `--access-references=no`, `--input-ipc-server=…`, `--title=…`, and
  `--ao=pulse,null` inside the cage (3.1.2); for a camera, also `--demuxer-lavf-o=video_size=…,input_format=…` (values from
  the closed lists) and `--log-file=<folder>/mpv.log` (the player's private
  folder: `--no-terminal` cuts mpv's error output, and the cause of a failure
  can only be read there); then `--` before the source. Embedded (`dans=`,
  3.1.1): `--wid=N` (a number from 1 to 2³²−1, otherwise refused),
  `--input-cursor=no`, `--input-vo-keyboard=no`; embedded camera: `CAMÉRA
  ACTIVE` written in the picture (`--osd-msg1`, with `--osd-level=1`,
  `--osd-scale-by-window=no`, `--osd-font-size=24`).
- **Camera** (`source=v4l2:///dev/videoN`): the node must be a real video
  device; the address given to mpv is built by the program; `taille=` and
  `format=` come from closed lists. Inside the cage only that node is visible
  (`--dev-bind`), with no network; the window says "CAMÉRA ACTIVE"; nothing is
  recorded; a busy camera is refused, never forced. Without bwrap, like a
  local file: no cage, and a message says so.
- **bubblewrap cage**: `--ro-bind / /`, `--dev /dev`, `--proc /proc`,
  `--tmpfs /tmp`, `--tmpfs $HOME`, `--tmpfs /run/user` (and the session
  folder if it lives elsewhere), `--unsetenv DBUS_SESSION_BUS_ADDRESS` (3.1.2),
  the player's directory writable, mpv and the
  local file read-only (for a camera, its node only, by `--dev-bind`), sound (PulseAudio) and display (Wayland, X11,
  `XAUTHORITY`) read-only, `--unshare-pid`, `--die-with-parent`,
  `--new-session`, and `--unshare-net` for a local file or a camera.
- **bwrap is mandatory for a network source**: without it, the source is
  refused.
- **At most 4 players** at once. Directories in `700` under
  `$XDG_RUNTIME_DIR/sermo-media`; a directory or socket open to the group or
  to others is refused.

**Limits of the player:**

- **Without bwrap, a local file or a camera is played without a cage.** Only a
  message
  says so (`Recommends: bubblewrap`, not `Depends`).
- **`--ro-bind /` leaves the whole system readable** outside `$HOME` and
  `/tmp`: mpv can read what the user can read elsewhere.
- **The cage gives access to the display (X11, Wayland) and to sound.** A
  compromised mpv could use them. Under X11 this already holds without
  embedding: X11 does not separate the windows of one screen, `--wid` adds no
  access.
- **Session sockets: fixed in 3.1.2.** Up to 3.1.1, the D-Bus session bus and
  the other sockets of `/run/user/<uid>` stayed reachable inside the cage
  (measured: `--ro-bind / /` does not prevent `connect()`). Since 3.1.2,
  `/run/user` is masked; only the player's folder, sound (pulse) and the
  display are mounted back, and `DBUS_SESSION_BUS_ADDRESS` is removed. PipeWire
  is not mounted (it also gives the screen and cameras): mpv plays sound through
  pulse. Still open: "abstract" sockets (X11 has one) stay reachable for a
  network source, which keeps the machine's network namespace.

## htmlsermo in serve mode (a page in the browser)

With `SERMO_HTML_MODE=serve`, htmlsermo serves the dialog as a web page and
reads the form sent back. It is the only port that opens a network socket.

- **Loopback only** (`127.0.0.1`), port picked by the kernel, a 128-bit token
  (from `/dev/urandom`) in the address; without the right token, 404. Only
  this page is served, GET and POST only, request capped at 256 KiB, timeout
  `SERMO_HTML_TIMEOUT` (300 s by default), a single form answer.
- **Since 3.1.0, only the variables declared by the dialog are returned**,
  with the value received (otherwise their starting value), and `EXIT`.
  Before, every field received came out: whoever fills in the page could add
  one (`PATH`, `LD_PRELOAD`…) with the browser's tools.
  Test: `tests/garde_html_serve.sh`.
- **Limit**: `EXIT` is whatever the browser sends (escaped); a script must not
  assume it is one of the dialog's buttons. Anyone who can read the address
  announced on standard error can fill in the page.

## Binary hardening

Core, backends and the 8 helper programs compile and link with: `-D_FORTIFY_SOURCE=3`,
`-fstack-protector-strong`, `-fstack-clash-protection`, `-fcf-protection=full`
(CET IBT/SHSTK) on x86, PIE, full RELRO, BIND_NOW, NX. Keep them on **every**
target — verified by `tests/garde_durcissement.sh`. On another processor or
system, CMake chooses after a test: see
[Outside Linux](#outside-linux-what-changes).

`_FORTIFY_SOURCE` only works with optimisation. Every `CMakeLists.txt` therefore
picks the `Release` build type when none is given; a `Debug` build loses this
protection. The core is checked separately, on its library, by
`tests/garde_fortify_coeur.sh`: on qt6, sdl3 and ncurses the optimised backend
brings its own fortified calls and would hide a core that is not.

⚠️ **CET is marked, not active.** The IBT and SHSTK notes are forced at link
time (`-Wl,-z,ibt -Wl,-z,shstk`), and the bench finds them. But on Debian testing,
neither glibc's startup objects, nor glibc itself, nor the toolkit libraries carry
them (measured with `readelf` on 2026-09-16, redone on 2026-10-17 on the 3.0.0-1
packages with glibc 2.43, GTK 3.24.52 and Qt 6.11.2). On that system CET therefore cannot
act at run time — deduced from the missing notes, not measured at run time.

## MCP servers

- **`sermoia-mcp`**: only the `sermo_run` tool launches the commands of the
  dialog, with the user's rights, as the script would. `sermo_check` and
  `sermo_render` run nothing: they set `SERMO_ALLOWED_CMDS` to a command that
  does not exist, which refuses every command and the `/bin/sh -c` fallback
  (`sermoia-mcp/src/sermoia-mcp.c`, `AUCUNE_COMMANDE`). Up to 2.8.1,
  `sermo_render` ran the dialog's `<input>`s. To bound `sermo_run`, set
  `SERMO_ALLOWED_CMDS` in the server's environment.
- **`sermoman-mcp`** is read-only: it serves the documentation, runs nothing
  and writes nothing.

Details: [sermoia-mcp/README.en.md](sermoia-mcp/README.en.md#security).

## Checking downloaded packages

The Debian packages are attached to each release with `SHA256SUMS` (one SHA-256
checksum per package) and its detached signature `SHA256SUMS.asc`, made with
the haplo-dialog key (`devel@haplo-dialog.fr`), whose fingerprint is:

```
66C7 F4AD 7A58 A037 967F  8339 3799 9F99 7570 F1ED
```

```sh
curl -fLO https://haplo-dialog.fr/cle-haplo-dialog.asc
gpg --import cle-haplo-dialog.asc
gpg --verify SHA256SUMS.asc SHA256SUMS      # "Good signature from haplo-dialog <devel@haplo-dialog.fr>"
sha256sum --ignore-missing -c SHA256SUMS
```

Compare the fingerprint shown with the one in the README on GitLab **and** on
GitHub: the public key comes from the website, and a key replaced on the
website would not be replaced everywhere at once. What is signed is the list of
checksums: the `.deb` files are not signed one by one, and there is no apt
repository (so no signed repository). Full command:
[README.en.md](README.en.md#install).

## Outside Linux: what changes

On Windows, Android, the BSDs and macOS, only the ncurses port runs. The core is
the same: same execution rules, same `SERMO_ALLOWED_CMDS` and
`SERMO_NO_SHELL_FALLBACK` variables. What differs:

- **The shell.** Commands with shell syntax go through `/bin/sh`, or through the
  program named by `SERMO_SHELL`: an **absolute** path to an executable; any other
  value is refused and reported, and the default shell is used. On Windows the
  default shell is the system's (`%ComSpec%`, cmd.exe), called as
  `cmd /S /C "command"`.
- **Windows reruns through the shell** a command without shell syntax that it
  cannot find as a program: `echo`, `dir`, `type`… are built into cmd.exe. This
  rerun obeys the two variables above: with `SERMO_NO_SHELL_FALLBACK=1` or a
  `SERMO_ALLOWED_CMDS` list, it is refused (checked under Wine on 2026-10-12).
- **No `<input>` cap on Windows.** `SERMO_INPUT_MAX` has no effect there: the CRT
  has neither `fopencookie` nor `funopen`. An endless command read by an
  `<input>` can exhaust memory. Elsewhere (Android, BSDs, macOS) the cap goes
  through `funopen` and holds (checked on Android on 2026-10-12).
- **Links on Windows**: `<linkbutton>` opens through `ShellExecute`, which would
  launch a program it was pointed at; sermo only passes `http`, `https` and
  `mailto`. Anything else is refused (tried on a real Windows 10: `calc.exe`
  refused).
- **Hardening depends on the target**:
  - ARM64 (Android, macOS Apple Silicon): BTI/PAC instead of CET. On Android our
    objects carry the BTI mark, but the final binary loses it at link time
    (Android's startup objects and Termux's `libncursesw` lack it): PAC is
    active, BTI is not enforced;
  - macOS: Apple's linker knows neither RELRO nor `-z`; PIE and a non-executable
    stack are its defaults;
  - Windows: high-entropy ASLR and DEP required at link time, statically linked
    binary, no DLL besides the system's;
  - `tests/garde_durcissement.sh` checks Linux (ELF) binaries: it was not run on
    these systems.

## What no automatic bench checks

Some statements in this document are guarded by no bench in `ci/bancs.sh`.
They rest on a reading of the code or on a manual trial:

- **Glade under an allow-list**: `tests/garde_glade.sh` checks that a Glade file
  loads, not `SERMO_ALLOWED_CMDS`. That its handlers take the same path as
  `<action>` is read in the code (`glade_support.c` calls `execute_action`),
  not measured.
- **`<terminal>` outside the two variables**: `tests/garde_terminal.sh` checks
  that the terminal runs the line it receives, without an allow-list. The
  bypass of the list was measured by hand on 2026-09-19 and 2026-10-17.
- **`--include` under an allow-list**: `tests/garde_include.sh` checks that the
  file loads, without an allow-list. The refusal was measured by hand on
  2026-09-19 and 2026-10-17.
- **`<linkbutton>`**: no bench. How each port opens links is read in the code
  (2026-10-18).
- **"sermo never calls `pkexec` or `sudo`"**: no bench. A search through the
  code of the core, the ports and the helper programs finds no such call
  (2026-10-18).
- **CET inactive**: `tests/garde_durcissement.sh` finds the IBT and SHSTK
  notes; their absence from glibc and the toolkits is measured by hand with
  `readelf` (dates above, in [Binary hardening](#binary-hardening)).
- **Signed checksums**: no bench; the check is the one you make with the
  commands in [Checking downloaded packages](#checking-downloaded-packages).

## Known limits

- **No sandbox for commands.** A command started by a dialog has the rights of
  the user who opened it. sermo bounds *how* it is started, not *what it does*.
  Only mpv, started by `sermo-cible-media`, is put in a cage (bubblewrap, with
  the limits given above).
- **sermo never raises its own privileges**: it calls neither `pkexec` nor
  `sudo`, is not setuid and asks for no administrator password. A **script** it
  runs may: the `examples/system-tools` example calls `pkexec` (root terminal,
  editing a system file, updating packages) and says so in its header. That
  escalation is the script author's, outside what sermo bounds.
- **No command list enforced by default**: trust goes to the script author. The
  optional `SERMO_ALLOWED_CMDS` list remains available (see above).
- **The cap applies to each `<input>`, not to the whole dialog.** A dialog that
  reads many `<input>`, or rereads them through `refresh:`, can hold 16 MiB
  several times over (see
  [Size of what an `<input>` reads](#size-of-what-an-input-reads)).
- **Memory grows in a dialog that refreshes forever** if a `<list>` or
  `<table>` is refreshed without `clear:`: it accumulates its rows. The loss
  of a few bytes at each export of the variables is closed since 3.0.0
  (`libsermocore/src/variables.c` frees the value read); the test
  `tests/garde_fuite_refresh.sh` (LeakSanitizer) checks it on ncurses and
  html.
- **`<terminal>` hands out a shell** (gtk3sermo, gtk4sermo): it spawns
  `/bin/sh` through VTE, outside `SERMO_ALLOWED_CMDS` and
  `SERMO_NO_SHELL_FALLBACK`, and the output of its `<input>` is typed into it as
  if on a keyboard (measured on 2026-09-19, redone on 2026-10-17 on 3.0.0; see above).
- **`--include` sends everything through `/bin/sh -c`**, and runs nothing at all
  when a command list is set (see above).
- **A Glade file is code**, and **`<linkbutton>`** opens its address outside the
  command list (see above).
- **No external audit.** What is verified is verified by the repository's benches
  (`ci/bancs.sh`) and by the maintainer's own trials; the code is not entirely
  reviewed. Open gaps are listed in
  [TODO.md](TODO.md) (French only).

## Reporting a vulnerability

- Write to **`devel@haplo-dialog.fr`**. Do not open a public issue while the
  vulnerability is not fixed.
- Include: the command you ran (`gtk3sermo`, `qt6sermo`…) and its `--version`
  output, the package version, a minimal XML script and the steps that lead to
  the problem.
- **Timelines** — sermo has a single maintainer: acknowledgement within **14
  days**; a fix, or a public advisory with a workaround, no later than **90
  days** after the report, sooner if the vulnerability is already public.
- **Supported versions**: only the latest published line receives fixes: 3.1.x
  (3.1.2 since 2026-10-19). The 1.x line no longer does: move to 3.1
  ([MIGRATION.en.md](MIGRATION.en.md)).
- If you wish, your name goes into the [CHANGELOG.en.md](CHANGELOG.en.md) with the
  fix.
