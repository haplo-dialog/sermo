# sermo

**sermo** opens a dialog box described in **XML** from a shell script, then
writes the values the user entered to its output (`NAME="Jane"`). The same
script runs on eight graphical libraries: GTK 3, GTK 4, Qt 6, FLTK, EFL, SDL 3,
LVGL, and ncurses in a terminal. A ninth port, html, opens nothing: it writes
the dialog as an HTML page.

![The same form rendered by the nine ports](doc/captures/formulaire-neuf-ports.png)

*The same script, [`examples/showcase/01-formulaire.sh`](examples/showcase/01-formulaire.sh),
rendered by the binaries of the 2.7.1-1 packages. efl1 keeps its dark theme even
when the light theme is requested ([TODO.md](TODO.md) (French only)). The image is rebuilt by
[`doc/captures/refaire.sh`](doc/captures/refaire.sh).*

[Français](README.md)

## Where sermo comes from

sermo is a fork of **gtkdialog 0.8.3**, written by Pere László (2003-2007) and
then taken over by Thunor (2011-2013), under GPL-2.0-or-later. The original line
continues at Puppy Linux: <https://github.com/puppylinux-woof-CE/gtkdialog>. Who
did what, when, and where the other descendants stand (gtk3dialog, gtkdialog3):
[HISTOIRE.en.md](HISTOIRE.en.md).

sermo keeps gtkdialog's XML language and parser. It replaces GTK 2 with nine
rendering layers around a shared core, and puts bounds on command execution.
What comes from upstream and what was written here is counted file by file in
[LICENCES.en.md](LICENCES.en.md).

## Project status

- Version **3.1.2**. sermo has **a single maintainer**, S. Cage. There is no team
  behind it: an answer may take time.
- Part of the code, documentation and tests was written with the help of an AI
  (Claude, by Anthropic), under the maintainer's direction. The maintainer tests
  it and reports the defects found, but no longer reviews all of the code line by
  line: checking relies mainly on the tests, which anyone can re-run
  ([AUTHORS.en](AUTHORS.en)).
- **What is verified**: 178 benches (XML parsing, exported values, security,
  clicks, opening the real examples, MCP servers) on the nine ports in the full
  run of 2026-10-11, detailed in [BILAN_SANTE.md](BILAN_SANTE.md) (French only).
  In the full run (`ci/bancs.sh`, helper programs included):
  262 tests, 0 failures (final run of 3.1.2, 2026-10-19). In the run of 2026-10-18, one bench was red (forbidden functions
  in the sound and video player code); it was fixed and then replayed green.
  Every figure can be replayed with a command.
- **What sermo reads and writes outside**, through helper programs installed
  separately: a PLC over Modbus or OPC UA (read-only); MQTT home automation
  (read, and publish if an allow-list permits it); a MAVLink drone (read its
  telemetry; set a parameter of the disarmed drone, send it a mission without
  starting it, or give a safety order: return to launch, land, motor cut-off in
  two presses); sound and video, played by mpv. sermo does not fly a drone: it
  does not arm it, take it off or steer it, apart from the safety return to
  launch and landing. The packages are under "Install".
- **Known gaps**: the ports without GTK still differ from gtk3sermo on a few
  points (`auto-refresh`, the label of the Cancel button, `<terminal>`…). They are
  listed in [TODO.md](TODO.md) (French only).
- **No apt repository**: the Debian packages are attached to each release (see
  "Install").
- The manuals are in French for now. The entry documents (this README,
  [SECURITY](SECURITY.en.md), [CONTRIBUTING](CONTRIBUTING.en.md),
  [MIGRATION](MIGRATION.en.md), [COMPILE](COMPILE.en.md),
  [PACKAGING](PACKAGING.en.md), [VERSIONING](VERSIONING.en.md),
  [DEPENDENCIES](DEPENDENCIES.en.md), [LICENCES](LICENCES.en.md)) exist in English.

## Example

```sh
export MAIN_DIALOG='
<window title="Hello">
  <vbox>
    <text><label>Your name?</label></text>
    <entry><variable>NAME</variable></entry>
    <hbox><button ok></button><button cancel></button></hbox>
  </vbox>
</window>'
sermo --program=MAIN_DIALOG
```

After typing "Jane" and clicking OK, the output is the same on every port that
displays the dialog:

```
NAME="Jane"
EXIT="OK"
```

A script reads these lines with `eval "$(sermo --program=MAIN_DIALOG)"`: the
values are escaped for that purpose. When someone other than the script's author
fills in the dialog, prefer `--do` ([SECURITY.en.md](SECURITY.en.md)).

## The nine ports

