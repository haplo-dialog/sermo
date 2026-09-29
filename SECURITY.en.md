# Security — sermo

[Français](SECURITY.md)

sermo runs shell commands described in an XML script. Its security rests on
**where** that execution happens and **how** it is bounded. Most of it happens in
the **core**, `libsermocore`, shared by the seven backends: `safe_exec.c` exists
in two copies, `src/` (six ports) and `src-gtk4/` (gtk4), which differ only by
three comment lines; the unit tests exercise both.

Two paths escape the core, and that is where the limits are: the `<terminal>` of
gtk3 and gtk4, and the `<linkbutton>` — see
[What these two variables do not reach](#what-these-two-variables-do-not-reach).

## Threat model

The **trust boundary is the local author of the XML script**. sermo is not a
sandbox for running hostile XML: whoever writes the script can already run
commands. sermo's job is to **not turn a user's input into code execution** the
author did not intend.

## Hardened execution (in the core)

- **`safe_system` / `safe_popen`** replace `system`/`popen`. `safe_system` opens
  a shell **only** if the command contains shell metacharacters; otherwise it
  runs directly via `argv[]`, no shell. The `/bin/sh -c` fallback is **logged**.
- **No** `strcpy`/`strcat`/`sprintf`: systematic bounding.
- **Output escaping**: exported values (`VAR="…"`) escape the four characters the
  shell expands inside double quotes (`\`, `"`, `$`, backtick), so
  `eval "$(sermo …)"` does not re-execute what a user typed. Verified by
  `tests/garde_echappement_sortie.sh` (the "guard" bench, 4 cases).
- **`--do`** is the recommended path when the dialog may be used by someone other
  than the author: values arrive through the environment and are **never** read
  back as code.
- **A Glade file is code** (`--glade-xml`, gtk3sermo and gtk4sermo): its signal
  handlers are commands. They go through the **same path** as `<action>`
  (`safe_system`, `SERMO_ALLOWED_CMDS`, `SERMO_NO_SHELL_FALLBACK`), and an
  interface file deserves the same trust as a dialog script — no more.

## Size of what an `<input>` reads

Since 2.7.3, each `<input>` — command or file — reads **at most 16 MiB**.
Beyond that, reading stops as if at end of file, and **one** warning goes to
standard error, never to standard output (which an `eval` reads):

    <input> « yes » : lecture arrêtée à 16777216 octets (limite SERMO_INPUT_MAX)

A command cut off this way gets `SIGPIPE` on its next write: `yes` stops by
itself. The limit is set in the core (`libsermocore/src/sermo_input.c`) and
applies to all seven backends.

- **`SERMO_INPUT_MAX`** — the limit, in bytes. `0` removes it. A value that is
  not a number of bytes (`16M`, `-1`) is reported, and the default limit
  applies.
- **The progress bar is not capped.** It keeps one line at a time, and must be
  able to follow a long command to the end: a copy that prints every file name
  soon goes past 16 MiB. Its memory stays flat, even on `yes`.

Up to 2.7.2, nothing bounded this read. Fed by `yes`, gtk3sermo went past
700 MB in 7 seconds, and ncurses ended on a segmentation fault (measured on
2026-09-17). Checked by `tests/garde_input_sans_fin.sh` (seven ports, with an
unlimited control run) and `tests/unit/test_sermo_input.c`.

## Optional execution bounding (two environment variables)

Two guards, **off by default**, let you deploy a dialog into a less-trusted
context (a kiosk, a guest session) without changing the script. Both are read in
the core (`safe_exec.c`, both copies) and therefore apply to all seven
backends:

- **`SERMO_NO_SHELL_FALLBACK`** — when set (to any value), the `/bin/sh -c`
  fallback (used for commands containing shell metacharacters) is **refused**
  instead of merely logged: the program **fails closed** rather than opening a
  shell. Commands without metacharacters still run directly via `exec()`.
  Verified by `tests/garde_option_do.sh`.
- **`SERMO_ALLOWED_CMDS`** — a comma-separated list of allowed commands (e.g.
  `SERMO_ALLOWED_CMDS=ls,cat,/usr/local/bin/tool`). While set, only those
  commands may run, **and** the `/bin/sh -c` fallback is refused (otherwise
  `sh -c '…'` would bypass the list). The comparison is on the command **as it
  will be run**:
  - an entry without `/` (`ls`) allows that name, and the exact path the
    program's `PATH` resolves it to (`/usr/bin/ls`);
  - an entry with `/` allows exactly that path, and the bare name if `PATH`
    resolves it to that path;
  - a program that bears an allowed name but lives elsewhere (`/tmp/x/ls`) is
    **refused**.

  An empty value is the same as an unset variable. Verified by
  `tests/garde_allowed_cmds.sh`.

The 1.x names, **`HAPLO_NO_SHELL_FALLBACK`** and **`HAPLO_ALLOWED_CMDS`**, are
still read when the `SERMO_…` name is not set, with a warning: a deployment
hardened under 1.x stays hardened under 2.x.

### What these two variables do not reach

Three known limits. The first two open an execution path that does not go
through `safe_exec.c`, where neither the allow-list nor the refused fallback
applies.

- **`<terminal>` opens a shell outside both guards** (gtk3sermo and gtk4sermo,
  the only ports that have the widget). It spawns `/bin/sh` through VTE
  (`sermo-backend-gtk3/src/widget_terminal.c`), and the output of its `<input>`
  is **typed into it** as if on a keyboard (`vte_terminal_feed_child`). An
  allowed command that prints a line therefore has that line run by the
  terminal's shell. Measured on 2026-09-19 on the 2.7.1 package binary, with
  both `SERMO_ALLOWED_CMDS` and `SERMO_NO_SHELL_FALLBACK` set: a plain
  `<action>` is refused at the very moment the `<terminal>` writes its witness
  file. **A dialog containing a `<terminal>` hands out a shell**: do not deploy
  one where you believe those variables define the boundary.
- **Link buttons (`<linkbutton>`)** open their address through the desktop
  browser (`xdg-open` or the toolkit's equivalent), outside this list. They do
  not go through a shell, though: the call is a direct `execlp("xdg-open", …)`.
- **`--include` cancels the direct path, and conflicts with the list.** With
  that option every command is rewritten as `. 'file'; command` before it runs:
  the string always carries `;` and `'`, so **everything** goes through
  `/bin/sh -c`, even a command without a single metacharacter. And since the
  fallback is refused as soon as `SERMO_ALLOWED_CMDS` is set, `--include` plus
  an allow-list runs nothing at all. Both measured on 2026-09-19 on the 2.7.1
  package binary.

## Binary hardening

Core and backends compile and link with: `-D_FORTIFY_SOURCE=3`,
`-fstack-protector-strong`, `-fstack-clash-protection`, `-fcf-protection=full`
(CET IBT/SHSTK), PIE, full RELRO, BIND_NOW, NX. Keep them on **every** target —
verified by `tests/garde_durcissement.sh`.

`_FORTIFY_SOURCE` only works with optimisation. Every `CMakeLists.txt` therefore
picks the `Release` build type when none is given; a `Debug` build loses this
protection. The core is checked separately, on its library, by
`tests/garde_fortify_coeur.sh`: on qt6, sdl3 and ncurses the optimised backend
brings its own fortified calls and would hide a core that is not.

⚠️ **CET is marked, not active.** The IBT and SHSTK notes are forced at link
time (`-Wl,-z,ibt -Wl,-z,shstk`), and the bench finds them. But on Debian testing,
neither glibc's startup objects, nor glibc itself, nor the toolkit libraries carry
them (measured with `readelf` on 2026-09-16). On that system CET therefore cannot
act at run time — deduced from the missing notes, not measured at run time.

## Known limits

- **No sandbox.** A command started by a dialog has the rights of the user who
  opened it. sermo bounds *how* it is started, not *what it does*.
- **No privilege escalation**: no `pkexec`, nothing that asks for an
  administrator password.
- **No command list enforced by default**: trust goes to the script author. The
  optional `SERMO_ALLOWED_CMDS` list remains available (see above).
- **The cap applies to each `<input>`, not to the whole dialog.** A dialog that
  reads many `<input>`, or rereads them through `refresh:`, can hold 16 MiB
  several times over (see
  [Size of what an `<input>` reads](#size-of-what-an-input-reads)).
- **Memory grows in a dialog that refreshes forever**: each export of the
  variables loses a few bytes, and a `<list>` or `<table>` refreshed without
  `clear:` accumulates its rows.
- **`<terminal>` hands out a shell** (gtk3sermo, gtk4sermo): it spawns
  `/bin/sh` through VTE, outside `SERMO_ALLOWED_CMDS` and
  `SERMO_NO_SHELL_FALLBACK`, and the output of its `<input>` is typed into it as
  if on a keyboard (measured on 2026-09-19; see above).
- **`--include` sends everything through `/bin/sh -c`**, and runs nothing at all
  when a command list is set (see above).
- **A Glade file is code**, and **`<linkbutton>`** opens its address outside the
  command list (see above).
- **No external audit.** What is verified is verified by the repository's benches
  (`ci/bancs.sh`) and by the maintainer's reviews. Open gaps are listed in
  [TODO.md](TODO.md) (French only).

## Reporting a vulnerability

- Write to **`devel@haplo-dialog.fr`**. Do not open a public issue while the
  vulnerability is not fixed.
- Include: the command you ran (`gtk3sermo`, `qt6sermo`…) and its `--version`
  output, the package version, a minimal XML script and the steps that lead to
  the problem.
- **Timelines** — sermo has a single maintainer: acknowledgement within **14
  days**; a fix, or a public advisory with a workaround, no later than **90
  days** after the report, sooner if the vulnerability is already public.
- **Supported versions**: only the latest published release (2.7.x today)
  receives fixes. The 1.x line no longer does: move to 2.x
  ([MIGRATION.en.md](MIGRATION.en.md)).
- If you wish, your name goes into the [CHANGELOG.en.md](CHANGELOG.en.md) with the
  fix.
