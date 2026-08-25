# Packaging — gtk4sermo

**Toolkit :** GTK4
**Version :** 1.0.0
**Binaire :** `/usr/bin/gtk4sermo`
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
cd gtk4sermo_1.0.0/
dpkg-buildpackage -b -us -uc
# Résultat : ../gtk4sermo_1.0.0-12_amd64.deb
```

### Fedora / RHEL

```bash
cp packaging/rpm/gtk4sermo.spec ~/rpmbuild/SPECS/
rpmbuild -ba ~/rpmbuild/SPECS/gtk4sermo.spec
```

### Arch Linux

```bash
cd packaging/arch/
makepkg -si
```

### Gentoo

```bash
cp -r packaging/gentoo/ /var/db/repos/local/app-misc/gtk4sermo/
emerge -av app-misc/gtk4sermo
```

### Slackware

```bash
cd packaging/slackware/
sudo sh gtk4sermo.SlackBuild
```

## Coexistence des binaires

Les deux ports s'installent **simultanément sans conflit** : chacun porte son
propre nom de binaire. Le nom historique `gtkdialog` n'est pris que par le
paquet `gtksermo`, qui est volontaire — c'est ce qui permet d'installer
`gtk3sermo` à côté de n'importe quelle autre implémentation de gtkdialog.

| Binaire | Paquet | Toolkit |
|---------|--------|---------|
| `/usr/bin/gtk3sermo` | gtk3sermo | GTK 3 |
| `/usr/bin/gtk4sermo` | gtk4sermo | GTK 4 |
| `/usr/bin/gtkdialog` (lien vers `gtk3sermo`) | gtksermo | compatibilité |

---

## Assistance IA

Ces fichiers de packaging ont été créés avec l'assistance de **Claude** (Anthropic).

> *"Ce projet a bénéficié de l'assistance de Claude (Anthropic) — mai 2026."*

---

*Document horodaté — dernière mise à jour : 2026-06-07 (audit #3 — haplo-dialog v1.0.0).*
