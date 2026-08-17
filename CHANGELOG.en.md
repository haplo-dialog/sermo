# Changelog - haplo-dialog

All notable changes to this project are documented here.  
Format: [Keep a Changelog](https://keepachangelog.com/en/1.0.0/)  
Versioning: [Semantic Versioning](https://semver.org/) starting from 1.0.0.

---

## [Unreleased] - v1.1.0 (in progress)

### Added
- **gtkdialog backwards compatibility (2026-06-06)** : `make install` installs a **`gtkdialog` → `gtk3dialog` symlink** (and `gtkdialog.1` → `gtk3dialog.1`) through the autotools hook; a legacy gtkdialog script (`export MAIN_DIALOG='<window …>'; gtkdialog --program=MAIN_DIALOG`) parses, runs and returns its output in the historical format (`VAR="value"`). The consistency of the symlink is carried through into every packaging recipe (Debian `.links`, RPM `%files`, etc.).
- `detect_terminal()` / `detect_editor()`, auto-detection of the graphical environment (xfce4-terminal, konsole, gnome-terminal, mousepad, kate, gedit…)
- XML suite extended to **52 test cases** (searchentry, levelbar, drawingarea, colorbutton, fontbutton, aspectframe, tree, table, menubar, statusbar, togglebutton, timer, edit, list, separators, infobar types, 3-page notebook, REFRESH/ENABLE/DISABLE/SHOW/HIDE/CLEAR actions, complex form)
- `AUTHORS` and `NEWS` at the root (GNU standard)

### Changed
- `LOGO_TMP` uses a fixed name per UID (`/tmp/haplo-logo-UID.png`), avoids the leak in case of `SIGKILL`
- ALLOWED_CMDS broadened: pacman, dnf, zypper, emerge, slackpkg, xbps-install, xbps-query, apk
- Licence **standardised to GPL-2.0-or-later** across the whole repository (source headers, packaging, `LICENCES.md`, `CONTRIBUTING.md`), the GPL-3.0+ attempt has been reverted, in accordance with the "either version 2 … any later version" clause of the sources and with upstream gtkdialog

### Fixed
- **Truncation of auto-generated widget names (2026-06-06)**: `g_snprintf(name, sizeof(name), …)` where `name` is a `char*`, `sizeof` therefore evaluated to 8 bytes, truncating the names, fixed by passing 64 as the buffer size; restores the original gtkdialog behaviour
- **Read of an uninitialised variable (2026-06-06)**: `instruction inst;` whose `inst.ival` was being read → `instruction inst = {0};`
- **Clean build (2026-06-06)**: 0 errors; the residual warnings stem from upstream gtkdialog idioms and from 2 untouchable bison shift/reduce conflicts

---

## Packaging revisions of 1.0.0

The **software** stays at version 1.0.0: these revisions change only the Debian
package. The binary shipped is identical from one revision to the next.

### [1.0.0-3] - 2026-08-16
- Package maintainer identity: the `Maintainer` field carried a personal name
  and a private domain, visible to every user through `apt show gtk3dialog`. It
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
- **gtk3dialog** : reference port (GTK 3 backend), providing the backwards-compatible `gtkdialog` alias
- **43 widgets** implemented
- New widgets: `<switch>`, `<password>`, `<searchentry>`, `<calendar>`, `<infobar>`, `<levelbar>`, `<spinner>`, `<aspectframe>`, `<drawingarea>`
- `safe_system()` / `safe_popen()`, secure replacement for `system()` / `popen()`
- Hardening: `FORTIFY_SOURCE=3`, PIE, Full RELRO, NX stack, stack canary (`-fstack-protector-strong`), CFI (`-fcf-protection`)
- roff manpage `gtk3dialog(1)`
- `haplo-dialog-xml(5)`, reference manpage for the XML syntax
- Texinfo documentation (`.texi` → `.info`)
- Documentation website (static HTML)
- Packaging: `.deb` (Debian), `.rpm` (Fedora/SUSE), `PKGBUILD` (Arch), `.ebuild` (Gentoo), `.SlackBuild` (Slackware)
- Example scripts covering every widget (`examples/`, 53 directories)
- XML regression suite (`tests/xml/`)
- `SECURITY.md`, `.clang-format`, `.editorconfig`
- `CONTRIBUTING.md`, `CODE_OF_CONDUCT.md`, `ROADMAP.md`

### Changed
- Binary renamed: `gtkdialog` → `gtk3dialog`
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
- Initial structure of the gtk3dialog port
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

