# sermo

**sermo** ouvre, depuis un script shell, une boîte de dialogue décrite en **XML**,
puis écrit sur sa sortie les valeurs saisies (`NOM="Jean"`). Le même script
tourne sur sept bibliothèques graphiques : GTK 3, GTK 4, Qt 6, FLTK, EFL, SDL 3,
et ncurses dans un terminal.

![Le même formulaire rendu par les sept ports](doc/captures/formulaire-sept-ports.png)

*Le même script, [`examples/showcase/01-formulaire.sh`](examples/showcase/01-formulaire.sh),
rendu par les binaires des paquets 2.7.1-1. efl1 garde son thème sombre même
quand le thème clair est demandé ([TODO.md](TODO.md)). L'image se refait avec
[`doc/captures/refaire.sh`](doc/captures/refaire.sh).*

[English](README.en.md)

## D'où vient sermo

sermo est un fork de **gtkdialog 0.8.3**, écrit par Pere László puis repris par
Thunor, sous GPL-2.0-or-later. La lignée d'origine continue ici :
<https://github.com/puppylinux-woof-CE/gtkdialog>.

sermo garde le langage XML et l'analyseur de gtkdialog. Il remplace GTK 2 par
sept couches de rendu autour d'un cœur commun, et borne l'exécution des
commandes. Ce qui vient de l'amont et ce qui a été écrit ici est compté fichier
par fichier dans [LICENCES.md](LICENCES.md).

## État du projet

- Version **2.7.4**. sermo est tenu par **un seul mainteneur**, S. Cage. Il n'y a
  pas d'équipe derrière : une réponse peut prendre du temps.
- Une partie du code, de la documentation et des tests a été écrite avec l'aide
  d'une IA (Claude, d'Anthropic), sous la direction et la relecture du mainteneur
  ([AUTHORS](AUTHORS)).
- **Ce qui est vérifié** : 139 bancs (analyse XML, valeurs exportées, sécurité,
  clics, ouverture des exemples réels) sur les sept ports, rejoués aussi sur les
  binaires extraits des paquets Debian. Chaque chiffre se rejoue par une
  commande : [BILAN_SANTE.md](BILAN_SANTE.md).
- **Écarts connus** : les ports sans GTK s'écartent encore de gtk3sermo sur
  quelques points (`auto-refresh`, sélection initiale des listes, `<terminal>`…).
  Ils sont listés dans [TODO.md](TODO.md).
- **Pas de dépôt apt** : les paquets Debian sont joints à chaque publication
  (voir « Installer »).

## Exemple

```sh
export MAIN_DIALOG='
<window title="Bonjour">
  <vbox>
    <text><label>Votre nom ?</label></text>
    <entry><variable>NOM</variable></entry>
    <hbox><button ok></button><button cancel></button></hbox>
  </vbox>
</window>'
sermo --program=MAIN_DIALOG
```

Après avoir tapé « Jean » et cliqué sur OK, la sortie est la même sur les sept
ports :

```
NOM="Jean"
EXIT="OK"
```

Un script lit ces lignes avec `eval "$(sermo --program=MAIN_DIALOG)"` : les
valeurs sont échappées pour cela. Quand quelqu'un d'autre que l'auteur remplit le
dialogue, préférez `--do` ([SECURITY.md](SECURITY.md)).

## Les sept ports

| Port | Bibliothèque | Commande | Paquet |
|---|---|---|---|
| gtk3 | GTK 3 | `gtk3sermo` | `sermo-backend-gtk3` |
| gtk4 | GTK 4 | `gtk4sermo` | `sermo-backend-gtk4` |
| qt6 | Qt 6 | `qt6sermo` | `sermo-backend-qt6` |
| fltk1 | FLTK 1.4 | `fltk1sermo` | `sermo-backend-fltk1` |
| efl1 | EFL (Enlightenment) | `efl1sermo` | `sermo-backend-efl1` |
| sdl3 | SDL 3 et Dear ImGui | `sdl3sermo` | `sermo-backend-sdl3` |
| ncurses | ncurses, en terminal | `ncursessermo` | `sermo-backend-ncurses` |

- **gtk3sermo est l'étalon** : les six autres ports sont mesurés contre lui.
- Seuls gtk3 et gtk4 ont `<terminal>` (VTE) et `--glade-xml` ; seul gtk3 a
  l'ancrage Wayland (`layer-shell`).
- Chaque paquet ne tire que sa bibliothèque : `sermo-backend-fltk1` n'installe ni
  GTK ni Qt, `sermo-backend-ncurses` ne demande que ncurses.

Le cœur `libsermocore` (analyse du XML, variables, actions, exécution des
commandes) est lié dans chaque binaire ; le backend ne fait que dessiner. Le port
ncurses a été ajouté sans changer une ligne du cœur. Architecture :
[MANUEL_DEVELOPPEUR.md](MANUEL_DEVELOPPEUR.md).

