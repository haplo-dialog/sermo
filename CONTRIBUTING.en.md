# Contribution guide — haplo-dialog

Thank you for your interest in haplo-dialog. This document explains how to contribute effectively to the project.

---

## Table of contents

1. [Before you begin](#1-before-you-begin)
2. [Project structure](#2-project-structure)
3. [Setting up the environment](#3-setting-up-the-environment)
4. [Contribution workflow](#4-contribution-workflow)
5. [Code standards](#5-code-standards)
6. [Adding a widget](#6-adding-a-widget)
7. [Tests](#7-tests)
8. [Documentation](#8-documentation)
9. [Security](#9-security)
10. [Code review](#10-code-review)

---

## 1. Before you begin

- Read [SECURITY.md](SECURITY.md) **in full** before any change to the core
- Follow the [Code of Conduct](CODE_OF_CONDUCT.md)
- For security bugs: write to `devel@haplo-dialog.fr` — **do not open a public ticket**
- For functional bugs: open a ticket with a minimal reproducible XML script

**Target: gtk3dialog (GTK3), which provides `gtkdialog`.**  
Fixes on these two ports are handled as a priority. The other ports are "community ports" — patches are welcome but review may be slower.

---

## 2. Project structure

```
haplo-dialog/
├── {port}dialog/{bin}_{version}/
│   ├── src/
│   │   ├── gtk3dialog.c          ← main entry point
│   │   ├── safe_exec.c      ← GPL-2.0-or-later
│   │   ├── variables.c      ← GPL-2.0-or-later
│   │   ├── stringman.c      ← GPL-2.0-or-later
│   │   ├── widget_*.c       ← one file per widget (GPL-2.0-or-later)
│   │   └── {compat}.h       ← toolkit compatibility layer
│   ├── tests/
│   │   ├── test_safe_exec.c ← CTest — compiles without a toolkit
│   │   └── test_*.c
│   ├── examples/
│   │   └── {widget}/demo.sh ← CC0 — usable without restriction
│   ├── doc/
│   └── packaging/
└── tests/xml/               ← XML regression common to all ports
```

---

## 3. Setting up the environment

### Minimal dependencies (gtk3dialog)

```sh
# Debian / Haplo-Linux
sudo apt install build-essential flex bison libgtk-3-dev pkg-config

# Arch
sudo pacman -S base-devel flex bison gtk3

# Fedora
sudo dnf install gcc flex bison gtk3-devel
```

### Development build

```sh
cd gtk3dialog/gtk3dialog_1.0.0
autoreconf -fi
./configure --prefix=/usr/local CFLAGS="-g -O0 -fsanitize=address"
make -j$(nproc)
make check        # run the tests

# Check gtkdialog compatibility: the same binary answers to both names
ls -l /usr/local/bin/gtkdialog   # -> gtk3dialog
```

### Recommended tools

```sh
sudo apt install clang-format cppcheck valgrind
# Check the formatting before committing:
clang-format --dry-run --Werror src/widget_button.c
```

---

## 4. Contribution workflow

```
1. Fork / clone the repository
2. Create a branch: git checkout -b fix/widget-entry-null-crash
3. Modify the code — one subject per branch
4. Test: make check && ctest
5. Check the format: clang-format -i src/*.c src/*.cpp
6. Commit: git commit -m "fix(widget_entry): handle NULL label gracefully"
7. Submit a patch by email to devel@haplo-dialog.fr
   or open a Merge Request if a public Git repository is available
```

### Commit message format

```
type(scope): short description in English (< 72 chars)

Optional body: explanation of the why, not the how.
Reference: Fixes #123
```

Valid types: `fix`, `feat`, `docs`, `test`, `refactor`, `security`, `build`, `ci`

Valid scopes: `core`, `widget_button`, `gtk3dialog`, `packaging`, `doc`

---

## 5. Code standards

### C (core and GTK/EFL widgets)

- Standard: **C11** (`-std=c11`)
- Formatting: `.clang-format` provided at the root — **mandatory**
- No direct `system()`, `popen()`, `strcpy()`, `sprintf()` — use `safe_system()`, `safe_popen()`, `g_strlcpy()`, `snprintf()`
- Any `FILE*` returned by `safe_popen()` → `fclose()`, **never `pclose()`**
- Check the return of `malloc()` / `calloc()` — `NULL` = fatal log + return
- No unbounded dynamic allocation in the widgets

```c
/* ✅ Correct */
WidgetNode *n = calloc(1, sizeof(WidgetNode));
if (!n) { fprintf(stderr, "OOM\n"); return NULL; }

/* ❌ Forbidden */
system("ls");
char *p = malloc(strlen(user_input) + 1); /* unbounded */
```

### C++ (Qt6/FLTK widgets)

- Standard: **C++17**
- No `new` without a matching `delete` — prefer Qt smart pointers (`QScopedPointer`) or FLTK RAII
- Classes inheriting from `QObject`: always pass the parent in the constructor

### Shell (examples)

- Shebang: `#!/bin/sh` (POSIX, not bash-specific unless necessary)
- `set -e` recommended in complex scripts
- No `eval` on user variables

---

## 6. Adding a widget

To add `<monwidget>` to an existing port:

**1. Create `src/widget_monwidget.c` (or `.cpp`)**

```c
/* widget_monwidget.c — <monwidget> for gtk3dialog
 * haplo-dialog 1.0.0 — GPL-2.0-or-later
 * The haplo-dialog team <devel@haplo-dialog.fr> */

#include "dialog_state.h"
#include "widget_monwidget.h"

GtkWidget *widget_monwidget_create(WidgetNode *wnode) {
    /* ... */
}

char *widget_monwidget_envvar_construct(WidgetNode *wnode) {
    /* Export the value to the shell environment */
    return g_strdup_printf("%s=%s", wnode->var_name, /* value */);
}
```

**2. Create `src/widget_monwidget.h`**

**3. Add to `src/automaton.h`** (or equivalent depending on the port):
```c
#define WIDGET_MONWIDGET  0x00C10000
```

**4. Wire it in `src/widgets.c` / `widgets.cpp`**:
```c
case WIDGET_MONWIDGET:
    widget = widget_monwidget_create(wnode);
    break;
```

**5. Add to `CMakeLists.txt` or `Makefile.am`**

**6. Create `examples/monwidget/demo.sh`**

**7. Update the port's `BILAN_SANTE.md` and `BILAN_GENERAL.md`**

---

## 7. Tests

### Unit tests (C core)

```sh
cd gtk3dialog/gtk3dialog_1.0.0/tests
make check        # test_safe_exec, test_stringman
```

The C core unit tests must **not** depend on GTK — only pure C.

### XML regression tests

```sh
cd tests/xml
./run_tests.sh gtk3dialog    # 52 XML conformance test cases
```

### Valgrind (memory)

```sh
valgrind --leak-check=full --error-exitcode=1 \
  gtk3dialog --file tests/xml/01_button.xml --do EXIT:exit
```

---

## 8. Documentation

- Manpages are in **roff** format (`src/{bin}.1`) — kept up to date with each new widget
- The common XML reference is in `haplo-dialog-xml(5)` — `man/haplo-dialog-xml.5`
- The Texinfo documentation is in `doc/{port}.texi`
- The examples (`examples/`) are **CC0** — use them without restriction

---

## 9. Security

**Absolute rules — any patch that violates them will be rejected:**

1. Never direct `system()` / `popen()` — always `safe_system()` / `safe_popen()`
2. gtk3dialog performs no privilege escalation — do not add any
3. Validate all file paths before opening
4. Do not bypass the shell-free fallback of `safe_exec.c` (metacharacters → `/bin/sh -c`, refusable via `HAPLO_NO_SHELL_FALLBACK`)
5. No `eval` on user data
6. `fclose()` on any `FILE*` — never `pclose()`

To report a vulnerability: `devel@haplo-dialog.fr` (GPG encryption available).

---

## 10. Code review

Review checks, in order:
1. **Security** — the 6 absolute rules above
2. **Compilation** — gcc and clang warning-free with `-Wall -Wextra`
3. **Formatting** — `clang-format` with no diff
4. **Tests** — `make check` / `ctest` passes
5. **Documentation** — BILAN_SANTE and manpage updated if a new widget
6. **Style** — clarity, naming consistent with the existing port

---

*Thank you for contributing to haplo-dialog — every shell script that displays a real interface is better than an `echo "Enter a choice: "` in a terminal.*

---

*Timestamped document — last updated: 2026-06-07 (audit #3 — haplo-dialog v1.0.0).*
