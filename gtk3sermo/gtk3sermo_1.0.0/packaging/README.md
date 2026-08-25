# Packaging — gtk3sermo

**Backend :** GTK 3
**Version :** 1.0.0
**Binaire :** `/usr/bin/gtk3sermo`. L'alias `gtkdialog` vient du paquet séparé `gtksermo`.
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
cd gtk3sermo_1.0.0/
dpkg-buildpackage -b -us -uc
# Résultat : ../gtk3sermo_1.0.0-10_amd64.deb
```

### Fedora / RHEL

```bash
cp packaging/rpm/gtk3sermo.spec ~/rpmbuild/SPECS/
rpmbuild -ba ~/rpmbuild/SPECS/gtk3sermo.spec
```

### Arch Linux

```bash
cd packaging/arch/
makepkg -si
```

### Gentoo

```bash
cp -r packaging/gentoo/ /var/db/repos/haplo/app-misc/gtk3sermo/
emerge -av app-misc/gtk3sermo
```

### Slackware

```bash
cd packaging/slackware/
sudo sh gtk3sermo.SlackBuild
```

## Binaire installé

Le paquet fournit un seul binaire, `/usr/bin/gtk3sermo`. L'alias
rétro-compatible `gtkdialog`, qui remplace sans friction le gtkdialog historique
(Laszlo Pere, abandonné en amont), est livré par un paquet SÉPARÉ, `gtksermo` :
il entre en conflit avec `gtkdialog` et `gtk3dialog`, on l'installe donc à
part.

---

