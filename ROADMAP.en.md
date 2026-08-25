# Roadmap — sermo

**haplo-dialog**, https://haplo-dialog.fr  
*Last updated: 2026-08-25*

---

## Current version: 1.0.0 (May 2026) ✅

First stable public version, **two ports**: `gtk3sermo` (GTK 3 backend,
**52 widget tags**) and `gtk4sermo` (GTK 4 backend, **56** — the same 52, plus
`flowbox`, `overlay`, `revealer`, `stack`). The backwards-compatible `gtkdialog`
alias ships in a third package, `gtksermo`.

The packages are downloadable from the GitLab releases. Packaging revisions
advance port by port and may diverge; that is normal (see `VERSIONING.md` §2).

---

## v1.1.0 - Quality & CI (Q3 2026)

Priority: make the project **automatically verifiable**.

- [x] **Both ports build**, each compiles and links a working binary (`autoreconf -fi && ./configure && make`) ✅
- [x] **XML tests green**, **55/55 per port**, headless (`--print-ir`) ✅
- [x] **gtkdialog backward compatibility**, `gtkdialog` → `gtk3sermo` symlink at install time; legacy XML, CLI and environment variables verified ✅
- [x] **Working CI** ✅ — on **GitLab** (`.gitlab-ci.yml`), not on Gitea/Forgejo. It builds both ports, replays the XML suite, the behaviour tests and **seven benches**, then **actually runs the examples** under Xvfb in the `fr_FR.UTF-8` locale. That second stage exists because the first was not enough: on 2026-08-22 both ports crashed in use while the pipeline stayed green, because it never opened a single window.
- [x] **Complete XML suite**, 55 test cases ✅ (tests/xml/: 55 files covering all widgets and actions)
- [x] **Examples run, not merely listed** ✅ — 55/55 for gtk3sermo and 58/58 for gtk4sermo really open their window, in French, on every push
- [x] **Hardening measured on the binary** ✅ — `readelf`, not the advertised flags: that is how we found CET did not survive linking
- [x] **Packages downloadable** ✅ — five `.deb` plus `SHA256SUMS` in the GitLab releases, re-downloaded anonymously to verify
- [ ] **Signed `.deb`**, the APT repository signature does not exist yet (the published SHA256 sums attest integrity, not origin)
- [ ] **Clean Valgrind**, zero leaks on the 55 reference XML files, in both ports
- [ ] **cppcheck CI**, automatic static analysis, zero errors

---

## v1.2.0 - Features (Q4 2026)

Priority: enrich without breaking compatibility.

- [ ] **`--validate` mode**, parses the XML and returns errors without displaying
- [ ] **External CSS theme** for gtk3sermo, `--style=file.css`
- [ ] **`<filewatcher>`**, haplo-dialog-specific widget, watches a file and refreshes
- [ ] **Internationalisation**, `gettext` support for fixed labels

---

## v2.0.0 - Architecture (2027)

Priority: make the project maintainable in the long term.

- [ ] **Shared library** `libhaplodialog.so`, C API for integration into other tools
- [ ] **Lua `<script>`**, scriptable actions in Lua (alternative to shell)
- [ ] **Accessibility (AT-SPI)**, screen reader support for gtk3sermo
- [ ] **PDF documentation**, `makeinfo --pdf` generated and distributed
- [ ] **Snap / Flatpak**, universal packaging format for gtk3sermo

---

## Reference port - level of guarantee

| Port | Guarantee | Maintainer | CI |
|------|----------|-----------|-----|
| `gtk3sermo` | ✅ Production | haplo-dialog | yes — build, XML, behaviour, 7 benches, real examples |
| `gtk4sermo` | ✅ Production | haplo-dialog | yes — same pipeline, same benches |

`gtk3sermo` is the reference port of sermo (GTK 3 backend, 52 widget tags). It provides the backward-compatible `gtkdialog` alias.

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

