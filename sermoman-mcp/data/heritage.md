# La vie de sermo — lignée, versions, feuille de route

Ce document raconte d'où vient sermo, ce qu'il est devenu, et où il va. Il
sert à situer chaque affirmation dans le temps : ce qui est hérité, ce qui a
changé, ce qui reste à faire. Chaque fait provient des fichiers du dépôt
(CHANGELOG, ROADMAP, BILAN_SANTE, TODO, VERSIONING, README).

---

## 1. La lignée : gtkdialog → haplo-dialog → sermo

**Point de départ — gtkdialog 0.8.3 (László Pere, GPL-2.0-or-later).** C'est une
boîte à dialogues pilotée par XML : on décrit une fenêtre en XML, un binaire la
dessine, le shell orchestre. Cette version d'origine est restée sur GTK 2.

**Étape 1 — haplo-dialog.** Reprend gtkdialog 0.8.3 et le modernise sur GTK 3, en
gardant la **syntaxe XML à l'identique**. Les scripts écrits pour gtkdialog
tournent sans réécriture. À ce stade, le projet est un binaire par toolkit.

**Étape 2 — sermo (édition modulaire, aujourd'hui).** Le projet ne se décrit plus
comme « un binaire par toolkit ». Il est **séparé en deux parties** :

- un **cœur** unique et durci, `libsermocore` — l'analyse XML (lexer/parser
  Flex/Bison), l'exécution sécurisée (`safe_system` / `safe_popen`), les
  variables, les actions, l'automate ; **sans aucune dépendance graphique** dans
  sa variante neutre ;
- des **backends de rendu** autonomes, un par bibliothèque graphique, qui ne font
  que **dessiner** ce que le cœur décrit.

Le cœur parle aux backends par un **contrat de frontière** (`sermo_backend.h`,
hooks `sermo_be_*`). Le même script XML rend une interface équivalente sur les
sept backends.

### Ce qui est hérité

- Le **langage XML** de gtkdialog : les descriptions de fenêtres restent lues
  telles quelles. Un script hérité tourne sans modification.
- La **licence** : GPL-2.0-or-later, dans la lignée de gtkdialog.
- Le **comportement observable** : sermo modulaire reprend le niveau
  fonctionnel de sermo monolithique 1.1.x (les mêmes widgets, les mêmes valeurs
  de sortie).
- **Du code, et on peut dire lequel.** L'amont est `gtkdialog 0.8.3`
  (Pere László, repris par Thunor en 2011), écrit pour **GTK2 et rien d'autre**.
  Comparaison faite fichier par fichier contre l'archive d'origine : `gtk3` en
  hérite 72 fichiers et `gtk4` 64 — ce sont des **portages** des widgets GTK2.
  Les cinq autres n'en héritent que 8 à 10 sur 112 à 124, et ce sont les en-têtes
  du cœur : leur couche de rendu est neuve, parce qu'on ne transpose pas du code
  GTK2 vers Qt, SDL, EFL, FLTK ou un terminal. Détail dans `LICENCES.md`.
- Le **copyright d'origine**, conservé dans chaque fichier concerné. Rien n'est
  retiré ni réattribué.

### Ce qui a changé

- **GTK 2 → toolkits multiples.** Là où l'origine ne connaissait que GTK, sermo
  rend le même XML sur sept moteurs : gtk3, gtk4, qt6, fltk1, efl1, sdl3 et
  ncurses (terminal).
- **Binaire unique → cœur + backends.** L'analyse et l'exécution vivent une
  seule fois dans `libsermocore` ; chaque backend ne porte que le dessin.
- **Durcissement de l'exécution.** `safe_system` / `safe_popen` remplacent
  `system()` / `popen()` : sans métacaractères shell, la commande est exécutée
  **sans shell**. La sortie est échappée. Ce durcissement est arrivé dans la
  lignée monolithique et conservé.

### Les sept backends

