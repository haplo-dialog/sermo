# Changelog — haplo-dialog

All notable changes to this project are documented here.  
Format: [Keep a Changelog](https://keepachangelog.com/en/1.0.0/)  
Versioning: [Semantic Versioning](https://semver.org/) starting from 1.0.0.

---

## [Unreleased] — v1.1.0 (in progress)

### Added
- `detect_terminal()` / `detect_editor()` — auto-detection of the graphical
  environment (xfce4-terminal, konsole, gnome-terminal, mousepad, kate, gedit…)
- XML regression suite extended to **50 test cases** (21–50: searchentry,
  levelbar, drawingarea, colorbutton, fontbutton, aspectframe, tree, table,
  menubar, statusbar, togglebutton, timer, edit, list, separators, infobar,
  3-page notebook, REFRESH/ENABLE/DISABLE/SHOW/HIDE/CLEAR actions, complex
  form)
- `AUTHORS` and `NEWS` at the root (GNU standard)

### Changed
- `LOGO_TMP` uses a fixed name per UID (`/tmp/haplo-logo-UID.png`) — avoids the
  leak in case of `SIGKILL`
- `ALLOWED_CMDS` broadened: pacman, dnf, zypper, emerge, slackpkg, xbps-install,
  xbps-query, apk
- Licence **standardised to GPL-2.0-or-later** across the whole repository
  (source headers, packaging, `LICENCES.md`, `CONTRIBUTING.md`) — in accordance
  with the "either version 2 … any later version" clause of the sources and with
  upstream gtkdialog

---

## [1.0.0] — 2026-05-29

First stable public release — rework of gtkdialog 0.8.3 onto GTK3.

### Added
- **gtk3dialog (GTK3)** — complete port of gtkdialog 0.8.3, XML syntax kept
  identical
- **gtkdialog compatibility**: the package **provides** `gtkdialog` (symbolic
  link) — a single binary answers to both names `gtkdialog` and `gtk3dialog`,
  legacy gtkdialog scripts run as is
- Around forty widgets, including the new ones: `<switch>`, `<password>`,
  `<searchentry>`, `<calendar>`, `<infobar>`, `<levelbar>`, `<spinner>`,
  `<aspectframe>`, `<drawingarea>`
- `safe_system()` / `safe_popen()` — secure replacement for `system()` /
  `popen()`
- Binary hardening: `FORTIFY_SOURCE=3`, PIE, Full RELRO, NX stack,
  stack canary
- Manpage `gtk3dialog(1)`
- `haplo-dialog-xml(5)` — reference manpage for the XML syntax
- Texinfo documentation (`.texi` → `.info`)
- Packaging: `.deb` (Debian), `.rpm` (Fedora/SUSE), `PKGBUILD` (Arch),
  `.ebuild` (Gentoo), `.SlackBuild` (Slackware)
- Example scripts (`examples/`)
- XML regression suite (`tests/xml/`)
- `SECURITY.md`, `.clang-format`, `.editorconfig`
- `CONTRIBUTING.md`, `CODE_OF_CONDUCT.md`, `ROADMAP.md`
- Mention and acknowledgement of Claude (Anthropic) throughout the
  documentation

### Changed
- Binary renamed: `gtkdialog` → `gtk3dialog` (with `gtkdialog` provided as a
  symbolic link for compatibility)
- `gtkdialog_parser.y` / `gtkdialog_lexer.l` — parser and lexer preserved
- `fclose()` on any `FILE*` returned by `safe_popen()` — `pclose()` banned

### Removed
- Residual GTK2 dependencies
- Direct `system()` and `popen()` in the widgets
- Dead code and stray comments

### Security
- No known vulnerability as of 2026-05-29
- `safe_system()` / `safe_popen()`: shell-free execution outside metacharacters,
  `/bin/sh -c` fallback refusable via `HAPLO_NO_SHELL_FALLBACK`
- File path validation before any access
- `--do` pattern: variables exported through the environment, never through
  `eval`

---

## [0.9.0-haplo1] — 2026-03-01 (internal)

Initial working version — port from gtkdialog 0.8.3.

### Added
- Initial structure of the GTK3 port
- Migration of the core (XML parser, variables, actions, signals, stack)
- Initial implementation of the widgets
- Autotools build system (`./configure && make`)

---

## [gtkdialog-0.8.3] — 2013 (original upstream)

Last release of the original upstream by László Pere and Thunor.  
Reference: https://github.com/01micko/gtkdialog

---

*haplo-dialog builds on gtkdialog 0.8.3 (László Pere, GPL-2.0+), modernised and  
ported to GTK3 (GPL-2.0-or-later).*
