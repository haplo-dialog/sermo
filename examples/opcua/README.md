<!-- SPDX-License-Identifier: CC0-1.0 -->
# opcua — lire un serveur OPC UA, sans rien y écrire

Une fenêtre qui montre en direct quelques **variables** d'un serveur OPC UA : une jauge et une valeur par
variable, et une ligne d'état. **Lecture seule** : le client `opcua-lire` n'appelle aucun service
d'écriture ni de méthode.

| Fichier | Rôle |
|---|---|
| `opcua-lecture.sh` | La fenêtre. Une connexion par tour pour toutes les variables ; chaque widget lit ensuite sa valeur dans un petit cache. |
| `opcua-lire.c` | Le petit client C, en lecture seule (bibliothèque open62541). |
| `faux-serveur-opcua.c` | Un faux serveur OPC UA sur `127.0.0.1`, pour essayer sans équipement. |
| `Makefile` | Construit `opcua-lire` et `faux-serveur-opcua`. |
| `certificats-essai.sh` | Fabrique des certificats **d'essai** (autosignés, 30 jours) pour le mode sécurisé. |

## Construire, puis essayer sans équipement

```sh
make                          # opcua-lire et faux-serveur-opcua
./faux-serveur-opcua &        # écoute sur 127.0.0.1:4841 (./faux-serveur-opcua PORT pour un autre)
./opcua-lecture.sh            # binaire « sermo » par défaut ; ou : ./opcua-lecture.sh gtk3sermo
```

Le faux serveur fait bouger quatre variables chaque seconde, dans l'espace de noms 1 : `temperature`
(°C), `pression` (mbar), `vitesse` (tr/min), `pieces`. Elles sont en lecture seule pour les clients.

## Lire un vrai serveur

```sh
OPCUA_ADRESSE=opc.tcp://192.168.1.30:4840 ./opcua-lecture.sh
```

| Réglage | Défaut | Rôle |
|---|---|---|
| `OPCUA_ADRESSE` | `opc.tcp://127.0.0.1:4841` (le faux serveur) | adresse du serveur |
| `OPCUA_INTERVALLE` | `1000` | millisecondes entre deux lectures (100 au moins) |
| `OPCUA_LIRE` | `./opcua-lire`, sinon celui du `PATH` | chemin du client |
| `OPCUA_MODE` | `none` | `sign` (signé) ou `chiffre` (signé et chiffré) |
| `OPCUA_CERTIFICAT`, `OPCUA_CLE` | — | certificat et clé de ce poste (DER ou PEM), obligatoires avec `sign` ou `chiffre` |
| `OPCUA_CONFIANCE` | — | certificat du serveur, obligatoire avec `sign` ou `chiffre` ; plusieurs : séparés par `:` |
| `OPCUA_URI` | `urn:sermo:opcua-lire` | ApplicationUri, celle du certificat du poste |

Avec `sign` ou `chiffre`, le client ne fait confiance **qu'aux** certificats de `OPCUA_CONFIANCE` : il n'y a
pas de mode « tout accepter ».

Les variables lues sont décrites dans le tableau `VARIABLES`, en tête du script : une ligne par variable,
`nœud|libellé|unité|jauge min|jauge max` (nœud en notation OPC UA, par exemple `ns=1;s=temperature` ;
jauge vide : pas de jauge). Adaptez-le à votre serveur.

Codes de sortie : `1` client introuvable (lancez `make`), outil absent (`awk`, `sed`, `timeout`) ou réglage
invalide ; sinon, le code que rend sermo à la fermeture.

## Essayer le mode sécurisé

```sh
./certificats-essai.sh /chemin/essai     # affiche ensuite les deux commandes à lancer
```

Le script crée le dossier, y écrit `serveur.{cert,cle}.der` et `client.{cert,cle}.der` (clés en mode 600),
puis affiche comment lancer `faux-serveur-opcua 4842 --securise …` et `opcua-lecture.sh` en
`OPCUA_MODE=chiffre`. **Jamais pour un vrai équipement** : là, le certificat du poste se fait selon les règles
du site, et celui du serveur se récupère chez son administrateur.

## Installation

```sh
sudo apt install libopen62541-1.4-dev pkg-config    # pour make
sudo apt install openssl                            # seulement pour certificats-essai.sh
```

## Ce que fait l'exemple

- L'adresse, qui part sur la ligne de commande du client, est contrôlée d'abord (`opc.tcp://…`).
- La lecture part **en fond**, une seule à la fois : un serveur injoignable affiche « pas de réponse » sans
  figer la fenêtre. Délai : 2 s de réponse, 10 s par lecture.
- Une variable illisible laisse sa case vide ; les autres restent à jour.
- Le cache vit dans `$XDG_RUNTIME_DIR` (sinon `~/.cache`) et disparaît avec la fenêtre.

## La même lecture en balise native

Ce script passe par `opcua-lire`. La balise `<input opcua>` fait la même lecture directement dans le XML,
sans script ni compilation (paquet `sermo-source-opcua`) :

```xml
<input opcua>opc.tcp://serveur:4840?noeud=ns=1;s=pieces</input>
```

Voir `MANUEL_UTILISATEUR.md`, « Lire un automate ».

## Où le trouver une fois installé

Le paquet `sermo-doc` installe les sources (`opcua-lecture.sh`, `certificats-essai.sh`, `Makefile`,
`opcua-lire.c`, `faux-serveur-opcua.c`) dans `/usr/share/doc/sermo-doc/examples/opcua/`. Un programme
compilé sur le poste n'en fait pas partie : lancez `make` dans une copie du dossier.
