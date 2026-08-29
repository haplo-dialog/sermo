# Packaging Gentoo — gtk3sermo

## Overlay local

```bash
# Créer un overlay local
mkdir -p /var/db/repos/local/app-misc/gtk3sermo
cp packaging/gentoo/gtk3sermo-1.1.3.ebuild /var/db/repos/local/app-misc/gtk3sermo/
# (pas de Manifest a copier : packaging/gentoo/Manifest.in est un GABARIT,
#  le vrai Manifest se genere ci-dessous avec la vraie archive)

# Générer le Manifest avec la vraie archive
ebuild /var/db/repos/local/app-misc/gtk3sermo/gtk3sermo-1.1.3.ebuild manifest

# Installer
emerge -av app-misc/gtk3sermo
```

## Dépendances

```bash
emerge -av x11-libs/gtk+:3 x11-libs/vte:2.91 sys-devel/flex sys-devel/bison
```

---

