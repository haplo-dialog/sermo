<!-- SPDX-License-Identifier: CC0-1.0 -->
# liaison-serie — écouter un port série, et n'y écrire qu'avec confirmation

Une fenêtre pour parler à un appareil sur une liaison série : adaptateur USB-série, carte de développement,
appareil de mesure, automate.

- **Écouter** : choisir le périphérique, le débit et le format (`8N1`, `8E1`, `8O1`, `8N2`, `7E1`, `7O1`).
  Ce qui arrive s'affiche en texte ou en **hexadécimal**, et la vue suit la fin du journal.
- **Envoyer** : une ligne de texte, avec la fin de ligne choisie (`CRLF`, `LF`, `CR` ou aucune).
  **Rien n'est écrit sans une fenêtre d'approbation** (`examples/approbation`), qui montre la trame exacte, octet par octet.
  Refuser ou fermer cette fenêtre : rien ne part.

```sh
./liaison-serie.sh                 # binaire « sermo » par défaut
./liaison-serie.sh gtk3sermo
```

Sans fenêtre, avec les mêmes contrôles (`LS_DIR` = dossier de session, à créer en mode 700) :

```sh
export LS_DIR=$(mktemp -d)
./liaison-serie.sh --ouvrir /dev/ttyUSB0 115200 8N1
./liaison-serie.sh --affiche true && cat "$LS_DIR/vue"
./liaison-serie.sh --envoyer /dev/ttyUSB0 115200 8N1 CRLF "AT"   # ouvre quand même la fenêtre d'approbation
./liaison-serie.sh --fermer
./liaison-serie.sh --xml                  # affiche le XML du dialogue, sans l'ouvrir
```

`LS_APPROBATION` : chemin d'une autre porte d'approbation (défaut : `../approbation/approbation.sh`, à côté
de ce dossier). Elle doit répondre comme `approbation.sh` : code 0 pour « Autoriser », autre chose pour non.

Codes de sortie : `0` fait · `1` envoi refusé · `2` outil absent (`socat`, `od`, `iconv`, `tail`), porte
d'approbation introuvable, échec de l'écriture par `socat`, ou sermo introuvable · `64` entrée invalide ou
option inconnue.

## Installation et droits

```sh
sudo apt install socat
sudo usermod -aG dialout "$USER"     # puis se reconnecter
```

Le script ne demande **aucune élévation** : l'accès au port passe par le groupe `dialout`.

## Essayer sans matériel

Une paire de ports virtuels (ce que fait le test) :

```sh
socat pty,raw,echo=0,link=/tmp/serie-a pty,raw,echo=0,link=/tmp/serie-b &
readlink -f /tmp/serie-a        # → /dev/pts/N : à mettre dans le champ « périphérique »
echo bonjour > /tmp/serie-b     # apparaît dans la fenêtre
```

## Ce que l'exemple garantit (testé par `tests/garde_liaison_serie.sh`)

- **Lecture seule par défaut.** L'envoi passe par `approbation.sh`. Seul « Autoriser » écrit sur la ligne, et les octets écrits sont ceux affichés.
- **Entrées vérifiées** avant tout usage :
  - périphérique : `/dev/ttyS*`, `ttyUSB*`, `ttyACM*`, `ttyAMA*`, `ttyXRUSB*`, `pts/*` ou `/dev/serial/by-*/…`. Il doit exister, être un périphérique caractère, lisible et inscriptible, et sa cible réelle doit rester sous `/dev` ;
  - débit et format : listes fermées ;
  - trame : une ligne, 1 024 caractères au plus.
- **Aucune écoute ne survit à la fenêtre.** Un gardien tue `socat` quand le processus maître disparaît, même tué par `SIGKILL`. Le script tue des PID exacts, jamais un groupe.
- **Journal plafonné** à 1 Mio (la lecture s'arrête, avec un message). La vue ne montre que les 8 derniers Kio, sans caractères de contrôle. Rien de ce qui est reçu n'entre dans le XML.
- Fichiers dans un dossier privé (700) de `$XDG_RUNTIME_DIR`, effacés à la fermeture.

## Limites

- Ce n'est pas un terminal interactif : une ligne à la fois, confirmée à chaque fois.
- Le plafond du journal est vérifié chaque seconde : un débit très élevé peut le dépasser d'une seconde de données.
- Sans affichage graphique (SSH), l'envoi est refusé, car la porte d'approbation ne peut pas s'ouvrir.
- Pas de contrôle de flux matériel (RTS/CTS) ni de lignes de modem en v1.
- Testé seulement sur une paire de ports virtuels `socat`, **pas sur un vrai adaptateur USB-série**.
