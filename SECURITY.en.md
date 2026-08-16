# Security policy — gtk3dialog

**Contact:** devel@haplo-dialog.fr

---

## Supported versions

| Component | Version | Security support |
|-----------|---------|------------------|
| gtk3dialog | 1.0.0 | ✅ Active |

The `gtk3dialog` package also provides the `gtkdialog` command (same binary);
both names share the same code and therefore the same support.

---

## Reporting a vulnerability

Send an email to **devel@haplo-dialog.fr** with:

- a description of the problem;
- the reproduction steps;
- the estimated impact.

Please allow us a reasonable delay to fix it before any public disclosure.

---

## Security mechanisms

### Command execution

`gtk3dialog` replaces `system()` and `popen()` with **`safe_system()` /
`safe_popen()`**:

- When the command **contains no shell metacharacters**, it is executed
  **directly, without going through a shell** — which avoids shell injection in
  this case.
- When metacharacters are present, it **falls back to `/bin/sh -c`** (full shell
  functionality). This fallback can be **refused** by setting the
  `HAPLO_NO_SHELL_FALLBACK` environment variable (the program then logs a refusal
  instead of executing through the shell).

> This is not a command allowlist: `safe_system()` reduces the attack surface by
> avoiding the shell when it is unnecessary, but the caller remains responsible
> for what it executes.

`safe_popen()` returns a `FILE*` obtained through `fdopen()` (never `popen()`):
callers must close it with `fclose()`, never `pclose()`.

### Compile-time hardening

Protections **verifiable on the installed binary**:

| Protection | State |
|---|---|
| PIE / ASLR | ✅ |
| Full RELRO | ✅ |
| FORTIFY_SOURCE | ✅ |
| Stack canary | ✅ |
| Non-executable stack (NX) | ✅ |

Check for yourself:

```bash
checksec --file=/usr/bin/gtk3dialog
readelf -d /usr/bin/gtk3dialog | grep BIND_NOW      # Full RELRO
cat /proc/sys/kernel/randomize_va_space             # System ASLR: must be 2
```

---

## Execution context

`gtk3dialog` is designed to run in an **unprivileged user context**. It embeds
no privilege escalation mechanism: if a script needs additional rights, it is up
to it to request them explicitly, outside of gtk3dialog.

---

*gtk3dialog derives from gtkdialog 0.8.3 (László Pere, GPL-2.0-or-later), ported
to GTK3.*
