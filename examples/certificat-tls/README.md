<!-- SPDX-License-Identifier: CC0-1.0 -->
# certificat-tls — le certificat d'un serveur, en une fenêtre sermo

Une fenêtre pour regarder, **sans droits**, le certificat TLS que présente un serveur (le vôtre ou un autre) :

- **Bilan** :
  - vérification de la chaîne **et du nom** : OK, ou la raison de l'échec telle qu'openssl la donne (`hostname mismatch`, `certificate has expired`, `self-signed certificate`…) ;
  - nombre de jours avant expiration, avec une **ATTENTION** en dessous de `CT_ALERTE` jours (30 par défaut) ;
  - protocole et suite de chiffrement négociés ;
  - nombre de certificats envoyés.
- **Chaîne** : chaque certificat envoyé, avec son sujet, son émetteur, sa fin de validité et les jours restants.
- **Autorité** (facultatif) : un fichier `.pem` d'une PKI interne, ajouté aux autorités du système.

```sh
./certificat-tls.sh                                    # binaire « sermo » par défaut
./certificat-tls.sh --verifier debian.org 443          # sans fenêtre
./certificat-tls.sh --verifier 192.168.1.10 8443 intranet.local 5 /etc/ssl/ma-pki.pem
./certificat-tls.sh --xml                              # affiche le XML du dialogue, sans l'ouvrir
```

`--verifier HÔTE PORT [NOM] [DÉLAI] [AUTORITÉ.pem]` : `NOM` est le nom à vérifier (par défaut l'hôte, s'il
n'est pas une IP) ; délai en secondes, 5 par défaut. `CT_DIR` : dossier de session (état, bilan, chaîne).
Sans lui, le script en crée un dans `$XDG_RUNTIME_DIR` (ou `$TMPDIR`, ou `/tmp`) et l'efface en sortant.

Codes de sortie : `0` fait · `2` `openssl` absent (ou `awk`, `iconv`, `timeout`, `date`), ou sermo
introuvable · `64` entrée invalide ou option inconnue.

Ce n'est pas `boites-a-outils/serveur/02-certificats.sh` : celui-là **obtient et renouvelle** les certificats
de la machine avec certbot, en root. Celui-ci **regarde** celui d'un serveur, depuis n'importe quel compte.

## Installation

`openssl` (paquet du même nom), présent sur presque toute installation Debian.

## Ce que l'exemple garantit (testé par `tests/garde_certificat_tls.sh`)

- **Entrées vérifiées avant tout usage** :
  - hôte : nom ou IP, jamais commençant par `-` ;
  - port : de 1 à 65535 ;
  - nom à vérifier : nom DNS ;
  - délai : de 1 à 30 s ;
  - autorité : chemin absolu d'un fichier lisible.

  Toute entrée refusée donne le code 64, et **`openssl` n'est pas lancé**. Les arguments sont passés séparément.
- **Ce qu'envoie le serveur est une donnée.** Sujets et émetteurs sont nettoyés (contrôles retirés, `|` remplacé, UTF-8 valide, 200 caractères), la chaîne est limitée à 10 certificats et la sortie à 1 Mio. Rien n'est inséré dans le XML.
- Le test fabrique ses propres certificats et serveurs locaux. Il vérifie chaque bilan : valide, expiré, expiration proche, mauvais nom, autosigné, sujet piégé, serveur muet.

## Limites

- **Avec une adresse IP sans nom**, seule la chaîne est vérifiée, pas le nom, et le bilan le dit.
- Pas de STARTTLS (SMTP, IMAP… en clair puis chiffré), pas de test de révocation (OCSP, CRL), pas d'inventaire des protocoles acceptés : seulement ce que le serveur négocie avec cet openssl.
- La fenêtre attend la fin de la connexion, au plus le délai choisi.
