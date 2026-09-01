# Packaging — qt6sermo

**Toolkit :** Qt6
**Version :** 1.0.0
**Binaire :** `/usr/bin/qt6sermo`
**Licence :** GPL-2.0-or-later
**Intégration bureau :** `data/qt6sermo.desktop` (lanceur masqué) et `data/qt6sermo.metainfo.xml` (AppStream) sont fournis — installés par le build amont et le paquet Debian.

## Formats disponibles

| Format | OS cibles | Dossier |
|--------|-----------|---------|
| `.deb` | Debian, Ubuntu, Linux Mint, haplo-dialog | `packaging/debian/` |
| `.rpm` (spec) | Fedora, RHEL, CentOS, openSUSE | `packaging/rpm/` |
| `PKGBUILD` | Arch Linux, Manjaro, EndeavourOS | `packaging/arch/` |
| `ebuild` | Gentoo, Calculate Linux | `packaging/gentoo/` |
| `.SlackBuild` | Slackware, Salix | `packaging/slackware/` |

## Construction rapide

### Debian / Ubuntu / haplo-dialog

```bash
cd qt6sermo_1.0.0/
dpkg-buildpackage -b -us -uc
# Résultat : ../qt6sermo_1.0.0-1_amd64.deb
```

### Fedora / RHEL

```bash
cp packaging/rpm/qt6sermo.spec ~/rpmbuild/SPECS/
rpmbuild -ba ~/rpmbuild/SPECS/qt6sermo.spec
```

### Arch Linux

```bash
cd packaging/arch/
makepkg -si
```

### Gentoo

```bash
cp -r packaging/gentoo/ /var/db/repos/haplo/app-misc/qt6sermo/
emerge -av app-misc/qt6sermo
```

### Slackware

```bash
cd packaging/slackware/
sudo sh qt6sermo.SlackBuild
```

## Coexistence des binaires

Les 6 ports dialog s'installent **simultanément sans conflit** :

| Binaire | Port | Toolkit |
|---------|------|---------|
| `/usr/bin/gtk3dialog` (+ symlink `gtkdialog`) | gtk3dialog | GTK3 |
| `/usr/bin/gtk4dialog` | gtk4dialog | GTK4 |
| `/usr/bin/fltk1dialog` | fltk1dialog | FLTK |
| `/usr/bin/efl1dialog` | efl1dialog | EFL |
| `/usr/bin/qt6sermo` | qt6sermo | Qt6 |
| `/usr/bin/sdl3dialog` | sdl3dialog | SDL3 |

---

## Assistance IA

Ces fichiers de packaging ont été créés avec l'assistance de **Claude** (Anthropic).

> *"Ce projet a bénéficié de l'assistance de Claude (Anthropic) — mai 2026."*

---

*Document horodaté — dernière mise à jour : 2026-06-10 (audit #6 — haplo-dialog v1.0.0).*