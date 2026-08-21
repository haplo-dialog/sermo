# Contribution guide - haplo-dialog

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

- Read [SECURITY.md](SECURITY.en.md) **in full** before any change to the core
- Follow the [Code of Conduct](CODE_OF_CONDUCT.md)
- For security bugs: write to `devel@haplo-dialog.fr`, **do not open a public ticket**
- For functional bugs: open a ticket with a minimal reproducible XML script

**Reference port:**  
haplo-dialog provides a single port, `gtk3sermo` (GTK 3 backend), which provides the backwards-compatible alias `gtkdialog`. It is a maintained descendant of gtkdialog (a fork of Laszlo Pere's gtkdialog 0.8.3), fixed and hardened. Other descendants exist, notably Mick Amadio's BunsenLabs fork, which also ports gtkdialog to GTK 3.

---

## 2. Project structure

```
haplo-dialog/
├── gtk3sermo/
│   └── gtk3sermo_1.0.0/
│       ├── src/
│       │   ├── gtk3sermo.c      ← main entry point
│       │   ├── safe_exec.c       ← GPL-2.0-or-later
│       │   ├── variables.c       ← GPL-2.0-or-later
│       │   ├── stringman.c       ← GPL-2.0-or-later
│       │   └── widget_*.c        ← one file per widget (GPL-2.0-or-later)
│       ├── tests/
│       ├── examples/             ← #!/bin/sh scripts, one per widget (CC0)
│       ├── doc/
│       └── packaging/
└── tests/
    ├── xml/                      ← XML regression (52 cases)
    ├── unit/test_safe_exec.c     ← behaviour tests (safe_exec)
    └── fuzz/                     ← fuzzing corpus
```

---

## 3. Setting up the environment

### Build dependencies

```sh
# Debian / Ubuntu
sudo apt install build-essential flex bison libgtk-3-dev pkg-config

# Arch
sudo pacman -S base-devel flex bison gtk3

# Fedora
sudo dnf install gcc flex bison gtk3-devel
```

### Development build

```sh
cd gtk3sermo/gtk3sermo_1.0.0
autoreconf -fi
./configure --prefix=/usr/local CFLAGS="-g -O0 -fsanitize=address"
make -j$(nproc)
make check        # run the tests
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
3. Modify the code, one subject per branch
4. Test: make check
5. Check the format: clang-format -i src/*.c
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

Valid scopes: `core`, `widget_button`, `gtk3sermo`, `packaging`, `doc`

---

## 5. Code standards

### C (core and GTK 3 widgets)

- Standard: **C11** (`-std=c11`)
- Formatting: `.clang-format` provided at the root, **mandatory**
- No direct `system()`, `popen()`, `strcpy()`, `sprintf()`, use `safe_system()`, `safe_popen()`, `g_strlcpy()`, `snprintf()`
- Any `FILE*` returned by `safe_popen()` → `fclose()`, **never `pclose()`**
- Check the return of `malloc()` / `calloc()`, `NULL` = fatal log + return
- No unbounded dynamic allocation in the widgets

```c
/* ✅ Correct */
char *buf = calloc(count, size);
if (!buf) { fprintf(stderr, "OOM\n"); return NULL; }

/* ❌ Forbidden */
system("ls");
char *p = malloc(strlen(user_input) + 1); /* unbounded */
```

### Shell (examples)

- Shebang: `#!/bin/sh` (POSIX, not bash-specific unless necessary)
- `set -e` recommended in complex scripts
- No `eval` on user variables

---

## 6. Adding a widget

A widget is wired in at **several points**: the implementation, the grammar
(lexer + parser) and the dispatch points (`automaton.c`, `widgets.c`). Take an
existing widget as a model, here `<switch>` (`src/widget_switch.c/.h`).

**1. Implement `src/widget_monwidget.c` + `src/widget_monwidget.h`**

The actual prototype follows that of the existing widgets (see `src/widget_switch.h`):

```c
/* widget_monwidget.h */
GtkWidget *widget_monwidget_create(AttributeSet *Attr, tag_attr *attr, gint Type);
gchar     *widget_monwidget_envvar_construct(GtkWidget *widget);
```

```c
/* widget_monwidget.c */
#include <gtk/gtk.h>
#include "gtk3d.h"
#include "widget_monwidget.h"

GtkWidget *widget_monwidget_create(AttributeSet *Attr, tag_attr *attr, gint Type)
{
    GtkWidget *widget = /* … create the native GtkWidget … */;
    return widget;
}

/* Exports the widget state in the historical VAR="value" format */
gchar *widget_monwidget_envvar_construct(GtkWidget *widget)
{
    /* … read the value and return the string … */
}
```

**2. Declare the type constant in `src/automaton.h`**

```c
#define WIDGET_MONWIDGET  0x00C00000   /* next free value in the sequence */
```

**3. Recognise the tags in the lexer `src/gtkdialog_lexer.l`**

Following the `<switch>` model, return the opening / with-attributes / closing tokens:

```
\<monwidget\>    { Token="<monwidget>"; return(MONWIDGET); }
\<monwidget[ ]+  { Token="<monwidget>"; BEGIN(ST_TAG_ATTR); return(PART_MONWIDGET); }
\<\/monwidget\>  { Token="</monwidget>"; return(EMONWIDGET); }
```

**4. Declare the tokens and the rules in `src/gtkdialog_parser.y`**

```
%token MONWIDGET PART_MONWIDGET EMONWIDGET
```

then add the productions that push the type, as for `SWITCH`, which
calls `token_store(PUSH | WIDGET_SWITCH)` (and `token_store_attr(...)` for the
form with attributes).

**5. Instantiate it in `src/automaton.c`**

This is where, and not in `widgets.c`, the widget is created (see the
`case WIDGET_SWITCH`):

```c
case WIDGET_MONWIDGET:
    Widget = widget_monwidget_create(Attr, tag_attributes, Widget_Type);
    push_widget(Widget, Widget_Type);
    break;
```

Also add the `case WIDGET_MONWIDGET:` to the `--print-ir` representation
in the same file.

**6. Dispatch the output in `src/widgets.c`**

In `widget_get_text_value()` (value export) and `widgets_to_str()`
(human-readable name):

```c
case WIDGET_MONWIDGET:
    string = widget_monwidget_envvar_construct(widget);
    return string;
/* … and, in widgets_to_str() … */
case WIDGET_MONWIDGET:
    type = "MONWIDGET";
    break;
```

**7. Add the sources to `src/Makefile.am`**

```
    widget_monwidget.c widget_monwidget.h \
```

**8. Provide an example and a test**

- an example script under `examples/monwidget/` (like the existing examples:
  `#!/bin/sh`, named after the widget);
- an XML regression case in `tests/xml/`.

---

## 7. Tests

### Unit tests (toolkit-free)

```sh
cd gtk3sermo/gtk3sermo_1.0.0
autoreconf -fi && ./configure && make check
```

The unit tests must **not** depend on GTK 3, only on the pure C core (`safe_exec`, `variables`, `stringman`).

### XML regression tests

```sh
cd tests/xml
./run_tests.sh gtk3sermo    # tests the 52 reference XML files (--print-ir mode, headless)
```

### Behaviour tests (unit)

Beyond parsing, the core logic is actually **executed** (`safe_exec`
security: `safe_system`/`safe_popen`). "Plain `main()`": neither libcheck nor an
X server required, so runnable in CI. 9 behaviour tests.

```sh
./tests/run_unit_tests.sh gtk3sermo
```

### Valgrind (memory)

```sh
valgrind --leak-check=full --error-exitcode=1 \
  gtk3sermo --file tests/xml/01_button.xml --do EXIT:exit
```

### Continuous integration (CI)

The repository provides `.gitlab-ci.yml` (GitLab CI, `debian:testing` image, target: Debian testing). On every push / merge request, the job:

1. installs the build dependencies;
2. compiles (autotools);
3. validates the binary against the 52 XML cases (`tests/xml/run_tests.sh`, `--print-ir`,
   without an X server).

A broken build or a single failing test fails the pipeline. Before pushing,
you can reproduce the job locally:

```sh
cd gtk3sermo/gtk3sermo_1.0.0
autoreconf -fi && ./configure --prefix="$PWD/inst" && make -j"$(nproc)" && make install
PATH="$PWD/inst/bin:$PATH" sh ../../tests/xml/run_tests.sh gtk3sermo
```

---

## 8. Documentation

- Manpages are in **roff** format (`src/gtk3sermo.1`), kept up to date with each new widget
- The XML reference is in `haplo-dialog-xml(5)`, `man/haplo-dialog-xml.5`
- The Texinfo documentation is in `doc/`
- The examples (`examples/`) are **CC0**, use them without restriction

---

## 9. Security

**Absolute rules, any patch that violates them will be rejected:**

1. Never direct `system()` / `popen()`, always `safe_system()` / `safe_popen()`
2. `<action>`/`<input>` commands go through `safe_exec`: direct execution without a shell when possible; **logged** `/bin/sh -c` fallback, which can be disabled (`HAPLO_NO_SHELL_FALLBACK`)
3. Systematic bounds on buffers, `g_strlcpy`/`g_strlcat`, never `strcpy`/`strcat`/`sprintf`
4. No `eval`: values come back through the environment/the output, never evaluated by the tool
5. `fclose()` on any `FILE*`, never `pclose()`

To report a vulnerability: `devel@haplo-dialog.fr` (GPG encryption available).

---

## 10. Code review

Review checks, in order:
1. **Security**, the 5 absolute rules above
2. **Compilation**, gcc and clang warning-free with `-Wall -Wextra`
3. **Formatting**, `clang-format` with no diff
4. **Tests**, `make check` passes
5. **Documentation**, manpage(s) and examples updated if a new widget
6. **Style**, clarity, naming consistent with the existing code

---

*Thank you for contributing to haplo-dialog, every shell script that displays a real interface is better than an `echo "Enter a choice: "` in a terminal.*

---

