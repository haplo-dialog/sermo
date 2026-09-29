# Contribuer à sermo

[English](CONTRIBUTING.en.md)

Merci de votre intérêt. sermo sépare un **cœur** durci (`libsermocore`) et des
**backends de rendu** autonomes, un par bibliothèque graphique.

sermo a **un seul mainteneur** : une relecture peut prendre du temps. Pour un
changement important, ouvrez d'abord un ticket pour en parler ; cela évite un
travail qui ne pourrait pas être fusionné.

## Signaler un défaut

- Ouvrez un ticket sur <https://gitlab.com/haplo-dialog/sermo/-/issues> avec un
  script XML minimal qui reproduit le défaut, la commande lancée (`gtk3sermo`,
  `qt6sermo`…) et sa sortie `--version`.
- Sans compte GitLab, le même rapport est bienvenu à `devel@haplo-dialog.fr`.
- **Une faille de sécurité ne va pas dans un ticket public** : voir
  [SECURITY.md](SECURITY.md).

## Avant de commencer

- Lisez le [Manuel développeur](MANUEL_DEVELOPPEUR.md) — architecture
  cœur/backend, variantes de build, anatomie d'un backend.
- Lisez [SECURITY.md](SECURITY.md) — les règles de sécurité ne sont **pas**
  négociables.
- Le [Code de conduite](CODE_OF_CONDUCT.md) s'applique aux tickets, aux demandes
  de fusion et aux courriels.

## Où envoyer une contribution

- **Une demande de fusion** sur GitLab :
  <https://gitlab.com/haplo-dialog/sermo/-/merge_requests>. La CI y joue les deux
  scripts décrits plus bas.
- **Sans compte GitLab** : des correctifs produits par `git format-patch`, envoyés
  à `devel@haplo-dialog.fr`.
- Le dépôt a une copie sur GitHub ; les contributions passent par GitLab ou par
  courriel. La copie joue les mêmes bancs de son côté
  (`.github/workflows/bancs.yml`, jumeau de `.gitlab-ci.yml`) : une demande qui
  y arriverait quand même est éprouvée, et le miroir ne peut pas diverger en
  silence.

## Signer ses commits (DCO)

Chaque commit porte une ligne `Signed-off-by:` avec votre nom et une adresse où
vous joindre. `git commit -s` l'ajoute. Par cette ligne, vous certifiez ce que dit
le **Developer Certificate of Origin 1.1** (<https://developercertificate.org/>),
en résumé :

- la contribution est de vous ; ou elle reprend un travail sous une licence libre
  qui vous permet de la soumettre ; ou quelqu'un qui a certifié la même chose vous
  l'a transmise sans que vous la modifiiez ;
- vous la soumettez sous la licence du fichier qu'elle touche (en général
  GPL-2.0-or-later, voir [LICENCES.md](LICENCES.md)) ;
- vous acceptez que la contribution et votre signature, nom et adresse compris,
  restent publiques dans l'historique du projet.

Le texte qui fait foi est celui du site. Un commit sans signature n'est pas
fusionné. Votre nom reste celui de l'auteur du commit.

## La règle d'or : les bancs

Toute contribution doit laisser **les trois portes au vert** pour le ou les
backends touchés :

```sh
TIMEOUT=5 bash tests/xml/run_tests.sh <BIN>                       # 55 PASS
bash tests/comportement/run.sh <BIN>                              # 53 au vert
xvfb-run -a bash tests/garde_echappement_sortie.sh <BIN>          # OK — 4 cas
```

Une demande de fusion qui rougit un banc n'est pas fusionnée. Si vous ajoutez une
capacité, ajoutez le cas de test qui la vérifie.

### Tout rejouer, comme la CI

La CI du dépôt ne fait rien d'autre que lancer deux scripts. Chez vous, sur une
Debian testing avec les paquets de [`ci/dependances.txt`](ci/dependances.txt) :

```sh
bash ci/construire.sh      # le cœur et les sept backends (ou quelques-uns : ci/construire.sh qt6 ncurses)
bash ci/bancs.sh           # tous les bancs ; un journal par banc dans _journaux/
```

`ci/bancs.sh` joue tout avant de conclure, affiche une ligne par banc, et rend 1
dès qu'un banc est rouge, figé, ou n'a rien vérifié. Il contrôle aussi qu'aucun
chemin personnel (`/home/<vous>/`) ni en-tête d'horodatage de poste n'entre dans
le dépôt.

## Où va le code

- **Comportement, grammaire, exécution, sécurité** → dans le **cœur**
  (`libsermocore`). C'est partagé par tous les backends : une correction y
  profite à tous.
- **Rendu d'un widget, glue toolkit** → dans le **backend** concerné
  (`sermo-backend-<t>/src/widget_*.c`).
- Dans le doute, préférez le cœur : la parité entre backends est l'objectif.
- **gtk3sermo est l'étalon** : un autre port qui s'en écarte se corrige vers lui.
  On ne change pas un fichier `.attendu` pour qu'il colle à un port.

## Style et sécurité

- C `-std=gnu11`, durcissement obligatoire (voir [COMPILE.md](COMPILE.md) §4).
- **Jamais** `system`/`popen`/`strcpy`/`strcat`/`sprintf`/`gets` — `safe_*` et
  bornes ; ni `atof`/`strtod`, qui dépendent de la langue du système
  (`g_ascii_strtod`). `tests/garde_fonctions_interdites.sh` le vérifie.
- En-tête SPDX sur chaque nouveau fichier : `GPL-2.0-or-later` pour le code et la
  documentation ; `CC0-1.0` accepté pour un test ou un exemple écrit de zéro.
- Écrivez du code qui ressemble au code voisin (nommage, densité de commentaires).

## Proposer un nouveau backend

Voir le [Manuel développeur](MANUEL_DEVELOPPEUR.md) §5. En résumé : fournir les
`widget_*.c`, `tag_set_property.c`, le point d'entrée `sermo_backend_toolkit_init`,
un `CMakeLists.txt` qui consomme `sermocore` par pkg-config, puis passer les
bancs.

## Contact

`devel@haplo-dialog.fr`.
