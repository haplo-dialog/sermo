# Packaging Slackware — gtk3sermo

## Construction du paquet

```bash
# Placer l'archive source
cp gtk3sermo-1.1.1.tar.gz /tmp/

# Lancer le SlackBuild
cd packaging/slackware/
sudo sh gtk3sermo.SlackBuild

# Résultat
ls /tmp/gtk3sermo-1.1.1-x86_64-1_sermo.tgz
```

## Installation

```bash
sudo installpkg /tmp/gtk3sermo-1.1.1-x86_64-1_sermo.tgz
# ou
sudo upgradepkg --install-new /tmp/gtk3sermo-1.1.1-x86_64-1_sermo.tgz
```

## Dépendances (à installer manuellement)

Slackware ne gère pas les dépendances automatiquement.
Vérifier que les bibliothèques suivantes sont présentes :
- gtk3
- vte291

---

