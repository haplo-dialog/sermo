# Versionnage — sermo

[English](VERSIONING.en.md)

sermo suit un **versionnage sémantique** `MAJEUR.MINEUR.CORRECTIF` :

- **MAJEUR** — rupture de compatibilité du langage XML, ou du contrat entre le cœur
  et les backends (`contract/sermo-contract.h`, fonctions `sermo_be_*`).
- **MINEUR** — nouvelle capacité rétro-compatible (widget, action, option, port).
- **CORRECTIF** — correction sans changement d'interface.

## Version courante

`2.7.3`, la **première version 2.x publiée**. Ce que chaque version apporte :
[CHANGELOG.md](CHANGELOG.md).

La ligne 2.x a commencé à `2.0.0`, version restée interne, comme toutes les
versions jusqu'à la 2.7.2. Le passage à 2 tient au nouveau contrat entre le cœur et
les backends : un backend de la lignée 1.x ne se lie pas au cœur 2.x. Le langage
XML, lui, n'a pas été rompu : les scripts écrits pour gtkdialog ou sermo 1.x
restent valides. Les comportements qui ont changé sont listés dans
[MIGRATION.md](MIGRATION.md).

## La 1.x

En 1.x, chaque port avait son numéro : gtk3sermo et gtk4sermo 1.1.4, qt6sermo
1.0.2. La 2.x donne un seul numéro au cœur et aux sept ports.

## Où le numéro est écrit — un seul endroit

Le fichier **`VERSION`**, à la racine, est la seule source. Les huit projets
CMake (le cœur et les sept backends) le lisent avant `project(...)` :

```cmake
file(READ "${CMAKE_CURRENT_SOURCE_DIR}/../VERSION" SERMO_VERSION)
string(STRIP "${SERMO_VERSION}" SERMO_VERSION)
project(gtk3sermo VERSION ${SERMO_VERSION} LANGUAGES C)
```

De là, le numéro descend tout seul : `sermocore.pc`, les `config.h` engendrés
depuis leur `config.h.in`, et les pages de manuel substituées à la construction.

Bumper une version demande donc de changer **deux** fichiers : `VERSION` et
`debian/changelog` (qui porte en plus la révision Debian). Le banc
[`tests/garde_version.sh`](tests/garde_version.sh) refuse tout écart : un
`project()` qui reprendrait un numéro en dur, un `config.h` réécrit à la main,
une page de manuel non substituée, ou un `debian/changelog` en retard.

Avant la 2.7.1, ce numéro était recopié dans quatre `CMakeLists.txt`, deux
`config.h` écrits à la main — dont le commentaire demandait de les « garder en
accord » — et les pages de manuel. Rien ne vérifiait cet accord : un oubli ne se
voyait pas à la compilation, le binaire sortait en annonçant un numéro faux.

## Cœur, backends et paquets

- Le cœur (`libsermocore`) et les sept backends portent le **même numéro** tant
  qu'ils sont livrés ensemble. Un changement incompatible du contrat
  `sermo-contract.h` impose un bump MAJEUR, car un backend plus ancien ne se lierait
  plus au cœur.
- `sermocore.pc` porte la version (`Version: 2.7.3`) : un backend construit à part
  peut exiger un cœur minimal avec
  `pkg_check_modules(SERMOCORE REQUIRED sermocore>=2.7)`.
- Les paquets Debian ajoutent une révision : `2.7.3-1` est la première
  construction Debian de la 2.7.3.

## Compatibilité des scripts

Les scripts XML écrits pour gtkdialog ou sermo 1.x restent valides. Pour les
scripts qui appellent la **commande** `gtkdialog`, installer le paquet séparé
`sermo-gtkdialog` (lien vers `gtk3sermo`).
