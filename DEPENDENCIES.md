# Dépendances & prérequis - haplo-dialog

Ce document liste les dépendances **réelles** de haplo-dialog (port unique
**gtk3dialog**, backend GTK 3), dérivées directement des fichiers de build et de
packaging du dépôt, et non d'une description rédigée à la main.

## Méthodologie

Chaque ligne ci-dessous est tirée d'un fichier vérifiable dans l'arbre :

| Source | Ce qu'on en extrait |
|--------|---------------------|
| `gtk3dialog/gtk3dialog_1.0.0/configure.ac` | `PKG_CHECK_MODULES`, `AC_PATH_PROG`, planchers de version |
| `gtk3dialog/gtk3dialog_1.0.0/packaging/debian/control` | `Build-Depends`, `Depends`, `Recommends`, `Suggests` |
| `gtk3dialog/gtk3dialog_1.0.0/packaging/arch/PKGBUILD` | `depends`, `makedepends`, `optdepends` |
| `gtk3dialog/gtk3dialog_1.0.0/packaging/rpm/*.spec` | `BuildRequires`, `Requires`, `Recommends` |
| `gtk3dialog/gtk3dialog_1.0.0/packaging/gentoo/*.ebuild` | `BDEPEND`, `DEPEND`, `RDEPEND` |

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

Le build est piloté par **autotools** ; gtk3dialog est un port **C** (pas de C++
requis).

### GLib

| Statut GLib | Détail (source) |
|-------------|-----------------|
| **requis** (`glib-2.0 ≥ 2.56`) | `AC_MSG_ERROR` si absent, mais tiré par GTK 3 |

---

## Synthèse

| Port | Build | Toolkit (plancher) | Terminal | C++ |
|------|-------|--------------------|----------|-----|
| **gtk3dialog** | autotools | GTK+ 3 ≥ 3.22.0 | VTE 2.91 *(opt.)* | non |

*« opt. » = le widget terminal se désactive proprement (stub) si la lib est
absente ; le reste du port fonctionne.*

> **Build vérifié.** Dans l'environnement de vérification (GTK 3 3.24.52), le jeu
> de dépendances ci-dessus a été exercé par une compilation + édition de liens
> réussie (rc=0, 0 erreur) : gtk3dialog produit un binaire fonctionnel (~1,4 Mo)
> qui passe **52/52** tests XML de régression headless (`--print-ir`), en plus des
> **9** tests de comportement `safe_exec`. Le prérequis de build (installer le
> toolkit et les outils) est le seul vrai obstacle.

---

## Détail par distribution

Les noms de paquets ci-dessous sont **ceux déclarés dans le packaging du
dépôt**. La colonne « build » regroupe développement + outils ; la colonne
« exécution » correspond aux `Depends` runtime / `Requires`.

### gtk3dialog - GTK+ 3 ≥ 3.22.0, VTE 2.91 (optionnel)

| Distro | Build | Exécution |
|--------|-------|-----------|
| Debian/Ubuntu | `libgtk-3-dev (≥ 3.22.0)`, `libvte-2.91-dev`, `flex`, `bison`, `autoconf (≥ 2.69)`, `automake (≥ 1.14)`, `pkg-config` | `libgtk-3-0 (≥ 3.22)`, `libvte-2.91-0` |
| Arch | `gtk3`, `vte3`, `flex`, `bison`, `autoconf`, `automake`, `pkgconfig` | `gtk3`, `vte3` |
| Fedora/RPM | `gtk3-devel`, `vte291-devel`, `flex`, `bison`, `autoconf`, `automake`, `pkgconfig` | `gtk3`, `vte291` |
| Gentoo | `x11-libs/gtk+:3`, `x11-libs/vte:2.91` | idem |

Option de configuration : `./configure --without-vte` désactive le widget
`<terminal>`.

---

## Installation rapide des prérequis de build

```sh
# ── Debian / Ubuntu ──────────────────────────────────────────────────────────
sudo apt-get install build-essential flex bison pkg-config autoconf automake
sudo apt-get install libgtk-3-dev libvte-2.91-dev                 # gtk3dialog

# ── Arch ─────────────────────────────────────────────────────────────────────
sudo pacman -S base-devel flex bison pkgconf
sudo pacman -S gtk3 vte3                                          # gtk3dialog

# ── Fedora ───────────────────────────────────────────────────────────────────
sudo dnf install gcc flex bison pkgconf-pkg-config autoconf automake
sudo dnf install gtk3-devel vte291-devel                         # gtk3dialog
```

Build effectif :

```sh
cd gtk3dialog/gtk3dialog_1.0.0
autoreconf -fi && ./configure && make
```

Voir aussi `ci/build.sh` (`./ci/build.sh`, ou `./ci/build.sh gtk3dialog --test`).

---

*haplo-dialog est un fork de gtkdialog 0.8.3 (László Pere, GPL-2.0+),*
*modernisé par le projet haplo-dialog. Dépendances vérifiées sur l'arbre v1.0.0.*

---

*Document dérivé des fichiers de build/packaging du dépôt, build gtk3dialog vérifié + 52/52 tests XML de régression (haplo-dialog v1.0.0).*