## Installer

Il n'y a pas de dépôt apt. Les paquets Debian sont joints à la publication
[**v2.7.4**](https://gitlab.com/haplo-dialog/sermo/-/releases/v2.7.4), avec leurs
sommes de contrôle : on télécharge, on vérifie, on installe.

```sh
U=https://gitlab.com/api/v4/projects/85674825/packages/generic/sermo/2.7.4
for f in sermo-backend-gtk3_2.7.4-1_amd64.deb sermo-gtkdialog_2.7.4-1_all.deb SHA256SUMS; do
    curl -fLO "$U/$f"
done
sha256sum --ignore-missing -c SHA256SUMS
sudo apt install ./sermo-backend-gtk3_2.7.4-1_amd64.deb \
                 ./sermo-gtkdialog_2.7.4-1_all.deb
```

Pour un autre port, remplacez `gtk3` par `gtk4`, `qt6`, `fltk1`, `efl1`, `sdl3` ou
`ncurses`. Les paquets sont construits sur **Debian testing (amd64)** et demandent
ses versions de bibliothèques (Qt ≥ 6.10.2 pour qt6, glibc ≥ 2.43 pour sdl3…).
Ailleurs, compilez depuis les sources.

Les sommes de contrôle protègent d'un téléchargement abîmé, pas d'un fichier
remplacé sur le serveur : les paquets ne sont pas signés.

**Construire les paquets vous-même** (Debian testing, Docker) :
`bash packaging/construire-paquets.sh` construit dans un conteneur vierge, passe
lintian, installe et purge les paquets pour vérifier qu'ils ne laissent rien.
Sur une Debian testing, `sudo apt build-dep ./` puis `dpkg-buildpackage -us -uc -b`
suffit aussi. Détails :
[PACKAGING.md](PACKAGING.md).

**Compiler depuis les sources** : [COMPILE.md](COMPILE.md).

Après installation :

- la commande **`sermo`** désigne l'un des ports installés, choisi par
  `update-alternatives` (gtk3 en premier) ;
  `sudo update-alternatives --config sermo` en choisit un autre ;
- pour les anciens scripts qui appellent **`gtkdialog`**, installez en plus
  `sermo-gtkdialog`. Il remplace les paquets `gtkdialog` et `gtk3dialog` : on
  garde l'un ou l'autre, pas les deux.

## Désinstaller

```sh
sudo apt purge sermo-gtkdialog sermo-backend-gtk3
```

La purge ne laisse ni fichier ni alternative (vérifié dans un conteneur vierge).
Compilé depuis les sources sans `install` : supprimez le dossier de construction.

## Venir de la 1.x

Installez les paquets 2.x des ports que vous utilisez (`sermo-backend-gtk3`,
`sermo-gtkdialog`…) : apt retire les paquets 1.x qu'ils remplacent (`gtk3sermo`,
`gtksermo`…). Les commandes gardent leur nom. Ce qui change et comment faire :
[MIGRATION.md](MIGRATION.md).

## Documentation

- [Manuel utilisateur](MANUEL_UTILISATEUR.md) — écrire des dialogues : widgets,
  actions, variables, options.
- [Manuel développeur](MANUEL_DEVELOPPEUR.md) — le cœur, les backends, ajouter un
  port.
- `man sermo` et `info sermo`, installés avec chaque port.
- [COMPILE.md](COMPILE.md) · [PACKAGING.md](PACKAGING.md) ·
  [DEPENDENCIES.md](DEPENDENCIES.md) · [VERSIONING.md](VERSIONING.md)
- [SECURITY.md](SECURITY.md) · [CONTRIBUTING.md](CONTRIBUTING.md) ·
  [CHANGELOG.md](CHANGELOG.md) · [BILAN_SANTE.md](BILAN_SANTE.md) ·
  [TODO.md](TODO.md) · [ROADMAP.md](ROADMAP.md)
- [`sermoman-mcp`](sermoman-mcp/README.md) — un serveur MCP en lecture seule qui
  donne à un assistant IA le manuel, l'architecture et les exemples de sermo.

## Signaler un problème

- **Un défaut** : ouvrez un ticket sur
  <https://gitlab.com/haplo-dialog/sermo/-/issues>, avec un script XML minimal
  qui le reproduit, la commande lancée (`gtk3sermo`, `qt6sermo`…) et sa sortie
  `--version`. Sans compte GitLab, le même rapport est bienvenu à
  `devel@haplo-dialog.fr`.
- **Une faille de sécurité** : pas de ticket public, voir [SECURITY.md](SECURITY.md).

## Licence

GPL-2.0-or-later, comme gtkdialog. Quelques fichiers ont une autre licence (le
contrat de frontière en MIT, Dear ImGui en MIT, des exemples en CC0) : voir
[LICENCES.md](LICENCES.md).
