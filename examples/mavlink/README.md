<!-- SPDX-License-Identifier: CC0-1.0 -->
# mavlink — lire la télémétrie d'un drone, lui donner un ordre de sécurité, le piloter

Trois fenêtres pour un drone qui parle MAVLink (ArduPilot), en balises natives, sans script entre les deux :

| Fichier | Rôle | Paquet |
|---|---|---|
| `drone-lecture.sh` | **Lecture seule** : batterie (jauge, tension), position (latitude, longitude, hauteur), armé, satellites. Rien n'est émis sur le lien. | `sermo-source-mavlink` |
| `drone-commande.sh` | Au sol, régler **un** paramètre (`WP_SPD`, vitesse de mission) ; en vol, **retour au départ**, **atterrir ici**, et en dernier recours **couper les moteurs** (deux appuis en moins de 5 s). Plus l'état : armé, mode, hauteur. | `sermo-cible-mavlink` (et `sermo-source-mavlink` pour l'état) |
| `drone-pilotage.sh` | **Pilotage** (3.2.0, ArduPilot Copter) : armer, décoller, changer de mode, aller à un point, désarmer ; deux appuis par ordre ; rien ne part sans la ligne `pilotage` de la liste blanche. Le télépilote reste responsable du vol. | `sermo-cible-mavlink` (et `sermo-source-mavlink` pour l'état) |
| `faux-drone.py` | Un faux drone qui diffuse de la télémétrie, pour essayer la lecture sans matériel. | `python3-pymavlink` |

Les deux premières fenêtres ne pilotent pas : ni armement, ni décollage, ni changement de mode, ni déplacement ; `drone-pilotage.sh` le fait. Depuis la 3.2.0, `<action mavlink>` sait piloter un ArduPilot Copter (`pilote=armer|decoller|mode|aller|desarmer`), seulement si une ligne `pilotage` de la liste blanche le permet : voir le manuel utilisateur, « Commander un drone », et `SECURITY.md`. Depuis la 3.3.0, `pilote=` est refusé pour un appareil qui n'est ni multirotor ni hélicoptère (avion, rover), et si deux autopilotes battent sur la liaison il faut ajouter `systeme=N` à l'adresse.

## Essayer la lecture sans drone

```sh
./faux-drone.py --tcp 5760 &     # sert en TCP sur 127.0.0.1:5760, comme le simulateur SITL
./drone-lecture.sh               # binaire « sermo » par défaut ; ou : ./drone-lecture.sh gtk3sermo
```

Le faux drone diffuse 5 fois par seconde des valeurs fixes (12,60 V, 76 %, 45,1234567 N 5,7654321 E,
30,25 m au-dessus du départ, 11 satellites…). Autres liaisons : `--udp HÔTE:PORT` (envoie vers cette adresse),
`--serie FICHIER` (un pseudo-terminal, dont le chemin est écrit dans `FICHIER`). Options : `--arme` (le drone se
dit armé), `--mode N`, `--sans-courant`, et pour les bancs `--intrus`, `--espion FICHIER`, `--journal FICHIER`,
`--repond` (répond aux lectures : paramètres de barrière, état au sol, position), `--barriere-coupee`,
`--barriere-nan`, `--barriere-type V`, `--lent`, `--px4`, `--signe`, `--sol`, `--type N`, `--second`,
`--systeme N` (détail en tête du script).

Il **n'accuse jamais un ordre** (avec `--journal` il note ce qu'il reçoit, avec `--repond` il répond aux
seules lectures) : pour essayer `drone-commande.sh` ou `drone-pilotage.sh`, il faut un autopilote, réel ou
simulé (ArduPilot en SITL, port TCP 5760).

## Réglages

| Variable | Défaut | Rôle |
|---|---|---|
| `MAVLINK_ADRESSE` | `tcp://127.0.0.1:5760` | `tcp://hôte:port`, `udp://:14550` derrière un relais comme MAVProxy, `serie:///dev/ttyUSB0` pour une radio |
| `MAVLINK_INTERVALLE` | `1000` | millisecondes entre deux lectures (`drone-lecture.sh`) |

Le binaire sermo est le 1er argument, sinon `SERMO`, sinon `sermo`. Les trois scripts remplacent leur
processus par sermo (`exec`) : leur code de sortie est celui de sermo.

## Commander : la liste blanche, d'abord

Rien ne part sans `~/.config/sermo/cibles-mavlink`, à vous, en `chmod 600`. Par exemple :

```
parametre WP_SPD 1..15
ordre rtl
ordre land
ordre terminaison
```

- Un paramètre n'est réglé que si le drone est **désarmé**, et le succès n'est dit que si le drone renvoie la
  même valeur. Un ordre ne réussit que si le drone répond « accepté ».
- « COUPER LES MOTEURS » : le premier appui n'envoie rien et le dit ; un second appui **dans les 5 s**
  envoie l'ordre (pas avant une demi-seconde : un rebond ne confirme rien). Le drone tombe.
- La conclusion de chaque appel s'affiche dans « Dernier ordre » (fichier
  `$XDG_RUNTIME_DIR/sermo-cible-mavlink/message`). `drone-commande.sh` et `drone-pilotage.sh` exigent `XDG_RUNTIME_DIR`.

Les autres garde-fous (familles de paramètres jamais réglées, un seul émetteur sur le lien, envoi d'une
mission jamais lancée) : voir `MANUEL_UTILISATEUR.md`, « Commander un drone ».

## Sur une radio seule

sermo ne demande pas de flux au drone (il n'émet rien en lecture) : sur une radio seule, sans station sol ni
relais, seul le battement (armé, mode) arrive. Le remède est côté drone : sur ArduPilot 4.7,
`MAV1_EXT_STAT`, `MAV1_POSITION` et `MAV1_EXTRA2` (par exemple à 2) ; avant la 4.7, `SR0_…` (les anciens noms
sont ignorés en silence).

## Installation

```sh
sudo apt install sermo-source-mavlink sermo-cible-mavlink
sudo apt install python3-pymavlink        # seulement pour faux-drone.py
```

## Licences

`drone-lecture.sh`, `drone-commande.sh` et `drone-pilotage.sh` sont en CC0-1.0 ; `faux-drone.py` est en GPL-2.0 ou plus récente.
Chaque fichier le dit en tête.

## Où le trouver une fois installé

Le paquet `sermo-doc` installe les quatre scripts dans `/usr/share/doc/sermo-doc/examples/mavlink/`.
