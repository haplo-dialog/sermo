# contrib/ — ce qui accompagne sermo sans en faire partie

Ce dossier reçoit des outils écrits **avec** sermo. Ils ne sont ni construits,
ni empaquetés, ni joués par la CI : un outil d'ici peut demander ce que la CI
n'a pas (systemd, un vrai terminal, des droits). Chacun dit ce qui a été essayé.

## trousses/ — des outils d'administration, une charte, deux hubs

| Pièce | Rôle |
|---|---|
| `trousses/CHARTE.md` | les règles d'un outil de trousse : enrober une commande de confiance, refuser proprement, motif `--do` / `--print`, pas d'image dans le tronc commun, contrat `--manifest` / `--check` |
| `trousses/servys/` | services systemd : liste, état, démarrer, arrêter, redémarrer (enrobe `systemctl`) |
| `trousses/hub-simple/` | lanceur : découvre les scripts de `modules/` qui répondent à `--manifest` |
| `trousses/hub-groupe/` | lanceur groupé, pastille de santé par `--check`, politique `groupes.conf` |
| `trousses/hub-simple/modules/info.sh` | informations système, en lecture seule |

Dans les deux hubs, `modules/servys.sh` est un lien vers `trousses/servys/servys.sh`.

Essayer :

```sh
bash contrib/trousses/servys/servys.sh gtk3sermo
bash contrib/trousses/hub-groupe/hub.sh qt6sermo
```

## État mesuré le 2026-10-04

Essais faits avec les sept ports construits depuis ce dépôt, sans rien démarrer
ni arrêter sur la machine.

| Essai | Résultat |
|---|---|
| La fenêtre s'ouvre (`tests/run_examples.sh`, écran virtuel) : servys, hub simple, hub groupé | **oui** sur gtk3, gtk4, qt6, fltk1, efl1, sdl3 |
| `servys.sh --print list`, `--print status`, `--manifest`, `--check` | rendent ce qu'ils annoncent ; un nom d'unité forgé est refusé |
| Les hubs trouvent leurs modules (`--print list`) | oui, liens compris |
| **Hub dans un terminal** (ncurses, pseudo-terminal) | **non** : le hub sort aussitôt, sans rien afficher |
| servys dans un terminal, pour de vrai | pas essayé |
| Démarrer, arrêter, redémarrer un service ; l'élévation | pas essayé |

## Ce qui manque pour tenir la promesse « en fenêtre et en terminal »

Ces outils veulent montrer la même interface en fenêtre et dans un terminal.
Trois points l'empêchent encore, tous du côté de sermo, pas des scripts :

1. **Une sortie capturée éteint l'écran ncurses.** Les hubs écrivent
   `sortie=$(choisir)` pour relire le choix. Quand sa sortie standard n'est pas
   un terminal, `ncursessermo` passe en mode sans écran : il rend les variables et
   sort. C'est aussi le cas de l'écriture courante `eval "$(… --program=…)"`.
2. **Les actions d'une liste ne sont jouées que par les ports GTK.** Dans servys,
   choisir un service devrait rafraîchir le panneau d'état. Les ports qt6, fltk1,
   efl1, sdl3 et ncurses ne lisent pas les `<action>` d'une `<list>` (lu dans leur
   code, pas essayé au clic) : il y faut passer par le bouton « Rafraîchir ».
3. **`<input>bash "$SCRIPT_PATH" …</input>` passe par `/bin/sh`.** Les guillemets
   sont de la syntaxe shell : sermo le dit sur la sortie d'erreur à chaque appel,
   et un réglage qui refuse le repli vers le shell refuserait ces outils.

Tant que ces points ne sont pas levés, ces outils sont à lire comme des exemples
de la charte, pas comme la preuve de la parité.

## Licence

Les scripts de `trousses/` sont sous CC0-1.0 (en-tête de chaque fichier), comme
les exemples écrits pour sermo. La charte et les LISEZ-MOI aussi.
