# Licences - haplo-dialog

## Summary

| Component | Files | Licence | Reason |
|-----------|-------|---------|--------|
| Core and widgets | `src/*.c`, `src/*.h`, `src/*.cpp` (Qt 6 port), `src/safe_exec.c`, `src/variables.c`, `src/stringman.c`, `src/stack.c` | **GPL-2.0-or-later** | Direct gtkdialog heritage (László Pere), strong copyleft |
| XML parser | `src/gtkdialog_lexer.l`, `src/gtkdialog_parser.y` | **GPL-2.0-or-later** | Direct heritage from the original gtkdialog |
| Documentation | `doc/*.texi`, `man/`, `*-site/*.html` | **GPL-2.0-or-later** | One licence for the whole tree. Documentation was announced as CC-BY-SA 4.0 while the texinfo manuals actually carried a GFDL notice of their own, and `debian/copyright` said GPL: three papers, three licences for the same files. Settled on 2026-08-25 in favour of the GPL. The texinfo manuals were written by the project and share no wording with László Pere's original manual, which was GFDL — measured: zero six-word sequences in common for `gtk3sermo`, and for `gtk4sermo` the only common ones came from a section the project had itself added to that manual. |
| Examples written by the project | `gtk3sermo/…/examples/showcase/` (5 scripts), `gtk3sermo/…/examples/system-tools/` (1 script), `tests/xml/*.xml` (55 files) | **CC0-1.0** | Public domain, usable without restriction. Checked on 2026-08-25: all 61 carry the header. The GTK 4 port has neither `showcase/` nor `system-tools/` — its examples are inherited from upstream gtkdialog and stay GPL-2.0-or-later. The Qt 6 port has neither directory either, but its examples are not inherited: its 32 `examples/*/demo.sh` scripts were written by the project and all carry the **CC0-1.0** header (checked on 2026-09-03). |
| Examples inherited from gtkdialog | the rest of `examples/` | **GPL-2.0-or-later** | Like the rest of the repository |
| Third-party icon sets | `examples/button/`, `examples/togglebutton/`, `examples/pfeme/` | **GPL-2** | elementary, fast-forward, nuvola — each with its own `COPYING` alongside |
| Packaging | `packaging/` | **GPL-2.0-or-later** | Build scripts tied to the project |

> **Status verified against the source.** The source files carry the
> clause "either version 2 of the License, or (at your option) any later
> version"; none of them references version 3. The SPDX headers and the
> `debian/copyright` files confirm **GPL-2.0-or-later** throughout, apart from the
> examples the project wrote itself (CC0-1.0).

## Full texts

- **GPL-2.0-or-later**: https://www.gnu.org/licenses/old-licenses/gpl-2.0.html
- **CC0-1.0**: https://creativecommons.org/publicdomain/zero/1.0/

## Provenance

haplo-dialog is a fork of **gtkdialog 0.8.3** by László Pere and Thunor,
distributed under GPL-2.0+. haplo-dialog keeps **GPL-2.0-or-later** (direct
continuity with upstream).

Original upstream: gtkdialog by László Pere (2003-2013), continued at
https://github.com/puppylinux-woof-CE/gtkdialog

Other descendants of the same code: https://github.com/01micko/gtkdialog and
https://github.com/BunsenLabs/gtk3dialog, both by Mick Amadio.

### Wayland anchoring

The `layer`, `edge`, `dist` and `reserve` attributes of `<window>` derive from
the **BunsenLabs gtk3dialog** fork (https://github.com/BunsenLabs/gtk3dialog),
also GPL-2.0-or-later: layer-shell support introduced by **Dima Krasner**
(2021), extended by **Mick Amadio** (edge anchoring, exclusive zone, opposite
edges, 2021-2024).

## SPDX identification in the sources

Every file under `src/`, in both GTK ports, carries an SPDX identifier on its
first line — 115 files for gtk3sermo, 131 for gtk4sermo, without exception. The
`tests/garde_spdx.sh` bench checks it on every CI run, so that this sentence
stays true.

The third port, `qt6sermo`, does **not** meet that rule yet. Measured on
2026-09-03 across its 123 `.c`/`.h`/`.cpp` files under `src/`: only four — the
ones taken from the shared core (`safe_exec.c`, `variables.c`, `stringman.c`,
`stack.c`) — carry an SPDX identifier; the others carry the licence in plain
text ("Licence : GPL-2.0-or-later") 32 carry the long upstream GPL header, and 26 carry no notice at all. The
`tests/garde_spdx.sh` bench is not run against it by CI. The port's licence is
not in doubt for all that: GPL-2.0-or-later, per
`qt6sermo/qt6sermo_1.0.0/packaging/debian/copyright` — what is missing is the
**form** of the notice, not the licence.

The `examples/` and `tests/xml/` directories fall under a different licence: see
above.

Form used:

```c
/* SPDX-License-Identifier: GPL-2.0-or-later */  // core, widgets, compat
```

```sh
# SPDX-License-Identifier: CC0-1.0               // showcase, system-tools, tests/xml
```

---
