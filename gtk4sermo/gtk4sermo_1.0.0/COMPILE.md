# Compiler gtk4sermo (GTK4)

**Haplo-Linux — 2026**

Ce document décrit la compilation du port GTK4 (`gtk4sermo`), installable
en parallèle du binaire GTK3 `gtk3d`.

---

## Dépendances requises

```bash
# Debian Testing / Haplo-Linux
sudo apt-get install \
  libgtk-4-dev \
  libvte-2.91-gtk4-dev \
  flex bison \
  autoconf automake \
  pkg-config gcc

# Arch Linux
sudo pacman -S gtk4 vte4 flex bison autoconf automake pkgconf gcc

# Fedora / RHEL
sudo dnf install gtk4-devel vte291-gtk4-devel flex bison autoconf automake pkgconf gcc
```

---

## Compilation

```bash
cd gtk4sermo_1.0.0/

# 1. Générer les scripts autoconf/automake
autoreconf -fiv

# 2. Configurer (prefix /usr pour cohabiter avec le paquet gtk3d GTK3)
./configure --prefix=/usr

# 3. Compiler
make -j$(nproc)

# 4. Installer
sudo make install
# Installe /usr/bin/gtk4sermo
```

### Compilation sans VTE (terminal désactivé)

Si `libvte-2.91-gtk4-dev` n'est pas disponible, VTE est automatiquement
désactivé (`HAVE_VTE=0`). Le widget `<terminal>` retournera un message
d'erreur à l'exécution.

---

## Coexistence avec gtk3d GTK3

Les deux binaires peuvent cohabiter sans conflit :

```
/usr/bin/gtk3d    ← GTK3 (1.0.0)
/usr/bin/gtk4sermo   ← GTK4 (1.0.0-gtk4)
```

Les scripts existants utilisant `gtk3d` restent inchangés.
Pour tester GTK4, remplacer `gtk3d` par `gtk4sermo` dans le script.

---

## Vérification de l'installation

```bash
# Versions
gtk4sermo --version
gtk4sermo --version

# Test fonctionnel GTK4
gtk4sermo --program='
<window title="Test GTK4">
  <vbox>
    <text><label>Bonjour depuis GTK4 !</label></text>
    <button ok></button>
  </vbox>
</window>'
```

---

## Tests XML communs (`tests/xml/`, sans display requis)

Suite XML partagée par les six ports (55 cas, parsés via `--print-ir`, headless) :

```bash
bash tests/run_tests.sh gtk4sermo
```

Résultat vérifié (2026-06-06) : **55/55 PASS**.

> **Note :** avant la réparation de la grammaire (commentaires `<!-- -->`,
> espaces entre attributs, câblage de 8 widgets : switch, password, calendar,
> infobar, spinner, searchentry, levelbar, drawingarea), ce port échouait à
> **tous** les tests XML (0/52). En `--print-ir`, l'analyse utilise
> `gtk_init_check()` et ne requiert pas de serveur X.

---

## Structure du port GTK4

Le port repose sur `src/gtk4-compat.h` (force-inclus via `-include`) qui
fournit des shims transparents pour ~25 APIs GTK3 supprimées en GTK4.

Trois widgets sont stubés (retournent un label d'avertissement) car leurs
API GTK2/3 sont complètement absentes de GTK4 :

- `<menu>` / `<menubar>` / `<menuitem>` — GtkMenu supprimé
- `<table>` — GtkCList supprimé (GTK2 uniquement)
- `<gvim>` — GtkSocket supprimé (X11 uniquement)

Voir `GTK4_MIGRATION.md` pour la feuille de route complète.

---

## Erreurs connues à la compilation

### `gtk4` non trouvé par pkg-config

```
configure: error: Package requirements (gtk4 >= 4.6.0) were not met
```

Installer `libgtk-4-dev`. Sur Debian Trixie :
```bash
sudo apt-get install libgtk-4-dev
```

### `vte-2.91-gtk4` non trouvé

```
Package 'vte-2.91-gtk4' not found
```

La compilation continue avec `HAVE_VTE=0` (widget terminal désactivé).
Pour activer VTE : `sudo apt-get install libvte-2.91-gtk4-dev`.

### Avertissements de compilation

La compilation aboutit **sans erreur** (make → rc=0) et produit `src/gtk4sermo`
(~1,45 Mo). Elle émet environ **77 avertissements** (réduits depuis 1549).
Ce sont des idiomes hérités du code amont gtkdialog 0.8.3
(assignation en condition `-Wparentheses`, casts entier↔pointeur,
variables positionnées mais inutilisées). **Aucun n'est bloquant.**

Les dépréciations GTK 4.22 (`-Wdeprecated-declarations`), absentes des
GTK4 plus anciens, sont volontairement filtrées via les flags de
`src/Makefile.am` (`-Wno-deprecated-declarations`,
`-Wno-missing-prototypes`, `-Wno-unused-variable`).

---

*Haplo-Linux — devel@haplo-dialog.fr — 2026*

---

*Document horodaté — dernière mise à jour : 2026-06-07 (audit #5 — grammaire réparée, tests XML 0/52 → 55/55).*
