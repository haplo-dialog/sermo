<!-- SPDX-License-Identifier: CC0-1.0 -->
# modbus — lire un automate Modbus/TCP, sans rien y écrire

Une fenêtre qui montre en direct quelques **registres de maintien** d'un automate : une jauge et une valeur
par registre, et une ligne d'état. **Lecture seule** : le script lit avec `mbpoll`, et aucune valeur à écrire
ne passe jamais sur sa ligne de commande.

| Fichier | Rôle |
|---|---|
| `modbus-lecture.sh` | La fenêtre. Une requête `mbpoll` par tour pour tous les registres ; chaque widget lit ensuite sa valeur dans un petit cache. |
| `faux-automate.py` | Un faux automate Modbus/TCP sur `127.0.0.1`, pour essayer sans matériel. |

## Essayer sans automate

```sh
./faux-automate.py &          # écoute sur 127.0.0.1:5020 (./faux-automate.py PORT pour un autre)
./modbus-lecture.sh           # binaire « sermo » par défaut ; ou : ./modbus-lecture.sh gtk3sermo
```

Le faux automate fait bouger quatre registres chaque seconde : 1 température ×10 (°C), 2 pression (mbar),
3 vitesse (tr/min), 4 compteur de pièces. Il écoute sur `127.0.0.1` seulement ; le port 502 demande les
droits root.

## Lire un vrai automate

```sh
MODBUS_HOTE=192.168.1.20 MODBUS_PORT=502 MODBUS_ESCLAVE=1 ./modbus-lecture.sh
```

| Réglage | Défaut | Rôle |
|---|---|---|
| `MODBUS_HOTE` | `127.0.0.1` | nom ou adresse de l'automate |
| `MODBUS_PORT` | `5020` (le faux automate) | `502` pour un vrai |
| `MODBUS_ESCLAVE` | `1` | numéro d'esclave, 0 à 255 |
| `MODBUS_INTERVALLE` | `1000` | millisecondes entre deux lectures (100 au moins) |

Les registres lus sont décrits dans le tableau `REGISTRES`, en tête du script : une ligne par registre,
`numéro|libellé|diviseur|unité|jauge min|jauge max` (numéros à partir de 1, comme `mbpoll` ; jauge vide :
pas de jauge). Adaptez-le à votre automate.

Codes de sortie : `1` outil absent (`mbpoll`, `awk`, `sed`, `timeout`) ou réglage invalide ; sinon, le code
que rend sermo à la fermeture.

## Installation

```sh
sudo apt install mbpoll              # la lecture
sudo apt install python3-pymodbus    # seulement pour faux-automate.py
```

## Ce que fait l'exemple

- Les réglages qui partent sur la ligne de commande de `mbpoll` (hôte, port, esclave, intervalle) sont
  contrôlés d'abord.
- La lecture part **en fond**, une seule à la fois : un automate injoignable affiche « pas de réponse » sans
  figer la fenêtre. Délai : 5 s par lecture.
- Le cache vit dans `$XDG_RUNTIME_DIR` (sinon `~/.cache`) et disparaît avec la fenêtre.

## La même lecture en balise native

Ce script passe par `mbpoll`. La balise `<input modbus>` fait la même lecture directement dans le XML, sans
script (paquet `sermo-source-modbus`) :

```xml
<input modbus>tcp://automate:502?esclave=1&amp;registre=1&amp;diviseur=10</input>
```

Voir `MANUEL_UTILISATEUR.md`, « Lire un automate ».

## Où le trouver une fois installé

Le paquet `sermo-doc` installe `modbus-lecture.sh` et `faux-automate.py` dans
`/usr/share/doc/sermo-doc/examples/modbus/`.
