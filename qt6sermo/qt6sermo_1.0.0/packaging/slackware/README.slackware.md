# Packaging Slackware — qt6sermo

## Construction du paquet

```bash
# Placer l'archive source
cp qt6sermo-1.0.0.tar.gz /tmp/

# Lancer le SlackBuild
cd packaging/slackware/
sudo sh qt6sermo.SlackBuild

# Résultat
ls /tmp/qt6sermo-1.0.0-x86_64-1_haplo.tgz
```

## Installation

```bash
sudo installpkg /tmp/qt6sermo-1.0.0-x86_64-1_haplo.tgz
# ou
sudo upgradepkg --install-new /tmp/qt6sermo-1.0.0-x86_64-1_haplo.tgz
```

## Dépendances (à installer manuellement)

Slackware ne gère pas les dépendances automatiquement.
Vérifier que les bibliothèques suivantes sont présentes :
- qt6-qtbase >= 6.2

---

> **TODO :** installer aussi `data/qt6sermo.desktop` et `data/qt6sermo.metainfo.xml` (fournis depuis la 1.0.0 ; déjà intégrés au build amont et au paquet Debian).

*Document horodaté — dernière mise à jour : 2026-06-10 (audit #6 — haplo-dialog v1.0.0).*