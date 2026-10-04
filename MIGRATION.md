# Passer de sermo 1.x à 2.x

[English](MIGRATION.en.md)

La 1.x livrait trois ports, chacun dans son dossier, avec sa version et son
paquet : gtk3sermo et gtk4sermo 1.1.4, qt6sermo 1.0.2, et `gtksermo` pour la
commande `gtkdialog`. La 2.x les réunit autour d'un cœur commun, ajoute quatre
ports et prend un seul numéro de version.

**Vos scripts XML restent valides** et **les commandes gardent leur nom**. Ce qui
change surtout : le nom des paquets, le nom de deux variables d'environnement, et
la façon de compiler. Les quelques comportements qui changent sont listés plus
bas.

## En bref

| | 1.x | 2.x |
|---|---|---|
| Paquets | `gtk3sermo`, `gtk4sermo`, `qt6sermo`, `gtksermo` | `sermo-backend-gtk3`, `-gtk4`, `-qt6`, `-fltk1`, `-efl1`, `-sdl3`, `-ncurses`, `sermo-gtkdialog`, `sermo-core-dev` |
| Commandes | `gtk3sermo`, `gtk4sermo`, `qt6sermo`, `gtkdialog` | les mêmes, plus `fltk1sermo`, `efl1sermo`, `sdl3sermo`, `ncursessermo` et `sermo` |
| Variables de sécurité | `HAPLO_ALLOWED_CMDS`, `HAPLO_NO_SHELL_FALLBACK` | `SERMO_ALLOWED_CMDS`, `SERMO_NO_SHELL_FALLBACK` (les anciens noms sont encore lus) |
| Sources | un dossier par port (`gtk3sermo/gtk3sermo_1.1.4/`…) | `libsermocore/` et `sermo-backend-<port>/`, construits par CMake |
| Recettes Arch, Gentoo, RPM, Slackware | fournies | retirées : seul `debian/` est maintenu |
| Version | une par port | une pour tout (2.7.7) |

## Mettre à jour les paquets Debian

### Depuis des fichiers `.deb` (comme la 1.x s'installait)

