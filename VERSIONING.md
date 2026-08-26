# Versionnage & conventions Git — sermo

Ce document fixe la manière dont le projet est **versionné**, **changé** et
**publié**. Il décrit les conventions *réellement pratiquées* dans le dépôt et
les formalise pour la suite.

---

## 1. Schéma de version - SemVer 2.0.0

Depuis la **1.0.0**, le projet suit [Semantic Versioning 2.0.0](https://semver.org/lang/fr/) :

```
MAJEUR . MINEUR . CORRECTIF        (ex. 1.2.0)
```

| Incrément | Quand | Exemple pour haplo-dialog |
|-----------|-------|---------------------------|
| **MAJEUR** | rupture de compatibilité | syntaxe XML incompatible, suppression d'un widget, changement d'interface CLI du binaire |
| **MINEUR** | ajout rétro-compatible | nouveau widget, nouvelle action, nouvelle option `--…` |
| **CORRECTIF** | correction rétro-compatible | bug fix, durcissement sécurité, correction de packaging |

La **« surface publique »** qui définit la compatibilité est :

1. la **syntaxe XML** (`haplo-dialog-xml(5)`) interprétée par `gtk3sermo` ;
2. le **comportement en ligne de commande** du binaire (`--program`, `--file`, variables d'environnement exportées…).

Les détails d'implémentation internes (organisation interne du code C)
ne font **pas** partie du contrat de version.

### Historique pré-1.0

| Version | Nature |
|---------|--------|
| `gtkdialog 0.8.3` | upstream original (László Pere / Thunor), GPL-2.0+ |
| `0.9.0-haplo1` | version de travail **interne** du fork (2026-03-01), non publiée |
| `1.0.0` | première version publique stable (2026-05-29) |

Avant 1.0.0, aucune garantie de compatibilité n'était offerte (phase `0.y`).

---

## 2. Où vit le numéro de version (sources de vérité)

haplo-dialog distribue **deux ports** : **`gtk3sermo`** (backend GTK 3), le port
de référence, et **`gtk4sermo`** (backend GTK 4). L'alias rétro-compatible
**`gtkdialog`** est fourni par un troisième paquet, **`gtksermo`**.

La version amont (`1.1.0`) est **commune aux deux ports** : c'est le même cœur, la
même grammaire. En revanche la **révision d'empaquetage** avance port par port,
puisqu'un correctif ne touche pas toujours les deux. Les deux numéros peuvent donc
diverger — `gtk3sermo 1.0.0-10` et `gtk4sermo 1.0.0-11` par exemple — et c'est
normal. Chaque port a son `debian/changelog`, qui fait foi pour lui.

### Emplacements à mettre à jour (check-list de release)

Une montée de version doit toucher les fichiers suivants, c'est la check-list
de release :

`<port>` vaut `gtk3sermo` ou `gtk4sermo` : sauf mention contraire, chaque ligne
est à faire **deux fois**.

| Emplacement | Fichier(s) | Champ |
|-------------|-----------|-------|
| Build autotools | `<port>/…/configure.ac` | `AC_INIT([<port>], [X.Y.Z], …)` |
| Build CMake — **gtk3sermo seulement** | `gtk3sermo/…/CMakeLists.txt` | `project(gtk3sermo VERSION X.Y.Z …)` |
| **Nom du dossier** | `<port>/<port>_X.Y.Z/` | la version est **dans le chemin** |
| Debian | `<port>/…/packaging/debian/changelog` | `<port> (X.Y.Z-N)` — voir ci-dessous |
| Arch | `<port>/…/packaging/arch/PKGBUILD` + `.SRCINFO` | `pkgver=X.Y.Z` |
| RPM | `<port>/…/packaging/rpm/<port>.spec` | `%global version X.Y.Z` |
| Gentoo — ebuild | `<port>/…/packaging/gentoo/<port>-X.Y.Z.ebuild` | version **dans le nom de fichier** (`git mv`) |
| Gentoo — Manifest | `<port>/…/packaging/gentoo/Manifest` | `DIST sermo-vX.Y.Z.tar.gz` + les deux commentaires |
| Gentoo — mode d'emploi | `<port>/…/packaging/gentoo/README.gentoo.md` | les deux commandes citent le nom de l'ebuild |
| Slackware — recette | `<port>/…/packaging/slackware/<port>.SlackBuild` | `VERSION=${VERSION:-X.Y.Z}` |
| Slackware — mode d'emploi | `<port>/…/packaging/slackware/README.slackware.md` | le nom du `.tgz` produit, 4 lignes |
| Script de build | `ci/build.sh` | `src_dir()` code le **chemin versionné** des deux ports |
| Image Docker | `ci/Dockerfile.gtk3sermo` | `COPY` du chemin versionné + `LABEL …image.version` |
| Manuel info | `<port>/doc/version.texi` | `@set EDITION` / `@set VERSION` |
| Doc | `CHANGELOG.md`, `CHANGELOG.en.md`, `SECURITY.md`, `SECURITY.en.md`, `NEWS`, badges des `README` | tableaux & en-têtes |

Le modèle RPM `<port>/<port>.spec.in` (ou `gtkdialog.spec.in`) n'est **pas** dans
cette liste : depuis la 1.1.0 il écrit `%define version @VERSION@`, que `configure`
remplit depuis `AC_INIT`. Il suit donc tout seul. Le `.spec` à la racine du port
est un fichier **produit** — ne jamais l'éditer à la main.

Le port GTK 4 n'a **pas** de `CMakeLists.txt` : il se construit par autotools
seulement. Le modèle `.cmake` du `.gitlab-ci.yml` n'est donc étendu par aucun job.

> ⚠️ **Particularité** : la version est encodée dans le **nom du dossier**
> (`gtk3sermo_1.1.0/`) et dans le **nom de l'ebuild**
> (`gtk3sermo-1.1.0.ebuild`). Une montée de version implique donc un `git mv`
> de ces chemins. *(Piste future : découpler la version du nom de dossier pour
> alléger les bumps.)*

Côté packaging, le **plus simple à bumper** est le `.spec` RPM (`%global version`
en tête) ; le **plus lourd** est Gentoo (renommage de fichier) et le dossier source.

---

## 3. Journal des modifications - Keep a Changelog

`CHANGELOG.md` suit [Keep a Changelog](https://keepachangelog.com/fr/1.0.0/).

- Les changements en cours s'accumulent sous **`## [Unreleased]`** (avec la cible,
  ex. *« v1.1.0 (en cours) »*).
- Au moment de publier, `[Unreleased]` devient `## [X.Y.Z], AAAA-MM-JJ`.
- Sous-sections autorisées (en français) :
  **Ajouté · Modifié · Corrigé · Supprimé · Déprécié · Sécurité**.
- Les entrées décrivent le **pourquoi/le quoi pour l'utilisateur**, pas le diff.

Les anciennes entrées (`0.9.0-haplo1`, `gtkdialog-0.8.3`) sont **préservées
telles quelles**, elles documentent la lignée et ne sont jamais réécrites.

---

## 4. Conventions de commit

**La référence est `CONTRIBUTING.md`, §« Format des messages de commit ».** Ce
document ne la redit pas, pour ne pas créer une deuxième version qui dérivera.

En deux lignes : le sujet est une **phrase française** de moins de 72 caractères
qui dit **ce qui changeait de comportement**, du point de vue de qui s'en sert —
pas quel fichier a bougé, et pas de préfixe typé. Le corps porte le pourquoi et
la mesure.

```
La barre de progression faisait tourner GTK depuis son thread de lecture
Les quatre recettes non-Debian téléchargeaient une source qui n'existe pas
Le durcissement annoncé n'était pas tout entier dans le binaire
```

`git log --oneline -20` donne le ton mieux qu'une règle écrite.

Ce document a longtemps prescrit un style `portée: résumé impératif`, et
`CONTRIBUTING.md` un style *conventional commits* en anglais. Ni l'un ni l'autre
n'a jamais été pratiqué : l'historique est fait de phrases françaises depuis le
premier commit. Les deux documents disent maintenant la même chose, et c'est
celle-là.

---

## 5. Branches

État actuel : développement **trunk-based** sur **`main`** (branche unique,
toujours compilable).

`main` est aujourd'hui la **seule** branche du dépôt : aucune branche de travail
n'a jamais existé dans l'historique. Ce n'est pas un oubli, c'est le mode de
développement.

Pour une contribution extérieure, le nom de branche est libre — qu'il décrive le
sujet suffit (`liste-plante-sur-item-vide`). Il n'y a **pas** de préfixe imposé :
en prescrire un que personne n'utilise ne ferait qu'ajouter une règle morte de
plus.

Les branches de travail sont **courtes** et fusionnées dans `main` une fois
vertes (le pipeline passe). `main` ne doit jamais être cassée.

---

## 6. Tags

- Format : **`vX.Y.Z`** (préfixe `v`), **tag annoté** (`git tag -a`), signé si
  une clé est disponible.
- Un tag pointe sur le commit où le CHANGELOG, les `configure.ac`/`CMakeLists.txt`
  et le packaging déclarent tous `X.Y.Z`.
- État actuel : **cinq étiquettes** posées — `v1.0.0`, `v1.0.0-2`, `v1.0.0-3`,
  `v1.0.0-4` et `v1.1.0`.
- Deux d'entre elles seulement ont une *release* GitLab avec des paquets joints :
  - `v1.0.0` porte les paquets **défectueux** (`1.0.0-10`, `1.0.0-11`), gardés
    en ligne pour l'historique — le `README` dit de ne pas les installer ;
  - `v1.0.0-4` porte les mêmes paquets **corrigés** (`1.0.0-11`, `1.0.0-12`).
- `v1.0.0-2` et `v1.0.0-3` sont des états intermédiaires du dépôt : aucune
  release ne leur est attachée.
- `v1.1.0` est la version courante ; ses paquets (`1.1.0-1`) remplacent tous
  les précédents.

```sh
# Poser une release :
git tag -a vX.Y.Z -m "sermo X.Y.Z"
git push gitlab vX.Y.Z
```

---

## 7. Procédure de release (check-list)

Pour publier `X.Y.Z` :

1. **Geler** : s'assurer que `main` compile et que les tests passent, **dans les
   deux ports** — `./ci/build.sh gtk3sermo --test` puis
   `./ci/build.sh gtk4sermo --test`, ou simplement attendre que le pipeline
   GitLab soit vert : il rejoue les deux ports, les sept garde-fous et les
   exemples réels sous Xvfb en locale française.
2. **Bumper la version** dans tous les emplacements de la §2, **pour chaque
   port** : `configure.ac`, `CMakeLists.txt` (gtk3sermo seulement), dossier
   `<port>_X.Y.Z/`, `PKGBUILD` + `.SRCINFO`, `*.spec` (`%global version`),
   ebuild renommé, `SlackBuild`, et une entrée dans chaque `debian/changelog`.
3. **Clore le CHANGELOG** : `## [Unreleased]` → `## [X.Y.Z], AAAA-MM-JJ`, et
   recréer une section `[Unreleased]` vide pour la suite.
4. **Mettre à jour** `SECURITY.md` (tableau des versions supportées), `NEWS`,
   badges `site-web/`.
5. **Commit**, sujet en phrase française, comme le reste de l'historique.
6. **Tag annoté** : `git tag -a vX.Y.Z -m "sermo X.Y.Z"`.
7. **Publier** : pousser commit + tag, construire les paquets distro.

Un script `ci/bump-version.sh` (à écrire) pourra automatiser l'étape 2, le
périmètre exact des fichiers est fixé par la table de la §2.

---

*haplo-dialog est un fork de gtkdialog 0.8.3 (László Pere, GPL-2.0+),*
*modernisé par le projet haplo-dialog. SemVer appliqué depuis la 1.0.0.*

---

