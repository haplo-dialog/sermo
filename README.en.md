# haplo-dialog

**A modern, hardened and maintained descendant of gtkdialog (GTK 3).**

> ## ⚠️ Renaming in progress
>
> This project shipped its package under the name **`gtk3dialog`**. That name is
> already used by a package distributed by **BunsenLabs**, which we had not noticed.
> Two packages sharing a name cannot coexist: one silently replaces the other.
>
> The GTK family is therefore being renamed to **`gtksermo`** (gtkdialog
> compatibility), **`gtk3sermo`** and **`gtk4sermo`**, under the **haplo-dialog**
> suite — which goes back to being what it always was: the name of the **product**,
> not of a single port.
>
> **In the meantime, do not install the package from release v1.0.0-3 if you are on
> BunsenLabs**: it would overwrite your `gtk3dialog`. A corrected release will follow.

[![Licence](https://img.shields.io/badge/licence-GPL--2.0--or--later-blue.svg)](LICENSE)
[![Version](https://img.shields.io/badge/version-1.0.0-informational.svg)](CHANGELOG.en.md)
[![Toolkit](https://img.shields.io/badge/toolkit-GTK%203-success.svg)](#the-gtk3sermo-port)
[![Tests](https://img.shields.io/badge/tests-52%2F52%20XML%20·%209%2F9%20behaviour-brightgreen.svg)](#tests--quality)

Describe an interface in XML, export it into a variable, run the binary —
a real **native GTK 3** window opens, and the values you enter come back
in your shell variables. Where **gtkdialog** has been abandoned, haplo-dialog
brings it back to life: fixed, hardened, maintained. From the Greek *haplóos*, "simple".

---

## The signature gesture

```sh
export MAIN_DIALOG='
<window title="Hello!" width-request="320">
  <vbox>
    <text><label>Enter your name:</label></text>
    <entry><variable>NAME</variable></entry>
    <hbox>
      <button ok></button>
      <button cancel></button>
    </hbox>
  </vbox>
</window>'

gtk3sermo --program=MAIN_DIALOG     # → a native GTK 3 window opens
```

On confirmation, the output comes back to the shell, ready to `eval`:

```sh
NAME="Ada"
EXIT="OK"
```

**~10 lines of shell → a native window.**

---

## Why haplo-dialog?

| | haplo-dialog | zenity / yad | gtkdialog (upstream) |
|---|:--:|:--:|:--:|
| Arbitrary XML interfaces (widgets, containers, signals) | ✅ | ⚠️ limited | ✅ |
| Values returned in shell variables | ✅ | partial | ✅ |
| Hardened by default (no `system()`, FORTIFY, PIE, RELRO) | ✅ | — | ❌ |
| Maintained | ✅ | ✅ | ❌ (abandoned) |
| Backward-compatible with gtkdialog | ✅ (alias) | — | — |

---

## The gtk3sermo port

A C core (flex/bison grammar + state machine + `safe_exec`) and a GTK 3 backend.

| Port | Binary | Toolkit | Widgets | Notes |
|------|---------|---------|:------:|---------------|
| **gtk3sermo** | `gtk3sermo` | GTK 3 | 43 | **Reference** · backward-compatible `gtkdialog` alias |

> **43 widgets** described by a single grammar. The binary provides the
> `gtkdialog` alias: scripts written for gtkdialog run without modification.

---

## Installation

### Debian package (.deb)

```sh
# The .deb is attached to every release of the repository, with its checksums.
# Download both, then:
sha256sum -c SHA256SUMS
sudo apt install ./haplo-dialog_1.0.0-8_amd64.deb
```

> **The package is named `haplo-dialog`; the command is still `gtk3sermo`.**
> The package name `gtk3sermo` is not free: BunsenLabs has shipped one under
> that exact name since July 2025, in its APT archive. Two packages carrying
> the same name are not rivals to apt, they are two versions of one package,
> and the higher one silently replaces the other. Distinct names make apt
> refuse and explain instead of substituting. Your scripts do not change:
> `/usr/bin/gtk3sermo` and its `gtkdialog` alias are installed as before.

### From source

```sh
# autotools
cd gtk3sermo/gtk3sermo_1.0.0 && autoreconf -fi && ./configure && make -j"$(nproc)" && sudo make install
```

Packaging recipes are provided (`packaging/`: Debian, RPM, Arch,
Gentoo, Slackware). See [PACKAGING.md](PACKAGING.md).

---

## Security

Hardened by default: execution through **`safe_exec`** (no `system()`),
**`_FORTIFY_SOURCE=3`**, **PIE**, **Full RELRO**, `stack-protector-strong`,
`-fcf-protection`. The parser **cleanly rejects** malformed XML (message +
non-zero exit code, never an `abort`), validated by **fuzzing** (`tests/fuzz/`).
Vulnerability report: see [SECURITY.md](SECURITY.en.md).

---

## Tests & quality

- **XML regression**: `./tests/xml/run_tests.sh all`, **52/52** (headless parse `--print-ir`).
- **Behaviour**: `./tests/run_unit_tests.sh all`, `safe_exec`, **9/9** (without an X server).
- **Fuzzing** of the parser: `./tests/fuzz/run_fuzz.sh 60` (afl++ or built-in fallback).
- **CI**: build + tests, `.gitlab-ci.yml` and `.gitea/workflows/`.

---

## Documentation

- User/developer manuals (`gtk3sermo/gtk3sermo_1.0.0/MANUEL_*.md`).
- Man pages `gtk3sermo(1)` and XML reference `haplo-dialog-xml(5)`.
- Texinfo manuals (`gtk3sermo/gtk3sermo_1.0.0/doc/`).
- Website: <https://haplo-dialog.fr>.

---

## Contributing & licence

Contributions are welcome, see [CONTRIBUTING.md](CONTRIBUTING.en.md) and
[CODE_OF_CONDUCT.md](CODE_OF_CONDUCT.md).

**GPL-2.0-or-later**, see [LICENSE](LICENSE). The examples (`examples/`) are under CC0.

---

## AI-assisted development - stated plainly

gtk3sermo was written with the help of an AI (Claude, by Anthropic) —
design, code, tests, documentation, under my review and my responsibility.
I say so openly: honesty is not negotiable. I believe that a tool used well,
with nothing concealed, serves quality instead of harming it. The code is
mine; so are the mistakes.

> *haplo-dialog*
