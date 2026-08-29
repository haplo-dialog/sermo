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
haplo-dialog provides two ports, `gtk3sermo` (GTK 3 backend) and `gtk4sermo` (GTK 4 backend). The backwards-compatible `gtkdialog` alias ships in a separate package, `gtksermo`. It is a maintained descendant of gtkdialog (a fork of Laszlo Pere's gtkdialog 0.8.3), fixed and hardened. Other descendants exist, notably Mick Amadio's BunsenLabs fork, which also ports gtkdialog to GTK 3.

---

## 2. Project structure

```
haplo-dialog/
├── gtk3sermo/
│   └── gtk3sermo_1.1.3/
│       ├── src/
│       │   ├── gtk3sermo.c      ← main entry point
│       │   ├── safe_exec.c       ← GPL-2.0-or-later
│       │   ├── variables.c       ← GPL-2.0-or-later
│       │   ├── stringman.c       ← GPL-2.0-or-later
│       │   └── widget_*.c        ← one file per widget (GPL-2.0-or-later)
│       ├── tests/
│       ├── examples/             ← #!/bin/sh scripts, one per widget (GPL; showcase/ and system-tools/ are CC0)
│       ├── doc/
│       └── packaging/
└── tests/
    ├── xml/                      ← XML regression (55 cases)
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
cd gtk3sermo/gtk3sermo_1.1.3
autoreconf -fi
./configure --prefix=/usr/local CFLAGS="-g -O0 -fsanitize=address"
make -j$(nproc)
make check        # run the tests
```

### Recommended tools

```sh
sudo apt install clang-format cppcheck valgrind
# On a NEW file only — inherited files are not conformant, and --Werror would
# always fail on them (see §5):
clang-format --dry-run src/my_new_file.c
```

---

## 4. Contribution workflow

```
1. Fork / clone the repository
2. Create a branch: git checkout -b list-crashes-on-empty-item
3. Modify the code, one subject per branch
4. Test the port you touched: make check, then
   sh tests/xml/run_tests.sh <port> and sh tests/run_unit_tests.sh <port>
5. If the fix touches BOTH ports, carry it into both: they share the core,
   and a silent divergence appears fast
6. Commit (see below)
7. Open a Merge Request on gitlab.com/haplo-dialog/sermo,
   or send the patch to devel@haplo-dialog.fr
```

### Commit message format

This repository does **not** use conventional commits (`fix(scope): …`). Its
convention, visible in `git log`, is a **French sentence** stating what changed
in behaviour — not which file moved:

```
La barre de progression faisait tourner GTK depuis son thread de lecture

Body: the why, the before/after measurement, the test that prevents a relapse.
Reference: Fixes #123
```

Three rules, and that is all:

- **Subject in French**, under 72 characters, no trailing period.
- It describes **the defect or the effect**, from the point of view of whoever
  uses it — "the repo told its readers to install a file that does not exist"
  rather than "fix README".
- The body carries **the why and the measurement**. A security or crash fix
  quotes the before/after figure and the bench that locks it down.

`git log --oneline -20` conveys the tone better than this list.

*Yes, the subject line is French while this document is in English: the project
is written in French and its history is one continuous French thread. Splitting
it by language would make `git log` unreadable.*

---

## 5. Code standards

### C (core and widgets, both ports)

- Standard: **C11** (`-std=c11`)
- Formatting: do **not** run `clang-format -i` on existing files. The
  `.clang-format` at the root describes the target style for **new** code; it
  does not describe the code inherited from gtkdialog. Measured on 2026-08-25
  over `gtkdialog.c`, `actions.c` and `variables.c` (3,766 lines): the current
  settings would reformat **2,460** of them, and the best settings tried still
  **1,321**. So `clang-format -i src/*.c` would produce an unreadable diff in
  which the actual fix could not be found. For new code:
  `clang-format --dry-run <your-file.c>`. For a touch-up inside an existing
  file: match the neighbouring lines (tabs, K&R braces).
- No direct `system()`, `popen()`, `strcpy()`, `strcat()`, `sprintf()`, `gets()`:
  use `safe_system()`, `safe_popen()`, `g_strlcpy()`, `g_strlcat()`,
  `g_snprintf()`. **This is not merely advice**:
  `tests/garde_fonctions_interdites.sh` replays it on both `src/` trees at every
  push, and CI turns red.
- No `gtk_*`/`gdk_*` call outside the main thread — a worker thread computes,
  then hands the result to the main loop through `g_idle_add()`.
  `gdk_threads_enter()` has locked nothing since GTK 3.6: inherited code that
  looks protected is not. Checked by `tests/garde_progressbar_thread.sh`.
- No `atof()` or `strtod()`: under a French locale they silently read `0.5` as
  `0`. Use `g_ascii_strtod()`.
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
cd gtk3sermo/gtk3sermo_1.1.3
autoreconf -fi && ./configure && make check
```

The unit tests must **not** depend on GTK 3, only on the pure C core (`safe_exec`, `variables`, `stringman`).

### XML regression tests

```sh
cd tests/xml
./run_tests.sh gtk3sermo    # tests the 55 reference XML files (--print-ir mode, headless)
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
3. validates the binary against the 55 XML cases (`tests/xml/run_tests.sh`, `--print-ir`,
   without an X server).

A broken build or a single failing test fails the pipeline. Before pushing,
you can reproduce the job locally:

```sh
cd gtk3sermo/gtk3sermo_1.1.3
autoreconf -fi && ./configure --prefix="$PWD/inst" && make -j"$(nproc)" && make install
PATH="$PWD/inst/bin:$PATH" sh ../../tests/xml/run_tests.sh gtk3sermo
```

---

## 8. Documentation

- Manpages are in **roff** format (`src/gtk3sermo.1`), kept up to date with each new widget
- The XML reference is in `haplo-dialog-xml(5)`, `man/haplo-dialog-xml.5`
- The Texinfo documentation is in `doc/`
- `examples/showcase/` and `examples/system-tools/` are **CC0**: use them without restriction. The rest of `examples/` comes from gtkdialog and stays **GPL-2.0-or-later**

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

