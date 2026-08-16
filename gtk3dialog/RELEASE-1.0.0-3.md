## gtk3dialog 1.0.0-3

Successeur moderne de **gtkdialog** : des interfaces graphiques natives décrites en XML,
lancées depuis un script shell. GTK 3, 43 widgets.

### Ce que change cette version

**Rien de fonctionnel.** Le binaire est identique à celui de la 1.0.0-2. Seules les
métadonnées du paquet changent :

- identité du mainteneur du paquet ;
- `Vcs-Git` et `Vcs-Browser` pointent désormais sur ce dépôt.

### Vérifications

| | |
|---|---|
| `lintian` | aucun signalement |
| Intégration continue | verte — 52 tests XML + tests unitaires, sur Debian testing |
| Durcissement | PIE, RELRO, stack-protector, FORTIFY vérifiés |

### Installation

```sh
sudo apt install ./gtk3dialog_1.0.0-3_amd64.deb
```

Sur une autre distribution, on construit depuis les sources — les recettes sont fournies
(autotools, ainsi que des variantes rpm, Arch et Slackware).

### Vérifier le téléchargement

```sh
sha256sum -c SHA256SUMS
```

### Licence

GPL-2.0-or-later. Fondé sur gtkdialog de László Pere (2003-2007) et Thunor (2011-2014).
