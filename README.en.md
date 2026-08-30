# haplo-dialog

**A modern, hardened and maintained descendant of gtkdialog — GTK 3 and GTK 4 ports.**

> ### ⚠️ If you installed a `gtk3dialog` package from this project
>
> That name also belongs to a **BunsenLabs** package. Two packages sharing a name
> cannot coexist: ours may have **silently replaced yours**. Ours have been
> withdrawn — if you had one, **reinstall your distribution's `gtk3dialog`**.
>
> Our packages are now called `gtk3sermo`, `gtk4sermo` and `gtksermo`.

[![Licence](https://img.shields.io/badge/licence-GPL--2.0--or--later-blue.svg)](LICENSE)
[![Version](https://img.shields.io/badge/version-1.1.3-informational.svg)](CHANGELOG.en.md)
[![Toolkit](https://img.shields.io/badge/toolkit-GTK%203%20%2B%20GTK%204-success.svg)](#the-two-ports)
[![Tests](https://img.shields.io/badge/tests-55%2F55%20XML%20·%209%2F9%20unit%20·%2011%2F11%20behaviour-brightgreen.svg)](#tests--quality)

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

On confirmation, the output comes back to the shell:

```sh
NAME="Ada"
EXIT="OK"
```

The simplest and safest route is to let `--do` do the work: the values arrive as
environment variables and never go back through the shell.

```sh
gtk3sermo --program=MAIN_DIALOG --do='echo "Hello $NAME"'
```

> **Do not pipe this output into `eval` without thinking.** A field's value is
> typed by whoever uses the dialog, and that is not necessarily whoever wrote the
> script. Since 2026-08-25 the program escapes the four characters the shell
> expands inside double quotes (`\`, `"`, `$` and the backtick), so `eval` no
> longer executes them — a bench checks it on every push. Before that date it
> escaped only two, and `eval` executed whatever had been typed.

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

## The two ports

One shared C core (flex/bison grammar + state machine + `safe_exec`), two backends.

| Port | Binary | Toolkit | Widget tags | Notes |
|------|---------|---------|:------:|---------------|
| **gtk3sermo** | `gtk3sermo` | GTK 3 | 52 | **Reference** · the most battle-tested |
| **gtk4sermo** | `gtk4sermo` | GTK 4 | 56 | the same 52, plus `flowbox`, `overlay`, `revealer`, `stack` |

> One grammar for both ports. The count is the number of **widget tags accepted by
> the grammar** (`src/gtkdialog_lexer.l`), aliases included — it does not count
> structural tags such as `<action>` or `<variable>`.
>
> The internal documents of each port quote a different figure — **43** and
> **50** — because they count **implementation files** (`ls src/widget_*.c`).
> Both are accurate: several tags share one file (aliases, `h`/`v` variants).
> Wherever a figure appears, it says which of the two it is.
>
> The `gtkdialog` command itself is NOT in these packages: it ships in a separate
> package, **`gtksermo`**. See [Installation](#installation).

---

## Installation

### Download the release

The packages are attached to the [**v1.1.3**](https://gitlab.com/haplo-dialog/sermo/-/releases/v1.1.3) release, with their checksums.
There is **no APT repository**: you download, you verify, you install.

```sh
U=https://gitlab.com/api/v4/projects/85674825/packages/generic/sermo/1.1.3
for f in gtk3sermo_1.1.3-1_amd64.deb gtksermo_1.1.3-1_all.deb SHA256SUMS; do
    curl -fLO "$U/$f"
done

sha256sum --ignore-missing -c SHA256SUMS
sudo apt install ./gtk3sermo_1.1.3-1_amd64.deb
```

| Package | Command installed | Conflicts |
|---|---|---|
| `gtk3sermo` 1.1.3-1 | `/usr/bin/gtk3sermo` | none |
| `gtk4sermo` 1.1.3-1 | `/usr/bin/gtk4sermo` | none |
| `gtksermo` 1.1.3-1 | `/usr/bin/gtkdialog` | **with `gtkdialog` and `gtk3dialog`** |

> ⚠️ **The packages from release `v1.0.0` have been withdrawn** (versions
> `1.0.0-10` and `1.0.0-11`). They carried three defects: the output handed to the
> shell could be executed by `eval` if whoever used the dialog typed `$(command)`
> into a field, clicking a `<switch>` killed the program, and under a French locale
> a number written with a dot silently read as zero. **If you installed one,
> replace it** with the matching package from `v1.1.3`.
>
> Release `v1.0.0-4` does carry the three fixes: its packages are sound, merely
> superseded by those of `v1.1.3`.

The first two install without conflict, including alongside BunsenLabs'
`gtk3dialog`. The third provides the `gtkdialog` command, so it conflicts with
it: install it only if you want that command.

The `-dbgsym` packages are only useful for reading a stack trace.

### Building the Debian package

```sh
git clone https://gitlab.com/haplo-dialog/sermo.git
cd sermo/gtk3sermo/gtk3sermo_1.1.3        # or gtk4sermo/gtk4sermo_1.1.3
dpkg-buildpackage -us -uc -b
```

The resulting packages — and the split is deliberate:

| Package | From | Installs | Note |
|---|---|---|---|
| `gtk3sermo` | GTK 3 port | `/usr/bin/gtk3sermo` | no conflict |
| `gtksermo` | GTK 3 port | `/usr/bin/gtkdialog` | **conflicts** with `gtkdialog` and `gtk3dialog` |
| `gtk4sermo` | GTK 4 port | `/usr/bin/gtk4sermo` | no conflict |

> **The `gtkdialog` command lives in `gtksermo`, not in `gtk3sermo`.**
> That is on purpose. BunsenLabs' `gtk3dialog` package carried the same name as
> the one we used to ship: two packages sharing a name are not rivals to apt, the
> higher one silently replaces the other. With distinct names, apt refuses and
> explains. Install `gtksermo` only if you want the `gtkdialog` command —
> otherwise your scripts work by calling `gtk3sermo` or `gtk4sermo` directly.

### From source, without packaging

```sh
cd gtk3sermo/gtk3sermo_1.1.3 && autoreconf -fi && ./configure && make -j"$(nproc)" && sudo make install
```

⚠️ Here `make install` also drops the `gtkdialog` symlink, without the safety net
of package conflicts: avoid it if you already have a `gtkdialog` or a
`gtk3dialog` installed by your distribution.

RPM, Arch, Gentoo and Slackware recipes exist under `packaging/`, but **none has
ever been built by us** and their source URL does not resolve yet. See
[PACKAGING.md](PACKAGING.md).

---

## Security

Hardened by default: execution through **`safe_exec`** (no `system()`),
**`_FORTIFY_SOURCE=3`**, **PIE**, **Full RELRO**, `stack-protector-strong`,
`-fcf-protection`. The parser **cleanly rejects** malformed XML (message +
non-zero exit code, never an `abort`), validated by **fuzzing** (`tests/fuzz/`).
Vulnerability report: see [SECURITY.md](SECURITY.en.md).

---

## Tests & quality

- **XML regression**: `./tests/xml/run_tests.sh <path-to-binary>`, **55/55**.
  ⚠️ This bench runs `--print-ir`: it **parses** the XML without building a
  single widget. It says nothing about behaviour.
- **Core unit tests**: `./tests/run_unit_tests.sh all`, `safe_exec`, **9/9 per
  port** (without an X server).
- **Real behaviour**: `./tests/comportement/run.sh <path-to-binary>`, **11/11** —
  it **launches** the dialogue under Xvfb and compares the variables it emits.
  `--diff <gtk3> <gtk4>` mode: the GTK 3 port acts as the oracle.
  Plus `./tests/comportement/geometrie.sh`, for what no variable reveals
  (`border-width`, theme icon size).
- **Fuzzing** of the parser: `./tests/fuzz/run_fuzz.sh gtk3sermo 60` (afl++ or built-in fallback).
- **CI**: build + tests, `.gitlab-ci.yml` and `.gitea/workflows/`.

---

## Documentation

- User/developer manuals (`gtk3sermo/gtk3sermo_1.1.3/MANUEL_*.md`).
- Man pages `gtk3sermo(1)` and XML reference `haplo-dialog-xml(5)`.
- Texinfo manuals (`gtk3sermo/gtk3sermo_1.1.3/doc/`).
- Website: <https://haplo-dialog.fr>.

---

## Contributing & licence

Contributions are welcome, see [CONTRIBUTING.md](CONTRIBUTING.en.md) and
[CODE_OF_CONDUCT.md](CODE_OF_CONDUCT.md).

**GPL-2.0-or-later**, see [LICENSE](LICENSE). The examples the project wrote
(`examples/showcase/`, `examples/system-tools/`) are CC0; the rest of `examples/`
comes from gtkdialog and follows the GPL, and three third-party icon sets carry
their own `COPYING`. Details in [LICENCES.en.md](LICENCES.en.md).

---

## AI-assisted development - stated plainly

gtk3sermo was written with the help of an AI (Claude, by Anthropic) —
design, code, tests, documentation, under my review and my responsibility.
I say so openly: honesty is not negotiable. I believe that a tool used well,
with nothing concealed, serves quality instead of harming it. The code is
mine; so are the mistakes.

> *haplo-dialog*
