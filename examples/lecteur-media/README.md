<!-- SPDX-License-Identifier: CC0-1.0 -->
# lecteur-media — lire un son ou une vidéo depuis une fenêtre sermo

L'exemple des balises `<action media>` et `<input media>` : une fenêtre avec **Lire, Pause, Reprendre,
Arrêter**, un curseur de **volume**, et l'**état**, la **position** et la **durée**, mis à jour chaque seconde.

```sh
./lecteur-media.sh ~/Musique/annonce.ogg        # binaire « sermo » par défaut
./lecteur-media.sh film.mkv gtk3sermo            # une vidéo : elle s'ouvre dans la fenêtre de mpv
./lecteur-media.sh                               # sans fichier : un son d'essai de 5 s (ffmpeg)
./lecteur-media.sh --xml                         # affiche le XML du dialogue (son d'essai), sans l'ouvrir
```

Arguments : `./lecteur-media.sh [FICHIER] [binaire sermo]`. Le binaire est le **2e** argument : pour choisir
un port, donnez d'abord le fichier. `SERMO_SOURCE_DIR` : dossier des programmes d'aide (défaut
`/usr/libexec/sermo`).

Codes de sortie : `0` fenêtre fermée · `2` sermo, `python3`, programme d'aide ou ffmpeg absent (ffmpeg n'est
nécessaire que sans FICHIER), ou son d'essai impossible à fabriquer · `64` fichier refusé (pas un fichier
ordinaire lisible) ou option inconnue.

## Comment c'est écrit

```xml
<button><label>Lire</label>
  <action media>media://lecteur?commande=lire&amp;source=file:///chemin/son.ogg</action></button>
<button><label>Volume</label>
  <action media>media://lecteur?commande=volume&amp;valeur=$VOLUME</action></button>
<text><variable>ETAT</variable><input media>media://lecteur?donnee=etat</input></text>
```

- `lecteur` est le **nom** du lecteur : plusieurs lecteurs peuvent tourner à la fois, 4 au plus.
- L'état vaut toujours `stopped`, `playing`, `paused` ou `error` : c'est la valeur exportée par sermo.
- Il n'y a pas d'évènement « fin de lecture » : un minuteur relit l'état, comme ici.
- Une `<action media>` n'est permise que sur un **bouton** ou une **entrée de menu** : lire est toujours un geste.

## Installation

```sh
sudo apt install sermo-cible-media sermo-source-media mpv bubblewrap python3 ffmpeg
```

`python3` encode le chemin du fichier pour l'adresse. `ffmpeg` sert seulement à fabriquer le son d'essai.
`bubblewrap` (la commande `bwrap`) isole mpv (voir plus bas).

Depuis les sources : `ci/construire.sh`, puis `SERMO_SOURCE_DIR=$PWD/sermo-source/_build ./examples/lecteur-media/lecteur-media.sh`.

## Ce que garantissent les programmes d'aide (banc `tests/garde_sermo_media.sh`)

- **Sources contrôlées avant mpv** : `file://` (chemin réel, fichier ordinaire), `http(s)://`, `rtsp(s)://`, et
  une caméra `v4l2:///dev/videoN[&taille=…][&format=mjpeg|yuyv422]` (nœud d'image réel ; taille `320x240`,
  `640x480`, `1280x720` ou `1920x1080`). mpv, lui, accepterait aussi `edl://` ou `av://lavfi:…` : ils sont refusés ici.
- **Caméra** : la fenêtre de mpv porte « CAMÉRA ACTIVE » dans son titre ; une caméra occupée est refusée, rien
  n'est forcé.
- **Seule une liste fixe de commandes** atteint mpv, avec des nombres bornés. La commande IPC `run` de mpv, qui
  exécute du shell, ne peut jamais être demandée.
- **bwrap** : obligatoire pour une source réseau. Pour un fichier local ou une caméra, il est facultatif : avec
  lui, le réseau est coupé ; sans lui, la source est lue sans cage et seul un message le dit.
- **mpv s'arrête quand la fenêtre se ferme**, même si sermo est tué.
- **Identifiants** d'un flux HTTP(S) : `identifiants=/chemin` d'un fichier `chmod 600` (`utilisateur:mot de passe`), jamais dans l'adresse. RTSP à mot de passe : non pris en charge.

## Limites

- La vidéo s'affiche dans la fenêtre de mpv, pas dans celle de sermo : cet exemple n'utilise pas `dans=` ; voir le manuel pour l'incrustation (gtk3, X11).
- Pas de liste de lecture, pas de sous-titres, pas d'enregistrement.
- Essayé sous X11 (et xvfb) ; **Wayland non essayé**.