Téléchargez et vérifiez les paquets 2.x joints à la publication, comme l'explique
le [README](README.md#installer), puis installez ceux des ports que vous utilisez.
apt retire lui-même les paquets 1.x qu'ils remplacent :

```sh
sudo apt install ./sermo-backend-gtk3_2.7.7-1_amd64.deb ./sermo-gtkdialog_2.7.7-1_all.deb
```

Ajoutez `./sermo-backend-gtk4_2.7.7-1_amd64.deb` ou
`./sermo-backend-qt6_2.7.7-1_amd64.deb` si vous aviez `gtk4sermo` ou `qt6sermo`.

Éprouvé le 2026-09-17 dans un conteneur Debian testing vierge, sans réseau :
`gtk3sermo`, `gtk4sermo`, `gtksermo` 1.1.4-2 et `qt6sermo` 1.0.2-2 installés, puis
la commande ci-dessus avec les trois backends. apt a retiré les quatre paquets 1.x,
installé les quatre paquets 2.x ; `gtk3sermo`, `gtk4sermo` et `qt6sermo` répondent
en 2.7.1, `gtkdialog` et `sermo` mènent à gtk3sermo, et `apt autoremove`
n'emporterait rien de sermo.

### Depuis un dépôt apt

Si les paquets 2.x vous arrivent par un dépôt, `sudo apt upgrade` suffit : les
paquets `gtk3sermo`, `gtk4sermo`, `qt6sermo` et `gtksermo` passent en 2.x, vides,
et font venir `sermo-backend-gtk3`, `-gtk4`, `-qt6` et `sermo-gtkdialog`.

Ces paquets de transition peuvent ensuite partir. **D'abord**, marquez comme voulus
les paquets 2.x que vous utilisez : venus comme dépendances, ils seraient sinon
emportés par `apt autoremove`.

```sh
sudo apt install sermo-backend-gtk3 sermo-gtkdialog      # ceux que vous utilisez
sudo apt purge gtk3sermo gtk4sermo qt6sermo gtksermo
```

Éprouvé par `packaging/construire-paquets.sh` (paquets 2.x servis par un dépôt
local) : voir [PACKAGING.md](PACKAGING.md).

## Commandes

- `gtk3sermo`, `gtk4sermo`, `qt6sermo` et `gtkdialog` existent toujours, au même
  endroit (`/usr/bin`).
- **Nouveau** : la commande `sermo` désigne l'un des ports installés, choisi par
  `update-alternatives` (gtk3 en premier). `sudo update-alternatives --config sermo`
  en choisit un autre. Un script qui appelle `gtk3sermo` n'a pas besoin de changer.
- Les options de gtk3sermo et gtk4sermo sont les mêmes qu'en 1.1.4, plus
  `--render-png`.

## Variables d'environnement

| 1.x | 2.x |
|---|---|
| `HAPLO_ALLOWED_CMDS` | `SERMO_ALLOWED_CMDS` |
| `HAPLO_NO_SHELL_FALLBACK` | `SERMO_NO_SHELL_FALLBACK` |
| `GTKDIALOG_PIXMAP_PATH` | inchangée |

Les anciens noms sont **encore lus** quand le nouveau n'est pas posé, avec un
avertissement sur la sortie d'erreur : un déploiement durci en 1.x reste durci.
Renommez-les quand même. `tests/garde_allowed_cmds.sh` et les tests unitaires le
vérifient.

⚠️ **`SERMO_ALLOWED_CMDS` compare désormais les chemins.** Une commande écrite avec
un chemin n'est acceptée que si ce chemin est listé, ou si c'est celui où le
`PATH` trouve un nom listé. Sur un système où le `PATH` trouve `/usr/bin/echo`,
lister `echo` n'autorise plus `/bin/echo` : listez `/bin/echo`, ou écrivez `echo`
dans le script. Détails : [SECURITY.md](SECURITY.md).

## Comportements qui changent

Mesurés sur gtk3sermo et gtk4sermo, 1.1.4 puis 2.7.1 :

- **`--include` avec un chemin relatif**, ou un nom qui contient une espace ou une
  apostrophe : la 1.1.4 ne chargeait rien, la 2.7.1 charge le fichier.
- **`--glade-xml`** : la 2.7.1 exporte les valeurs des widgets du fichier, ce que
  la 1.1.4 ne faisait pas, et un fichier absent ou une fenêtre inconnue donnent une
  erreur propre (code 1) au lieu d'un avortement (code 134). Les cinq autres ports
  refusent l'option.

Mesurés sur qt6sermo 1.0.2 et 2.7.1 (cas de banc 43 et 44), la 2.7.1 suivant
gtk3sermo :

- **`<input file>`** remplit `<entry>`, `<text>` et `<comboboxtext>` ; la 1.0.2 ne
  lisait pas le fichier.
- **`<text>` rempli par une commande** garde le saut de ligne final.
- **`<combobox>` ne se remplit plus par `<input>`**, comme sur gtk3, qui ne l'a
  jamais fait : utilisez `<comboboxtext>`.

Les écarts qui restent entre les ports sont listés dans [TODO.md](TODO.md).

## Compiler depuis les sources

La 1.x se construisait dans le dossier de chaque port. La 2.x se construit en deux
temps : le cœur `libsermocore`, dans la variante qui convient au port, puis le port
contre ce cœur. `bash ci/construire.sh` fait les deux pour les sept ports. Voir
[COMPILE.md](COMPILE.md).

Deux dépendances sont désormais **exigées** : VTE pour gtk3 et gtk4
(`libvte-2.91-dev`, `libvte-2.91-gtk4-dev`), et gtk-layer-shell pour gtk3
(`libgtk-layer-shell-dev`). Sans elles, la construction s'arrête au lieu de
produire un binaire privé de `<terminal>` ou de l'ancrage Wayland. Pour s'en
passer : `-DSERMOCORE_VTE=OFF`, `-DSERMO_LAYER_SHELL=OFF`.

Les recettes Arch, Gentoo, RPM et Slackware de la 1.x ne sont plus fournies. Qui
veut empaqueter pour une autre distribution peut partir de `debian/rules`, qui
montre l'ordre de construction.
