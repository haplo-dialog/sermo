# Versioning & conventions Git - haplo-dialog

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

1. la **syntaxe XML** (`haplo-dialog-xml(5)`) interprétée par `gtk3dialog` ;
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

haplo-dialog ne distribue qu'un **seul port**, **`gtk3dialog`** (backend GTK 3) :
c'est le **port de référence** (43 widgets), qui fournit l'alias rétro-compatible
**`gtkdialog`**. Il n'y a donc **qu'un seul numéro de version** à faire évoluer.

### Emplacements à mettre à jour (check-list de release)

Une montée de version doit toucher les fichiers suivants, c'est la check-list
de release :

| Emplacement | Fichier(s) | Champ |
|-------------|-----------|-------|
| Build autotools | `gtk3dialog/…/configure.ac` | `AC_INIT([gtk3dialog], [X.Y.Z], …)` |
| Build CMake | `gtk3dialog/…/CMakeLists.txt` | `project(gtk3dialog VERSION X.Y.Z …)` |
| **Nom du dossier** | `gtk3dialog/gtk3dialog_X.Y.Z/` | la version est **dans le chemin** |
| Arch | `packaging/arch/PKGBUILD` + `.SRCINFO` | `pkgver=X.Y.Z` |
| RPM | `packaging/rpm/gtk3dialog.spec` | `%global version X.Y.Z` |
| Gentoo | `packaging/gentoo/gtk3dialog-X.Y.Z.ebuild` | version **dans le nom de fichier** |
| Doc | `CHANGELOG.md`, `SECURITY.md`, `NEWS`, badges `site-web/` | tableaux & en-têtes |

> ⚠️ **Particularité** : la version est encodée dans le **nom du dossier**
> (`gtk3dialog_1.0.0/`) et dans le **nom de l'ebuild**
> (`gtk3dialog-1.0.0.ebuild`). Une montée de version implique donc un `git mv`
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

Le style pratiqué est **`portée: résumé impératif`** (français), pas Conventional
Commits strict. Exemples du style pratiqué :

```
gtk3dialog: uniformise vers v1.0.0 (gtk3d → gtk3dialog)
site-web: uniformise noms canoniques + v1.0.0
ci: applique le contenu canonique de Dockerfile.gtk3dialog
docs: ajoute DEPENDENCIES.md (dépendances réelles)
tests: ajoute les tests de comportement safe_exec
```

### Règles

1. **Portée** = le périmètre touché. Valeurs usuelles :
   - le port : `gtk3dialog`
   - transversal : `docs`, `site-web`, `ci`, `packaging`, `tests`, `core`, `build`, `release`
2. **Résumé** à l'impératif présent, en minuscule, sans point final, ≤ ~70 caractères.
3. **Corps** (optionnel) séparé par une ligne vide : explique le *pourquoi*,
   liste les points notables, mentionne les invariants préservés.
4. **Un commit = un changement cohérent.** On ne mélange pas un renommage et
   l'ajout d'une fonctionnalité.

> *Évolution possible :* adopter plus tard les préfixes typés Conventional
> Commits (`feat:`, `fix:`, `docs:`…) pour automatiser la génération du
> CHANGELOG. Non requis aujourd'hui.

---

## 5. Branches

État actuel : développement **trunk-based** sur **`main`** (branche unique,
toujours compilable).

Convention recommandée pour la suite :

| Préfixe | Usage | Exemple |
|---------|-------|---------|
| `main` | tronc stable, taggable | — |
| `feature/…` | nouveau widget / fonctionnalité | `feature/gtk3-colorpicker` |
| `fix/…` | correction ciblée | `fix/deb-packaging` |
| `release/X.Y` | stabilisation d'une release (si besoin) | `release/1.1` |

Les branches de travail sont **courtes** et fusionnées dans `main` une fois
vertes (build OK). `main` ne doit jamais être cassée.

---

## 6. Tags

- Format : **`vX.Y.Z`** (préfixe `v`), **tag annoté** (`git tag -a`), signé si
  une clé est disponible.
- Un tag pointe sur le commit où le CHANGELOG, les `configure.ac`/`CMakeLists.txt`
  et le packaging déclarent tous `X.Y.Z`.
- État actuel : **aucun tag** dans le dépôt. Le premier à poser est `v1.0.0`.

```sh
# Poser la release courante :
git tag -a v1.0.0 -m "haplo-dialog 1.0.0, première version publique stable"
git push origin v1.0.0
```

---

## 7. Procédure de release (check-list)

Pour publier `X.Y.Z` :

1. **Geler** : s'assurer que `main` compile et que les tests passent
   (`./ci/build.sh gtk3dialog --test`).
2. **Bumper la version** dans tous les emplacements de la §2 :
   `configure.ac`, `CMakeLists.txt`, dossier `gtk3dialog_X.Y.Z/`, `PKGBUILD` + `.SRCINFO`,
   `*.spec` (`%global version`), ebuild renommé `gtk3dialog-X.Y.Z.ebuild`.
3. **Clore le CHANGELOG** : `## [Unreleased]` → `## [X.Y.Z], AAAA-MM-JJ`, et
   recréer une section `[Unreleased]` vide pour la suite.
4. **Mettre à jour** `SECURITY.md` (tableau des versions supportées), `NEWS`,
   badges `site-web/`.
5. **Commit** : `release: vX.Y.Z`.
6. **Tag annoté** : `git tag -a vX.Y.Z -m "haplo-dialog X.Y.Z"`.
7. **Publier** : pousser commit + tag, construire les paquets distro.

Un script `ci/bump-version.sh` (à écrire) pourra automatiser l'étape 2, le
périmètre exact des fichiers est fixé par la table de la §2.

---

*haplo-dialog est un fork de gtkdialog 0.8.3 (László Pere, GPL-2.0+),*
*modernisé par le projet haplo-dialog. SemVer appliqué depuis la 1.0.0.*

---

