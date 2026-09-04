# Dépendances & prérequis — sermo

Ce document liste les dépendances **réelles** des trois ports de sermo —
**gtk3sermo** (backend GTK 3, port de référence), **gtk4sermo** (backend GTK 4)
et **qt6sermo** (backend Qt 6) — dérivées directement des fichiers de build et de
packaging du dépôt, et non d'une description rédigée à la main.

## Méthodologie

Chaque ligne ci-dessous est tirée d'un fichier vérifiable dans l'arbre :

`<port>` vaut `gtk3sermo` ou `gtk4sermo` : les deux ports autotools ont chacun
leur jeu complet de fichiers, et chaque ligne ci-dessous a été relue dans **les
deux**. Le troisième port, **qt6sermo**, ne se lit pas dans ce tableau : il se
construit par **CMake**, sa source de vérité est
`qt6sermo/qt6sermo_1.0.2/CMakeLists.txt`, et son packaging vit sous
`qt6sermo/qt6sermo_1.0.2/packaging/` (mêmes cinq formats).

| Source | Ce qu'on en extrait |
|--------|---------------------|
| `<port>/<port>_1.1.0/configure.ac` | `PKG_CHECK_MODULES`, `AC_ARG_WITH`, planchers de version |
| `<port>/<port>_1.1.0/packaging/debian/control` | `Build-Depends`, `Depends`, `Recommends`, `Suggests` |
| `<port>/<port>_1.1.0/packaging/arch/PKGBUILD` | `depends`, `makedepends`, `optdepends` |
| `<port>/<port>_1.1.0/packaging/rpm/*.spec` | `BuildRequires`, `Requires`, `Recommends` |
| `<port>/<port>_1.1.0/packaging/gentoo/*.ebuild` | `BDEPEND`, `DEPEND`, `RDEPEND` |

En cas de divergence entre ce que le build exige réellement et ce que le
packaging déclare, c'est le **système de build** (le compilateur et l'éditeur de
liens) qui fait foi, pas les métadonnées de paquet.

---

## Prérequis de build

Le cœur C (parser XML, variables, actions, signaux, pile) et le parseur
`gtkdialog_lexer.l` / `gtkdialog_parser.y` imposent :

| Outil | Détecté par | Plancher |
|-------|-------------|----------|
| Compilateur C | `AC_PROG_CC` | C11 |
| **flex** | `AM_PROG_LEX` | — |
| **bison** | `AC_PROG_YACC` | — |
| **pkg-config** | `PKG_CHECK_MODULES` | — |
| **autoconf** | — | ≥ 2.69 |
| **automake** | — | ≥ 1.14 |

Le build des deux ports GTK est piloté par **autotools**, et tous deux sont
écrits en **C** — aucun compilateur C++ n'est requis. Le port **qt6sermo** fait
exception : il se construit par **CMake ≥ 3.20**, et son interface Qt est en
**C++17** — un compilateur C++ y est donc requis (`g++` dans ses `Build-Depends`).

### GLib

| Statut GLib | Détail (source) |
|-------------|-----------------|
| **requis** (`glib-2.0 ≥ 2.56`) | `AC_MSG_ERROR` si absent, mais tiré par GTK 3 |

---

## Synthèse

| Port | Build | Toolkit (plancher) | Terminal | C++ |
|------|-------|--------------------|----------|-----|
| **gtk3sermo** | autotools | GTK+ 3 ≥ 3.22.0 | VTE 2.91 *(opt.)* | non |
| **gtk4sermo** | autotools | GTK 4 ≥ 4.6.0 | VTE 2.91 GTK4 *(opt.)* | non |
| **qt6sermo** | CMake ≥ 3.20 | Qt 6 ≥ 6.2 | QTermWidget (`qtermwidget6`) *(opt.)* | oui |

| Port | Ancrage Wayland |
|------|-----------------|
| **gtk3sermo** | gtk-layer-shell ≥ 0.8.0 *(opt.)* |
| **gtk4sermo** | *aucun* — l'ancrage n'est pas porté sur GTK 4 |
| **qt6sermo** | *aucun* — l'ancrage n'existe que sur `gtk3sermo` |

*« opt. » = le widget terminal se désactive proprement (stub) si la lib est
absente ; le reste du port fonctionne. De même pour gtk-layer-shell : sans
lui, les attributs `layer`/`edge`/`dist` de `<window>` sont ignorés et la
fenêtre s'ouvre normalement.*

