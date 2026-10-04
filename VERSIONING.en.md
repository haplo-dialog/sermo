# Versioning — sermo

[Français](VERSIONING.md)

sermo follows **semantic versioning**, `MAJOR.MINOR.PATCH`:

- **MAJOR** — a compatibility break in the XML language, or in the contract
  between the core and the backends (`contract/sermo-contract.h`, the `sermo_be_*`
  functions).
- **MINOR** — a new backward-compatible capability (widget, action, option, port).
- **PATCH** — a fix without an interface change.

## Current version

`2.7.7`. The **first published 2.x release** was `2.7.3`. What each version brings:
[CHANGELOG.en.md](CHANGELOG.en.md).

The 2.x line started at `2.0.0`, a version that stayed internal, like every
version up to 2.7.2. The move to 2 comes from the new contract between the core
and the backends: a backend from the 1.x lineage does not link against the 2.x
core. The XML language, however, was not broken: scripts written for gtkdialog or
sermo 1.x stay valid. The behaviours that changed are listed in
[MIGRATION.en.md](MIGRATION.en.md).

## 1.x

In 1.x, each port had its own number: gtk3sermo and gtk4sermo 1.1.4, qt6sermo
1.0.2. 2.x gives a single number to the core and the seven ports.

## Where the number lives — one place only

The **`VERSION`** file at the repository root is the single source. All eight
CMake projects (the core and the seven backends) read it before `project(...)`:

```cmake
file(READ "${CMAKE_CURRENT_SOURCE_DIR}/../VERSION" SERMO_VERSION)
string(STRIP "${SERMO_VERSION}" SERMO_VERSION)
project(gtk3sermo VERSION ${SERMO_VERSION} LANGUAGES C)
```

From there the number flows on its own: `sermocore.pc`, the `config.h` files
generated from their `config.h.in`, and the manual pages substituted at build
time.

Bumping a version therefore means editing **two** files: `VERSION` and
`debian/changelog` (which also carries the Debian revision). The
[`tests/garde_version.sh`](tests/garde_version.sh) guard rejects any drift: a
`project()` carrying a hard-coded number, a hand-written `config.h`, a manual
page that was not substituted, or a `debian/changelog` left behind.

Before 2.7.1 this number was copied into four `CMakeLists.txt`, two hand-written
`config.h` files — whose comments asked you to "keep them in sync" — and the
manual pages. Nothing checked that agreement: a miss did not show up at compile
time, the binary simply shipped announcing a wrong number.

## Core, backends and packages

- The core (`libsermocore`) and the seven backends carry the **same number** as
  long as they are shipped together. An incompatible change to the
  `sermo-contract.h` contract requires a MAJOR bump, because an older backend
  would no longer link against the core.
- `sermocore.pc` carries the version (`Version: 2.7.7`): a backend built
  separately can require a minimum core with
  `pkg_check_modules(SERMOCORE REQUIRED sermocore>=2.7)`.
- Debian packages add a revision: `2.7.7-1` is the first Debian build of 2.7.7.

## Script compatibility

XML scripts written for gtkdialog or sermo 1.x stay valid. For scripts that call
the **`gtkdialog` command**, install the separate `sermo-gtkdialog` package (a link
to `gtk3sermo`).
