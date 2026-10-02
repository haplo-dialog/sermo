# Packaging — sermo

[Français](PACKAGING.md)

sermo produces **separate** Debian packages: a shared development core and one
backend per toolkit. The `debian/` directory (debhelper) describes them; the
`packaging/construire-paquets.sh` script builds them and tests them in a clean
Debian testing system.

## What is produced

| Package | Contents | Depends on |
|---|---|---|
| `sermo-doc` | Texinfo manual (`info sermo-doc`) and the common documentation: copyright, Debian changelog | no dependency |
| `sermo-core-dev` | `libsermocore.a`, headers (`/usr/include/sermo`), `sermocore.pc` | no dependency |
| `sermo-backend-gtk3` | `gtk3sermo` binary, man page, Texinfo manual | GTK 3, VTE (`<terminal>`), gtk-layer-shell (Wayland anchoring) |
| `sermo-backend-gtk4` | `gtk4sermo` binary, man page, Texinfo manual | GTK 4, VTE (`<terminal>`) |
| `sermo-backend-qt6` | `qt6sermo` binary, man page, Texinfo manual, menu entry | Qt 6 |
| `sermo-backend-fltk1` | `fltk1sermo` binary, man page, Texinfo manual | FLTK 1.4 |
| `sermo-backend-efl1` | `efl1sermo` binary, man page, Texinfo manual | Enlightenment/Elementary |
| `sermo-backend-sdl3` | `sdl3sermo` binary, man page, Texinfo manual | SDL 3 |
| `sermo-backend-ncurses` | `ncursessermo` binary, man page, Texinfo manual | ncursesw (terminal) |
| `sermo-gtkdialog` | the `gtkdialog` command and the `gtkdialog(1)` page, links to gtk3sermo | `sermo-backend-gtk3` |
| `gtk3sermo`, `gtk4sermo`, `qt6sermo`, `gtksermo` | transitional packages, empty | the 2.x package that replaces them, and `sermo-doc` |

That is **14 packages**, plus one `-dbgsym` package (debug symbols) per backend.
The dependencies are computed by `dh_shlibdeps` from the libraries actually
linked: they are neither guessed nor copied by hand.

### What is NOT in the packages

**The upstream changelog.** `debhelper` installed it in each of the thirteen
packages: `CHANGELOG.md` weighs 51 kB plain, 19.8 kB gzipped — and being
already compressed, it went through the `.deb` compression without shrinking,
where `copyright` (27.9 kB of text) packs down well. That was **251 kB across
the set, 13 % of the shipped weight**, for one file repeated thirteen times.
It stays in the repository and on the website.

**A copy of the common documentation, for the architecture independent
packages.** The five of them (`sermo-gtkdialog` and the four transitional ones)
attach their `/usr/share/doc` to `sermo-doc`'s through a symlink. A
transitional package held nothing else: `gtk3sermo` went from 32 kB to
**1.2 kB**.

⚠️ The eight architecture dependent packages **keep their own `copyright`**, as
policy requires: `--link-doc` does not cross the "all" / "any" boundary, and
`debhelper` actively rejects the combination since compatibility level 10
(CAVEAT 2 of `dh_installdocs`).

Measured on 2026-09-20 against packages built in a pristine Debian:
**2011 kB → 1700 kB, that is −15 %**, lintian 0 errors 0 warnings.

## Why the backends do NOT depend on the core (static linking)

A fair question: shouldn't `sermo-backend-fltk1` have `Depends: sermo-core-dev`
to pull in the C engine? **No — on purpose.**

The `libsermocore` core is a **static** library (`.a`), **linked into every
backend binary at build time**. `ldd fltk1sermo` therefore shows **no**
`libsermocore`: the C engine is **embedded** in the binary. Consequences:

- **No run-time dependency on the core.** A user who installs
  `sermo-backend-fltk1` gets a **self-contained** binary (the engine is inside).
  Adding `Depends: sermo-core-dev` would be a **mistake**: it would force
  **build** files (`.a` + headers) on users who have no use for them at run time.
