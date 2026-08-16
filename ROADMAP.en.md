# Roadmap — haplo-dialog

https://haplo-dialog.fr

---

## Current version: 1.0.0 (May 2026) ✅

First stable public release: gtk3dialog (GTK3), gtkdialog-compatible,
strengthened security.

---

## v1.1.0 — Quality & CI (Q3 2026)

Priority: make the project **automatically verifiable**.

- [ ] **Working CI** — gtk3dialog compiles on Gitea/Forgejo on every push
- [x] **Complete XML suite** — 50 test cases ✅ (tests/xml/: 50 files covering all widgets and actions)
- [ ] **Clean Valgrind** — zero leaks on the 50 reference XML files with gtk3dialog
- [ ] **cppcheck CI** — automatic static analysis, zero errors
- [x] **haplo-dialog-xml(5) in the packages** — manpage installed by default ✅
- [ ] **gtk3dialog in Haplo-Linux** — official signed `.deb` package
- [ ] **gtk3dialog examples** — complete `demo.sh` covering the widgets

---

## v1.2.0 — Features (Q4 2026)

Priority: enrich without breaking gtkdialog compatibility.

- [ ] **`--validate` mode** — parses the XML and returns errors without displaying
- [ ] **External CSS theme** for gtk3dialog — `--style=file.css`
- [ ] **`<filewatcher>`** — haplo-dialog-specific widget, watches a file and refreshes
- [ ] **Internationalisation** — `gettext` support for fixed labels

---

## v2.0.0 — Architecture (2027)

Priority: make the project maintainable in the long term.

- [ ] **Shared library** `libhaplodialog.so` — C API for integration into other tools
- [ ] **Lua `<script>`** — actions scriptable in Lua (alternative to shell)
- [ ] **Accessibility (AT-SPI)** — screen reader support for gtk3dialog
- [ ] **PDF documentation** — `makeinfo --pdf` generated and distributed
- [ ] **Snap / Flatpak** — universal packaging format for gtk3dialog

---

## What will NOT be done

To avoid scattering effort:

- No Windows port (out of scope)
- No macOS port
- No Python/Ruby binding (use the binary directly from the shell)
- No GTK2 support (abandoned)
- No replacement of Zenity for simple cases — haplo-dialog targets complex dialogs

---

## How to contribute to the roadmap

Open a ticket with the `roadmap` label or write to `devel@haplo-dialog.fr`.  
Priorities are discussed openly — any suggestion is welcome.

---

*haplo-dialog — "The shell deserves a real graphical interface."*  
*Developed with the assistance of Claude (Anthropic) — May 2026.*