| Backend | Toolkit | Binaire |
|---|---|---|
| `sermo-backend-gtk3` | GTK 3 (étalon) | `gtk3sermo` |
| `sermo-backend-gtk4` | GTK 4 | `gtk4sermo` |
| `sermo-backend-qt6` | Qt 6 | `qt6sermo` |
| `sermo-backend-fltk1` | FLTK 1.4 | `fltk1sermo` |
| `sermo-backend-efl1` | Enlightenment / Elementary | `efl1sermo` |
| `sermo-backend-sdl3` | SDL 3 + Dear ImGui | `sdl3sermo` |
| `sermo-backend-ncurses` | ncurses (terminal) | `ncursessermo` |

**gtk3** est l'**étalon** : c'est son comportement que les six autres doivent
reproduire.

### Trois variantes de build du cœur

Le cœur se construit en trois formes, depuis une source largement unifiée :

- **neutre** (défaut, sans toolkit) — sert qt6, fltk1, efl1, sdl3 et ncurses ;
- **`SERMOCORE_GLIB`** — vraie GLib/GTK 3, pour le backend gtk3 ;
- **`SERMOCORE_GTK4`** — fichiers cœur réécrits pour gtk4 (GTK 4 a retiré
  `gtk_main`, `gtk_socket` et l'ancien modèle d'évènements).

### Packaging — 13 paquets .deb

Le dossier `debian/` (debhelper) produit **13 paquets .deb** aux dépendances
**disjointes** (calculées par `dh_shlibdeps`) ; `packaging/construire-paquets.sh`
les construit et les éprouve dans une Debian testing vierge :

- `sermo-core-dev` — le cœur statique + en-têtes + `sermocore.pc` (ne dépend que
  de libc6) ;
- **sept** `sermo-backend-<t>` — un par toolkit, chacun ne tirant que **sa**
  bibliothèque graphique ;
- `sermo-gtkdialog` — paquet **séparé et optionnel** posant la commande
  historique `gtkdialog` (lien → `gtk3sermo`). Il déclare
  `Conflicts`/`Replaces`/`Provides` sur `gtkdialog`, `gtk3dialog`, `haplo-dialog`
  pour rester mutuellement exclusif avec le gtkdialog original et le `gtk3dialog`
  de BunsenLabs ;
- **quatre paquets de transition** `gtk3sermo`, `gtk4sermo`, `qt6sermo`,
  `gtksermo` — les noms de la 1.x, désormais vides, qui font venir leurs
  successeurs lors d'un `apt upgrade`.

L'alias `sermo` (`/usr/bin/sermo`) est géré par `update-alternatives` : chaque
backend installé s'y enregistre, l'utilisateur choisit le défaut.

---

## 2. Historique des versions

### 2.0.0 — Édition modulaire publique (2026-09-09)

Première version publique de l'édition modulaire ; elle remplace la lignée
monolithique en ligne. Le bump MAJEUR tient au changement du **contrat de
frontière cœur↔backend** (`sermo_backend.h`, ponts `sermo_be_*` de la Phase 3) :
un backend monolithique ne se lie plus au nouveau cœur. Le **langage XML reste
rétro-compatible**. Nouveautés depuis 1.1.1 : 7e backend `ncurses`, thème
clair/sombre par backend (`SERMO_DARK`), version unifiée sur les sept backends.

### 1.1.1 — Modularisation (Phase 1)

L'édition modulaire sépare le sermo monolithique en un cœur unique
(`libsermocore`) et des backends de rendu autonomes. Le langage XML et le
comportement observable restent identiques à sermo 1.1.x — la nouveauté est
**structurelle**, pas fonctionnelle, d'où le maintien du numéro 1.1.1.

**Ajouté :**

- `libsermocore` : cœur syntaxique + exécution durcie, sans dépendance graphique
  dans la variante neutre.
- Les trois variantes de build du cœur (neutre, `SERMOCORE_GLIB`,
  `SERMOCORE_GTK4`).
