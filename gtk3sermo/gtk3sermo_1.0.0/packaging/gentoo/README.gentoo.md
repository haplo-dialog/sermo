# Packaging Gentoo — gtk3sermo

## Overlay local

```bash
# Créer un overlay local
mkdir -p /var/db/repos/haplo/app-misc/gtk3sermo
cp packaging/gentoo/gtk3sermo-1.0.0.ebuild /var/db/repos/haplo/app-misc/gtk3sermo/
cp packaging/gentoo/Manifest /var/db/repos/haplo/app-misc/gtk3sermo/

# Générer le Manifest avec la vraie archive
ebuild /var/db/repos/haplo/app-misc/gtk3sermo/gtk3sermo-1.0.0.ebuild manifest

# Installer
emerge -av app-misc/gtk3sermo
```

## Dépendances

```bash
emerge -av x11-libs/gtk+:3 x11-libs/vte:2.91 sys-devel/flex sys-devel/bison
```

---

