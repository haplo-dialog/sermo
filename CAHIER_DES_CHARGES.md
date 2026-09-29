# Cahier des Charges — sermo (édition modulaire)

**Projet :** sermo (édition modulaire) — séparation du binaire monolithique en cœur + backends
**Distribution cible :** Debian Testing
**Lignée :** gtkdialog → sermo monolithique → sermo (édition modulaire) (GPL-2.0-or-later)
**Distributeur :** haplo-dialog <devel@haplo-dialog.fr>
**Statut :** Phase 1 & 2 terminées ; Phase 3 (frontière propre) en cours

---

## Table des matières

1. [Contexte](#1-contexte)
2. [Périmètre](#2-périmètre)
3. [Objectifs](#3-objectifs)
4. [Exigences fonctionnelles](#4-exigences-fonctionnelles)
5. [Exigences non fonctionnelles](#5-exigences-non-fonctionnelles)
6. [Contraintes techniques](#6-contraintes-techniques)
7. [Architecture cible](#7-architecture-cible)
8. [Livrables](#8-livrables)
9. [Jalons](#9-jalons)
10. [Critères d'acceptation](#10-critères-dacceptation)
11. [Risques](#11-risques)
12. [Chantier proposé — deux conteneurs de disposition](#12-chantier-proposé--deux-conteneurs-de-disposition)

---

## 1. Contexte

sermo (successeur moderne et durci de gtkdialog) existait comme un binaire
**monolithique par toolkit** : chaque port (gtk3, gtk4, qt6, fltk1, efl1, sdl3)
embarquait sa propre copie du lexer, du parser et de la logique d'exécution.
Cette duplication rendait coûteuse toute correction du cœur (à répercuter six
fois) et forçait chaque installation à tirer la boîte à outils du port.

L'**édition modulaire** de sermo répond à ce problème en **factorisant le cœur**.

## 2. Périmètre

**Inclus** : extraction d'un cœur commun `libsermocore` ; portage des sept
backends dessus ; packaging séparé ; documentation ; conservation stricte du
langage XML et du comportement observable.

**Exclu de la Phase 1** : refonte de la frontière d'évènements (Part 3),
cloisonnement de processus (IPC), nouveaux backends.

## 3. Objectifs

1. **Un seul cœur** partagé, durci, audité une fois.
2. **Backends légers** aux dépendances **disjointes** : installer un backend ne
   tire que sa toolkit.
3. **Parité stricte** : le même XML rend le même comportement sur tous les
   backends (mesuré contre l'étalon gtk3sermo).
4. **Rétrocompatibilité** : les scripts gtkdialog/sermo continuent de marcher.

## 4. Exigences fonctionnelles

- EF1 — Le cœur analyse le XML (lexer/parser Flex/Bison) et pilote variables,
  actions, exécution shell durcie, sans dépendance graphique (variante neutre).
- EF2 — Chaque backend fournit les widgets natifs et la boucle d'évènements de sa
  toolkit via le contrat `sermo_backend.h`.
- EF3 — Le langage XML et les valeurs exportées (`NAME="value"`) sont identiques
  sur tous les backends.
- EF4 — L'alias `sermo` (update-alternatives) est fourni ; la commande
  `gtkdialog` l'est par un **paquet séparé** `sermo-gtkdialog` (lien →
  `gtk3sermo`, `Conflicts`/`Replaces`/`Provides: gtkdialog` pour ne pas entrer en
  conflit avec le `gtk3dialog` de BunsenLabs).
- EF5 — **Disposition** : le langage doit permettre d'ALIGNER (grille) et de
  PARTAGER l'espace (poignée déplaçable), pas seulement d'empiler (`hbox`/`vbox`).
  ✅ **Tenue** : `<grid>` le 2026-09-13, `<paned>` le 2026-09-14 (sept ports)
  — [§12](#12-chantier-proposé--deux-conteneurs-de-disposition).

## 5. Exigences non fonctionnelles

- ENF1 — **Sécurité** : `safe_system`/`safe_popen`, pas de shell hors
  métacaractères, échappement de sortie, durcissement (FORTIFY=3, PIE, RELRO,
  CET) — voir [SECURITY.md](SECURITY.md).
- ENF2 — **Vérifiabilité** : trois bancs rejouables (XML 55, comportement 52,
  et les gardes `tests/garde_*.sh`) gardent chaque étape.
- ENF3 — **Légèreté** : `sermo-core-dev` ne dépend que de `libc6` ; chaque
  backend, que de sa toolkit (dépendances dérivées, non devinées).
- ENF4 — **Thème** : chaque backend suit le thème système courant.

## 6. Contraintes techniques

- C `-std=gnu11`, CMake ≥ 3.16, Flex/Bison.
- Trois variantes de build du cœur (neutre / GLib / GTK4) imposées par les
  différences d'ABI entre familles de toolkits.
- Consommation du cœur par pkg-config (`sermocore.pc`).

## 7. Architecture cible

Cœur `libsermocore` (analyse + exécution) ← contrat `sermo_backend.h` →
backends `sermo-backend-<t>` (rendu + boucle). Détail :
[MANUEL_DEVELOPPEUR.md](MANUEL_DEVELOPPEUR.md).

## 8. Livrables

- `libsermocore` (3 variantes) + `sermocore.pc`.
- 7 backends (gtk4 et ncurses inclus) ; 9 paquets `.deb` (`sermo-core-dev` +
  7 backends + `sermo-gtkdialog`).
- Jeu documentaire complet (manuels, man page, Texinfo, bilan, changelog…).

## 9. Jalons

Voir [ROADMAP.md](ROADMAP.md). Phase 1 ✅ (cœur + 7 backends + packaging) ;
Phase 2 (gtk4 24/24, doc, 9 .deb) ✅ ; Phase 3 (frontière propre) en cours
(rename `sermo_be_*` fait ; gtk4 vendoré dans `src-gtk4/`, plus de dépendance
à l'ancienne édition ; reste la source unique gtk3+gtk4 + l'IR).

## 10. Critères d'acceptation

- CA1 — Les sept backends lient et tournent sur `libsermocore`. ✅
- CA2 — Chaque backend passe les trois bancs. ✅ **7/7** à 53/53 (comportement
  rejoué par `tests/comportement/run_all.sh`), XML 55/55, gardes — ncurses joue
  le comportement en mode batch headless — [BILAN_SANTE.md](BILAN_SANTE.md).
- CA3 — Dépendances runtime disjointes, prouvées. ✅
- CA4 — Aucun banc rouge introduit par la modularisation. ✅

## 11. Risques

- **R1 — Divergence d'ABI GTK4** (matérialisé) : GTK 4 a retiré
  `gtk_main`/`gtk_socket` ; parade = variante `SERMOCORE_GTK4` dédiée, **vendorée
  dans `libsermocore/src-gtk4/`** (plus de dépendance externe). Reste à fondre en
  source unique en Phase 3.
- **R2 — Couplage cœur→backend** (`widget_*_create`, ponts `qt6_*`) : empêche une
  `.so` partagée propre ; levé par la formalisation d'une IR (Part 3).
- **R3 — Fuite d'attribution** à la publication : filtrer en-têtes de poste et
  métadonnées d'auteur avant toute copie publique.

---

## 12. Chantier proposé — deux conteneurs de disposition

> **Statut : SOLDÉ.** `<grid>` (2.2.0) et `<paned>` (2.3.0) sont
> livrés sur les sept ports — bancs 7 × 30 au vert, captures à l'appui.

### 12.1 Le manque

Le langage compte 62 types de widgets mais **deux** conteneurs de disposition :
`<hbox>` et `<vbox>`. Un formulaire à deux colonnes s'écrit en empilant des
`<hbox>` dans une `<vbox>` — et **rien n'aligne les colonnes entre elles**.
Il manque aussi toute **poignée** permettant à l'utilisateur de redistribuer
l'espace entre deux zones.

⚠️ `<table>` est déjà pris : c'est la **liste à colonnes** héritée de gtkdialog,
pas une mise en page. Le conteneur s'appelle donc `<grid>`.

### 12.2 Les deux balises

```xml
<grid columns="2" row-spacing="4" column-spacing="8">
  <text><label>Nom</label></text>      <entry><variable>NOM</variable></entry>
  <text><label>Courriel</label></text> <entry><variable>MEL</variable></entry>
</grid>

<paned orientation="horizontal" position="30%">
  <list><variable>FICHIERS</variable></list>
  <edit><variable>CONTENU</variable></edit>
</paned>
```

- `<grid>` ✅ **fait** : placement **en flot** (ordre du document, retour à la
  ligne tous les `columns` enfants). Les coordonnées explicites de cellule et
  les fusions restent une **v2** (le « point dur » — corrigé
  depuis : les attributs de l'enfant SONT atteignables par
  `find_variable_by_widget()`, sans toucher à la pile).
- `<paned>` ✅ **fait** : **exactement deux** enfants. Un troisième est
  **refusé avec un message**, jamais ignoré en silence.

### 12.3 Pourquoi ces deux-là

L'invariant du produit est « le même XML rend une interface équivalente sur les
sept ports ». Vérifié en-têtes à l'appui sur la machine de développement : la
grille et le séparateur existent sur **tous** les toolkits (`gtkgrid.h`,
`QGridLayout`, `Fl_Grid.H`, `elm_table.h`, `gtkpaned.h`, `QSplitter`,
`Fl_Tile.H`, `elm_panes.h`) ; seuls sdl3 et ncurses demandent du code.
À l'inverse, un cadran ou un graphique n'existent que sur deux toolkits :
**refusés** tant que l'invariant tient — `<drawingarea>` est là pour ça.

### 12.4 Coût, mesuré

| Repère | Total sur les 7 ports |
|---|---|
| `<levelbar>` (widget simple, ajouté récemment) | 944 lignes |
| `<frame>` (conteneur) | 1 341 lignes |
| `<notebook>` (conteneur) | 1 748 lignes |

Plus le rendu dans `render_*.c` pour les deux ports neutres. **Ordre de
grandeur retenu : ~1 500 lignes par conteneur**, soit ~3 000 lignes pour les
deux, lexer, grammaire, doc et bancs compris.

### 12.5 Ordre de travail (non négociable — appliqué pour `<grid>`)

1. Écrire les cas `28-grid`, `29-grid-flot`, `30-paned` **avant** tout code, et
   fabriquer les `.attendu` avec l'étalon gtk3 — jamais l'inverse.
2. Les jouer sur les sept ports pour **chiffrer l'écart de départ**.
3. Coder `<grid>` seul, jusqu'au vert sur les sept ports. Puis `<paned>`.
   Jamais deux conteneurs neufs en vol en même temps.
4. Corpus de comportement : 27 → **30 cas** (28-grid, 29-grid-flot, 30-paned).

⚠️ Un conteneur n'exporte rien par lui-même : chaque cas doit contenir des
enfants **qui exportent de vraies valeurs**, sinon le `.attendu` est vide et le
cas est mort. C'est ce trou qui avait laissé `<eventbox>` perdre son contenu sur
cinq ports.