> **Build vérifié — les deux ports GTK.** Le jeu de dépendances ci-dessus a été exercé
> par une compilation + édition de liens réussie (rc=0, 0 erreur) pour chacun d'eux :
> chaque port produit un binaire fonctionnel qui passe **55/55** tests XML de
> régression headless (`--print-ir`) et les **9** tests de comportement `safe_exec`,
> puis ouvre réellement ses exemples livrés sous Xvfb en locale `fr_FR.UTF-8`.
> C'est aussi ce que rejoue l'intégration continue à chaque poussée. Le prérequis
> de build (installer le toolkit et les outils) est le seul vrai obstacle.

> **Build vérifié — qt6sermo.** Le port Qt 6 est branché à la même intégration
> continue depuis le **2026-09-02**, pipeline vert : **55/55** à la suite XML
> partagée et **24/24** à son banc de comportement, chaque cas comparé à la
> **valeur** rendue par le port GTK 3 de référence. Son `.deb`, en revanche,
> n'est **pas encore construit** : source et recettes seulement.

---

## Détail par distribution

Les noms de paquets ci-dessous sont **ceux déclarés dans le packaging du
dépôt**. La colonne « build » regroupe développement + outils ; la colonne
« exécution » correspond aux `Depends` runtime / `Requires`.

### gtk3sermo - GTK+ 3 ≥ 3.22.0, VTE 2.91 et gtk-layer-shell (optionnels)

| Distro | Build | Exécution |
|--------|-------|-----------|
| Debian/Ubuntu | `libgtk-3-dev (≥ 3.22.0)`, `libvte-2.91-dev`, `libgtk-layer-shell-dev (≥ 0.8.0)`, `flex`, `bison`, `autoconf (≥ 2.69)`, `automake (≥ 1.14)`, `pkg-config` | `libgtk-3-0 (≥ 3.22)`, `libvte-2.91-0`, `libgtk-layer-shell0 (≥ 0.8.0)` |
| Arch | `gtk3`, `vte3`, `gtk-layer-shell`, `flex`, `bison`, `autoconf`, `automake`, `pkgconfig` | `gtk3`, `vte3`, `gtk-layer-shell` |
| Fedora/RPM | `gtk3-devel`, `vte291-devel`, `gtk-layer-shell-devel`, `flex`, `bison`, `autoconf`, `automake`, `pkgconfig` | `gtk3`, `vte291`, `gtk-layer-shell` |
| Gentoo | `x11-libs/gtk+:3`, `x11-libs/vte:2.91`, `gui-libs/gtk-layer-shell` | idem |


### gtk4sermo - GTK 4 ≥ 4.6.0 et VTE 2.91 GTK4

Le port GTK 4 n'a **pas** l'ancrage Wayland : `layer`, `edge`, `dist` et
`reserve` ne sont implémentés que côté GTK 3.

| Distro | Build | Exécution |
|--------|-------|-----------|
| Debian/Ubuntu | `libgtk-4-dev (≥ 4.6.0)`, `libvte-2.91-gtk4-dev`, `flex`, `bison`, `autoconf (≥ 2.69)`, `automake (≥ 1.14)`, `pkg-config` | `libgtk-4-1 (≥ 4.6)`, `libvte-2.91-gtk4-0` |
| Arch | `gtk4`, `vte4`, `flex`, `bison`, `autoconf`, `automake`, `pkgconfig` | `gtk4`, `vte4` |
| Fedora/RPM | `gtk4-devel`, `vte291-gtk4-devel`, `flex`, `bison`, `autoconf`, `automake`, `pkgconfig` | `gtk4`, `vte291-gtk4` |
| Gentoo | `gui-libs/gtk:4`, `x11-libs/vte:2.91-gtk4` | idem |

### qt6sermo - Qt 6 ≥ 6.2, CMake ≥ 3.20, QTermWidget optionnel

Troisième port, hors GTK : il ne lie ni GTK ni GLib, et n'a **pas** l'ancrage
Wayland. Il se construit par **CMake** et non par autotools, et demande un
compilateur **C++**. Le widget `<terminal>` s'appuie sur **QTermWidget** (module
pkg-config `qtermwidget6`) : détecté automatiquement, absent il se réduit à un
stub. Aucune recette du port ne le déclare en dépendance — il n'apparaît donc
pas dans les colonnes ci-dessous.

