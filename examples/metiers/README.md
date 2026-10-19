# Scripts métier

[English](README.en.md)

Soixante scripts prêts à lancer, cinq par métier, qui montrent à quoi sert sermo dans
un vrai travail : sauvegarder un dossier, déclarer une note de frais, saisir des
mesures à l'atelier, convertir une vidéo, régler un serveur par SSH…

Ils ont un second rôle : ce sont le **test de réussite de chaque port**.
`verifie.sh` joue chacun de leurs dialogues sur les neuf ports, et vérifie que
chaque port rend les mêmes valeurs que gtk3sermo, l'étalon.

## Les douze métiers

| Dossier | Métier | Port conseillé |
|---|---|---|
| `01-admin-systeme` | administrateur système | gtk3sermo ; ncursessermo par SSH |
| `02-developpeur-devops` | développeur, DevOps | gtk3sermo, qt6sermo |
| `03-support-informatique` | support informatique | gtk3sermo |
| `04-distribution-integrateur` | distribution, intégrateur | gtk3sermo, gtk4sermo |
| `05-bornes-ecrans` | bornes et écrans dédiés | lvglsermo |
| `06-atelier-industrie-labo` | atelier, industrie, laboratoire | lvglsermo, fltk1sermo |
| `07-pme-association` | PME, association, secrétariat | gtk3sermo, htmlsermo |
| `08-enseignement-formation` | enseignement, formation | gtk3sermo, htmlsermo |
| `09-multimedia` | multimédia | qt6sermo, gtk3sermo |
| `10-securite-informatique` | sécurité informatique | gtk3sermo, ncursessermo |
| `11-serveur-sans-ecran` | serveur sans écran | ncursessermo |
| `12-web-local-intranet` | web local, intranet | htmlsermo |

Chaque script dit en tête son métier, son besoin, la commande qu'il enrobe, le port
conseillé et ce qu'on perd dans un terminal.

## Les lancer sans risque

```sh
examples/metiers/01-admin-systeme/1-sauvegarde.sh gtk3sermo
```

Le premier argument est le port (défaut : `sermo`). **Par défaut, rien n'est
modifié** : la commande que le script lancerait est affichée (`à blanc : rsync …`).
Pour agir pour de vrai, après avoir lu cette commande :

```sh
SERMO_A_BLANC=0 examples/metiers/01-admin-systeme/1-sauvegarde.sh gtk3sermo
```

Une action destructive demande en plus une confirmation dans un second dialogue.
Aucun script n'appelle `sudo` ni `pkexec` : s'il faut des droits, il le dit.

## Comment ils sont écrits

Les règles sont dans [CONVENTION.md](CONVENTION.md). En bref :
- chaque dialogue est une fonction `dialogue_<nom>` qui écrit son XML ;
- les valeurs reviennent par `--do` (dans l'environnement), jamais par `eval` ;
- `EXIT` dit quel bouton a été cliqué : seul le bouton d'action vaut accord ;
- toute action passe par `agir`, qui l'affiche en mode à blanc.

Ils sont en domaine public (CC0-1.0) : copiez-les et adaptez-les. Le banc
`verifie.sh`, lui, est sous GPL-2.0 ou plus récente.

## Le banc

```sh
examples/metiers/verifie.sh                       # tous les scripts, neuf ports
examples/metiers/verifie.sh --images dossier/     # plus une image par dialogue
examples/metiers/verifie.sh --ports "gtk3 lvgl" examples/metiers/05-bornes-ecrans/*.sh
examples/metiers/verifie.sh --garder passe/ --ports ncurses   # garder la passe, pour la comparer ailleurs
examples/metiers/verifie.sh --rejouer passe/ --ports gtk3     # rejouer une passe gardée, et comparer
```

- `--garder DOSSIER` range la passe (dialogues extraits, valeurs de chaque port) dans `DOSSIER`. Sur un
  système sans GTK, avec `--ports "ncurses"`, rien n'est comparé sur place.
- `--rejouer DOSSIER` reprend une passe gardée sans réextraire : les ports de `--ports` jouent les **mêmes**
  dialogues, et leurs valeurs sont comparées à celles déjà là.

Codes de sortie : `0` tout concorde · `1` un écart · `77` rien vérifié (gtk3sermo absent, ou passe à rejouer
introuvable).

Il vérifie l'en-tête et la structure de chaque script, fait analyser chaque dialogue
par les neuf ports, puis **joue** chaque dialogue sur chaque port. Un minuteur
invisible ferme le dialogue, et les commandes du XML sont bloquées. Les valeurs
rendues doivent être celles de gtk3sermo. `ci/bancs.sh` le lance.

Ces dialogues sont aussi servis par `sermoia-mcp` (`sermo_examples`), sous un sujet
par métier.

Voir aussi les [boîtes à outils](../boites-a-outils/README.md) (système, serveur,
administrateur, utilisateur), écrites selon les mêmes règles et testées par le même
banc.
