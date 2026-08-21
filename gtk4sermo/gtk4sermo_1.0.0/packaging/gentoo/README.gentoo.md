<div align="right"><sub><code>maj :     2026-08-20 20:34:10 · par claude</code></sub></div>
<div align="right"><sub><code>réindex : —</code></sub></div>

# Packaging Gentoo — gtk4sermo

## Overlay local

```bash
# Créer un overlay local
mkdir -p /var/db/repos/haplo/app-misc/gtk4sermo
cp packaging/gentoo/gtk4sermo-1.0.0.ebuild /var/db/repos/haplo/app-misc/gtk4sermo/
cp packaging/gentoo/Manifest /var/db/repos/haplo/app-misc/gtk4sermo/

# Générer le Manifest avec la vraie archive
ebuild /var/db/repos/haplo/app-misc/gtk4sermo/gtk4sermo-1.0.0.ebuild manifest

# Installer
emerge -av app-misc/gtk4sermo
```

## Dépendances

```bash
emerge -av gui-libs/gtk:4 x11-libs/vte:2.91-gtk4 sys-devel/flex sys-devel/bison
```

---

*Document horodaté — dernière mise à jour : 2026-06-07 (audit #3 — haplo-dialog v1.0.0).*
