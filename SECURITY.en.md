# Security Policy — haplo-dialog

**Maintainer:** haplo-dialog — devel@haplo-dialog.fr
**Date:** 2026-05-29

---

## Supported versions

| Port | Version | Security support |
|------|---------|-----------------|
| gtk3dialog | 1.0.0 | ✅ Active |

gtk3dialog is the single, reference port (GTK 3 backend, 43 widgets); it provides
the backwards-compatible `gtkdialog` alias.

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
| PIE | `-fPIE -pie` | ASLR — randomised addresses |
| Full RELRO | `-Wl,-z,relro -Wl,-z,now` | Read-only GOT |
| NX Stack | `-Wl,-z,noexecstack` | Non-executable stack |
| Format security | `-Wformat=2 -Werror=format-security` | Blocks dangerous format strings |
| Overflow format | `-Wformat-overflow=2` | printf overflow detection |

### Run-time protection

| Mechanism | Implementation |
|-----------|---------------|
| `safe_exec()` / `safe_popen()` | Neither `system()` nor `popen()`. A command **without** shell metacharacters → direct `exec()` (argv, no shell). A command **with** metacharacters → fallback to `/bin/sh -c`, **logged**. |
| “Fail-closed” refusal | `HAPLO_NO_SHELL_FALLBACK=1` refuses any `/bin/sh -c` fallback (fails instead of executing). |
| Command length | Bounded in `safe_exec()`/`safe_popen()`. |
| Child environment | Filtered: the `DIALOG` block (several KiB of XML) is not inherited by spawned processes. |
| Memory safety | `g_strlcpy` (variable names), bounded widget copy, spawn `argv` always NUL-terminated; no forbidden function (`strcpy`/`strcat`/`sprintf`/`system`/`popen`). |
| XML parser | Clean rejection of malformed XML (message + non-zero exit code, never an `abort`); parser subjected to **fuzzing** (`tests/fuzz/`). |

### Trust model

gtk3dialog runs the interface described by the **author of the XML script** — just
as a shell script runs what its author writes. The `<action>`/`<input>` tags can
launch commands: **this is intended and documented**. The trust boundary is
therefore the **local** author of the script, not a remote third party. The
hardening above protects against **malformed input** and **memory-safety bugs** —
not against a hostile script author, who can launch commands anyway. To interpret
XML coming from a less trustworthy source, set `HAPLO_NO_SHELL_FALLBACK=1`.

### Warnings treated as errors

```
-Wall -Wextra -Wshadow -Wnull-dereference
-Wstrict-prototypes -Wimplicit-fallthrough=3
-Werror=format-security
```

---

## Patch history

| Date | Port | Description | Severity |
|------|------|-------------|----------|
| 2026-08-11 | gtk3dialog | Memory-safety pass: `g_strlcpy` (variable names), widget copy bounded to `MAXWIDGETS`, NUL-terminated spawn `argv`, child environment filtering | Medium |
| 2026-05-29 | gtk3dialog | FORTIFY_SOURCE upgrade 2→3, stack-clash added, noexecstack | Medium |
| 2026-05-22 | gtk3dialog | Binaries renamed, gtkdialog symlink fixed | Low |
| 2026-05-01 | gtk3dialog | Initial port — safe_exec, PIE, RELRO | High |

---

## Post-installation verification

```bash
# Check the protections of the installed binary:
hardening-check /usr/bin/gtk3dialog
checksec --file=/usr/bin/gtk3dialog

# Check that ASLR is active on the system:
cat /proc/sys/kernel/randomize_va_space   # must be 2
```

---

*haplo-dialog is a fork of gtkdialog (László Pere, GPL-2.0-or-later).*
*haplo-dialog — devel@haplo-dialog.fr — 2026*
