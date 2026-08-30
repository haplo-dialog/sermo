# Security policy — sermo

**Maintainer:** haplo-dialog, devel@haplo-dialog.fr
**Date:** 2026-08-27

---

## Supported versions

| Port | Version | Security support |
|------|---------|-----------------|
| gtk3sermo | 1.1.3 | ✅ Active |
| gtk3sermo | 1.1.2 | ⚠️ **Crash** — see below. Take 1.1.3. |
| gtk3sermo | 1.1.1 | ⚠️ **Crash** — see below. Take 1.1.3. |
| gtk3sermo | 1.0.0 | ⚠️ No longer supported — three defects fixed in 1.1.0 ; see also 1.1.1 |
| gtk4sermo | 1.1.3 | ✅ Active |
| gtk4sermo | 1.1.2 | ⚠️ **Crash** — see below. Take 1.1.3. |
| gtk4sermo | 1.1.1 | ⚠️ **Crash** — see below. Take 1.1.3. |
| gtk4sermo | 1.0.0 | ⚠️ No longer supported — three defects fixed in 1.1.0 ; see also 1.1.1 |

> ### ⚠️ Every version before 1.1.3 crashes on `<infobar>`
>
> Clicking an `<infobar>`'s close button kills the program (SIGSEGV). The
> `response` signal handler received an integer where it expected a pointer —
> measured arity `(GtkInfoBar *, gint, gpointer)` against the connected
> `(GtkWidget *, AttributeSet *)`. Reproduced on the **published** packages,
> downloaded anonymously: `rc=139`.
>
> The defect affects **both ports**, `gtk3sermo` included. It is fixed only in
> **1.1.3**.


Both ports share the same C core and the same grammar. Since 2026-08-24 they
have the **same memory-safety posture**: `g_strlcpy` for name copies, an explicit
bound on copying a container's widgets, and none of the
`strcpy`/`strcat`/`sprintf`/`gets` family — which a bench replays on both `src/`
trees at every push. The backwards-compatible `gtkdialog`
alias ships in a separate package, `gtksermo`.

⚠️ The GTK 4 port is the younger one: it received the same pass, but it has less
real-world use behind it. What remains open is listed, port by port, in the
`TODO-SECURITY.md` files.

---

## Reporting a vulnerability

Send an email to **devel@haplo-dialog.fr** with:

- a description of the problem
- reproduction steps
- estimated impact (CVE if applicable)
- your contact details (so we can credit you)

**Response time:** 48 hours maximum.
**Responsible disclosure:** we ask for 90 days before public disclosure.

---

## Security mechanisms in place

### Compile-time hardening

| Mechanism | Flag | Effect |
|-----------|------|--------|
| FORTIFY_SOURCE=3 | `-D_FORTIFY_SOURCE=3` | Buffer overflow detection at compile and run time |
| Stack canary | `-fstack-protector-strong` | Stack smashing detection |
| Stack clash | `-fstack-clash-protection` | Protection against stack clash attacks |
| Control Flow | `-fcf-protection=full` | Hardware CFI (x86 CET) |
| PIE | `-fPIE -pie` | ASLR, randomised addresses |
| Full RELRO | `-Wl,-z,relro -Wl,-z,now` | Read-only GOT |
| NX Stack | `-Wl,-z,noexecstack` | Non-executable stack |
| Format security | `-Wformat=2 -Werror=format-security` | Blocks dangerous format strings |
| Overflow format | `-Wformat-overflow=2` | printf overflow detection |

### Run-time protection