- Les sept backends consommant le cœur par pkg-config (`sermocore.pc`).
- Le contrat de frontière `sermo_backend.h` (hook d'init du toolkit remplaçant
  l'appel `gtk_init` en dur).
- Le packaging séparé (`sermo-core-dev` + sept `sermo-backend-*` +
  `sermo-gtkdialog`).

**Modifié :**

- Le bridge d'attributs GObject **sort du cœur** vers chaque backend
  (`tag_set_property.c`) ; le cœur ne garde que les helpers indépendants du
  toolkit.
- `main()` vit désormais dans le cœur et rappelle le hook d'init du backend.

**Corrigé — le 2026-09-09 :**

- **efl1 — calendar à J-1.** Le round-trip `time_t` d'`elm_calendar` était cassé
  au passage heure d'été/hiver ; la date canonique est gardée hors du
  round-trip. efl1 repassé 24/24.
- **gtk4 — 3 écarts de valeur** (`05-filechooser`, `11-infobar`, `13-password`).
  Cause : **deux énums `WIDGET_*` distincts** dans la variante `SERMOCORE_GTK4`
  (le parser généré voyait un `automaton.h`, les fichiers cœur de gtk4 en
  voyaient un autre, bloc réordonné). `GTK4SRC` placé en tête du chemin
  d'inclusion → un seul énum. gtk4 repassé 24/24.
- **qt6 — togglebutton mort au clic.** Un togglebutton nu passait par le chemin
  « bouton sans action → sortie » et **quittait au clic** au lieu de basculer.
  Corrigé pour suivre la sémantique de l'étalon gtk3 (bouton *checkable*, valeur
  = état actif). Non-régression : qt6 24/24, XML 55/55.
- **qt6 — icônes de boutons absentes** sous Xvfb nu : repli `sermo_icon_lookup`
  quand `QIcon::fromTheme` échoue faute de thème de plateforme.

### Historique amont (sermo monolithique, 1.0.0 → 1.1.x)

Avant la modularisation, sermo était monolithique. Cette lignée a apporté :

- **43+ widgets** ;
- le **durcissement `safe_system`** (exécution sans shell hors métacaractères) ;
- l'**échappement de la sortie** ;
- l'**unification du lexer/parser** sur l'étalon gtk3sermo.

L'édition modulaire repart de ce niveau fonctionnel (1.1.1) et le conserve.

---

## 3. Feuille de route

### Phase 1 — Modularisation ✅ (faite)

Cœur `libsermocore` extrait, source unifiée sur l'étalon gtk3, bridge GObject
sorti vers les backends, les trois variantes de build, `sermocore.pc`, les sept
backends portés, le packaging séparé. **Faite.**

### Phase 2 — Consolidation (en cours)

Déjà acquis :
- Les 3 écarts de valeur de gtk4 résorbés (le 2026-09-09) → gtk4 = 24/24.
- Documentation complète + page de manuel + Texinfo.
- gtk4 intégré à `packaging/build-debs.sh` → les 9 `.deb`.

### Phase 3 — Frontière propre

- ✅ Ponts `qt6_*` renommés `sermo_be_*` (cœur neutre + les cinq backends
  neutres + gtk3) : le cœur ne nomme plus un port. La boucle porte le nom
  `sermo_be_run_loop()`.
- ⏳ Remettre gtk4 sur la source unifiée (retirer `SERMOCORE_GTK4` et la
  dépendance de build à l'ancien gtk4). La voie compile déjà contre le cœur
  neutre ; reste l'**auto-câblage des signaux gtk4** (le cœur neutre no-ope
  `g_signal_connect`, sinon le `<timer>` ne démarre pas). gtk4 reste sur la
  variante dédiée, 24/24, en attendant.
- ⏳ Formaliser une **IR** (représentation intermédiaire) consommée par le
  backend → permettre une `.so` partagée propre (le cœur n'appellerait plus
  `widget_*_create`).

