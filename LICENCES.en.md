# Licences - haplo-dialog

## Summary

| Component | Files | Licence | Reason |
|-----------|-------|---------|--------|
| Core and widgets | `src/*.c`, `src/*.h`, `src/safe_exec.c`, `src/variables.c`, `src/stringman.c`, `src/stack.c` | **GPL-2.0-or-later** | Direct gtkdialog heritage (László Pere), strong copyleft |
| XML parser | `src/gtkdialog_lexer.l`, `src/gtkdialog_parser.y` | **GPL-2.0-or-later** | Direct heritage from the original gtkdialog |
| Documentation | `doc/*.texi`, `doc/*.info`, `man/`, `*-site/*.html` | **CC-BY-SA 4.0** | Standard free documentation |
| Examples | `examples/`, `tests/xml/*.xml` | **CC0-1.0** | Public domain, usable without restriction |
| Packaging | `packaging/` | **GPL-2.0-or-later** | Build scripts tied to the project |

> **Status verified against the source.** The source files carry the
> clause "either version 2 of the License, or (at your option) any later
> version"; none of them references version 3. The SPDX headers and the
> `debian/copyright` files confirm **GPL-2.0-or-later** throughout, apart from the
> examples (CC0-1.0).

## Full texts

- **GPL-2.0-or-later**: https://www.gnu.org/licenses/old-licenses/gpl-2.0.html
- **CC-BY-SA 4.0**: https://creativecommons.org/licenses/by-sa/4.0/
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

Every file under `src/`, in both ports, carries an SPDX identifier on its first
line — 115 files for gtk3sermo, 131 for gtk4sermo, without exception. The
`tests/garde_spdx.sh` bench checks it on every CI run, so that this sentence
stays true.

The `examples/` and `tests/xml/` directories fall under a different licence: see
above.

Form used:

```c
/* SPDX-License-Identifier: GPL-2.0-or-later */  // core, widgets, compat
```

```sh
# SPDX-License-Identifier: CC0-1.0               // examples
```

---
