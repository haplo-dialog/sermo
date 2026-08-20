# Packaging Debian — gtk3dialog

**Toolkit :** GTK3
**Paquet source :** `gtk3dialog` · **Paquet binaire :** `gtk3dialog`
**Version :** 1.0.0
**Cibles :** Debian, Ubuntu, Linux Mint

## Dépendances de construction

```bash
sudo apt-get install build-essential debhelper devscripts \
  libgtk-3-dev (>= 3.22.0), libvte-2.91-dev, flex, bison, autoconf, automake, pkg-config
```

## Construction du paquet

```bash
cd gtk3dialog_1.0.0/
dpkg-buildpackage -b -us -uc
# Résultat : ../haplo-dialog_1.0.0-8_amd64.deb
```

Pour un build signé et complet (source + binaire) :

```bash
dpkg-buildpackage -us -uc
```

## Installation

```bash
sudo dpkg -i ../haplo-dialog_1.0.0-8_amd64.deb
sudo apt-get -f install   # résout les dépendances runtime si besoin
```

Dépendances runtime tirées automatiquement : `libgtk-3-0, libvte-2.91-0`.

## Contenu du paquet

Les fichiers installés sont listés dans `debian/gtk3dialog.install` (binaire `/usr/bin/gtk3dialog`,
exemples et documentation).

## Fichiers debian/

| Fichier | Rôle |
|---------|------|
| `control` | Métadonnées, dépendances build et runtime |
| `changelog` | Historique des versions (format Debian) |
| `rules` | Recette de build (debhelper) |
| `copyright` | Licence GPL-2.0-or-later + héritage gtkdialog |
| `gtk3dialog.install` | Liste des fichiers installés |
| `compat` / `source/` | Niveau debhelper et format source |

## Désinstallation

```bash
sudo apt-get remove gtk3dialog
```

---

