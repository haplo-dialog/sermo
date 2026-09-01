# Packaging Debian — qt6sermo

**Toolkit :** Qt6
**Paquet source :** `qt6sermo` · **Paquet binaire :** `qt6sermo`
**Version :** 1.0.0
**Cibles :** Debian, Ubuntu, Linux Mint, haplo-dialog

## Dépendances de construction

```bash
sudo apt-get install build-essential debhelper devscripts \
  qt6-base-dev (>= 6.2.0), qt6-tools-dev, flex, bison, cmake (>= 3.20), pkg-config, g++
```

## Construction du paquet

```bash
cd qt6sermo_1.0.0/
dpkg-buildpackage -b -us -uc
# Résultat : ../qt6sermo_1.0.0-1_amd64.deb
```

Pour un build signé et complet (source + binaire) :

```bash
dpkg-buildpackage -us -uc
```

## Installation

```bash
sudo dpkg -i ../qt6sermo_1.0.0-1_amd64.deb
sudo apt-get -f install   # résout les dépendances runtime si besoin
```

Dépendances runtime tirées automatiquement : `libqt6core6, libqt6widgets6, libqt6gui6`.

## Contenu du paquet

Les fichiers installés sont listés dans `debian/qt6sermo.install` (binaire `/usr/bin/qt6sermo`,
exemples et documentation).

## Fichiers debian/

| Fichier | Rôle |
|---------|------|
| `control` | Métadonnées, dépendances build et runtime |
| `changelog` | Historique des versions (format Debian) |
| `rules` | Recette de build (debhelper) |
| `copyright` | Licence GPL-2.0-or-later + héritage gtkdialog |
| `qt6sermo.install` | Liste des fichiers installés |
| `compat` / `source/` | Niveau debhelper et format source |

## Dépôt APT (en préparation)

La publication apt des paquets de la famille est **en préparation** : le dépôt
signé (pool + dists, `Release`/`InRelease`) se construit avec
`tools/make-apt-repo.sh` à la racine du dépôt source.

## Désinstallation

```bash
sudo apt-get remove qt6sermo
```

---

## Assistance IA

Ces fichiers de packaging ont été créés avec l'assistance de **Claude** (Anthropic).

---

*Document horodaté — dernière mise à jour : 2026-06-10 (audit #6 — haplo-dialog v1.0.0).*