### Phase 4 — Écosystème

- ⏳ Cloisonnement de processus (IPC) : le shell confiné au cœur, backend séparé.
- ⏳ Backends supplémentaires via le contrat de frontière.
- ⏳ Publication produit (dépôt apt, site).

> La progression est **gardée par les trois bancs** (XML 55 · comportement 52 ·
> gardes) : aucune phase n'avance en laissant un banc rouge.

---

## 4. Bilan de santé et TODO

État vérifié le **2026-09-16**, sur un clone neuf. Les chiffres ci-dessous ne
sont pas des notes d'auto-évaluation : ce sont des **comptes de bancs rejouables**,
chacun reproductible par une commande (`ci/bancs.sh` les enchaîne tous).

### Les trois portes, par backend

| Backend | XML (55) | Comportement (53) | Gardes |
|---|:--:|:--:|:--:|
| gtk3 (étalon) | 55/55 | 53/53 | ✅ |
| gtk4 | 55/55 | 53/53 | ✅ |
| qt6 | 55/55 | 53/53 | ✅ |
| fltk1 | 55/55 | 53/53 | ✅ |
| efl1 | 55/55 | 53/53 | ✅ |
| sdl3 | 55/55 | 53/53 | ✅ (clic_widgets mesuré depuis le 2026-09-16) |
| ncurses | 55/55 | 53/53 ‡ | ✅ (terminal : mot de passe, sources) |

Le banc XML (55) et le banc comportement (52) ont été **rejoués le 2026-09-16
sur les sept binaires**. Parité : 7/7.

