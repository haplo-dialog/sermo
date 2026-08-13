# Packaging — gtk3dialog

**Backend :** GTK 3
**Version :** 1.0.0
**Binaire :** `/usr/bin/gtk3dialog` (+ alias rétro-compatible `gtkdialog`)
**Licence :** GPL-2.0-or-later

## Formats disponibles

| Format | OS cibles | Dossier |
|--------|-----------|---------|
| `.deb` | Debian, Ubuntu, Linux Mint | `packaging/debian/` |
| `.rpm` (spec) | Fedora, RHEL, CentOS, openSUSE | `packaging/rpm/` |
| `PKGBUILD` | Arch Linux, Manjaro, EndeavourOS | `packaging/arch/` |
| `ebuild` | Gentoo, Calculate Linux | `packaging/gentoo/` |
| `.SlackBuild` | Slackware, Salix | `packaging/slackware/` |

## Construction rapide

### Debian / Ubuntu

```bash
cd gtk3dialog_1.0.0/
dpkg-buildpackage -b -us -uc
# Résultat : ../gtk3dialog_1.0.0-1_amd64.deb
```

### Fedora / RHEL

```bash
cp packaging/rpm/gtk3dialog.spec ~/rpmbuild/SPECS/
rpmbuild -ba ~/rpmbuild/SPECS/gtk3dialog.spec
```

### Arch Linux

```bash
cd packaging/arch/
makepkg -si
```

### Gentoo

```bash
cp -r packaging/gentoo/ /var/db/repos/haplo/app-misc/gtk3dialog/
emerge -av app-misc/gtk3dialog
```

### Slackware

```bash
cd packaging/slackware/
sudo sh gtk3dialog.SlackBuild
```

## Binaire installé

Le paquet fournit un seul binaire, `/usr/bin/gtk3dialog`, accompagné de
l'alias rétro-compatible `gtkdialog` pour remplacer sans friction le
gtkdialog historique (Laszlo Pere), abandonné en amont.

---

