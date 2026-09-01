# Packaging Gentoo — qt6sermo

## Overlay local

```bash
# Créer un overlay local
mkdir -p /var/db/repos/haplo/app-misc/qt6sermo
cp packaging/gentoo/qt6sermo-1.0.0.ebuild /var/db/repos/haplo/app-misc/qt6sermo/
cp packaging/gentoo/Manifest /var/db/repos/haplo/app-misc/qt6sermo/

# Générer le Manifest avec la vraie archive
ebuild /var/db/repos/haplo/app-misc/qt6sermo/qt6sermo-1.0.0.ebuild manifest

# Installer
emerge -av app-misc/qt6sermo
```

## Dépendances

```bash
emerge -av dev-qt/qtbase:6 dev-qt/qttools:6 sys-devel/flex sys-devel/bison dev-build/cmake
```

---

> **TODO :** installer aussi `data/qt6sermo.desktop` et `data/qt6sermo.metainfo.xml` (fournis depuis la 1.0.0 ; déjà intégrés au build amont et au paquet Debian).

*Document horodaté — dernière mise à jour : 2026-06-10 (audit #6 — haplo-dialog v1.0.0).*