| Distro | Build | Exécution |
|--------|-------|-----------|
| Debian/Ubuntu | `qt6-base-dev (≥ 6.2.0)`, `cmake (≥ 3.20)`, `g++`, `flex`, `bison`, `pkg-config` | `libqt6core6 (≥ 6.2)`, `libqt6widgets6`, `libqt6gui6` |
| Arch | `qt6-base`, `qt6-tools`, `cmake`, `flex`, `bison`, `pkgconfig` | `qt6-base` |
| Fedora/RPM | `qt6-qttools-devel`, `cmake ≥ 3.20`, `gcc-c++`, `flex`, `bison` | `qt6-qtbase ≥ 6.2` |
| Gentoo | `dev-qt/qtbase:6`, `dev-qt/qttools:6`, `dev-build/cmake`, `sys-devel/flex`, `sys-devel/bison` | `dev-qt/qtbase:6` |

#### Options de configuration — elles diffèrent d'un port à l'autre

| Option | gtk3sermo | gtk4sermo |
|--------|-----------|-----------|
| `--without-vte` | oui — désactive le widget `<terminal>` | **n'existe pas** : VTE est détecté automatiquement, et son absence se solde par `HAVE_VTE=0` sans intervention |
| `--without-layer-shell` | oui — désactive l'ancrage Wayland | **n'existe pas** : l'ancrage n'est pas porté sur GTK 4 |
| `--enable-debug` | oui | oui |
| `--enable-unit-tests` | oui (requiert `check ≥ 0.9.6`) | **n'existe pas** |

Passer à `gtk4sermo` une option qu'il ne connaît pas ne fait pas échouer le
`configure` : autoconf l'ignore avec un simple avertissement. L'option n'aura donc
aucun effet — ce n'est pas un moyen de désactiver quoi que ce soit.

`qt6sermo` n'a **aucune** de ces options : il n'a pas de `configure`. Ce qui s'y
règle passe par CMake (`-DCMAKE_BUILD_TYPE=…`), et QTermWidget y est détecté
automatiquement, sans drapeau.

---

## Installation rapide des prérequis de build

```sh
# ── Debian / Ubuntu ──────────────────────────────────────────────────────────
sudo apt-get install build-essential flex bison pkg-config autoconf automake
sudo apt-get install libgtk-3-dev libvte-2.91-dev libgtk-layer-shell-dev   # gtk3sermo
sudo apt-get install libgtk-4-dev libvte-2.91-gtk4-dev                     # gtk4sermo
sudo apt-get install qt6-base-dev cmake g++                                # qt6sermo

# ── Arch ─────────────────────────────────────────────────────────────────────
sudo pacman -S base-devel flex bison pkgconf
sudo pacman -S gtk3 vte3 gtk-layer-shell                          # gtk3sermo
sudo pacman -S gtk4 vte4                                          # gtk4sermo
sudo pacman -S qt6-base qt6-tools cmake                           # qt6sermo

# ── Fedora ───────────────────────────────────────────────────────────────────
sudo dnf install gcc flex bison pkgconf-pkg-config autoconf automake
sudo dnf install gtk3-devel vte291-devel gtk-layer-shell-devel   # gtk3sermo
sudo dnf install gtk4-devel vte291-gtk4-devel                    # gtk4sermo
sudo dnf install qt6-qtbase-devel qt6-qttools-devel cmake gcc-c++  # qt6sermo
```

Build effectif — le même pour les deux ports GTK, seul le dossier change :

```sh
cd gtk3sermo/gtk3sermo_1.1.4     # ou : cd gtk4sermo/gtk4sermo_1.1.4
autoreconf -fi && ./configure && make
```

Le port Qt 6 se construit autrement — CMake, et son propre numéro de version :

```sh
cd qt6sermo/qt6sermo_1.0.2
cmake -B build -DCMAKE_BUILD_TYPE=Release && cmake --build build
```

Voir aussi `ci/build.sh` (`./ci/build.sh`, ou `./ci/build.sh gtk3sermo --test`).
Ce script ne connaît que les **deux ports autotools** : `qt6sermo` se construit
par les deux commandes CMake ci-dessus.

---

*haplo-dialog est un fork de gtkdialog 0.8.3 (László Pere, GPL-2.0+),*
*modernisé par le projet haplo-dialog sous le nom sermo. Dépendances relues sur*
*l'arbre v1.0.0, dans les deux ports.*

---

*Document dérivé des fichiers de build/packaging du dépôt ; builds gtk3sermo ET*
*gtk4sermo vérifiés, 55/55 tests XML de régression chacun (sermo v1.0.0).*
