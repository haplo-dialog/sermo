<!-- SPDX-License-Identifier: CC0-1.0 -->
# journal-direct — suivre le journal systemd en direct, en une fenêtre sermo

Une fenêtre pour regarder ce qui se passe sur la machine **pendant** qu'on agit : démarrer un service, brancher
un appareil, rejouer une erreur.

- **Source** : `système` (tout le journal) ou `session` (vos services utilisateur).
- **Gravité** : `urgence` … `débogage` ; on voit la gravité choisie **et pire** (par défaut : `avertissement`).
- **Unité** (facultatif) : une unité systemd, par exemple `ssh.service` ou `cups`.
- **Filtre** : un texte, sans distinction de casse, appliqué à l'affichage à chaque rafraîchissement (toutes
  les 2 s).
- **Tableau** : heure, gravité, source et message. Le plus récent est en haut, 300 lignes au plus.

```sh
./journal-direct.sh                 # binaire « sermo » par défaut
```

Sans fenêtre, avec les mêmes contrôles (`JD_DIR` = dossier de session, à créer en mode 700) :

```sh
export JD_DIR=$(mktemp -d)
./journal-direct.sh --ouvrir système erreur ssh.service
./journal-direct.sh --vue "refused"
./journal-direct.sh --fermer
./journal-direct.sh --xml           # affiche le XML du dialogue, sans l'ouvrir
```

Codes de sortie : `0` fait · `2` `journalctl` ou `python3` absent, sermo introuvable, ou dossier de travail
impossible à créer · `64` entrée invalide ou option inconnue · `129`, `130`, `143` fenêtre interrompue par
un signal (HUP, INT, TERM).

## Droits

Aucune élévation. Pour lire le journal **système** en entier, il faut être dans le groupe `systemd-journal`
(ou `adm` sur Debian). Sinon, `journalctl` ne montre que vos propres messages, sans erreur.

## Ce que l'exemple garantit (testé par `tests/garde_journal_direct.sh`)

- **Entrées vérifiées** avant tout usage :
  - source et gravité : listes fermées ;
  - unité : nom d'unité systemd, jamais commençant par `-` ;
  - filtre : une ligne, 200 caractères au plus.

  Toute entrée refusée donne le code 64, et **`journalctl` n'est pas lancé**. Les arguments sont passés séparément.
- **Les messages sont des données.** Ils sont lus en JSON (`-o json`) par `python3 -I`. Les messages binaires (tableau d'octets) sont décodés, les caractères de contrôle remplacés, `|` remplacé par `¦`, et chaque champ est plafonné (message à 300 caractères ; une entrée de plus de 64 Kio est ignorée). Rien n'est inséré dans le XML.
- **Aucun suivi ne survit à la fenêtre.** Un gardien tue `journalctl` quand le processus maître disparaît, même tué par `SIGKILL`.
- **Journal de session plafonné** à 8 Mio, avec un message.

## Confidentialité

Le journal contient souvent des noms de machines, d'utilisateurs, des adresses. Ce qui est lu reste dans un
dossier privé (mode 700) de `$XDG_RUNTIME_DIR`, **effacé à la fermeture**. Rien n'est exporté.

## Limites

- Les 100 dernières entrées sont reprises au démarrage, puis le suivi est en direct. Pas de recherche dans l'historique : `journalctl --since …` reste l'outil pour cela.
- Une seule unité à la fois, pas de filtre par champ (`_PID=`, `_UID=`…).
- Le filtre texte agit sur l'affichage : le journal de session garde tout ce qui a été lu.
