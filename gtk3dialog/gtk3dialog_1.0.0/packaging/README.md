# Packaging — gtk3dialog

**Backend :** GTK 3
**Version :** 1.0.0
**Binaire :** `/usr/bin/gtk3dialog` (+ alias rétro-compatible `gtkdialog`)
**Licence :** GPL-2.0-or-later

## Formats disponibles, et ce qu'ils valent

**Une recette fournie n'est pas un paquet testé.** Seul le `.deb` est construit
et vérifié par le projet à chaque version. Les quatre autres sont un point de
départ pour un empaqueteur : elles n'ont jamais tourné chez nous.

| Format | OS cibles | Dossier | Construit par le projet |
|--------|-----------|---------|:--:|
| `.deb` | Debian, Ubuntu, Linux Mint | `packaging/debian/` | ✅ à chaque version |
| `.rpm` (spec) | Fedora, RHEL, CentOS, openSUSE | `packaging/rpm/` | ❌ |
| `PKGBUILD` | Arch Linux, Manjaro, EndeavourOS | `packaging/arch/` | ❌ |
| `ebuild` | Gentoo, Calculate Linux | `packaging/gentoo/` | ❌ |
| `.SlackBuild` | Slackware, Salix | `packaging/slackware/` | ❌ |

Vous en faites tourner une ? Dites-le nous (`devel@haplo-dialog.fr`) : elle
passera dans la colonne de droite, avec la version et la distribution où elle
a marché.

## Construction rapide

### Debian / Ubuntu

```bash
cd gtk3dialog_1.0.0/
dpkg-buildpackage -b -us -uc
# Résultat : ../haplo-dialog_1.0.0-8_amd64.deb
```

### Fedora / RHEL

```bash
cp packaging/rpm/haplo-dialog.spec ~/rpmbuild/SPECS/
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