| Port | Library | Command | Package |
|---|---|---|---|
| gtk3 | GTK 3 | `gtk3sermo` | `sermo-backend-gtk3` |
| gtk4 | GTK 4 | `gtk4sermo` | `sermo-backend-gtk4` |
| qt6 | Qt 6 | `qt6sermo` | `sermo-backend-qt6` |
| fltk1 | FLTK 1.4 | `fltk1sermo` | `sermo-backend-fltk1` |
| efl1 | EFL (Enlightenment) | `efl1sermo` | `sermo-backend-efl1` |
| sdl3 | SDL 3 and Dear ImGui | `sdl3sermo` | `sermo-backend-sdl3` |
| lvgl | LVGL 9.6, embedded (Wayland, X11 or framebuffer) | `lvglsermo` | `sermo-backend-lvgl` |
| ncurses | ncurses, in a terminal | `ncursessermo` | `sermo-backend-ncurses` |
| html | none: a standalone HTML page | `htmlsermo` | `sermo-backend-html` |

- **gtk3sermo is the reference**: the eight other ports are measured against it.
- **html is an exporter**: it opens no window, it writes the dialog as one HTML
  page (embedded CSS, no JavaScript by default) into the file named by `SERMO_HTML_OUT`
  (default `sermo-output.html`). With `SERMO_HTML_MODE=serve`, a local
  micro-server serves the page and returns the values entered.
- Only gtk3 and gtk4 have `<terminal>` (VTE) and `--glade-xml`; only gtk3 has
  Wayland anchoring (`layer-shell`).
- Each package pulls in its own library only: `sermo-backend-fltk1` installs
  neither GTK nor Qt, `sermo-backend-ncurses` needs nothing but ncurses.

The core, `libsermocore` (XML parsing, variables, actions, command execution),
is linked into every binary; the backend only draws. The ncurses port was added
without changing a single line of the core. Architecture:
[MANUEL_DEVELOPPEUR.en.md](MANUEL_DEVELOPPEUR.en.md).

## Other systems

Outside Linux, a single port runs: **ncurses**, in a terminal.

| System | How to get it | Verified |
|---|---|---|
| Windows 10 and 11 | `sermo-<version>-windows-x86_64.zip`, attached to the release: a single `ncursessermo.exe`, nothing to install | behaviour test under Wine; display and accented letters on a real Windows 11, commands and accented letters on a real Windows 10 |
| Android (Termux) | recipe [`packaging/autres-os/termux/build.sh`](packaging/autres-os/termux/build.sh), built inside Termux | on a phone (Android 15, arm64) |
| FreeBSD, OpenBSD | ports [`packaging/autres-os/freebsd/`](packaging/autres-os/freebsd/) and [`openbsd/`](packaging/autres-os/openbsd/) | in virtual machines (FreeBSD 15.1, OpenBSD 7.9) |
| macOS | Homebrew formula [`packaging/autres-os/homebrew/sermo.rb`](packaging/autres-os/homebrew/sermo.rb) | on a macOS runner (Apple Silicon) |

"Verified" means: the port returns the same values as gtk3sermo on the behaviour
test and on the dialogs of the job scripts. Only Windows has a published binary;
for Android, the BSDs and macOS there are only the recipes.

- Building: [COMPILE.en.md](COMPILE.en.md) §7.
- What changes for security (no `<input>` cap on Windows, hardening depending on
  the processor): [SECURITY.en.md](SECURITY.en.md).
- Details of each test: [packaging/autres-os/README.md](packaging/autres-os/README.md) (French only).

## Install

