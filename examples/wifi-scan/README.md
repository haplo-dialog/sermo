<!-- SPDX-License-Identifier: CC0-1.0 -->
# wifi-scan — les réseaux Wi-Fi à portée, en une fenêtre sermo

Une fenêtre pour le dépannage Wi-Fi courant (*« pourquoi c'est lent ? »*, *« quel canal choisir ? »*), en
**lecture seule** :

- **Réseaux** : utilisé ou non, SSID, BSSID, canal, bande (2,4 / 5 / 6 GHz), signal, sécurité. Du plus fort
  au plus faible. Un réseau **ouvert** (`⚠ ouvert`) ou mal protégé (WEP, WPA1 seul : `⚠ faible`) est signalé.
- **Occupation des canaux** : combien de réseaux par canal, et le signal le plus fort sur chacun. Le canal le
  plus chargé apparaît en premier.
- Rafraîchi toutes les 15 s, à partir des résultats que NetworkManager garde en mémoire. « Relancer la
  recherche » demande un nouveau balayage radio.

```sh
./wifi-scan.sh                 # binaire « sermo » par défaut
./wifi-scan.sh --liste         # sans fenêtre : résultats en mémoire de NetworkManager
./wifi-scan.sh --liste relancer   # demande d'abord un nouveau balayage radio
./wifi-scan.sh --canaux
./wifi-scan.sh --xml           # affiche le XML du dialogue, sans l'ouvrir
```

Codes de sortie : `0` fait · `2` `nmcli` ou `python3` absent, ou sermo introuvable · `64` usage.

## Installation

```sh
sudo apt install network-manager python3
```

`network-manager` fournit `nmcli` ; `python3` découpe et nettoie ses lignes.

## Confidentialité

Les noms des réseaux voisins (et leurs adresses BSSID) sont des données personnelles : l'exemple les affiche,
**ne les garde pas et ne les envoie nulle part**. Pas de fichier, pas de journal.

## Ce que l'exemple garantit (testé par `tests/garde_wifi_scan.sh`)

- **Lecture seule** : `nmcli device wifi list` seulement. Aucune connexion, aucun mot de passe lu ni demandé.
- **Les SSID sont des données arbitraires** (jusqu'à 32 octets quelconques) : contrôles remplacés, `|` remplacé, UTF-8 invalide réparé, 32 caractères. Jamais insérés dans le XML.
- Le découpage suit l'échappement de `nmcli -t` (`\:` dans un SSID ou un BSSID) ; une ligne dont un champ n'a pas la forme attendue (BSSID, canal, fréquence, signal) est ignorée.
- Délai de 20 s par appel, 500 réseaux au plus.

## Limites

- Ce que voit **cette** carte, à **cet** endroit : rien sur le débit réel ni sur les interférences hors Wi-Fi.
- Un balayage sur demande peut être refusé par la politique de NetworkManager ; la liste en mémoire reste affichée.
- Pas de choix de l'interface si la machine en a plusieurs : NetworkManager les mélange.