- **`sermo-core-dev` is a DEVELOPMENT package only**: it contains nothing but
  `libsermocore.a` + headers + `sermocore.pc` (no `.so`). Its only use is to
  **build a new backend**. The library and `sermocore.pc` live in the multiarch
  directory (`/usr/lib/x86_64-linux-gnu/…`).
- **No need for a trimmed run-time package** (`sermo-core.deb`): the "run time"
  of a non-developer user is the **backend package itself**, self-contained. You
  install `sermo-backend-<toolkit>` (and possibly `sermo-gtkdialog`), nothing
  else.

**Accepted trade-off**: each backend embeds its own copy of the core (about
170 KB of code). The idiomatic Debian model — a shared `libsermocore1` `.so`
(run time) + `libsermocore-dev` (headers), with backends depending on the run
time — is NOT possible today: the core calls back into backend symbols
(`widget_*_create`, the `sermo_be_*` bridges), so a standalone `.so` would have
unresolved symbols. That is the goal of **Part 3** (an IR + inverting the event
loop); it will make the `.so` + run-time split possible and remove the
duplication.

Three variants of the core are used for the build: neutral (qt6, fltk1, efl1,
sdl3, ncurses), GLib (gtk3) and GTK 4 (gtk4). Only the neutral variant ships in
`sermo-core-dev`.

## The `sermo` alias

Each backend registers with `update-alternatives` for `/usr/bin/sermo`
(`debian/sermo-backend-<t>.alternatives`, installed by `dh_installalternatives`).
Priorities: gtk3=50, gtk4=45, qt6=40, fltk1=30, efl1=20, sdl3=10, ncurses=5; the
GTK 3 backend is therefore the default when several are installed. The `sermo(1)`
page and the `sermo` Texinfo manual follow the same choice.

```sh
update-alternatives --display sermo          # who answers to "sermo"
sudo update-alternatives --config sermo      # pick another one
```

## gtkdialog compatibility — a separate package

The `gtkdialog` **command** (for legacy scripts) lives in a **separate**
package, `sermo-gtkdialog`:

- it contains only the links `/usr/bin/gtkdialog → gtk3sermo` and
  `gtkdialog.1.gz → gtk3sermo.1.gz`;
- `Depends: sermo-backend-gtk3` (it needs the real binary);
- `Provides: gtkdialog`, `Conflicts: gtk3dialog, gtkdialog, haplo-dialog`,
  `Replaces: gtk3dialog, gtkdialog, haplo-dialog`.

**Why separate**: other projects also own `/usr/bin/gtkdialog` — the original
`gtkdialog` and above all **BunsenLabs' `gtk3dialog`**. Shipping the command
from the backend itself would create a **file conflict** with them. As a separate,
mutually exclusive package (`Conflicts`/`Replaces`), the user installs
`sermo-gtkdialog` **or** one of the others, never both — without breaking the
GTK 3 backend installation for those who already have a `gtkdialog`.

## Moving from 1.x

1.x shipped under the names `gtk3sermo`, `gtk4sermo`, `qt6sermo` and `gtksermo`,
as `.deb` files attached to the release. Installing the wanted 2.x packages from
their files removes the 1.x packages they replace:

```sh
sudo apt install ./sermo-backend-gtk3_2.7.5-1_amd64.deb ./sermo-gtkdialog_2.7.5-1_all.deb
```

Tested on 2026-09-17 in a clean Debian testing container, without network,
starting from `gtk3sermo`, `gtk4sermo`, `gtksermo` 1.1.4-2 and `qt6sermo`
1.0.2-2 (outside `construire-paquets.sh`). The full guide is
[MIGRATION.en.md](MIGRATION.en.md).

