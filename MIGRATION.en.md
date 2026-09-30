# Moving from sermo 1.x to 2.x

[Français](MIGRATION.md)

1.x shipped three ports, each in its own directory, with its own version and
package: gtk3sermo and gtk4sermo 1.1.4, qt6sermo 1.0.2, and `gtksermo` for the
`gtkdialog` command. 2.x brings them together around a shared core, adds four
ports and uses a single version number.

**Your XML scripts stay valid** and **the commands keep their names**. What
mostly changes: the package names, the names of two environment variables, and
the way to compile. The few behaviours that change are listed below.

## In short

| | 1.x | 2.x |
|---|---|---|
| Packages | `gtk3sermo`, `gtk4sermo`, `qt6sermo`, `gtksermo` | `sermo-backend-gtk3`, `-gtk4`, `-qt6`, `-fltk1`, `-efl1`, `-sdl3`, `-ncurses`, `sermo-gtkdialog`, `sermo-core-dev` |
| Commands | `gtk3sermo`, `gtk4sermo`, `qt6sermo`, `gtkdialog` | the same, plus `fltk1sermo`, `efl1sermo`, `sdl3sermo`, `ncursessermo` and `sermo` |
| Security variables | `HAPLO_ALLOWED_CMDS`, `HAPLO_NO_SHELL_FALLBACK` | `SERMO_ALLOWED_CMDS`, `SERMO_NO_SHELL_FALLBACK` (the old names are still read) |
| Sources | one directory per port (`gtk3sermo/gtk3sermo_1.1.4/`…) | `libsermocore/` and `sermo-backend-<port>/`, built with CMake |
| Arch, Gentoo, RPM, Slackware recipes | provided | removed: only `debian/` is maintained |
| Version | one per port | one for everything (2.7.4) |

## Upgrading the Debian packages

### From `.deb` files (the way 1.x was installed)

Download and check the 2.x packages attached to the release, as the
[README](README.en.md#install) explains, then install those of the ports you use.
apt removes the 1.x packages they replace by itself:

```sh
sudo apt install ./sermo-backend-gtk3_2.7.4-1_amd64.deb ./sermo-gtkdialog_2.7.4-1_all.deb
```

Add `./sermo-backend-gtk4_2.7.4-1_amd64.deb` or
`./sermo-backend-qt6_2.7.4-1_amd64.deb` if you had `gtk4sermo` or `qt6sermo`.

Tested on 2026-09-17 in a clean Debian testing container, without network:
`gtk3sermo`, `gtk4sermo`, `gtksermo` 1.1.4-2 and `qt6sermo` 1.0.2-2 installed, then
the command above with the three backends. apt removed the four 1.x packages and
installed the four 2.x packages; `gtk3sermo`, `gtk4sermo` and `qt6sermo` answer
2.7.1, `gtkdialog` and `sermo` lead to gtk3sermo, and `apt autoremove` would
remove nothing from sermo.

### From an apt repository

If the 2.x packages reach you through a repository, `sudo apt upgrade` is enough:
the `gtk3sermo`, `gtk4sermo`, `qt6sermo` and `gtksermo` packages move to 2.x,
empty, and bring in `sermo-backend-gtk3`, `-gtk4`, `-qt6` and `sermo-gtkdialog`.

These transitional packages can then go. **First**, mark the 2.x packages you use
as wanted: they came in as dependencies, and `apt autoremove` would otherwise
remove them.

```sh
sudo apt install sermo-backend-gtk3 sermo-gtkdialog      # the ones you use
sudo apt purge gtk3sermo gtk4sermo qt6sermo gtksermo
```

Tested by `packaging/construire-paquets.sh` (2.x packages served by a local
repository): see [PACKAGING.en.md](PACKAGING.en.md).

## Commands

- `gtk3sermo`, `gtk4sermo`, `qt6sermo` and `gtkdialog` still exist, in the same
  place (`/usr/bin`).
- **New**: the `sermo` command points to one of the installed ports, chosen by
  `update-alternatives` (gtk3 first). `sudo update-alternatives --config sermo`
  picks another one. A script that calls `gtk3sermo` does not need to change.
- The options of gtk3sermo and gtk4sermo are the same as in 1.1.4, plus
  `--render-png`.

## Environment variables

| 1.x | 2.x |
|---|---|
| `HAPLO_ALLOWED_CMDS` | `SERMO_ALLOWED_CMDS` |
| `HAPLO_NO_SHELL_FALLBACK` | `SERMO_NO_SHELL_FALLBACK` |
| `GTKDIALOG_PIXMAP_PATH` | unchanged |

The old names are **still read** when the new one is not set, with a warning on
standard error: a deployment hardened under 1.x stays hardened. Rename them
anyway. `tests/garde_allowed_cmds.sh` and the unit tests check this.

⚠️ **`SERMO_ALLOWED_CMDS` now compares paths.** A command written with a path is
accepted only if that path is listed, or if it is where `PATH` finds a listed
name. On a system where `PATH` finds `/usr/bin/echo`, listing `echo` no longer
allows `/bin/echo`: list `/bin/echo`, or write `echo` in the script. Details:
[SECURITY.en.md](SECURITY.en.md).

## Behaviours that change

Measured on gtk3sermo and gtk4sermo, 1.1.4 then 2.7.1:

- **`--include` with a relative path**, or a name containing a space or an
  apostrophe: 1.1.4 loaded nothing, 2.7.1 loads the file.
- **`--glade-xml`**: 2.7.1 exports the values of the file's widgets, which 1.1.4
  did not, and a missing file or an unknown window give a clean error (exit code
  1) instead of an abort (exit code 134). The five other ports refuse the option.

Measured on qt6sermo 1.0.2 and 2.7.1 (bench cases 43 and 44), 2.7.1 following
gtk3sermo:

- **`<input file>`** fills `<entry>`, `<text>` and `<comboboxtext>`; 1.0.2 did not
  read the file.
- **`<text>` filled by a command** keeps its final newline.
- **`<combobox>` is no longer filled by `<input>`**, as on gtk3, which never did
  it: use `<comboboxtext>`.

The gaps that remain between ports are listed in [TODO.md](TODO.md) (French only).

## Compiling from source

1.x was built inside each port's directory. 2.x is built in two steps: the
`libsermocore` core, in the variant that suits the port, then the port against
that core. `bash ci/construire.sh` does both for the seven ports. See
[COMPILE.en.md](COMPILE.en.md).

Two dependencies are now **required**: VTE for gtk3 and gtk4 (`libvte-2.91-dev`,
`libvte-2.91-gtk4-dev`), and gtk-layer-shell for gtk3 (`libgtk-layer-shell-dev`).
Without them the build stops instead of producing a binary without `<terminal>`
or Wayland anchoring. To do without them: `-DSERMOCORE_VTE=OFF`,
`-DSERMO_LAYER_SHELL=OFF`.

The Arch, Gentoo, RPM and Slackware recipes of 1.x are no longer provided. Anyone
packaging for another distribution can start from `debian/rules`, which shows the
build order.
