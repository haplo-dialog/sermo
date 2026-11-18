<!-- SPDX-License-Identifier: CC0-1.0 -->
# camera-capture — photo et vidéo d'une caméra, après confirmation

Une fenêtre qui montre la webcam **dans la fenêtre** (gtk3 et qt6 sous X11, gtk3 sous
Wayland ; ailleurs, dans la fenêtre de mpv), avec cinq boutons de caméra, et « Fermer »,
qui l'éteint d'abord :

| Bouton | Ce qu'il fait |
|---|---|
| Allumer | ouvre la caméra (`CC_CAMERA`, `/dev/video0` par défaut) ; « CAMÉRA ACTIVE » est écrit dans l'image (dans le titre de la fenêtre de mpv quand elle n'est pas incrustée) |
| Photo… | demande confirmation, puis écrit une image JPEG |
| Enregistrer 30 s… | demande confirmation, puis enregistre au plus `CC_DUREE` secondes (1 à 600) ; « ENREGISTREMENT » dans l'image et le titre |
| Couper | finit l'enregistrement avant la durée |
| Éteindre | arrête la caméra |

```sh
./camera-capture.sh                 # binaire « sermo » par défaut
./camera-capture.sh gtk3sermo
./camera-capture.sh --xml           # le XML, sans l'ouvrir
```

Codes de sortie : `0` fait · `1` refusé ou annulé · `2` outil absent · `64` entrée invalide.

## Installation

```sh
sudo apt install sermo-cible-media mpv bubblewrap
```

- `SERMO_SOURCE_DIR` : dossier des programmes d'aide (`/usr/libexec/sermo` par défaut).
- `CC_APPROBATION` : chemin d'une autre porte d'approbation (`../approbation/approbation.sh` par défaut).
- Sans la commande `xdg-user-dir`, les fichiers vont dans le dossier personnel.

## La confirmation

Photo et vidéo passent par la porte [`approbation`](../approbation/README.md). Elle montre le
**chemin exact** du fichier (dans `Images` ou `Vidéos`, ou `CC_PHOTOS` / `CC_VIDEOS`) et, pour une
vidéo, la **durée maximale**. Seul « Autoriser » déclenche l'écriture ; « Refuser », la fermeture ou
le délai écoulé n'écrivent rien.

## Ce que garantit `<action media>` (programme `sermo-cible-media`)

- Photo et vidéo **pour une caméra en cours seulement**.
- Le fichier est le chemin donné, **jamais écrasé**, créé en **600** (vous seul), dans un dossier à
  vous que ni le groupe ni les autres ne peuvent écrire.
- Vidéo : format MJPEG de la caméra (les images sont recopiées telles quelles, sans réencodage) ;
  `format=yuyv422` ne s'enregistre pas.
- Avant d'être recopié, le fichier passe par le dossier privé du lecteur (mode 700, en mémoire sous
  `$XDG_RUNTIME_DIR`) ; il y est effacé aussitôt.

## Ce que l'exemple garantit (testé par `tests/garde_camera_capture.sh`, sans ouvrir la caméra)

- Porte refusée, délai écoulé ou porte indisponible : le programme d'aide **n'est pas appelé**.
- Le chemin écrit est **exactement** celui montré (dossier à espace et accent compris), et la durée
  montrée est celle demandée.
- Le XML ne contient aucune adresse d'écriture : photo et vidéo ne partent que du script, après la
  porte.
