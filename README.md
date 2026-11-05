# sermo

**[English](README.en.md)** · Français

[![Pipeline GitLab](https://gitlab.com/haplo-dialog/sermo/badges/main/pipeline.svg)](https://gitlab.com/haplo-dialog/sermo/-/pipelines) [![Dernière version](https://img.shields.io/gitlab/v/release/haplo-dialog%2Fsermo?label=version)](https://gitlab.com/haplo-dialog/sermo/-/releases)

**sermo** ouvre, depuis un script shell, une boîte de dialogue décrite en **XML**,
puis écrit sur sa sortie les valeurs saisies (`NOM="Jean"`). Le même script
tourne sur huit bibliothèques graphiques : GTK 3, GTK 4, Qt 6, FLTK, EFL, SDL 3,
LVGL, et ncurses dans un terminal. Un neuvième port, html, n'ouvre rien : il
écrit le dialogue en page HTML.

![Le même formulaire rendu par les neuf ports](doc/captures/formulaire-neuf-ports.png)

*Le même script, [`examples/showcase/01-formulaire.sh`](examples/showcase/01-formulaire.sh),
rendu par les binaires des paquets 2.7.1-1. efl1 garde son thème sombre même
quand le thème clair est demandé ([TODO.md](TODO.md)). L'image se refait avec
[`doc/captures/refaire.sh`](doc/captures/refaire.sh).*

## D'où vient sermo

sermo est un fork de **gtkdialog 0.8.3**, écrit par Pere László (2003-2007) puis
repris par Thunor (2011-2013), sous GPL-2.0-or-later. La lignée d'origine continue
chez Puppy Linux : <https://github.com/puppylinux-woof-CE/gtkdialog>. Qui a fait
quoi, quand, et où en sont les autres descendants (gtk3dialog, gtkdialog3) :
[HISTOIRE.md](HISTOIRE.md).

sermo garde le langage XML et l'analyseur de gtkdialog. Il remplace GTK 2 par
neuf couches de rendu autour d'un cœur commun, et borne l'exécution des
commandes. Ce qui vient de l'amont et ce qui a été écrit ici est compté fichier
par fichier dans [LICENCES.md](LICENCES.md).

## État du projet

- Version **3.2.0**. sermo est tenu par **un seul mainteneur**, S. Cage. Il n'y a
  pas d'équipe derrière : une réponse peut prendre du temps.
- Une partie du code, de la documentation et des tests a été écrite avec l'aide
  d'une IA (Claude, d'Anthropic), sous la direction du mainteneur. Il l'essaie et
  en signale les défauts, mais ne relit plus tout le code ligne à ligne : la
  vérification repose surtout sur les bancs, que chacun peut rejouer
  ([AUTHORS](AUTHORS)).
- **Ce qui est vérifié** : 178 bancs (analyse XML, valeurs exportées, sécurité,
  clics, ouverture des exemples réels, serveurs MCP) sur les neuf ports au
  passage complet du 2026-10-11, détaillé dans [BILAN_SANTE.md](BILAN_SANTE.md).
  Au passage complet (`ci/bancs.sh`, programmes d'aide compris) :
  265 bancs, 0 échec (branche `master`, 2026-10-31, pilotage de drone compris). Au passage du 2026-10-18, un banc était rouge (des fonctions
  interdites dans le code du lecteur son et vidéo) ; il a été corrigé puis
  rejoué vert. Chaque chiffre se rejoue par une commande.
- **Ce que sermo lit et écrit dehors**, par des programmes d'aide installés à
  part : un automate en Modbus ou OPC UA (lecture seule) ; la domotique MQTT
  (lire, et publier si une liste blanche le permet) ; un drone MAVLink (lire sa
  télémétrie ; régler un paramètre du drone désarmé, lui envoyer une mission
  sans la lancer, ou donner un ordre de sécurité : retour au départ,
  atterrissage, coupure des moteurs en deux appuis) ; le son et la vidéo, joués
  par mpv. sermo ne pilote pas un drone : il ne l'arme pas, ne le fait pas
  décoller et ne le dirige pas, hors retour au départ et atterrissage de
  sécurité. Les paquets sont dans « Installer ».
- **Écarts connus** : les ports sans GTK s'écartent encore de gtk3sermo sur
  quelques points (`auto-refresh`, libellé du bouton Annuler, `<terminal>`…).
  Ils sont listés dans [TODO.md](TODO.md).
- **Pas de dépôt apt** : les paquets Debian sont joints à chaque publication
  (voir « Installer »).

## Fait avec une IA : comment chaque changement est vérifié

Une partie de sermo est écrite par une IA, sous la direction du
mainteneur (voir « État du projet »). La confiance ne repose donc pas sur une
relecture ligne à ligne, mais sur des bancs que chacun peut rejouer :

- **Deux commandes, les mêmes partout** : `bash ci/construire.sh` construit le
  cœur et les neuf ports, `bash ci/bancs.sh` joue tous les bancs. La CI lance ces
  deux scripts et rien d'autre ; chez soi, on obtient le même résultat. Dernier
  passage complet : 265 bancs, 0 échec (2026-10-31).
- **Un étalon** : gtk3sermo. Le banc de comportement ouvre chaque cas sur les
  neuf ports et compare les valeurs rendues à celles de gtk3sermo. Quand un port
  s'écarte, on répare le port ; on ne corrige jamais la valeur attendue.
- **Sans écran** : gtk3, gtk4 et fltk1 tournent sous Xvfb, qt6, sdl3 et efl1 sur
  leur plateforme hors écran, ncurses en mode batch ; html écrit sa page. Même
  sur un poste de travail, aucune fenêtre ne s'ouvre : la mesure est celle de la CI.
- **Un banc qui n'a rien vérifié compte comme un échec** (outil absent, binaire
  introuvable, banc figé) : pas de vert par défaut.
- **Deux forges, deux CI jumelles** : `.gitlab-ci.yml` sur GitLab et
  `.github/workflows/bancs.yml` sur GitHub jouent les mêmes deux commandes, pour
  qu'une contribution n'entre sans contrôle par aucune des deux. GitHub construit
  aussi le port ncurses sur macOS (`macos.yml`).
- **En option** : `SERMO_BANC_SANITIZERS=1` rejoue le banc de comportement sous
  ASan et UBSan, erreurs fatales.

Les affirmations de sécurité qu'aucun banc ne garde (vérifiées à la main ou
lues dans le code) sont listées dans
[SECURITY.md](SECURITY.md#ce-quaucun-banc-automatique-ne-vérifie) ; les écarts
connus entre ports, dans [TODO.md](TODO.md).

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
sermo --program=MAIN_DIALOG --do='
    if [ "$EXIT" = "OK" ]; then
        echo "Bonjour $NOM"
    fi'
```

Après avoir tapé « Jean » et cliqué sur OK, la sortie est la même sur chaque
port qui affiche le dialogue :

```
NOM="Jean"
EXIT="OK"
Bonjour Jean
```

Avec `--do`, les valeurs arrivent dans la commande comme variables
d'environnement : ce que l'utilisateur a tapé n'est **jamais relu comme du
code** (un `$(id)` tapé dans le champ reste du texte). C'est la voie
recommandée ([SECURITY.md](SECURITY.md)).

**L'héritage de gtkdialog : `eval`.** Les deux premières lignes, sermo les écrit
toujours, comme gtkdialog. Un ancien script les lit avec
`eval "$(sermo --program=MAIN_DIALOG)"` et marche tel quel. sermo échappe ce que
le shell développerait entre guillemets (`$`, accent grave, `\`, `"`), mais
`eval` exécute toute la sortie comme du shell : une erreur de guillemets dans le
script suffit pour que la saisie devienne une commande. À garder pour reprendre
l'existant, pas pour un nouveau script.

## Sécurité en bref

- Un fichier de dialogue **est un programme** : ses `<action>` lancent des
  commandes. N'ouvrez pas un XML venu d'une source dont vous ne lanceriez pas un
  script shell.
- Avec `--do`, ce que l'utilisateur tape n'est jamais exécuté ; avec `eval`, voir
  la limite ci-dessus.
- Pour une borne, une session invité ou un dialogue écrit par une IA, deux
  variables bornent l'exécution : `SERMO_NO_SHELL_FALLBACK` (pas de repli sur
  `/bin/sh -c`) et `SERMO_ALLOWED_CMDS` (seules les commandes listées). Elles sont
  éteintes par défaut pour que les scripts gtkdialog marchent tels quels.
