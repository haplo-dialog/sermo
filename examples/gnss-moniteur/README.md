<!-- SPDX-License-Identifier: CC0-1.0 -->
# gnss-moniteur — l'état d'un récepteur GNSS/GPS, en une fenêtre sermo

La fenêtre montre :
- **position** : fixation (aucune / 2D / 3D), latitude, longitude, altitude, vitesse, cap, heure UTC ;
- **qualité** : HDOP / PDOP, satellites vus et utilisés ;
- **satellites** : un tableau avec constellation, PRN, élévation, azimut, signal, et si le satellite est utilisé ou non.

Elle se met à jour toutes les `GNSS_PERIODE` secondes (de 1 à 10, 2 par défaut).

```sh
./gnss-moniteur.sh                              # lit gpsd par gpspipe
./gnss-moniteur.sh gtk3sermo                    # un port précis
./gnss-moniteur.sh --demo echantillons/demo.jsonl   # sans gpsd ni récepteur
GNSS_PERIODE=5 ./gnss-moniteur.sh
./gnss-moniteur.sh --xml                        # affiche le XML du dialogue, sans l'ouvrir
```

Codes de sortie : `0` fenêtre fermée · `2` `gpspipe`, `python3` ou sermo absent · `64` usage.

## Installation

```sh
sudo apt install gpsd gpsd-clients python3 xdg-utils
```

`xdg-utils` fournit `xdg-open`, qui sert seulement au bouton « Ouvrir la carte ».

`gpsd` doit voir le récepteur. Sur Debian, réglez `DEVICES=` dans `/etc/default/gpsd` (par exemple
`/dev/ttyACM0`), puis redémarrez le service. Le script ne demande **aucun droit** : il parle à `gpsd`, et
c'est `gpsd` qui lit le port série.

## Le mode démo

`echantillons/demo.jsonl` est un enregistrement **fabriqué**, pas capturé sur un vrai récepteur. Il suit le
format de `gpspipe -w` (classes `TPV` et `SKY`), avec une position fictive à l'Observatoire de Paris qui
passe de « aucune » à 2D puis à 3D. Le mode démo le rejoue 40 lignes à la fois, en boucle.

Pour remplacer cet échantillon par un vrai : `gpspipe -w -n 80 > mon-recepteur.jsonl`, puis
`--demo mon-recepteur.jsonl`. **Attention**, un tel fichier contient votre position : ne le publiez pas tel quel.

## Confidentialité

La position est une donnée personnelle. **Rien ne quitte la machine**, sauf si vous cliquez sur
« Ouvrir la carte ». Ce bouton ouvre OpenStreetMap dans le navigateur, avec la position dans l'adresse.
« Copier la position » (`lat,lon`) n'apparaît que si `wl-copy`, `xclip` ou `xsel` est installé.
Le cache vit dans un dossier privé (mode 700) de `$XDG_RUNTIME_DIR`, effacé à la fermeture.

## Ce que l'exemple garantit (testé par `tests/garde_gnss_moniteur.sh`)

- Le JSON reçu est lu par `python3 -I`. Le script n'en garde que :
  - des **nombres bornés** : latitude entre −90 et 90, signal entre 0 et 99, etc. ;
  - une heure au format ISO strict.

  Une chaîne à la place d'un nombre, `NaN`, une valeur hors bornes ou une ligne malformée donnent « — ». Rien de ce qui est reçu n'entre dans le XML.
- Position masquée sans fixation 2D, altitude masquée sans fixation 3D.
- Lecture en fond, une seule à la fois : la fenêtre ne se fige pas si `gpsd` tarde (comme `examples/modbus`).

## Limites

- **Pas de carte dans la fenêtre**, ni de ciel des satellites dessiné : le lien vers la carte est un geste explicite.
- Les champs de `gpsd` changent selon les versions. Seuls ceux cités plus haut sont lus, et une absence est tolérée.
  Testé seulement sur l'échantillon fabriqué : **aucun vrai récepteur n'a été essayé**.
