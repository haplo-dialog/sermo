# Licences — haplo-dialog

## Summary

| Component | Files | Licence | Reason |
|-----------|-------|---------|--------|
| Core and widgets | `src/*.c`, `src/*.h`, `src/safe_exec.c`, `src/variables.c`, `src/stringman.c`, `src/stack.c` | **GPL-2.0-or-later** | Direct gtkdialog heritage (László Pere), strong copyleft |
| XML parser | `src/gtkdialog_lexer.l`, `src/gtkdialog_parser.y` | **GPL-2.0-or-later** | Direct heritage from the original gtkdialog |
| Documentation | `doc/*.texi`, `doc/*.info`, `man/`, `*-site/*.html` | **CC-BY-SA 4.0** | Standard free documentation |
| Examples | `examples/`, `tests/xml/*.xml` | **CC0-1.0** | Public domain — usable without restriction |
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

Original upstream: https://github.com/01micko/gtkdialog

## SPDX identification in the sources

Every source file carries an SPDX identifier in its header:

```c
/* SPDX-License-Identifier: GPL-2.0-or-later */  // core, widgets, compat
```

```sh
# SPDX-License-Identifier: CC0-1.0               // examples
```

---
