# Packaging Debian — gtk4sermo

**Toolkit :** GTK4
**Paquet source :** `gtk4sermo` · **Paquet binaire :** `gtk4sermo`
**Version :** 1.0.0
**Cibles :** Debian, Ubuntu, Linux Mint, Haplo-Linux

## Dépendances de construction

```bash
sudo apt-get install build-essential debhelper devscripts \
  libgtk-4-dev (>= 4.6.0), libvte-2.91-gtk4-dev, flex, bison, autoconf, automake, pkg-config
```

## Construction du paquet

```bash
cd gtk4sermo_1.0.0/
dpkg-buildpackage -b -us -uc
# Résultat : ../gtk4sermo_1.0.0-1_amd64.deb
```

Pour un build signé et complet (source + binaire) :

```bash
dpkg-buildpackage -us -uc
```

## Installation

```bash
sudo dpkg -i ../gtk4sermo_1.0.0-1_amd64.deb
sudo apt-get -f install   # résout les dépendances runtime si besoin
```

Dépendances runtime tirées automatiquement : `libgtk-4-1, libvte-2.91-gtk4-0`.

## Contenu du paquet

Les fichiers installés sont listés dans `debian/rules (le paquet prend ce que `make install` pose ; il n'y a pas de fichier .install)` (binaire `/usr/bin/gtk4sermo`,
exemples et documentation).

## Fichiers debian/

| Fichier | Rôle |
|---------|------|
| `control` | Métadonnées, dépendances build et runtime |
| `changelog` | Historique des versions (format Debian) |
| `rules` | Recette de build (debhelper) |
| `copyright` | Licence GPL-2.0-or-later + héritage gtkdialog |
| `gtk4sermo.install` | Liste des fichiers installés |
| `compat` / `source/` | Niveau debhelper et format source |

## Désinstallation

```bash
sudo apt-get remove gtk4sermo
```

---

## Assistance IA

Ces fichiers de packaging ont été créés avec l'assistance de **Claude** (Anthropic).

---

*Document horodaté — dernière mise à jour : 2026-06-07 (audit #3 — haplo-dialog v1.0.0).*
