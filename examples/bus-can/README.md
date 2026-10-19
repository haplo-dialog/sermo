<!-- SPDX-License-Identifier: CC0-1.0 -->
# bus-can — écouter un bus CAN, et n'y envoyer une trame qu'avec confirmation

Une fenêtre pour regarder un bus CAN (véhicule, robot, machine, carte de développement) :

- **Écouter** une interface (`can0`, `vcan0`…). Un tableau montre chaque **identifiant vu** : dernières
  données, nombre de trames, il y a combien de temps. Mise à jour chaque seconde.
- **Envoyer UNE trame** (identifiant + 0 à 8 octets). **Rien ne part sans la fenêtre d'approbation**
  (`examples/approbation`), qui montre le bus, l'identifiant et les octets exacts. Refuser ou fermer cette
  fenêtre : rien n'est envoyé.

```sh
./bus-can.sh                 # binaire « sermo » par défaut
./bus-can.sh gtk3sermo
```

Sans fenêtre, avec les mêmes contrôles (`BC_DIR` = dossier de session, à créer en mode 700) :

```sh
export BC_DIR=$(mktemp -d)
./bus-can.sh --ouvrir can0
./bus-can.sh --inventaire                 # ID|données|trames|il y a
./bus-can.sh --envoyer can0 123 DEADBEEF  # ouvre quand même la fenêtre d'approbation
./bus-can.sh --fermer
./bus-can.sh --xml                        # affiche le XML du dialogue, sans l'ouvrir
```

`BC_APPROBATION` : chemin d'une autre porte d'approbation (défaut : `../approbation/approbation.sh`, à côté
de ce dossier). Elle doit répondre comme `approbation.sh` : code 0 pour « Autoriser », autre chose pour non.

Codes de sortie : `0` fait · `1` envoi refusé · `2` outil absent (`candump`, `cansend`, `ip`, `awk`), porte
d'approbation introuvable, échec de `cansend`, ou sermo introuvable · `64` entrée invalide ou option inconnue.

## Installation

```sh
sudo apt install can-utils iproute2
```

`can-utils` fournit `candump` et `cansend` ; `iproute2` fournit `ip`, qui vérifie l'interface. L'envoi a
besoin de la porte d'approbation (`examples/approbation`, donc d'un affichage).

L'interface CAN elle-même se prépare **hors de ce script**, une fois. Pour un vrai adaptateur (débit à adapter) :
`sudo ip link set can0 up type can bitrate 500000`. Le script ne demande aucune élévation.

## Essayer sans matériel : interface virtuelle

```sh
sudo modprobe vcan
sudo ip link add dev vcan0 type vcan && sudo ip link set up vcan0
cansend vcan0 123#DEADBEEF        # depuis un autre terminal : apparaît dans la fenêtre
```

Sans root, dans un espace isolé, comme le fait le test : `unshare -rn` puis les deux commandes `ip` (sans `sudo`).

## Ce que l'exemple garantit (testé par `tests/garde_bus_can.sh`)

- **Lecture seule par défaut.** Un envoi passe toujours par `approbation.sh`. Seul « Autoriser » envoie, et la trame envoyée est celle affichée.
- **Entrées vérifiées** avant tout usage :
  - interface : `canN` ou `vcanN`, existante et de type CAN (`ip link`) ;
  - identifiant : 3 chiffres hexadécimaux jusqu'à `7FF` (11 bits), ou 8 jusqu'à `1FFFFFFF` (29 bits) ;
  - données : 0 à 8 octets en hexadécimal.

  Toute entrée refusée donne le code 64, et **`cansend` n'est jamais lancé**.
- **Aucune écoute ne survit à la fenêtre** : le gardien tue `candump` quand le processus maître disparaît, même tué par `SIGKILL`. Le script tue des PID exacts, jamais un groupe.
- **Journal plafonné** à 4 Mio, et 500 identifiants affichés au plus. Seules les lignes au format strict de `candump -L` sont lues, et rien de ce qui est reçu n'entre dans le XML.

## Limites

- **CAN classique seulement** : pas de CAN FD (jusqu'à 64 octets), pas de trames distantes (RTR), pas de filtres d'identifiants.
- Le tableau garde la **dernière** valeur de chaque identifiant, pas l'historique. Le journal complet reste dans le dossier de session, effacé à la fermeture.
- Testé seulement sur une interface virtuelle `vcan`, **pas sur un vrai bus**.
- Envoyer sur un bus réel (véhicule, machine) peut avoir des effets physiques. La confirmation rend le geste explicite ; elle ne dit pas s'il est sans danger.
