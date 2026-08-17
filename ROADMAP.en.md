# Roadmap - haplo-dialog

**haplo-dialog**, https://haplo-dialog.fr  
*Last updated: 2026-06-07*

---

## Current version: 1.0.0 (May 2026) ✅

First stable public version. gtk3dialog port (GTK 3 backend), 43 widgets, hardened security.

---

## v1.1.0 - Quality & CI (Q3 2026)

Priority: make the project **automatically verifiable**.

- [x] **gtk3dialog build**, the reference port compiles and links a working binary (autoreconf -fi && ./configure && make) ✅
- [x] **XML tests green**, 52/52 (headless, `--print-ir`) ✅
- [x] **gtkdialog backward compatibility**, `gtkdialog` → `gtk3dialog` symlink at install time; legacy XML, CLI and environment variables verified ✅
- [ ] **Working CI**, gtk3dialog compiles on Gitea/Forgejo on every push
- [x] **Complete XML suite**, 52 test cases ✅ (tests/xml/: 52 files covering all widgets and actions)
- [ ] **Clean Valgrind**, zero leaks on the 52 reference XML files with gtk3dialog
- [ ] **cppcheck CI**, automatic static analysis, zero errors
- [x] **haplo-dialog-xml(5) in the packages**, manpage installed by default ✅
- [ ] **Official signed `.deb`**, signed binary release package
- [x] **Examples**, `examples/` covers all widgets (53 directories, `#!/bin/sh` scripts)

---

## v1.2.0 - Features (Q4 2026)

Priority: enrich without breaking compatibility.

- [ ] **`--validate` mode**, parses the XML and returns errors without displaying
- [ ] **External CSS theme** for gtk3dialog, `--style=file.css`
- [ ] **`<filewatcher>`**, haplo-dialog-specific widget, watches a file and refreshes
- [ ] **Internationalisation**, `gettext` support for fixed labels

---

## v2.0.0 - Architecture (2027)

Priority: make the project maintainable in the long term.

- [ ] **Shared library** `libhaplodialog.so`, C API for integration into other tools
- [ ] **Lua `<script>`**, scriptable actions in Lua (alternative to shell)
- [ ] **Accessibility (AT-SPI)**, screen reader support for gtk3dialog
- [ ] **PDF documentation**, `makeinfo --pdf` generated and distributed
- [ ] **Snap / Flatpak**, universal packaging format for gtk3dialog

---

## Reference port - level of guarantee

| Port | Guarantee | Maintainer | CI |
|------|----------|-----------|-----|
| `gtk3dialog` | ✅ Production | haplo-dialog | Yes (v1.1) |

`gtk3dialog` is the reference port of haplo-dialog (GTK 3 backend, 43 widgets). It provides the backward-compatible `gtkdialog` alias.

---

## What will NOT be done

To avoid losing focus:

- No Windows port (out of scope for the project)
- No macOS port
- No Python/Ruby binding (use the binary directly from the shell)
- No GTK2 support (abandoned)
- No replacement of Zenity for simple cases, haplo-dialog targets complex dialogs

---

## How to contribute to the roadmap

Open a ticket with the `roadmap` label or write to `devel@haplo-dialog.fr`.  
Priorities are discussed openly, all suggestions are welcome.

---

*haplo-dialog, "The shell deserves a real graphical interface."*

---