There is no apt repository. The Debian packages are attached to the
[**v3.1.2**](https://gitlab.com/haplo-dialog/sermo/-/releases/v3.1.2) release,
with their checksums: download, check, install.

```sh
U=https://gitlab.com/api/v4/projects/85674825/packages/generic/sermo/3.1.2
for f in sermo-backend-gtk3_3.1.2-2_amd64.deb sermo-gtkdialog_3.1.2-2_all.deb SHA256SUMS SHA256SUMS.asc; do
    curl -fLO "$U/$f"
done
curl -fLO https://haplo-dialog.fr/cle-haplo-dialog.asc
gpg --import cle-haplo-dialog.asc
gpg --verify SHA256SUMS.asc SHA256SUMS
sha256sum --ignore-missing -c SHA256SUMS
sudo apt install ./sermo-backend-gtk3_3.1.2-2_amd64.deb \
                 ./sermo-gtkdialog_3.1.2-2_all.deb
```

For another port, replace `gtk3` with `gtk4`, `qt6`, `fltk1`, `efl1`, `sdl3`,
`lvgl`, `ncurses` or `html`. The packages are built on **Debian testing (amd64)** and require its
library versions (Qt ≥ 6.11.2 for qt6, glibc ≥ 2.43 for sdl3…). Elsewhere,
compile from source.

The checksums are signed with the haplo-dialog key. `gpg --verify` must report a
good signature from "haplo-dialog <devel@haplo-dialog.fr>", with the fingerprint:

```
66C7 F4AD 7A58 A037 967F  8339 3799 9F99 7570 F1ED
```

The public key is on the website (`https://haplo-dialog.fr/cle-haplo-dialog.asc`);
the fingerprint is here, in the repository. Compare it with the one in this
README on GitLab (<https://gitlab.com/haplo-dialog/sermo>) **and** on GitHub
(<https://github.com/haplo-dialog/sermo>): a key replaced on the website would
not be replaced everywhere at once.

What is signed is the list of checksums (`SHA256SUMS`), not each `.deb` one by
one: a package can be trusted because its checksum is in a list whose signature
is good. There is no signed apt repository either.

**Reading and writing outside**: each helper program has its own package, to
install on top of the port (same download, same check):

| For | Packages |
|---|---|
| reading a PLC | `sermo-source-modbus`, `sermo-source-opcua` |
| MQTT home automation | `sermo-source-mqtt` (read), `sermo-cible-mqtt` (publish) |
| a MAVLink drone | `sermo-source-mavlink` (read), `sermo-cible-mavlink` (set, mission, safety orders) |
| sound and video | `sermo-cible-media` (play; pulls in `mpv`, recommends `bubblewrap`), `sermo-source-media` (follow the state) |

Installing only the reading side cannot write anything. The allow-lists and
the guards are in [SECURITY.en.md](SECURITY.en.md) and the
[user manual](MANUEL_UTILISATEUR.en.md).

**Build the packages yourself** (Debian testing, Docker):
`bash packaging/construire-paquets.sh` builds in a clean container, runs lintian,
installs and purges the packages to check they leave nothing behind. On a Debian
testing system, `sudo apt build-dep ./` then `dpkg-buildpackage -us -uc -b`
works too. Details:
[PACKAGING.en.md](PACKAGING.en.md).

**Compile from source**: [COMPILE.en.md](COMPILE.en.md).

After installing:

- the **`sermo`** command points to one of the installed ports, chosen by
  `update-alternatives` (gtk3 first); `sudo update-alternatives --config sermo`
  picks another one;
- for old scripts that call **`gtkdialog`**, also install `sermo-gtkdialog`. It
  replaces the `gtkdialog` and `gtk3dialog` packages: keep one or the other, not
  both.

## Uninstall

```sh
sudo apt purge sermo-gtkdialog sermo-backend-gtk3
```

The purge leaves no file and no alternative behind (checked in a clean
container). Compiled from source without `install`: delete the build directory.

## Coming from 1.x

Install the current (3.1.0) packages of the ports you use (`sermo-backend-gtk3`,
`sermo-gtkdialog`…): apt removes the 1.x packages they replace (`gtk3sermo`,
`gtksermo`…). The commands keep their names. What changes and what to do:
[MIGRATION.en.md](MIGRATION.en.md).

## Documentation

- [User manual](MANUEL_UTILISATEUR.en.md) — writing dialogs: widgets,
  actions, variables, options.
- [Developer manual](MANUEL_DEVELOPPEUR.en.md) — the core, the backends,
  adding a port.
- `man sermo` and `info sermo`, installed with every port.
- [COMPILE.en.md](COMPILE.en.md) · [PACKAGING.en.md](PACKAGING.en.md) ·
  [VERSIONING.en.md](VERSIONING.en.md) · [DEPENDENCIES.en.md](DEPENDENCIES.en.md)
- [SECURITY.en.md](SECURITY.en.md) · [CONTRIBUTING.en.md](CONTRIBUTING.en.md) ·
  [CHANGELOG.en.md](CHANGELOG.en.md) · [ROADMAP.en.md](ROADMAP.en.md)
- [`sermoman-mcp`](sermoman-mcp/README.md) (French only) — a read-only MCP server that gives an
  AI assistant sermo's manual, architecture and examples.
- [`sermoia-mcp`](sermoia-mcp/README.en.md) — an MCP server that lets an AI
  assistant write, check and see a dialog; previews run no command.

## Reporting a problem

- **A bug**: open an issue at <https://gitlab.com/haplo-dialog/sermo/-/issues>,
  with a minimal XML script that reproduces it, the command you ran
  (`gtk3sermo`, `qt6sermo`…) and its `--version` output. Without a GitLab
  account, the same report is welcome at `devel@haplo-dialog.fr`.
- **A security vulnerability**: no public issue, see
  [SECURITY.en.md](SECURITY.en.md).

## License

GPL-2.0-or-later, like gtkdialog. A few files have another license (the boundary
contract under MIT, Dear ImGui, LVGL and ThorVG under MIT, the MAVLink headers
under MIT once compiled, some examples under CC0): see
[LICENCES.en.md](LICENCES.en.md).
