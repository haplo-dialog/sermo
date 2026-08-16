# haplo-dialog

**Build native GTK3 graphical interfaces from your shell scripts.**

```sh
export MAIN_DIALOG='
<window title="Hello!" width-request="320" height-request="140">
  <vbox>
    <text><label>Enter your name:</label></text>
    <entry><variable>NOM</variable></entry>
    <hbox>
      <button ok></button>
      <button cancel></button>
    </hbox>
  </vbox>
</window>'

gtk3dialog --program MAIN_DIALOG
echo "Hello, $NOM!"
```

This script works **as is**, whether you call it with `gtk3dialog` or with
`gtkdialog`: it's the **same binary** (see below). Scripts written for
gtkdialog run without modification.

---

## Quick install

```sh
# Debian and derivatives — a .deb is attached to every release of the repository.
# Download it along with SHA256SUMS, then:
sha256sum -c SHA256SUMS
sudo apt install ./gtk3dialog_1.0.0-3_amd64.deb

# From source
cd gtk3dialog/gtk3dialog_1.0.0
./configure && make && sudo make install
```

There is **no APT repository** to add: download the package, verify its checksum,
install it.

The package installs the `gtk3dialog` command and **provides `gtkdialog`**
(symbolic link): both names launch the same program.

---

## gtk3dialog, the successor to gtkdialog

haplo-dialog builds on **gtkdialog 0.8.3** (László Pere, GPL-2.0+), which
remained on GTK2, and modernises it onto **GTK3** while keeping its XML syntax
**identical**.

- **One single binary, two names.** `gtkdialog` is a link to `gtk3dialog`. The
  same window, described just once in XML, opens identically through both
  commands — your old gtkdialog scripts work without touching them.
- **Same XML language.** gtkdialog window descriptions are read as is.
- **GTK3.** Modern native rendering, GNOME and Xfce integration.

---

## Why haplo-dialog?

The shell is a first-class language. It orchestrates, filters, decides — but
delegates display. haplo-dialog is the missing link: a declarative XML syntax,
a binary to invoke, a native interface that integrates with the desktop.

**Versus the alternatives:**
- `zenity` / `kdialog`: limited to a few fixed dialog boxes
- `yad`: richer, but a command-line syntax that is hard to maintain
- `python3 + tkinter`: requires Python, breaks the shell paradigm
- `haplo-dialog`: structured XML, around forty widgets, gtkdialog-compatible

**Philosophy:**
- Strict separation of structure (XML) and logic (shell)
- Security — `safe_system()` executes without a shell when possible (see below)
- gtkdialog compatibility — a legacy script runs without rewriting
- Minimal runtime dependencies

---

## Complete example — form with validation

```sh
#!/bin/sh
# Developed with the assistance of Claude (Anthropic).

export DIALOG='
<window title="New connection" width-request="400" height-request="220">
  <vbox>
    <frame><label>Credentials</label>
      <vbox>
        <hbox>
          <text><label>User:</label></text>
          <entry><variable>USER_INPUT</variable></entry>
        </hbox>
        <hbox>
          <text><label>Password:</label></text>
          <password><variable>PASS_INPUT</variable></password>
        </hbox>
      </vbox>
    </frame>
    <hbox>
      <button ok></button>
      <button cancel></button>
    </hbox>
  </vbox>
</window>'

gtk3dialog --program DIALOG

if [ -n "$USER_INPUT" ]; then
    echo "Connection from: $USER_INPUT"
fi
```

---

## Available widgets

`<window>` `<button>` `<entry>` `<password>` `<checkbox>` `<radiobutton>`  
`<switch>` `<combobox>` `<comboboxtext>` `<list>` `<tree>` `<table>`  
`<hbox>` `<vbox>` `<frame>` `<notebook>` `<expander>` `<separator>`  
`<progressbar>` `<hscale>` `<vscale>` `<spinbutton>` `<calendar>`  
`<menubar>` `<terminal>` `<pixmap>` `<text>` `<edit>` `<statusbar>`  
`<timer>` `<infobar>` `<levelbar>` `<spinner>` `<searchentry>`  
`<drawingarea>` `<aspectframe>` `<togglebutton>` and more…

Full reference: `man 5 haplo-dialog-xml` or `doc/reference/`

---

## Repository structure

```
haplo-dialog/
├── README.md               ← you are here
├── CHANGELOG.md            ← version history
├── AUTHORS                 ← authors and contributors
├── NEWS                    ← user announcements
├── CONTRIBUTING.md         ← how to contribute
├── SECURITY.md             ← security policy
├── LICENCES.md             ← licence summary
├── ROADMAP.md              ← roadmap
├── gtk3dialog/             ← the GTK3 port
│   └── gtk3dialog_1.0.0/
│       ├── src/            ← C sources
│       ├── examples/       ← demonstration scripts
│       ├── doc/            ← Texinfo documentation
│       └── packaging/      ← .deb, .rpm, PKGBUILD, .ebuild
└── tests/
    └── xml/                ← XML regression suite
```

---

## Security

haplo-dialog is designed to run in an unprivileged user context.

- Commands go through `safe_system()` / `safe_popen()`, which replace
  `system()` / `popen()`: when the command **contains no shell metacharacters**,
  it is executed **directly, without a shell** — which avoids shell injection in
  this common case. When metacharacters are present, it falls back to
  `/bin/sh -c` (full shell functionality); this fallback can be **refused** by
  setting the `HAPLO_NO_SHELL_FALLBACK` variable.
- Binaries compiled with: `FORTIFY_SOURCE`, PIE, Full RELRO, non-executable
  stack (NX), stack canary — verifiable via `checksec --file=/usr/bin/gtk3dialog`.
- Full security policy: [SECURITY.md](SECURITY.md)
- Vulnerability reporting: `devel@haplo-dialog.fr`

---

## Contributing

See [CONTRIBUTING.md](CONTRIBUTING.md) for the complete workflow.

In short:
1. Read `SECURITY.md` before any change to the core
2. Follow the provided `.clang-format` and `.editorconfig` style
3. Test with `make check`
4. One patch per feature, commit message in English

---

## Licence

| Component | Licence |
|-----------|---------|
| Code — binary, widgets, C core | GPL-2.0-or-later |
| Documentation | CC-BY-SA 4.0 |
| Examples (`examples/`) | CC0 (public domain) |

Inherits from **gtkdialog 0.8.3** (László Pere, GPL-2.0+) — modernised and
ported to GTK3.

---

## Contact

`devel@haplo-dialog.fr` · https://haplo-dialog.fr

> *Developed with the assistance of Claude (Anthropic).*
