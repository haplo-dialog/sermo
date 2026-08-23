<div align="right"><sub><code>maj :     2026-08-18 12:40:07 · par claude</code></sub></div>
<div align="right"><sub><code>réindex : —</code></sub></div>

# Packaging Debian — gtk3sermo

**Toolkit :** GTK3
**Paquet source :** `gtk3sermo` · **Paquet binaire :** `gtk3sermo`
**Version :** 1.0.0
**Cibles :** Debian, Ubuntu, Linux Mint

## Dépendances de construction

```bash
sudo apt-get install build-essential debhelper devscripts \
  libgtk-3-dev (>= 3.22.0), libvte-2.91-dev, flex, bison, autoconf, automake, pkg-config
```

## Construction du paquet

```bash
cd gtk3sermo_1.0.0/
dpkg-buildpackage -b -us -uc
# Résultat : ../gtk3sermo_1.0.0-1_amd64.deb
```

Pour un build signé et complet (source + binaire) :

```bash
dpkg-buildpackage -us -uc
```

## Installation

```bash
sudo dpkg -i ../gtk3sermo_1.0.0-1_amd64.deb
sudo apt-get -f install   # résout les dépendances runtime si besoin
```

Dépendances runtime tirées automatiquement : `libgtk-3-0, libvte-2.91-0`.

## Contenu du paquet

Les fichiers installés sont listés dans `debian/gtk3sermo.install` (binaire `/usr/bin/gtk3sermo`,
exemples et documentation).

## Fichiers debian/

| Fichier | Rôle |
|---------|------|
| `control` | Métadonnées, dépendances build et runtime |
| `changelog` | Historique des versions (format Debian) |
| `rules` | Recette de build (debhelper) |
| `copyright` | Licence GPL-2.0-or-later + héritage gtkdialog |
| `gtk3sermo.install` | Liste des fichiers installés |
| `compat` / `source/` | Niveau debhelper et format source |

## Désinstallation

```bash
sudo apt-get remove gtk3sermo
```

---

