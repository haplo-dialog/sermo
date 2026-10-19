<!-- SPDX-License-Identifier: CC0-1.0 -->
# reseau-testeur — connexions TCP, test de port, trame et HTTP, en une fenêtre sermo

Une fenêtre pour le dépannage réseau courant, **sans droits particuliers** :

| Partie | Ce qu'elle fait | Outil appelé |
|---|---|---|
| Connexions TCP | liste les sockets de la machine (état, adresses, processus), avec un filtre texte | `ss -tanpH` |
| Tester le port | dit si `hôte:port` est **ouvert**, **fermé**, **filtré**, en **délai**, ou si le nom ne se résout pas | `bash` (`/dev/tcp`) + `timeout` |
| Ping | 4 échos ICMP : pertes, temps min/moy/max | `ping` (`iputils-ping`) |
| Trame TCP | envoie une ligne de texte (terminée par CR LF) et montre la réponse | `bash` (`/dev/tcp`) + `timeout` |
| Datagramme UDP | envoie une ligne (terminée par LF) et montre **un** datagramme de réponse | `bash` (`/dev/udp`) + `dd` + `timeout` |
| Requête HTTP | montre les en-têtes et le début du corps d'une URL `http://` ou `https://` | `curl` |
| Journal | les 20 dernières actions de la session, effacées à la fermeture | — |

```sh
./reseau-testeur.sh                 # binaire « sermo » par défaut
./reseau-testeur.sh gtk3sermo       # ou un port précis
```

Sans fenêtre, avec les mêmes contrôles (pratique dans un script) :

```sh
./reseau-testeur.sh --tester 127.0.0.1 631 2
./reseau-testeur.sh --trame 127.0.0.1 25 3 "EHLO essai"
./reseau-testeur.sh --udp 127.0.0.1 53 2 "essai"
./reseau-testeur.sh --ping 192.168.1.1 2
./reseau-testeur.sh --http https://example.org/ 10
./reseau-testeur.sh --sockets LISTEN
./reseau-testeur.sh --xml           # affiche le XML du dialogue, sans l'ouvrir
```

Codes de sortie : `0` fait · `2` outil absent · `64` entrée invalide.

## Outils requis

`bash`, `coreutils` (`timeout`, `head`, `dd`, `wc`, `tr`), `gawk` ou `mawk`, `iconv` (paquet `libc-bin`),
`iproute2` (`ss`), `iputils-ping` (`ping`) et `curl` pour la partie HTTP.

Les actions du dialogue passent par `/bin/sh -c`, à cause des guillemets autour des variables, comme dans `examples/modbus`. Le comportement sous `SERMO_ALLOWED_CMDS` / `SERMO_NO_SHELL_FALLBACK` **n'a pas été essayé** pour cet exemple.

`nc` n'est **pas** utilisé : ses variantes (traditionnelle, OpenBSD, ncat) n'ont pas les mêmes options.
`bash` sait ouvrir une connexion TCP seul (`/dev/tcp/hôte/port`), et c'est pareil partout.

## Ce que l'exemple garantit (testé par `tests/garde_reseau_testeur.sh`)

- **Entrées vérifiées avant tout usage**, valeur entière :
  - hôte : nom d'hôte, IPv4 ou IPv6 stricts, jamais commençant par `-` ;
  - port : de 1 à 65535 ;
  - délai : de 1 à 60 s ;
  - trame : une seule ligne, 4 096 octets au plus ;
  - URL : `http(s)://` seulement, sans identifiants (`utilisateur:mot@hôte` refusé), protocoles de `curl` limités à http/https, redirections comprises.

  Toute entrée refusée donne le code 64, et **aucun outil n'est lancé**.
- **Pas d'`eval`, pas de chaîne de commande** : les champs arrivent par l'environnement, puis en arguments séparés.
- **Les réponses sont des données** :
  - plafonnées à 64 Kio, avec une mention « tronqué » visible ;
  - débarrassées des caractères de contrôle et ramenées à de l'UTF-8 valide ;
  - affichées depuis un fichier, jamais insérées dans le XML.
- **Fichiers temporaires** dans un dossier privé (mode 700) de `$XDG_RUNTIME_DIR`, supprimés à la fermeture.

## Limites (à savoir)

- Pendant un test, une trame ou une requête, **la fenêtre attend** la fin de l'action, au plus le délai choisi. Gardez des délais courts.
- « Filtré » et « délai » se distinguent mal de l'extérieur. Un pare-feu qui jette les paquets sans répondre donne « délai ». Un pare-feu qui répond par un refus ICMP donne « filtré ».
- Sans droits, `ss` ne montre que **vos** processus. Les autres affichent « — ».
- **UDP n'a pas de connexion** : une réponse prouve que quelque chose écoute ; un refus ICMP « port injoignable » donne « fermé » ; un **silence ne prouve rien** (fermé sans ICMP, filtré, ou service qui ne répond pas à ce texte), et l'écran le dit.
- `ping` sans droits suppose `net.ipv4.ping_group_range` ouvert (le cas sur Debian récent) ; sinon il échoue, et le message le montre.
- Pas de capture de paquets (`tcpdump` exige des droits), pas de balayage de réseau.
- Tester des machines dont vous n'êtes pas responsable peut être interdit ou mal pris : limitez-vous aux vôtres.