For those who follow an apt repository, these names are also **transitional
packages**: empty, they depend on the package that replaces them
(`sermo-backend-gtk3`, `sermo-backend-gtk4`, `sermo-backend-qt6`,
`sermo-gtkdialog`). The new packages declare `Breaks` and `Replaces` on the old
ones `(<< 2.0~)`. A plain `apt upgrade` therefore brings in the 2.x backends.

The transitional packages can then go. **First** mark the 2.x packages you use as
wanted: they came in as dependencies, they are recorded as "automatically
installed", and `apt autoremove` would remove them along with the transitional
packages.

```sh
sudo apt install sermo-backend-gtk3 sermo-gtkdialog      # the ones you use
sudo apt purge gtk3sermo gtk4sermo qt6sermo gtksermo
```

## Building the packages

### In a clean Debian testing system (reference method)

```sh
bash packaging/construire-paquets.sh
SERMO_PAQUETS_1X=/path/to/1.x-packages bash packaging/construire-paquets.sh   # + the move from 1.x
```

The script archives the **current commit** (`git archive`: nothing uncommitted
gets into the packages), then, in throwaway `debian:testing` containers:

1. installs the build dependencies and builds (`dpkg-buildpackage`) as an
   ordinary user;
2. runs `lintian` (no error allowed; the exceptions are justified in
   `debian/*.lintian-overrides`);
3. installs every package in a clean system: each binary finds its libraries and
   answers `--version`, the `sermo` alternative and the `gtkdialog` command lead
   to gtk3sermo, a dialog runs in a terminal; then purges, and checks that no file
   and no alternative remain;
4. if `SERMO_PAQUETS_1X` is set: installs 1.x, serves the 2.x packages through a
   local apt repository, checks that `apt upgrade` plans the transition, performs
   it, then removes the transitional packages as described above and checks that
   the 2.x backends remain.

Results and logs go to `packaging/sortie/`. The containers download from the
Debian mirrors (around 550 MB the first time); the `packaging/sortie/cache-apt`
cache and the workstation's apt cache shorten later runs. Docker is required.

### Replaying the benches on the packaged binaries

A package's binary is not the CI's binary: `dpkg-buildflags` adds its flags, the
debug symbols move into the `-dbgsym` package. That binary is what ships, so that
binary is what gets measured:

```sh
SERMO_TASKSET=0,1 bash packaging/bancs-sur-paquets.sh packaging/sortie/paquets <commit>
```

The script clones the commit, builds the tree (the core benches need it),
replaces the tree's seven binaries with those extracted from the `.deb` files
(SHA-256 sums compared), checks that each binary finds its symbols in its
`-dbgsym` package, then runs `ci/bancs.sh` unchanged. `SERMO_TASKSET` restricts
the measurement to given CPU cores, like a shared CI runner: run nothing else on
them meanwhile, since the graphical benches wait a bounded time for their windows
(10 s for the examples).

### Directly, on Debian testing

```sh
sudo apt build-dep ./
dpkg-buildpackage -us -uc -b
```

`debian/rules` builds the three core variants and the seven backends, and during
the build replays the unit tests and the XML bench on every binary
(`DEB_BUILD_OPTIONS=nocheck` skips them). The packages land in the parent
directory.

### Licenses: `debian/copyright`

`debian/copyright` (DEP-5 format) is **generated** by
`packaging/copyright-dep5.py` from the files tracked by git: every file list is
exact. Run it again when files come or go (CC0 test cases, sources taken from
upstream…). Provenance, port by port, is detailed in
[LICENCES.en.md](LICENCES.en.md).

## Installing and uninstalling

```sh
sudo apt install ./sermo-backend-gtk3_2.7.5-1_amd64.deb      # GTK 3
sudo apt install ./sermo-gtkdialog_2.7.5-1_all.deb           # + the gtkdialog command
sudo apt install ./sermo-core-dev_2.7.5-1_amd64.deb          # to build a backend

sudo apt purge sermo-gtkdialog sermo-backend-gtk3            # remove everything
```

There is no apt repository: the packages are attached to each release, with
their checksums (see the [README](README.en.md#install)).