- Modèle de menace, durcissement et limites connues : [SECURITY.md](SECURITY.md).

## Les neuf ports

**Quel port choisir.** Sans raison particulière : **gtk3sermo** sur un bureau Linux,
**ncursessermo** par SSH, **`ncursessermo.exe`** sous Windows. Le même fichier XML
s'ouvre sur tous les ports ; changer de port, c'est changer de commande (quelques
balises n'existent que sur certains ports :
[tableau des balises par port](MANUEL_UTILISATEUR.md#59-balises-par-port)).

| Votre cas | Port | Pourquoi |
|---|---|---|
| Un bureau Linux, sans préférence | **gtk3sermo** (par défaut) | l'étalon : les autres ports sont mesurés contre lui ; le seul qui a tout (`<terminal>`, `--glade-xml`, ancrage Wayland) |
| Un bureau en GTK 4 (GNOME récent) | gtk4sermo | a aussi `<terminal>` et `--glade-xml` |
| Un bureau KDE, ou des applications Qt | qt6sermo | demande Qt ≥ 6.11.2 |
| Le moins de bibliothèques possible | fltk1sermo | ne tire que FLTK, ni GTK ni Qt |
| Le bureau Enlightenment | efl1sermo | widgets Elementary |
| Une fenêtre sans toolkit de bureau | sdl3sermo | SDL 3, dessin Dear ImGui |
| Une borne, un écran embarqué, une console sans bureau | lvglsermo | LVGL est dans le paquet ; fenêtre Wayland ou X11, ou framebuffer. Version légère (`sermo-backend-lvgl-leger`) : un tiers plus petite, sans icônes SVG |
| Par SSH, dans un terminal, sur un serveur sans écran | **ncursessermo** | ne demande que ncurses |
| Windows 10 ou 11 | **`ncursessermo.exe`** | dans le zip de la publication, rien à installer ; seul binaire publié hors Linux |
| Android, BSD, macOS | ncursessermo | à construire depuis les recettes (« Autres systèmes ») |
| Pas de fenêtre : une page dans un navigateur | htmlsermo | écrit une page HTML autonome ; avec `SERMO_HTML_MODE=serve`, renvoie les valeurs saisies |

| Port | Bibliothèque | Commande | Paquet |
|---|---|---|---|
| gtk3 | GTK 3 | `gtk3sermo` | `sermo-backend-gtk3` |
| gtk4 | GTK 4 | `gtk4sermo` | `sermo-backend-gtk4` |
| qt6 | Qt 6 | `qt6sermo` | `sermo-backend-qt6` |
| fltk1 | FLTK 1.4 | `fltk1sermo` | `sermo-backend-fltk1` |
| efl1 | EFL (Enlightenment) | `efl1sermo` | `sermo-backend-efl1` |
| sdl3 | SDL 3 et Dear ImGui | `sdl3sermo` | `sermo-backend-sdl3` |
| lvgl | LVGL 9.6, embarqué (Wayland, X11 ou framebuffer) | `lvglsermo` | `sermo-backend-lvgl` |
| ncurses | ncurses, en terminal | `ncursessermo` | `sermo-backend-ncurses` |
| html | aucune : page HTML autonome | `htmlsermo` | `sermo-backend-html` |

- **gtk3sermo est l'étalon** : les huit autres ports sont mesurés contre lui.
- **html est un exportateur** : il n'ouvre pas de fenêtre, il écrit le dialogue
  en une page HTML autonome (CSS embarqué, sans JavaScript par défaut) dans le fichier que nomme
  `SERMO_HTML_OUT` (défaut `sermo-output.html`). Avec `SERMO_HTML_MODE=serve`,
  un micro-serveur local sert la page et renvoie les valeurs saisies.
- Seuls gtk3 et gtk4 ont `<terminal>` (VTE) et `--glade-xml` ; seul gtk3 a
  l'ancrage Wayland (`layer-shell`).
- Chaque paquet ne tire que sa bibliothèque : `sermo-backend-fltk1` n'installe ni
  GTK ni Qt, `sermo-backend-ncurses` ne demande que ncurses.

Le cœur `libsermocore` (analyse du XML, variables, actions, exécution des
commandes) est lié dans chaque binaire ; le backend ne fait que dessiner. Le port
ncurses a été ajouté sans changer une ligne du cœur. Architecture :
[MANUEL_DEVELOPPEUR.md](MANUEL_DEVELOPPEUR.md).

## Autres systèmes

Hors de Linux, un seul port tourne : **ncurses**, dans un terminal.

| Système | Comment l'obtenir | Vérifié |
|---|---|---|
| Windows 10 et 11 | `sermo-<version>-windows-x86_64.zip`, joint à la publication : un seul `ncursessermo.exe`, rien à installer | banc de comportement sous Wine ; affichage et accents sur un vrai Windows 11, commandes et accents sur un vrai Windows 10 |
| Android (Termux) | recette [`packaging/autres-os/termux/build.sh`](packaging/autres-os/termux/build.sh), à construire dans Termux | sur un téléphone (Android 15, arm64) |
| FreeBSD, OpenBSD | ports [`packaging/autres-os/freebsd/`](packaging/autres-os/freebsd/) et [`openbsd/`](packaging/autres-os/openbsd/) | dans des machines virtuelles (FreeBSD 15.1, OpenBSD 7.9) |
| macOS | formule Homebrew [`packaging/autres-os/homebrew/sermo.rb`](packaging/autres-os/homebrew/sermo.rb) | sur un runner macOS (Apple Silicon) |

« Vérifié » veut dire : le port y rend les mêmes valeurs que gtk3sermo sur le banc
de comportement et sur les dialogues des scripts métier. Seul Windows a un
binaire publié ; pour Android, les BSD et macOS, il n'y a que les recettes.

- Construire : [COMPILE.md](COMPILE.md) §7.
- Ce qui change pour la sécurité (pas de plafond des `<input>` sous Windows,
  durcissement selon le processeur) : [SECURITY.md](SECURITY.md).
- Le détail de chaque essai : [packaging/autres-os/README.md](packaging/autres-os/README.md).

## Installer

Il n'y a pas de dépôt apt. Les paquets Debian sont joints à la publication
[**v3.2.0**](https://gitlab.com/haplo-dialog/sermo/-/releases/v3.2.0), avec leurs
sommes de contrôle : on télécharge, on vérifie, on installe.

```sh
U=https://gitlab.com/api/v4/projects/85674825/packages/generic/sermo/3.2.0
for f in sermo-backend-gtk3_3.2.0-1_amd64.deb sermo-gtkdialog_3.2.0-1_all.deb SHA256SUMS SHA256SUMS.asc; do
    curl -fLO "$U/$f"
done
curl -fLO https://haplo-dialog.fr/cle-haplo-dialog.asc
gpg --import cle-haplo-dialog.asc
gpg --verify SHA256SUMS.asc SHA256SUMS
sha256sum --ignore-missing -c SHA256SUMS
sudo apt install ./sermo-backend-gtk3_3.2.0-1_amd64.deb \
                 ./sermo-gtkdialog_3.2.0-1_all.deb
```

Pour un autre port, remplacez `gtk3` par `gtk4`, `qt6`, `fltk1`, `efl1`, `sdl3`,
`lvgl`, `ncurses` ou `html`. Les paquets sont construits sur **Debian testing (amd64)** et demandent
ses versions de bibliothèques (Qt ≥ 6.11.2 pour qt6, glibc ≥ 2.43 pour sdl3…).
Ailleurs, compilez depuis les sources.

Les sommes sont signées par la clé de haplo-dialog. `gpg --verify` doit dire
« Bonne signature de haplo-dialog <devel@haplo-dialog.fr> », avec l'empreinte :

```
66C7 F4AD 7A58 A037 967F  8339 3799 9F99 7570 F1ED
```

La clé publique est sur le site (`https://haplo-dialog.fr/cle-haplo-dialog.asc`) ;
l'empreinte, ici, dans le dépôt. Comparez-la à celle de ce README sur GitLab
(<https://gitlab.com/haplo-dialog/sermo>) **et** sur GitHub
(<https://github.com/haplo-dialog/sermo>) : une clé remplacée sur le site ne le
serait pas partout à la fois.

Ce qui est signé, c'est la liste des sommes (`SHA256SUMS`), pas chaque `.deb`
un par un : un paquet est sûr parce que sa somme figure dans une liste dont la
signature est bonne. Il n'y a pas non plus de dépôt apt signé.

**Lire et écrire dehors** : chaque programme d'aide a son paquet, à installer
en plus du port (même téléchargement, même vérification) :

| Pour | Paquets |
|---|---|
| lire un automate | `sermo-source-modbus`, `sermo-source-opcua` |
| la domotique MQTT | `sermo-source-mqtt` (lire), `sermo-cible-mqtt` (publier) |
| un drone MAVLink | `sermo-source-mavlink` (lire), `sermo-cible-mavlink` (régler, mission, ordres de sécurité) |
| le son et la vidéo | `sermo-cible-media` (jouer ; tire `mpv`, recommande `bubblewrap`), `sermo-source-media` (suivre l'état) |

Qui n'installe que la lecture ne peut rien écrire. Les listes blanches et les
gardes sont dans [SECURITY.md](SECURITY.md) et le
[manuel utilisateur](MANUEL_UTILISATEUR.md).

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

Installez les paquets actuels (3.1.0) des ports que vous utilisez (`sermo-backend-gtk3`,
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
- [`sermoia-mcp`](sermoia-mcp/README.md) — un serveur MCP qui fait écrire,
  vérifier et voir un dialogue à un assistant IA ; l'aperçu n'exécute aucune
  commande.
- [`sermo.schema.json`](sermo.schema.json) — le langage XML lu dans le code
  (balises, conteneurs, contenu, attributs lus par chaque port, actions,
  signaux), généré par `outils/schema/generer.py` et tenu à jour par un banc ;
  pour les éditeurs et les assistants IA. Ce n'est pas un XSD : le dialecte
  hérité de gtkdialog (`<button ok>`) n'est pas du XML strict.

## Signaler un problème

- **Un défaut** : ouvrez un ticket sur
  <https://gitlab.com/haplo-dialog/sermo/-/issues>, avec un script XML minimal
  qui le reproduit, la commande lancée (`gtk3sermo`, `qt6sermo`…) et sa sortie
  `--version`. Sans compte GitLab, le même rapport est bienvenu à
  `devel@haplo-dialog.fr`.
- **Une faille de sécurité** : pas de ticket public, voir [SECURITY.md](SECURITY.md).

## Licence

GPL-2.0-or-later, comme gtkdialog. Quelques fichiers ont une autre licence (le
contrat de frontière en MIT, Dear ImGui, LVGL et ThorVG en MIT, les en-têtes
MAVLink en MIT une fois compilés, des exemples en CC0) : voir
[LICENCES.md](LICENCES.md).
