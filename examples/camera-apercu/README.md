<!-- SPDX-License-Identifier: CC0-1.0 -->
# camera-apercu — voir une caméra sur un clic, sans rien enregistrer

Une fenêtre pour **regarder** une webcam ou une caméra IP.

- **Caméra de la machine** : la liste vient de `/sys/class/video4linux`, **lue sans ouvrir les caméras** (les
  nœuds de métadonnées sont écartés). Choisissez une ligne, une résolution et un format, puis « Voir ».
- **Caméra IP** : une adresse `rtsp://`, `rtsps://`, `http://` ou `https://`, et si elle demande un mot de passe,
  le **chemin d'un fichier** d'identifiants (voir plus bas).
- L'image s'ouvre dans une fenêtre `ffplay`, titrée **« sermo — CAMÉRA ACTIVE — … »**. Le dialogue affiche
  **● CAMÉRA ACTIVE** tant qu'elle tourne. « Arrêter l'aperçu » ou fermer le dialogue l'arrête.

```sh
./camera-apercu.sh                                     # binaire « sermo » par défaut
./camera-apercu.sh qt6sermo                            # un autre port
./camera-apercu.sh --cameras                           # liste, sans ouvrir
./camera-apercu.sh --xml                               # affiche le XML du dialogue, sans l'ouvrir
```

Sans fenêtre, les mêmes gestes que les boutons du dialogue :

```sh
export CA_DIR=$(mktemp -d)                             # dossier de session, commun aux appels
./camera-apercu.sh --voir-locale /dev/video0 640x480 mjpeg
./camera-apercu.sh --voir-ip https://camera.local/flux.mjpg [FICHIER_IDENTIFIANTS]
./camera-apercu.sh --etat                              # « ● CAMÉRA ACTIVE — … » ou le dernier message
./camera-apercu.sh --arreter
```

- `--voir-locale NŒUD RÉSOLUTION FORMAT` : résolution parmi `640x480 1280x720 1920x1080 320x240`
  (défaut `640x480`), format parmi `mjpeg yuyv422` (défaut `mjpeg`).
- `--voir-ip ADRESSE [FICHIER]` : le fichier d'identifiants est un chemin absolu (voir plus bas).
- `CA_DIR` : dossier de session (état, numéros des processus). S'il n'existe pas, le script en crée un dans
  `$XDG_RUNTIME_DIR` (ou `$TMPDIR`, ou `/tmp`). Pour que `--etat` et `--arreter` retrouvent un aperçu lancé
  par `--voir-…`, donnez le même `CA_DIR` à chaque appel.
- `V4L_SYSFS` : dossier lu pour la liste (défaut `/sys/class/video4linux`) ; le banc y met un faux `/sys`.

Codes de sortie :

| Code | Sens |
|---|---|
| `0` | fait |
| `2` | outil absent (`ffplay`, `curl`, `bwrap` exigé), pas d'affichage graphique, échec de l'aperçu, sermo introuvable |
| `4` | caméra **occupée** |
| `64` | entrée invalide, ou option inconnue (le script affiche alors son mode d'emploi) |
| `129` / `130` / `143` | arrêté par SIGHUP / SIGINT / SIGTERM |

## Installer

```sh
sudo apt install ffmpeg curl bubblewrap
```

`ffmpeg` fournit `ffplay` (obligatoire). `curl` sert seulement pour une caméra HTTP(S) à mot de passe.
`bubblewrap` (la commande `bwrap`) est facultatif : il isole le lecteur (voir « Bac à sable »).

## Les règles

- **Un geste, toujours.** La caméra ne s'ouvre que sur « Voir » : jamais à l'ouverture du dialogue, jamais par minuteur.
- **Rien n'est enregistré** : ni photo, ni vidéo, ni fichier temporaire d'image.
- **Une caméra occupée est refusée, jamais forcée.** Si un autre programme tient la caméra, l'aperçu répond
  `caméra occupée … réessayez` (code 4), sans tuer ni suspendre personne.
- **Les identifiants ne passent jamais en argument.** Une adresse `…://utilisateur:mot@…` est refusée : les
  arguments d'un programme sont lisibles par tous les comptes de la machine.

## Caméra IP avec mot de passe

Fichier à vous, en `chmod 600`, avec une ligne `utilisateur:mot de passe` :

```sh
printf 'camera:motdepasse\n' > ~/.config/camera-salon.id && chmod 600 ~/.config/camera-salon.id
```

- **HTTP(S)** (MJPEG, MPEG-TS…) : `curl` lit l'adresse et les identifiants **sur son entrée standard** (`-K -`). Le flux passe par un tube vers `ffplay`. Ni `curl` ni `ffplay` n'ont le mot de passe dans leurs arguments, et la configuration temporaire (600) est effacée dès que `curl` l'a lue.
- **RTSP** : **non pris en charge avec mot de passe**. `ffplay` ne sait lire des identifiants RTSP que dans l'adresse, donc en argument. Mesuré le 2026-10-18 : la voie « fichier liste » (`-f concat`) ne lit pas un flux en direct.

## Bac à sable

Avec `bwrap` (`CA_BAC=auto`, par défaut), `ffplay` voit le système en lecture seule, **seulement le nœud
`/dev/videoN` choisi**, et pas de réseau pour une caméra locale. `CA_BAC=require` refuse sans `bwrap`
(code 2), `CA_BAC=off` le désactive. L'état est affiché à côté de « CAMÉRA ACTIVE ».

## Ce que l'exemple garantit (testé par `tests/garde_camera_apercu.sh`)

Le banc **n'ouvre jamais la vraie caméra**. Il utilise un faux `/sys`, un faux `ffplay`, et un vrai `ffplay`
sur une mire HTTP servie localement par `ffmpeg`.
- Entrées piégées : refusées (64), et le lecteur n'est jamais lancé.
- Caméra occupée : code 4, aucun `kill`.
- Indicateur présent pendant l'aperçu, absent après. Pas d'orphelin, même après SIGKILL du maître.
- Mot de passe absent des arguments de tous les processus, et la configuration de `curl` effacée.
- Le XML de `--xml` est bien formé.

## Limites

- Les formats proposés sont une liste fixe : une caméra qui ne connaît pas la combinaison choisie refuse, et le message d'`ffplay` s'affiche.
- Pas de son. Pas de capture d'image.
- Sans affichage graphique (SSH sans X), l'aperçu est impossible.
