<!-- SPDX-License-Identifier: CC0-1.0 -->
# usb-moniteur — les appareils USB et leurs branchements, en une fenêtre sermo

Une fenêtre pour répondre à *« qu'est-ce qui est branché ? »* et *« qu'est-ce qui vient de se brancher ? »*,
sans droits et sans rien modifier :

- **Branchés maintenant** : bus-port, identifiant `fabricant:produit`, nom du fabricant, nom du produit, classe.
  La liste est lue dans `/sys/bus/usb/devices`, rafraîchie toutes les 10 s ou par « Actualiser ».
- **Événements** : branchés, débranchés, (dé)liés à un pilote, depuis l'ouverture de la fenêtre, le plus récent
  en haut. Ils viennent de `udevadm monitor` et sont rafraîchis toutes les 2 s.

```sh
./usb-moniteur.sh                 # binaire « sermo » par défaut
./usb-moniteur.sh --inventaire    # sans fenêtre : bus-port|ID|fabricant|produit|classe
./usb-moniteur.sh --xml           # affiche le XML du dialogue, sans l'ouvrir
```

Les événements, sans fenêtre (`UM_DIR` = dossier de session, commun aux appels) :

```sh
export UM_DIR=$(mktemp -d)
./usb-moniteur.sh --ouvrir        # lance l'écoute (udevadm monitor) en fond
./usb-moniteur.sh --evenements    # instant|action|bus-port, les 200 derniers, le plus récent en haut
./usb-moniteur.sh --fermer        # arrête l'écoute
```

Sans `UM_DIR`, le script crée un dossier dans `$XDG_RUNTIME_DIR` (ou `$TMPDIR`, ou `/tmp`).
`USB_SYSFS` : autre racine que `/sys/bus/usb/devices`, pour les essais (le banc y met un faux `/sys`).

Codes de sortie : `0` fait · `2` `udevadm` absent (paquet `udev`), ou sermo introuvable · `64` usage.

## Ce que l'exemple garantit (testé par `tests/garde_usb_moniteur.sh`)

- **Lecture seule, sans droits.** L'exemple ne fait que lire `/sys` et écouter les événements du noyau.
- **Un appareil coincé ne fige pas la fenêtre.** Chaque attribut est lu avec un délai d'**1 s** : un appareil
  qui ne répond plus s'affiche avec `—` au lieu de tout bloquer.
- **Les textes des appareils sont des données.** Fabricant et produit sont écrits par l'appareil lui-même : contrôles retirés, `|` remplacé, 80 caractères, UTF-8 valide, jamais insérés dans le XML.
- **Le numéro de série n'est pas lu.**
- Seules les lignes au format strict d'`udevadm monitor` sont gardées, et seulement pour les appareils (pas leurs interfaces).
- **Aucune écoute ne survit à la fenêtre**, même si le processus maître est tué par `SIGKILL`. Journal plafonné à 2 Mio.

## Limites

- Les instants des événements sont ceux du noyau (secondes depuis le démarrage), pas l'heure.
- Pas de descripteurs détaillés (`lsusb -v` interroge l'appareil : c'est justement ce qui peut bloquer).
- Un appareil fantôme (déjà débranché, mais que le noyau n'a pas encore retiré) reste listé tant que le noyau le garde.
