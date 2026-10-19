# Sécurité — sermo

[English](SECURITY.en.md)

sermo exécute des commandes shell décrites dans un script XML. Sa sécurité tient
à **où** cette exécution se fait et **comment** elle est bornée. L'essentiel se
fait dans le **cœur** `libsermocore`, commun aux neuf backends : `safe_exec.c`
n'existe qu'en un exemplaire (`libsermocore/src/safe_exec.c`), compilé dans les
trois variantes du cœur.

Deux chemins échappent au cœur, et c'est là que sont les limites : le
`<terminal>` de gtk3 et gtk4, et le `<linkbutton>` — voir
[Ce que ces deux variables n'atteignent pas](#ce-que-ces-deux-variables-natteignent-pas).

## Modèle de menace

La **frontière de confiance est l'auteur local du script XML**. sermo n'est pas
un bac à sable qui exécuterait du XML hostile : quiconque écrit le script peut
déjà lancer des commandes. Le rôle de sermo est de **ne pas transformer une
saisie utilisateur en exécution de code** que l'auteur n'a pas voulue.

## Exécution durcie (dans le cœur)

- **`safe_system` / `safe_popen`** remplacent `system`/`popen`. `safe_system`
  n'ouvre un shell **que** si la commande contient des métacaractères ; sinon
  elle exécute directement par `argv[]`, sans shell. Le repli `/bin/sh -c` est
  **journalisé**.
- **Aucun** `strcpy`/`strcat`/`sprintf` : bornage systématique. Le banc
  `tests/garde_fonctions_interdites.sh` passe sur le cœur, les neuf ports et
  aussi sur `sermo-source` (les programmes d'aide) ; le code du lecteur son et
  vidéo n'a plus ni `strcpy` ni `strtod` depuis le 2026-10-18.
- **Échappement de la sortie** : les valeurs exportées (`VAR="…"`) échappent les
  quatre caractères que le shell développe entre guillemets doubles
  (`\`, `"`, `$`, accent grave), pour que `eval "$(sermo …)"` ne réexécute pas ce
  qu'un utilisateur a tapé dans un champ. Vérifié par
  `tests/garde_echappement_sortie.sh` (banc « garde », 4 cas : `$(`, `${`,
  accent grave, et la ligne `EXIT=`). `\` et `"` ne sont pas joués par ce banc.
- **`--do`** est la voie recommandée quand le dialogue peut être utilisé par
  quelqu'un d'autre que l'auteur : les valeurs arrivent par l'environnement et ne
  sont **jamais** relues comme du code.
- **Un fichier Glade est du code** (`--glade-xml`, gtk3sermo et gtk4sermo) : ses
  gestionnaires de signaux sont des commandes. Ils passent par le **même chemin**
  que les `<action>` (`safe_system`, `SERMO_ALLOWED_CMDS`,
  `SERMO_NO_SHELL_FALLBACK`), et un fichier d'interface mérite la même confiance
  qu'un script de dialogue — pas davantage.

## Taille de ce que lit un `<input>`

Depuis la 2.7.3, chaque `<input>` — commande ou fichier — lit **au plus
16 Mio**. Au-delà, la lecture s'arrête comme sur une fin de fichier, et **un**
avertissement part sur la sortie d'erreur, jamais sur la sortie (qu'un `eval`
lit) :

    <input> « yes » : lecture arrêtée à 16777216 octets (limite SERMO_INPUT_MAX)

Une commande coupée ainsi reçoit `SIGPIPE` à sa prochaine écriture : `yes`
s'arrête de lui-même. La limite est posée dans le cœur
(`libsermocore/src/sermo_input.c`) et vaut pour les neuf backends.

- **`SERMO_INPUT_MAX`** — la limite, en octets. `0` la retire. Une valeur qui
  n'est pas un nombre d'octets (`16M`, `-1`) est signalée, et la limite par
  défaut s'applique.
- **La barre de progression n'est pas plafonnée.** Elle ne garde qu'une ligne à
  la fois, et doit pouvoir suivre une longue commande jusqu'au bout : une copie
  qui affiche chaque nom de fichier dépasse vite 16 Mio. Sa mémoire reste stable,
  même sur `yes`.

Jusqu'à la 2.7.2, rien ne bornait cette lecture. Branché sur `yes`, gtk3sermo
dépassait 700 Mo en 7 secondes, et ncurses finissait sur une erreur de
segmentation (mesuré le 2026-09-17). Vérifié par
`tests/garde_input_sans_fin.sh` (sur chaque port, avec un témoin sans limite) et
`tests/unit/test_sermo_input.c`.

## Bornage optionnel de l'exécution (deux variables d'environnement)

Deux garde-fous, **éteints par défaut**, permettent de déployer un dialogue dans
un contexte moins fiable (kiosque, session invité) sans changer le script. Ils
sont lus dans le cœur (`safe_exec.c`) et valent donc pour les
neuf backends :

- **`SERMO_NO_SHELL_FALLBACK`** — quand elle est posée (à n'importe quelle
  valeur), le repli `/bin/sh -c` (utilisé pour les commandes contenant des
  métacaractères shell) est **refusé** au lieu d'être seulement journalisé : le
  programme **échoue proprement** (*fail-closed*) plutôt que d'ouvrir un shell.
  Les commandes sans métacaractère continuent de s'exécuter en direct par
  `exec()`. Vérifié par `tests/garde_option_do.sh`.
- **`SERMO_ALLOWED_CMDS`** — liste des commandes autorisées, séparées par des
  virgules (ex. `SERMO_ALLOWED_CMDS=ls,cat,/usr/local/bin/outil`). Tant qu'elle
  est posée, seules ces commandes peuvent être lancées, **et** le repli
  `/bin/sh -c` est refusé (sinon `sh -c '…'` contournerait la liste). La
  comparaison porte sur la commande **telle qu'elle sera lancée** :
  - une entrée sans `/` (`ls`) autorise ce nom, et le chemin exact auquel le
    `PATH` du programme le résout (`/usr/bin/ls`) ;
  - une entrée avec `/` autorise exactement ce chemin, et le nom seul si le
    `PATH` le résout vers ce chemin ;
  - un programme qui porte un nom autorisé mais se trouve ailleurs
    (`/tmp/x/ls`) est **refusé**.

  Une valeur vide équivaut à une variable absente. Vérifié par
  `tests/garde_allowed_cmds.sh`.

Les noms de la 1.x, **`HAPLO_NO_SHELL_FALLBACK`** et **`HAPLO_ALLOWED_CMDS`**,
sont encore lus quand le nom `SERMO_…` n'est pas posé, avec un avertissement :
un déploiement durci en 1.x le reste en 2.x.

### Ce que ces deux variables n'atteignent pas

Trois limites connues. Les deux premières ouvrent un chemin d'exécution qui ne
passe pas par `safe_exec.c` : la liste et le refus de repli y sont sans effet.

- **`<terminal>` ouvre un shell hors des deux garde-fous** (gtk3sermo et
  gtk4sermo, les seuls qui aient ce widget). Le widget lance `/bin/sh` par VTE
  (`sermo-backend-gtk3/src/widget_terminal.c`), et la sortie de son `<input>`
  y est **tapée** comme au clavier (`vte_terminal_feed_child`). Une commande
  autorisée qui imprime une ligne fait donc exécuter cette ligne par le shell
  du terminal. Mesuré le 2026-09-19 sur le binaire du paquet 2.7.1, puis le
  2026-10-17 sur gtk3sermo et gtk4sermo des paquets 3.0.0-1, avec
  `SERMO_ALLOWED_CMDS` **et** `SERMO_NO_SHELL_FALLBACK` posés : une `<action>`
  ordinaire est refusée au même moment où le `<terminal>` écrit son fichier
  témoin. **Un dialogue qui contient un `<terminal>` donne un shell** : ne le
  déployez pas dans un contexte que vous croyez borné par ces variables.
- **Les boutons-liens (`<linkbutton>`)** ouvrent leur adresse par le navigateur
  du bureau, sans passer par cette liste. Ils ne passent pas par un shell pour
  autant. Selon le port :
  - efl1, fltk1, html et lvgl : `execlp("xdg-open", …)` direct, l'adresse en
    un argument ;
  - ncurses : `xdg-open` lancé par le lanceur du cœur, en argv (sous Windows,
    `ShellExecute` : voir [Hors de Linux](#hors-de-linux--ce-qui-change)) ;
  - gtk3 et gtk4 : le bouton-lien de GTK (`gtk_link_button`) ouvre l'adresse
    lui-même ;
  - qt6 : `QDesktopServices::openUrl` ; sdl3 : `SDL_OpenURL`.
- **`--include` annule la voie directe, et s'oppose à la liste.** Avec cette
  option, chaque commande est réécrite en `. 'fichier'; commande` avant d'être
  lancée : la chaîne porte toujours `;` et `'`, donc **tout** part par
  `/bin/sh -c`, même une commande sans le moindre métacaractère. Et comme le
  repli est refusé dès que `SERMO_ALLOWED_CMDS` est posée, `--include` avec une
  liste blanche n'exécute plus rien du tout. Les deux mesures ont été faites le
  2026-09-19 sur le binaire du paquet 2.7.1, et refaites le 2026-10-17 sur
  gtk3sermo du paquet 3.0.0-1. Le préfixe s'applique aussi aux commandes des
  programmes d'aide (`<input mqtt>`, `<action media>`…) : avec `--include`,
  elles partent elles aussi par `/bin/sh -c` (sans danger pour l'adresse, qui
  est encodée sans métacaractère), et sous liste blanche ou avec
  `SERMO_NO_SHELL_FALLBACK` elles sont refusées comme les autres (lu dans le
  code : `sermo-backend-gtk3/src/widgets.c`, `libsermocore/src/actions.c`,
  `safe_exec.c` ; pas mesuré sur un binaire).

## Programmes d'aide : lire et écrire vers l'extérieur (3.1.0)

Un `<input>` ou une `<action>` de protocole (`modbus`, `opcua`, `mqtt`,
`mavlink`, `media`) ne parle pas lui-même au réseau. Le cœur le traduit en une
commande qui lance un **programme d'aide** :
`$SERMO_SOURCE_DIR/sermo-<source|cible>-<protocole> <adresse>`
(`libsermocore/src/sermo_source.c`, `libsermocore/include/sermo_source.h`).

### Règles communes

- **En argv, jamais par le shell.** L'adresse part en un seul argument,
  encodée « pour cent » : tout octet hors de `[A-Za-z0-9._:/=,+@-]` devient
  `%XX`. La commande n'a donc aucun métacaractère (sauf avec `--include`, voir
  plus haut).
- **`SERMO_SOURCE_DIR`** : dossier des programmes d'aide, `/usr/libexec/sermo`
  par défaut. Un chemin relatif, ou qui porte un caractère hors de l'ensemble
  ci-dessus, est refusé à l'analyse du XML.
- **Sous `SERMO_ALLOWED_CMDS`**, la liste doit nommer le programme d'aide,
  comme toute autre commande.
- **Écrire demande un clic.** `<action mqtt>`, `<action mavlink>` et
  `<action media>` ne sont admises que dans un `<button>` (ou ses variantes
  ok, annuler, aide, oui, non) ou un `<menuitem>` ; ailleurs (minuteur, champ,
  case…), l'analyse s'arrête sur une erreur
  (`libsermocore/src/gtkdialog_parser.y`, `cible_verifier`). Un nom
  `sermo-cible-` écrit à la main dans un `<input>` ou une `<action>` est
  refusé de même. Modbus et OPC UA n'ont pas de programme d'écriture.
- **Chaque programme est un paquet à part.** Sans `sermo-cible-mqtt`,
  `sermo-cible-mavlink` ou `sermo-cible-media` installé, sermo ne peut que
  lire.
- **Adresse bornée.** Au plus 4096 octets, aucun caractère de contrôle (même
  après décodage), au plus 32 paramètres, et un paramètre inconnu est refusé
  (`sermo-source/commun.c`).
- **Durcissement.** Les 8 programmes d'aide sont construits avec les mêmes
  options que les ports, et `ci/bancs.sh` passe `tests/garde_durcissement.sh`
  (8 garanties) sur chacun d'eux.

### Modbus et OPC UA : lecture seule

- **Modbus** (`sermo-source-modbus`) : seules les fonctions de lecture 1 à 4
  sont appelées (bobines, entrées discrètes, registres de maintien et
  d'entrée). Le protocole n'a ni chiffrement ni authentification.
- **OPC UA** (`sermo-source-opcua`) : lecture d'une valeur, rien d'autre.
  `mode=none` par défaut, donc **en clair**. `mode=sign` et `mode=chiffre`
  exigent un certificat, une clé et au moins une `confiance=` (certificat de
  serveur admis) ; il n'y a aucun mode « tout accepter ».

### MQTT

- **`sermo-source-mqtt` ne publie jamais** (aucun appel à
  `mosquitto_publish`).
- **`mqtt://` est en clair.** `mqtts://` exige `confiance=` (le certificat de
  l'autorité qui a signé le serveur) ; le certificat **et** le nom du serveur
  sont vérifiés, jamais en mode « insecure » (`sermo-source/mqtt-commun.c`).
- **Identifiants** : `identifiants=` désigne un fichier à vous, lisible par
  vous seul (`chmod 600`) ; ils ne passent jamais en argument.
- **Nouveau en 3.1.0 : identifiants refusés sur `mqtt://`**, avant toute
  connexion, car ils passeraient en clair. Seule exception : l'adresse porte
  `clair=oui` (réseau local de confiance). `clair=` n'admet que `oui`, et ne
  vaut qu'avec `mqtt://`.
- **`sermo-cible-mqtt`** publie une valeur sur un sujet, seulement si une
  **liste blanche** le permet. Elle est **fermée par défaut** :
  `~/.config/sermo/cibles-mqtt` (à vous, lisible par vous seul ; le dossier
  personnel vient de la base des comptes, `getpwuid`, pas de `$HOME`) et
  `/etc/sermo/cibles-mqtt` (à root, que personne d'autre ne peut écrire). Un
  fichier mal protégé est refusé en entier. La publication n'est pas retenue
  par le serveur (pas de « retain »).

### MAVLink (drones)

- **`sermo-source-mavlink` n'émet rien** : il écoute la télémétrie.
- **`sermo-cible-mavlink`** n'agit que si sa liste blanche le permet (fermée
  par défaut, mêmes règles que MQTT : `~/.config/sermo/cibles-mavlink`,
  `/etc/sermo/cibles-mavlink`). Il sait :
  - **régler un paramètre**, seulement si le drone est **désarmé**. Les
    familles `ARMING_*`, `FLTMODE*`, `BRD_SAFETY*`, `FENCE_*` et
    `SERVOn_FUNCTION` sont **toujours** refusées, dans la liste comme dans
    l'adresse ;
  - **donner un ordre de sécurité** : `rtl` (retour au départ), `land`
    (atterrir), coupure des moteurs. La coupure demande **deux appuis** vers le
    même drone en moins de 5 secondes, et `$XDG_RUNTIME_DIR` pour garder la
    marque du premier appui. ⚠️ **Ces trois ordres partent aussi en vol** :
    c'est leur rôle ;
  - **envoyer une mission** (points de passage) au drone désarmé, relue et
    comparée ; elle n'est **jamais lancée**.
- **Un seul émetteur.** Une demande de lecture part d'abord, puis le
  programme écoute : si une autre station sol parle déjà, il refuse.
- **Succès seulement sur la preuve du drone** : la valeur relue, ou un accusé
  « accepté ». Une seule tentative, jamais répétée.
- **Pas de signature MAVLink** : rien n'authentifie les messages, et un autre
  appareil sur la même liaison peut parler au drone.
- **Aucun pilotage** : armer, décoller, changer de mode ou diriger le drone
  n'est pas dans cette version (prévu pour plus tard), hors retour au départ
  et atterrissage de sécurité (`rtl`, `land`, ci-dessus).

### Son et vidéo (`sermo-cible-media`, `sermo-source-media`)

- **8 commandes, liste fermée** : `lire`, `pause`, `reprendre`, `arreter`,
  `aller`, `volume`, `muet`, `son`. Les requêtes JSON envoyées à mpv sont
  écrites par le programme, avec des nombres bornés qu'il formate lui-même ;
  jamais la chaîne reçue. La commande IPC `run` de mpv (qui lance du shell)
  n'est jamais envoyée (`sermo-source/sermo-cible-media.c`,
  `sermo-source/media-commun.c`).
- **Sources** : `file://` (chemin absolu, fichier ordinaire lisible),
  `http://`, `https://`, `rtsp://`, `rtsps://`, et `v4l2://` pour une caméra
  (ci-dessous) seulement ; jamais
  d'identifiants dans l'adresse (`…@`). `edl://`, `av://`, `concat:`… sont
  refusés.
- **Caméra** (`source=v4l2:///dev/videoN`) : le nœud doit être un vrai
  périphérique d'image ; l'adresse donnée à mpv est construite par le
  programme ; `taille=` et `format=` en listes fermées. Dans la cage, ce nœud
  seul est visible (`--dev-bind`), sans réseau ; la fenêtre dit « CAMÉRA
  ACTIVE » ; rien n'est enregistré ; une caméra occupée est refusée, jamais
  forcée. Sans bwrap, comme un fichier local : pas de cage, un message le dit.
- **Identifiants HTTP(S)** : `identifiants=` désigne un fichier à vous, en mode
  exactement `600`. Le programme écrit l'en-tête dans un autre fichier `600`,
  passé à mpv par `--include=`, puis effacé. RTSP avec mot de passe : refusé
  (mpv ne le lirait que dans l'adresse, donc en argument).
- **Options de mpv** : `--no-config`, `--no-terminal`, `--idle=no`,
  `--ytdl=no`, `--load-scripts=no`, `--load-auto-profiles=no`,
  `--input-default-bindings=no`, `--load-unsafe-playlists=no`,
  `--access-references=no`, `--input-ipc-server=…`, `--title=…`, et
  `--ao=pulse,null` dans la cage (3.1.2) ; pour une caméra, en plus, `--demuxer-lavf-o=video_size=…,input_format=…` (valeurs
  des listes fermées) et `--log-file=<dossier>/mpv.log` (dossier privé du
  lecteur : `--no-terminal` coupe la sortie d'erreur de mpv, et la cause d'un
  échec ne se lit que là) ; puis `--` avant la source. Incrusté (`dans=`,
  3.1.1) : `--wid=N` (un nombre de 1 à 2³²−1, sinon refus), `--input-cursor=no`,
  `--input-vo-keyboard=no` ; caméra incrustée : `CAMÉRA ACTIVE` écrit dans
  l'image (`--osd-msg1`, avec `--osd-level=1`, `--osd-scale-by-window=no`,
  `--osd-font-size=24`).
- **Cage bubblewrap** : `--ro-bind / /`, `--dev /dev`, `--proc /proc`,
  `--tmpfs /tmp`, `--tmpfs $HOME`, `--tmpfs /run/user` (et le dossier de
  session s'il est ailleurs), `--unsetenv DBUS_SESSION_BUS_ADDRESS` (3.1.2),
  le dossier du lecteur en écriture, mpv et
  le fichier local en lecture seule (pour une caméra, son seul nœud, par
  `--dev-bind`), le son (PulseAudio) et l'affichage
  (Wayland, X11, `XAUTHORITY`) en lecture seule, `--unshare-pid`,
  `--die-with-parent`, `--new-session`, et `--unshare-net` pour un fichier
  local ou une caméra.
- **bwrap est obligatoire pour une source réseau** : sans lui, elle est
  refusée.
- **4 lecteurs au plus** en même temps. Dossiers en `700` sous
  `$XDG_RUNTIME_DIR/sermo-media` ; un dossier ou un socket ouvert au groupe ou
  aux autres est refusé.

**Limites du lecteur :**

- **Sans bwrap, un fichier local ou une caméra est lu sans cage.** Seul un
  message le dit
  (`Recommends: bubblewrap`, pas `Depends`).
- **`--ro-bind /` laisse lisible tout le système** hors de `$HOME` et `/tmp` :
  mpv peut lire ce que l'utilisateur peut lire ailleurs.
- **La cage donne l'accès à l'affichage (X11, Wayland) et au son.** Un mpv
  compromis pourrait s'en servir. Sous X11, cela vaut déjà sans incrustation :
  X11 ne sépare pas les fenêtres d'un même écran, `--wid` n'ajoute pas d'accès.
- **Sockets de session : corrigé en 3.1.2.** Jusqu'à la 3.1.1, le bus D-Bus de
  session et les autres sockets de `/run/user/<uid>` restaient joignables dans
  la cage (mesuré : `--ro-bind / /` n'empêche pas `connect()`). Depuis la 3.1.2,
  `/run/user` est masqué ; seuls le dossier du lecteur, le son (pulse) et
  l'affichage sont remontés, et `DBUS_SESSION_BUS_ADDRESS` est retiré. PipeWire
  n'est pas remonté (il donne aussi l'écran et les caméras) : mpv sort le son
  par pulse. Reste ouvert : les sockets « abstraits » (X11 en a un) restent
  joignables pour une source réseau, qui garde l'espace réseau de la machine.

## htmlsermo en mode serve (une page dans le navigateur)

Avec `SERMO_HTML_MODE=serve`, htmlsermo sert le dialogue comme une page web et
lit le formulaire renvoyé. C'est le seul port qui ouvre une socket réseau.

- **Boucle locale seulement** (`127.0.0.1`), port tiré par le noyau, jeton de
  128 bits (tiré de `/dev/urandom`) dans l'adresse ; sans le bon jeton, 404.
  Seule cette page est servie, GET et POST seulement, requête bornée à
  256 Kio, délai `SERMO_HTML_TIMEOUT` (300 s par défaut), une seule réponse
  de formulaire.
- **Depuis la 3.1.0, seules les variables déclarées par le dialogue sont
  rendues**, avec la valeur reçue (sinon leur valeur de départ), et `EXIT`.
  Avant, chaque champ reçu ressortait : celui qui remplit la page pouvait en
  ajouter un (`PATH`, `LD_PRELOAD`…) avec les outils du navigateur.
  Banc : `tests/garde_html_serve.sh`.
- **Limite** : `EXIT` vaut ce que le navigateur envoie (échappé) ; un script
  ne doit pas supposer qu'il est l'un des boutons du dialogue. Toute personne
  qui peut lire l'adresse annoncée sur la sortie d'erreur peut remplir la page.

## Durcissement du binaire

Cœur, backends et les 8 programmes d'aide compilent et lient avec : `-D_FORTIFY_SOURCE=3`,
`-fstack-protector-strong`, `-fstack-clash-protection`, `-fcf-protection=full`
(CET IBT/SHSTK) sur x86, PIE, RELRO complet, BIND_NOW, NX. À conserver sur
**chaque** cible — vérifié par `tests/garde_durcissement.sh`. Sur un autre
processeur ou un autre système, CMake choisit après essai : voir
[Hors de Linux](#hors-de-linux--ce-qui-change).

`_FORTIFY_SOURCE` n'agit qu'avec l'optimisation. Chaque `CMakeLists.txt` choisit
donc le type de build `Release` quand aucun n'est donné ; un build `Debug` perd
cette protection. Le cœur est contrôlé à part, sur sa bibliothèque, par
`tests/garde_fortify_coeur.sh` : sur qt6, sdl3 et ncurses, le backend optimisé
apporte ses propres appels fortifiés et masquerait un cœur qui ne l'est pas.

⚠️ **CET est posé, pas actif.** Les notes IBT et SHSTK sont forcées à l'édition
de liens (`-Wl,-z,ibt -Wl,-z,shstk`), et le banc les trouve. Mais sur Debian testing,
ni les objets de démarrage de la glibc, ni la glibc, ni les bibliothèques des
toolkits n'en portent (mesuré au `readelf` le 2026-09-16, refait le 2026-10-17 sur les paquets
3.0.0-1 avec la glibc 2.43, GTK 3.24.52 et Qt 6.11.2). Sur ce système, CET ne
peut donc pas agir à l'exécution — déduit de l'absence des notes, pas mesuré en
exécution.

## Serveurs MCP

- **`sermoia-mcp`** : seul l'outil `sermo_run` lance les commandes du dialogue,
  avec les droits de l'utilisateur, comme le ferait le script. `sermo_check` et
  `sermo_render` n'exécutent rien : ils posent `SERMO_ALLOWED_CMDS` sur une
  commande qui n'existe pas, ce qui refuse toute commande et le repli
  `/bin/sh -c` (`sermoia-mcp/src/sermoia-mcp.c`, `AUCUNE_COMMANDE`). Jusqu'à la
  2.8.1, `sermo_render` exécutait les `<input>` du dialogue. Pour borner
  `sermo_run`, poser `SERMO_ALLOWED_CMDS` dans l'environnement du serveur.
- **`sermoman-mcp`** est en lecture seule : il sert la documentation, ne lance
  rien et n'écrit rien.

Détails : [sermoia-mcp/README.md](sermoia-mcp/README.md#sécurité).

## Vérifier les paquets téléchargés

Les paquets Debian sont joints à chaque publication avec `SHA256SUMS` (une somme
SHA-256 par paquet) et sa signature détachée `SHA256SUMS.asc`, faite par la clé
de haplo-dialog (`devel@haplo-dialog.fr`), d'empreinte :

```
66C7 F4AD 7A58 A037 967F  8339 3799 9F99 7570 F1ED
```

```sh
curl -fLO https://haplo-dialog.fr/cle-haplo-dialog.asc
gpg --import cle-haplo-dialog.asc
gpg --verify SHA256SUMS.asc SHA256SUMS      # « Bonne signature de haplo-dialog <devel@haplo-dialog.fr> »
sha256sum --ignore-missing -c SHA256SUMS
```

Comparez l'empreinte affichée à celle du README sur GitLab **et** sur GitHub :
la clé publique vient du site, et une clé remplacée sur le site ne le serait
pas partout à la fois. Ce qui est signé, c'est la liste des sommes : les `.deb`
ne sont pas signés un par un, et il n'y a pas de dépôt apt (donc pas de dépôt
signé). Commande complète : [README.md](README.md#installer).

## Hors de Linux : ce qui change

Sous Windows, Android, les BSD et macOS, seul le port ncurses tourne. Le cœur est
le même : mêmes règles d'exécution, mêmes variables `SERMO_ALLOWED_CMDS` et
`SERMO_NO_SHELL_FALLBACK`. Ce qui diffère :

- **Le shell.** Les commandes à syntaxe shell passent par `/bin/sh`, ou par le
  programme que désigne `SERMO_SHELL` : un chemin **absolu** vers un exécutable ;
  toute autre valeur est refusée et signalée, et le shell par défaut sert.
  Sous Windows, le shell par défaut est celui du système (`%ComSpec%`, cmd.exe),
  appelé en `cmd /S /C "commande"`.
- **Windows relance par le shell** une commande sans syntaxe shell qu'il ne
  trouve pas comme programme : `echo`, `dir`, `type`… sont internes à cmd.exe.
  Cette relance obéit aux deux variables ci-dessus : avec
  `SERMO_NO_SHELL_FALLBACK=1` ou une liste `SERMO_ALLOWED_CMDS`, elle est
  refusée (vérifié sous Wine le 2026-10-12).
- **Pas de plafond des `<input>` sous Windows.** `SERMO_INPUT_MAX` n'y agit pas :
  le CRT n'a ni `fopencookie` ni `funopen`. Une commande sans fin lue par un
  `<input>` peut épuiser la mémoire. Ailleurs (Android, BSD, macOS), le plafond
  passe par `funopen` et tient (vérifié sous Android le 2026-10-12).
- **Liens sous Windows** : `<linkbutton>` ouvre par `ShellExecute`, qui lancerait
  un programme qu'on lui désignerait ; sermo n'y passe que `http`, `https` et
  `mailto`. Le reste est refusé (essayé sur un vrai Windows 10 : `calc.exe`
  refusé).
- **Durcissement selon la cible** :
  - ARM64 (Android, macOS Apple Silicon) : BTI/PAC à la place de CET. Sous
    Android, nos objets portent la marque BTI, mais le binaire final la perd au
    lien (les objets de démarrage d'Android et la `libncursesw` de Termux ne
    l'ont pas) : PAC agit, BTI n'est pas imposé ;
  - macOS : le lieur d'Apple ne connaît ni RELRO ni `-z` ; PIE et pile non
    exécutable y sont le défaut ;
  - Windows : ASLR haute entropie et DEP exigés au lien, binaire lié en statique,
    sans autre DLL que celles du système ;
  - `tests/garde_durcissement.sh` vérifie des binaires Linux (ELF) : il n'a pas
    été passé sur ces systèmes.

## Ce qu'aucun banc automatique ne vérifie

Certaines affirmations de ce document ne sont gardées par aucun banc de
`ci/bancs.sh`. Elles tiennent à une lecture du code ou à un essai à la main :

- **Glade sous liste blanche** : `tests/garde_glade.sh` vérifie le chargement
  d'un fichier Glade, pas `SERMO_ALLOWED_CMDS`. Que ses gestionnaires passent
  par le même chemin que les `<action>` est lu dans le code
  (`glade_support.c` appelle `execute_action`), pas mesuré.
- **`<terminal>` hors des deux variables** : `tests/garde_terminal.sh` vérifie
  que le terminal exécute la ligne reçue, sans liste blanche. Le contournement
  de la liste a été mesuré à la main le 2026-09-19 et le 2026-10-17.
- **`--include` sous liste blanche** : `tests/garde_include.sh` vérifie le
  chargement du fichier, sans liste blanche. Le refus a été mesuré à la main le
  2026-09-19 et le 2026-10-17.
- **`<linkbutton>`** : pas de banc. La façon d'ouvrir de chaque port est lue
  dans le code (2026-10-18).
- **« sermo n'appelle jamais `pkexec` ni `sudo` »** : pas de banc. Une recherche
  dans le code du cœur, des ports et des programmes d'aide n'en trouve aucun
  appel (2026-10-18).
- **CET inactif** : `tests/garde_durcissement.sh` trouve les notes IBT et
  SHSTK ; leur absence dans la glibc et les toolkits est mesurée à la main au
  `readelf` (dates plus haut, dans
  [Durcissement du binaire](#durcissement-du-binaire)).
- **Sommes signées** : pas de banc ; la vérification est celle que vous faites
  avec les commandes de [Vérifier les paquets téléchargés](#vérifier-les-paquets-téléchargés).

## Limites connues

- **Pas de bac à sable pour les commandes.** Une commande lancée par un
  dialogue a les droits de l'utilisateur qui l'a ouvert. sermo borne *comment*
  elle est lancée, pas *ce qu'elle fait*. Seul mpv, lancé par
  `sermo-cible-media`, est mis en cage (bubblewrap, avec les limites dites
  plus haut).
- **sermo n'élève jamais ses droits** : il n'appelle ni `pkexec` ni `sudo`, n'est pas
  setuid et ne demande aucun mot de passe administrateur. Un **script** qu'il
  exécute peut, lui, en demander : l'exemple `examples/system-tools` appelle
  `pkexec` (terminal root, édition d'un fichier système, mise à jour des paquets),
  et le dit dans son en-tête. Cette élévation est alors celle de l'auteur du
  script, hors de ce que sermo borne.
- **Pas de liste de commandes imposée par défaut** : la confiance va à l'auteur du
  script. La liste optionnelle `SERMO_ALLOWED_CMDS` reste disponible (voir plus
  haut).
- **Le plafond vaut pour chaque `<input>`, pas pour tout le dialogue.** Un
  dialogue qui lit beaucoup d'`<input>`, ou qui les relit par `refresh:`, peut
  garder plusieurs fois 16 Mio (voir
  [Taille de ce que lit un `<input>`](#taille-de-ce-que-lit-un-input)).
- **La mémoire grossit dans un dialogue qui se rafraîchit sans fin** si
  `<list>` ou `<table>` sont rafraîchies sans `clear:` : elles accumulent
  leurs rangées. La perte de quelques octets à chaque export des variables
  est fermée depuis la 3.0.0 (`libsermocore/src/variables.c` libère la
  valeur lue) ; le banc `tests/garde_fuite_refresh.sh` (LeakSanitizer) le
  vérifie sur ncurses et html.
- **`<terminal>` donne un shell** (gtk3sermo, gtk4sermo) : il lance `/bin/sh`
  par VTE, hors de `SERMO_ALLOWED_CMDS` et de `SERMO_NO_SHELL_FALLBACK`, et la
  sortie de son `<input>` y est tapée comme au clavier (mesuré le 2026-09-19, refait le 2026-10-17 en 3.0.0 ;
  voir plus haut).
- **`--include` fait tout passer par `/bin/sh -c`**, et n'exécute plus rien si
  une liste de commandes est posée (voir plus haut).
- **Un fichier Glade est du code**, et **`<linkbutton>`** ouvre son adresse hors
  de la liste des commandes (voir plus haut).
- **Pas d'audit externe.** Ce qui est vérifié l'est par les bancs du dépôt
  (`ci/bancs.sh`) et par les essais du mainteneur ; le code n'est pas relu en
  entier. Les écarts ouverts sont
  dans [TODO.md](TODO.md).

## Signaler une faille

- Écrivez à **`devel@haplo-dialog.fr`**. N'ouvrez pas de ticket public tant que la
  faille n'est pas corrigée.
- Donnez : la commande lancée (`gtk3sermo`, `qt6sermo`…) et sa sortie `--version`,
  la version du paquet, un script XML minimal et les étapes qui mènent au
  problème.
- **Délais** — sermo a un seul mainteneur : accusé de réception sous **14 jours** ;
  correctif, ou avis public avec contournement, au plus tard **90 jours** après
  le signalement, plus tôt si la faille est déjà publique.
- **Versions suivies** : seule la dernière ligne publiée reçoit des
  correctifs : la 3.1.x (la 3.1.2 depuis le 2026-10-19). La ligne 1.x n'en reçoit plus : passez à la 3.1
  ([MIGRATION.md](MIGRATION.md)).
- Si vous le souhaitez, votre nom figure dans le [CHANGELOG.md](CHANGELOG.md) avec
  le correctif.
