# Convention des scripts métier et des boîtes à outils

Chaque script de `examples/metiers/<NN-métier>/` et de
`examples/boites-a-outils/<boîte>/` suit ces règles. Elles servent
deux buts : un exemple qu'on peut copier sans risque, et un **test de réussite
de chaque port** (`verifie.sh` joue chaque dialogue sur les neuf ports).

## 1. Fichier

- `#!/bin/bash`, puis `# SPDX-License-Identifier: CC0-1.0` (domaine public, comme
  `examples/showcase/`).
- Nom : `N-sujet.sh` (N de 1 à 5), exécutable.

## 2. En-tête (lu par `verifie.sh`)

```
# Métier   : Administrateur système
# Besoin   : sauvegarder un dossier vers un disque, en voyant d'abord ce qui partira
# Enrobe   : rsync
# Ports    : gtk3sermo conseillé ; ncursessermo par SSH
# Terminal : rien de perdu (ou : ce qu'on perd en ncurses)
# Usage    : ./1-sauvegarde.sh [binaire sermo]
#            SERMO_A_BLANC=0 pour agir pour de vrai (défaut : la commande est affichée)
```

## 3. Structure

- Le premier argument est le binaire sermo (défaut : `$SERMO`, sinon `sermo`).
- **Chaque dialogue est une fonction `dialogue_<nom>`** qui écrit son XML sur la
  sortie standard. Elle marche sans argument (valeurs de démonstration) ; elle
  peut en prendre pour afficher des données. Elle ne lance **aucune commande**
  (pas de `$(…)` qui calcule) : les données arrivent en argument, ou par
  `<input>` dans le XML.
- La fenêtre contient **un seul `<vbox>` racine** : le banc y glisse un minuteur
  qui ferme le dialogue tout seul.
- Le script se termine par `[ "${BASH_SOURCE[0]}" = "$0" ] && main "$@"` : le banc
  peut le **lire** sans le lancer.
- Toute valeur insérée dans le XML passe par `xml` (échappe `& < > "`).
  Le remplacement s'écrit entre guillemets (`${1//&/"&amp;"}`) : depuis bash 5.2,
  un `&` nu dans le remplacement vaut le motif trouvé, et `<` devenait `<lt;`.
  sermo décode ces entités : la variable rendue est la valeur d'origine.

## 4. Sécurité

- Les valeurs reviennent par **`--do`** : le script se rappelle lui-même et lit
  les variables dans l'environnement. Jamais `eval` sur la sortie de sermo.
  Forme exacte :
  ```
  export SERMO_SCRIPT="$0"                       # en tête du script
  "$SERMO" --program=DIALOGUE --do='"$SERMO_SCRIPT" --suite <étape>'
  ```
  C'est le `/bin/sh` de sermo qui lit la variable. `printf '%q' "$0"` ne convient
  pas : en locale C (ncurses par SSH, cron), un chemin accentué devient `$'…'`,
  que dash ne comprend pas, et l'outil ne fait rien, sans un mot.
- Une commande passée à un **autre shell** (`ssh hôte "…"`, `at`, une commande
  `<input>` du XML) ne reçoit jamais une valeur citée par `printf %q`, pour la même
  raison (une apostrophe y devient alors une injection). Soit la valeur passe par
  l'environnement (`at` le garde ; sermo aussi), soit elle est citée par
  `shq` : `shq() { local s=${1//\'/\'\\\'\'}; printf "'%s'" "$s"; }`.
- Une valeur du script n'est jamais **collée dans une commande `<input>`** au moment
  où le XML est fabriqué : un texte multiligne passe par `<default>` (échappé par
  `xml`), ou par `\$VARIABLE` / `printenv VARIABLE` lus par sermo.
- Un **chemin saisi** est normalisé (`realpath -e`, ou `-m` pour un chemin à créer)
  AVANT d'être comparé à une liste de dossiers interdits ou permis : sinon `/etc/`,
  `//etc` ou `/srv/../etc` passent.
- On teste `EXIT` : seul le bouton d'action vaut accord ; `abort`, `closewindow`
  ou `Cancel` valent abandon.
- **Toute action qui modifie quelque chose passe par `agir`.** Par défaut
  (`SERMO_A_BLANC=1`), `agir` affiche la commande au lieu de la lancer. Une action
  destructive demande en plus une confirmation dans un second dialogue.
- Les `<input>` ne font que **lire** (lister, afficher un état), jamais modifier.
- Un prérequis manquant (commande absente) : le script le dit et s'arrête, avec le
  code 1, avant d'ouvrir une fenêtre.
- Ni `sudo` ni `pkexec` dans le script : s'il faut des droits, le script le dit et
  affiche la commande à lancer.

## 5. Boîtes à outils

`examples/boites-a-outils/<boîte>/` : un `lanceur.sh` et dix outils
`NN-<outil>.sh` (01 à 10). L'en-tête commence par `# Boîte    : <Nom>` au lieu
de `# Métier`. Le lanceur présente les outils, un bouton par outil
(`exit:NN-outil`) ; sa suite vérifie EXIT contre `^[0-9][0-9]-[a-z0-9-]+$` et
l'existence du fichier, lance l'outil, puis se rouvre.

## 6. Ce que le banc vérifie (`verifie.sh`)

Pour chaque script :
1. `bash -n`, en-tête complet, présence de `main` et de la garde de fin ;
2. chaque `dialogue_*` s'analyse sur les neuf ports (`--print-ir`, code 0) ;
3. chaque `dialogue_*` est **joué** sur chaque port (minuteur glissé dans le
   `<vbox>` racine, commandes du XML bloquées) : les variables rendues doivent être
   **les mêmes que celles de gtk3sermo**, l'étalon ;
4. en option (`--images DIR`), une image par dialogue, pour la relecture.