| Mechanism | Implementation |
|-----------|---------------|
| `safe_exec()` / `safe_popen()` | Neither `system()` nor `popen()`. A command **without** shell metacharacters → direct `exec()` (argv, no shell). A command **with** metacharacters → fallback to `/bin/sh -c`, **logged**. |
| “Fail-closed” refusal | `HAPLO_NO_SHELL_FALLBACK=1` refuses any `/bin/sh -c` fallback (fails instead of executing). |
| Optional command list | `HAPLO_ALLOWED_CMDS=ls,cat,date` bounds which commands may run. **Unset by default**: the language exists to run commands, and 14 of the shipped examples call `bash` or `sh` — a list on by default would break the product without protecting anyone, since the command comes from the script the caller wrote. It targets whoever **deploys** a dialog into a less-trusted context. The name compared is the base name (`/bin/ls` = `ls`), and while it is set the `/bin/sh -c` fallback is refused too — otherwise `sh -c` would walk through it. |
| Command length | Bounded in `safe_exec()`/`safe_popen()`. |
| Child environment | Filtered: the `DIALOG` block (several KiB of XML) is not inherited by spawned processes. |
| Memory safety | `g_strlcpy` (variable names), container widget copy bounded to `MAXWIDGETS` with a named refusal, spawn `argv` always NUL-terminated; no forbidden function (`strcpy`/`strcat`/`sprintf`/`gets`/`system`/`popen`) called in `src/` — **checked on every push** by `tests/garde_fonctions_interdites.sh`, which skips comments and requires a word boundary on the left (`safe_system` does not count as `system`). |
| Thread safety | No `gtk_*`/`gdk_*` call outside the main thread: the progress-bar thread hands off through `g_idle_add`. `gdk_threads_enter()` has locked nothing since GTK 3.6 — inherited code that looks protected is not. Checked by `tests/garde_progressbar_thread.sh`. |
| Output handed to the shell | The `NAME="value"` and `EXIT="value"` lines escape all **four** characters the shell expands inside double quotes: `\`, `"`, `$` and the backtick. Without that, a value **typed by whoever uses the dialog** — not necessarily whoever wrote the script — becomes code as soon as the caller runs `eval`. Measured on 2026-08-25 before the fix: typing `$(touch /tmp/proof)` into a field and evaluating the line created the file. Checked on every push by `tests/garde_echappement_sortie.sh`, which opens a real window, clicks, and watches a witness file. |
| XML parser | Clean rejection of malformed XML (message + non-zero exit code, never an `abort`); parser subjected to **fuzzing** (`tests/fuzz/`). |

### Trust model

Both ports run the interface described by the **author of the XML script**, just
as a shell script runs what its author writes. The `<action>`/`<input>` tags can
launch commands: **this is intended and documented**. The trust boundary is
therefore the **local** author of the script, not a remote third party. The
hardening above protects against **malformed input** and **memory-safety bugs** —
not against a hostile script author, who can launch commands anyway. To interpret
XML coming from a less trustworthy source, set `HAPLO_NO_SHELL_FALLBACK=1`.

### One more boundary: whoever USES the dialog

The script's author and the person in front of the screen are not always the
same. A script that shows a window to someone else and then runs `eval` on the
output hands them command execution if that output is not escaped. That was the
case until 2026-08-25.

Two pieces of advice, in order:

1. **Prefer `--do`.** Values arrive through the environment and are never read
   back as code. It is the only route that does not depend on the quality of an
   escaping routine.
2. If you insist on `eval`, know that the line is now escaped for a
   double-quoted context — but `eval` is still `eval`: do not use it on output
   you did not produce yourself.

### Compiler warnings

Exactly one is treated as an **error**, the one that stops the build:

```
-Werror=format-security
```

The others are enabled but **stay warnings**: the build succeeds with them. 24
remain on the GTK 3 side and 36 on the GTK 4 side, inherited from upstream
gtkdialog.

```
-Wall -Wextra -Wformat=2 -Wformat-overflow=2 -Wshadow -Wnull-dereference
-Wstrict-prototypes -Wimplicit-fallthrough=3
```

Clearing them and switching to a global `-Werror` is an open track, not a fact:
claiming it before doing it would be a guarantee the compiler contradicts.

---

## Patch history

| Date | Port | Description | Severity |
|------|------|-------------|----------|
| 2026-08-24 | gtk4sermo | Memory-safety pass: `g_strlcpy` for the variable name (a name of 512 characters or more was left unterminated — CWE-170, then CWE-125), `MAXWIDGETS` bound on the widget copy (CWE-787: 300 direct children were silently accepted), and `action_append()` copying its first parameter from the start of the string. Regression test: `tests/garde_maxwidgets.sh` | Medium |
| 2026-08-11 | gtk3sermo | Memory-safety pass: `g_strlcpy` (variable names), widget copy bounded to `MAXWIDGETS`, NUL-terminated spawn `argv`, child environment filtering | Medium |
| 2026-05-29 | gtk3sermo | FORTIFY_SOURCE upgrade 2→3, stack-clash added, noexecstack | Medium |
| 2026-05-22 | gtk3sermo | Binaries renamed, gtkdialog symlink fixed | Low |
| 2026-05-01 | gtk3sermo | Initial port, safe_exec, PIE, RELRO | High |

---

## Post-installation verification

```bash
# Check the protections of the installed binary:
hardening-check /usr/bin/gtk3sermo
checksec --file=/usr/bin/gtk3sermo

# Check that ASLR is active on the system:
cat /proc/sys/kernel/randomize_va_space   # must be 2
```

---

*haplo-dialog is a fork of gtkdialog (László Pere, GPL-2.0-or-later).*
*haplo-dialog, devel@haplo-dialog.fr, 2026*
