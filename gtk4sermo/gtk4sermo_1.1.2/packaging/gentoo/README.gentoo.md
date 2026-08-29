# Packaging Gentoo — gtk4sermo

## Overlay local

```bash
# Créer un overlay local
mkdir -p /var/db/repos/local/app-misc/gtk4sermo
cp packaging/gentoo/gtk4sermo-1.1.2.ebuild /var/db/repos/local/app-misc/gtk4sermo/
# (pas de Manifest a copier : packaging/gentoo/Manifest.in est un GABARIT,
#  le vrai Manifest se genere ci-dessous avec la vraie archive)

# Générer le Manifest avec la vraie archive
ebuild /var/db/repos/local/app-misc/gtk4sermo/gtk4sermo-1.1.2.ebuild manifest

# Installer
emerge -av app-misc/gtk4sermo
```

## Dépendances

```bash
emerge -av gui-libs/gtk:4 x11-libs/vte:2.91-gtk4 sys-devel/flex sys-devel/bison
```

---

*Document horodaté — dernière mise à jour : 2026-06-07 (audit #3 — haplo-dialog v1.0.0).*
