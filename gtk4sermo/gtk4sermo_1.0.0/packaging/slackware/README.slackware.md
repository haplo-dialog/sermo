<div align="right"><sub><code>maj :     2026-08-20 20:34:10 · par claude</code></sub></div>
<div align="right"><sub><code>réindex : —</code></sub></div>

# Packaging Slackware — gtk4sermo

## Construction du paquet

```bash
# Placer l'archive source
cp gtk4sermo-1.0.0.tar.gz /tmp/

# Lancer le SlackBuild
cd packaging/slackware/
sudo sh gtk4sermo.SlackBuild

# Résultat
ls /tmp/gtk4sermo-1.0.0-x86_64-1_haplo.tgz
```

## Installation

```bash
sudo installpkg /tmp/gtk4sermo-1.0.0-x86_64-1_haplo.tgz
# ou
sudo upgradepkg --install-new /tmp/gtk4sermo-1.0.0-x86_64-1_haplo.tgz
```

## Dépendances (à installer manuellement)

Slackware ne gère pas les dépendances automatiquement.
Vérifier que les bibliothèques suivantes sont présentes :
- gtk4
- vte291

---

*Document horodaté — dernière mise à jour : 2026-06-07 (audit #3 — haplo-dialog v1.0.0).*
