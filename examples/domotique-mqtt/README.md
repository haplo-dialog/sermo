<!-- SPDX-License-Identifier: CC0-1.0 -->
# domotique-mqtt — lire des capteurs et commander d'un clic, en balises natives

Le seul exemple qui montre les deux balises MQTT de sermo, **sans aucun script entre les deux** :

- `<input mqtt>` **lit** un sujet. Ici : température, humidité, état de la lampe, rafraîchis toutes les
  `MAISON_INTERVALLE` secondes. Programme d'aide : `sermo-source-mqtt`, lecture seule.
- `<action mqtt>` **publie** une valeur, et **seulement depuis un bouton**. Ici : *Allumer* et *Éteindre*
  la lampe, et *Appliquer* la consigne du curseur (`valeur=$CONSIGNE`). Programme d'aide : `sermo-cible-mqtt`.

```sh
./fausse-maison.sh &          # serveur MQTT d'essai sur 127.0.0.1:18830, capteurs qui bougent
./domotique.sh                # ou : ./domotique.sh gtk3sermo
./domotique.sh --xml          # affiche le XML du dialogue, sans l'ouvrir
```

`fausse-maison.sh` écoute sur `127.0.0.1` seulement. `FAUSSE_MAISON_PORT` change son port (défaut `18830`) ;
il lit aussi `MAISON_PREFIXE`. Avec un autre port, posez le même dans `MAISON_SERVEUR` (et dans la liste
blanche). Il a besoin de `mosquitto` et `mosquitto-clients` ; codes : `2` outil absent ou serveur muet,
`64` port ou préfixe invalide.

Codes de sortie : `0` fenêtre fermée · `2` sermo ou `sermo-source-mqtt` absent · `64` réglage invalide.

| Réglage | Défaut | Rôle |
|---|---|---|
| `MAISON_SERVEUR` | `mqtt://127.0.0.1:18830` | `mqtt://hôte[:port]` ou `mqtts://…`, sans chemin ni paramètre |
| `MAISON_PREFIXE` | `maison/salon` | début des sujets, sans joker `+` ni `#` |
| `MAISON_INTERVALLE` | `2` | secondes entre deux lectures (1 à 60) |

## Écrire : la liste blanche, d'abord

Sans liste blanche, **les boutons ne publient rien**, et `sermo-cible-mqtt` le dit sur la sortie d'erreur.
C'est voulu : on règle exprès ce qu'une fenêtre a le droit de commander.

```sh
mkdir -p ~/.config/sermo
cp cibles-mqtt.exemple ~/.config/sermo/cibles-mqtt
chmod 600 ~/.config/sermo/cibles-mqtt      # sinon le fichier entier est refusé
```

Chaque ligne permet **un sujet précis** et **des valeurs bornées** : `on|off`, ou `18..24` pour la
consigne. Une valeur hors liste ou hors plage est refusée. Ce script n'écrit jamais ce fichier.

## Installation

```sh
sudo apt install sermo-source-mqtt sermo-cible-mqtt     # programmes d'aide de sermo
sudo apt install mosquitto mosquitto-clients            # seulement pour fausse-maison.sh
```

Depuis les sources : `ci/construire.sh` construit `sermo-source/_build`, puis
`SERMO_SOURCE_DIR=$PWD/sermo-source/_build ./examples/domotique-mqtt/domotique.sh`.

Pour un vrai serveur (Home Assistant, Zigbee2MQTT, Tasmota…), changez `MAISON_SERVEUR`, `MAISON_PREFIXE`,
les noms de sujets dans `domotique.sh` et les lignes de la liste blanche.
- **Serveur chiffré** (`mqtts://`) : ajoutez `&confiance=/chemin/ac.pem` aux adresses.
- **Serveur qui demande un mot de passe** : ajoutez `&identifiants=/chemin/fichier`. Le fichier, en `chmod 600`, contient `utilisateur:mot de passe` : jamais le mot de passe dans l'adresse.
  Sur `mqtt://` (non chiffré), des identifiants sont **refusés avant toute connexion** : ils passeraient en clair
  sur le réseau. Utilisez `mqtts://`, ou, sur un réseau local de confiance seulement, ajoutez `&clair=oui`.
  `clair=` n'admet que `oui` et ne vaut qu'avec `mqtt://`. La règle vaut pour la lecture et pour l'écriture.

Voir `MANUEL_UTILISATEUR.md`, « Lire la domotique » et « Commander la domotique ».

## Ce que l'exemple garantit (testé par `tests/garde_domotique_mqtt.sh`)

Le test joue la vraie chaîne : vrai `mosquitto`, vrai port sermo sous xvfb, vrais programmes d'aide, clics au
clavier avec `xdotool`.
- Les capteurs publiés s'affichent dans la fenêtre.
- **Avec** la liste blanche, *Allumer* publie `on` et *Appliquer* publie la consigne. **Sans** liste blanche, les mêmes clics ne publient **rien**.
- Un message piégé (`<b>`, `&`, `$(…)`) s'affiche comme du texte et n'exécute rien.
- Réglages contrôlés avant usage (code 64). Après l'arrêt de `fausse-maison.sh`, plus aucun processus, port fermé.

La liste blanche se lit dans le vrai dossier personnel (`$HOME` n'y fait rien). Le test tourne donc dans un
espace isolé (`unshare -rm`, `/root` en tmpfs) : **il ne lit ni n'écrit jamais votre `~/.config`**.

## Limites

- La valeur affichée est le dernier message **retenu** ou reçu dans le délai de lecture (1,5 s par défaut).
  Un capteur muet laisse le champ vide.
- **Les valeurs exportées gardent leur saut de ligne final** (`TEMPERATURE="21.5⏎"`) : c'est le cas de tout
  `<text>` dont la valeur vient d'un `<input>`, sur tous les ports (voir `MANUEL_UTILISATEUR.md`, `<text>`).
  Un script qui lit la sortie de sermo le retire avec `${TEMPERATURE%$'\n'}`.
- **Une case peut rester vide au premier affichage.** La lecture ne bloque jamais la fenêtre : si une lecture a déjà été tentée sans valeur récente, la relecture part en fond (`pas de valeur récente … (relecture en cours)` sur la sortie d'erreur), et la valeur arrive au tour suivant du minuteur.
- Un seul serveur par fenêtre dans cet exemple (rien n'empêche d'en mélanger dans un dialogue à vous).
- Essayé seulement avec `fausse-maison.sh`, **pas avec une vraie installation domotique**.