**Exemples réels** (`tests/run_examples.sh`, qui ouvre chaque exemple
d'`examples/`) : 55 sur 55 sur gtk3 et gtk4 ; 54 sur 54 sur qt6, fltk1, efl1 et
sdl3, où l'exemple `glade` est réservé aux ports GTK.

- `--glade-xml` (gtk3sermo, gtk4sermo **seulement**) charge un fichier
  d'interface GtkBuilder, dans le format de la version de GTK du port ; les cinq
  autres ports refusent l'option (code 1). Ne le proposez que pour ces deux-là.
- `<progressbar>` suit sa commande `<input>` ligne par ligne sur les sept ports,
  et la ligne qui vaut 100 déclenche ses actions. Jusqu'à la 2.6.8, seuls gtk3 et
  gtk4 le faisaient.

- ‡ **ncurses est un backend terminal.** Le banc comportement s'y rejoue en mode
  batch (non-tty, `SERMO_NCURSES_BATCH=1` : il exporte sans ouvrir de fenêtre).
  Les gardes purement graphiques (clic à l'écran) ne s'y appliquent pas ; les
  gardes sur source (durcissement, SPDX, fonctions interdites) si.

### Comment vérifier

```sh
# les trois portes sur UN backend <BIN> (= sermo-backend-<t>/_build/<t>sermo)
TIMEOUT=5 bash tests/xml/run_tests.sh <BIN>              # 55 PASS
bash tests/comportement/run.sh <BIN>                     # 53 au vert (étalon gtk3, headless)
xvfb-run -a bash tests/garde_clic_widgets.sh <BIN>       # 17 widgets, aucun plantage

# le comportement sur LES SEPT backends d'un coup :
bash tests/comportement/run_all.sh                       # tableau 7/7
```

### Ce qui est acquis

- Cœur unifié sans toolkit (variante neutre) : `libsermocore.a` sans dépendance
  graphique.
- Trois familles sur un cœur (neutre + GLib + GTK4).
- Consommation par pkg-config (`sermocore.pc` installé).
- Packaging séparé, dépendances disjointes (9 `.deb`).
- 7e backend : ncurses (terminal), le plus léger.
- Alias `sermo` (`update-alternatives`) + commande `gtkdialog` (paquet séparé).
- Durcissement conservé (FORTIFY=3, PIE, RELRO, BIND_NOW, pile non exécutable,
  protection de pile), vérifié par banc. CET : les notes sont posées au lien,
  mais la glibc et les toolkits de Debian testing n'en portent pas — sur ce
  système, CET ne peut pas agir à l'exécution (déduit, pas mesuré en exécution).
- Thème système suivi (`sermo_desktop_is_dark()` partagé) sur gtk3, gtk4, qt6,
  fltk1 et sdl3 ; efl1 reste sombre même avec `SERMO_DARK=0`.
- Rendu PNG (`--render-png`) : la fenêtre sur les six ports graphiques, à la
  taille demandée. ncurses rend un aperçu texte.

### Dette connue (Phase 3)

- gtk4 tourne encore sur la variante `SERMOCORE_GTK4`, pas sur le cœur unifié. La
  voie unifiée compile mais le `<timer>` gtk4 n'y démarre pas encore (le cœur
  neutre no-ope `g_signal_connect` ; gtk4 doit auto-câbler ses signaux). Chantier
  dédié. Sans impact fonctionnel : gtk4 reste à parité sur la variante.
- La `.so` partagée propre et le cloisonnement IPC attendent une **IR
  sérialisable** (chantier ultérieur).

### TODO

**Court terme** — fait : les 3 écarts gtk4, le calendar efl1 à J-1, l'intégration
de gtk4 dans `build-debs.sh` (tous le 2026-09-09).

**Moyen terme (Part 3)** — fait : renommage `qt6_*` → `sermo_be_*`.
Reste : remettre gtk4 sur la source unifiée (auto-câblage des signaux) ;
formaliser l'IR.

**Long terme** — cloisonnement IPC ; backends supplémentaires.

**Garde-fous, toujours vrais** — ne jamais régresser ce que joue `ci/bancs.sh`
(XML 55, comportement 52, gardes, exemples réels) ; ne jamais retirer un drapeau
de durcissement.

---

## 5. Versionnage

sermo suit un **versionnage sémantique** `MAJEUR.MINEUR.CORRECTIF`, aligné sur
la lignée sermo.

- **MAJEUR** — rupture de compatibilité du langage XML ou du contrat de frontière
  cœur↔backend (`sermo_backend.h`).
- **MINEUR** — nouvelles capacités rétro-compatibles (widget, action, backend).
- **CORRECTIF** — corrections sans changement d'interface.

**Version courante : `2.7.7` ; la première version 2.x publiée a été la `2.7.3`.** La ligne 2.x s'est
ouverte sur `2.0.0`, première édition modulaire, restée interne, dont le bump MAJEUR reflétait le changement du contrat de frontière cœur↔backend
(`sermo_backend.h`, ponts `sermo_be_*`) ; le langage XML, lui, reste
rétro-compatible avec gtkdialog / sermo 1.x.

Le cœur (`libsermocore`) et les backends **partagent le numéro de version** tant
qu'ils sont livrés ensemble. Le contrat de frontière `sermo_backend.h` est le
point de compatibilité : un changement incompatible impose un bump MAJEUR, car un
backend ancien ne se lierait plus au cœur. `sermocore.pc` porte la version
(`Version: 2.7.7`) ; un backend peut exiger une version minimale du cœur via
`pkg_check_modules(SERMOCORE REQUIRED sermocore>=X.Y)`.

**Compatibilité des scripts.** Les scripts XML écrits pour gtkdialog / sermo
restent valides. Pour l'appel historique par la **commande** `gtkdialog`,
installer le paquet séparé `sermo-gtkdialog` (lien → `gtk3sermo`).

---

## Licence

GPL-2.0-or-later, dans la lignée de gtkdialog 0.8.3 (László Pere). Documentation
sous la même licence ; la plupart des exemples viennent de gtkdialog et la suivent
aussi. Contact : `devel@haplo-dialog.fr`.
