# Packaging Gentoo — gtk3dialog

## Overlay local

```bash
# Créer un overlay local
mkdir -p /var/db/repos/haplo/app-misc/gtk3dialog
cp packaging/gentoo/gtk3dialog-1.0.0.ebuild /var/db/repos/haplo/app-misc/gtk3dialog/
cp packaging/gentoo/Manifest /var/db/repos/haplo/app-misc/gtk3dialog/

# Générer le Manifest avec la vraie archive
ebuild /var/db/repos/haplo/app-misc/gtk3dialog/gtk3dialog-1.0.0.ebuild manifest

# Installer
emerge -av app-misc/gtk3dialog
```

## Dépendances

```bash
emerge -av x11-libs/gtk+:3 x11-libs/vte:2.91 sys-devel/flex sys-devel/bison
```

---

