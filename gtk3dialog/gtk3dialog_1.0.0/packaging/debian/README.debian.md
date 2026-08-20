# Packaging Debian — haplo-dialog

**Toolkit :** GTK3
**Paquet source :** `haplo-dialog` · **Paquet binaire :** `haplo-dialog`

Le paquet s'appelle `haplo-dialog`, la commande reste `gtk3dialog`. Le nom de
paquet `gtk3dialog` n'est pas libre : le fork BunsenLabs en publie un sous ce
nom exact.
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

Les fichiers installés sont posés par `debian/rules` et `debian/haplo-dialog.links` (binaire `/usr/bin/gtk3dialog`,
exemples et documentation).

## Fichiers debian/

| Fichier | Rôle |
|---------|------|
| `control` | Métadonnées, dépendances build et runtime |
| `changelog` | Historique des versions (format Debian) |
| `rules` | Recette de build (debhelper) |
| `copyright` | Licence GPL-2.0-or-later + héritage gtkdialog |
| `haplo-dialog.links` | Alias `gtkdialog` et sa page de manuel |
| `haplo-dialog.lintian-overrides` | Exceptions lintian assumées |
| `compat` / `source/` | Niveau debhelper et format source |

## Désinstallation

```bash
sudo apt-get remove haplo-dialog
```

---

