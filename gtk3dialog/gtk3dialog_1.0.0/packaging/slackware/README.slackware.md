# Packaging Slackware — gtk3dialog

## Construction du paquet

```bash
# Placer l'archive source
cp gtk3dialog-1.0.0.tar.gz /tmp/

# Lancer le SlackBuild
cd packaging/slackware/
sudo sh gtk3dialog.SlackBuild

# Résultat
ls /tmp/gtk3dialog-1.0.0-x86_64-1_haplo.tgz
```

## Installation

```bash
sudo installpkg /tmp/gtk3dialog-1.0.0-x86_64-1_haplo.tgz
# ou
sudo upgradepkg --install-new /tmp/gtk3dialog-1.0.0-x86_64-1_haplo.tgz
```

## Dépendances (à installer manuellement)

Slackware ne gère pas les dépendances automatiquement.
Vérifier que les bibliothèques suivantes sont présentes :
- gtk3
- vte291

---

