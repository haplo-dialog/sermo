# Bilan de santé — sermo (édition modulaire) 2.7.3

## État vérifié — 2026-09-24

> **Ce que valent ces chiffres.** Ce ne sont pas des notes d'auto-évaluation :
> ce sont des **comptes de bancs rejouables**. Chaque ligne ci-dessous se
> reproduit par une commande, à la date indiquée. Ne leur accordez de crédit que
> parce qu'elles se rejouent — et là où un banc n'est **pas** rejouable, c'est
> dit explicitement.

Tout ce qui suit sort de **deux commandes**, jouées sur un clone neuf limité à
deux cœurs (la taille d'un exécuteur partagé de CI). Le 2026-09-24, elles ont
tourné par `packaging/bancs-sur-paquets.sh` (`SERMO_TASKSET=0,1`) : les sept
binaires mesurés sont ceux des paquets 2.7.3-1, pas ceux de l'arbre :

```sh
bash ci/construire.sh     # cœur ×3 variantes, sept backends
bash ci/bancs.sh          # tous les bancs ; un journal par banc dans _journaux/
```

### Par backend

| Backend | XML (55) | Comportement (53) | Durcissement (8) | Commandes permises · sources sans fin · `--do` · limite de widgets | `--include` | `--glade-xml` | `<terminal>` | Échappement (4) | Clic (17) | Exemples réels |
|---|:--:|:--:|:--:|:--:|:--:|:--:|:--:|:--:|:--:|:--:|
| gtk3 (étalon) | ✅ | ✅ | ✅ | ✅ | ✅ | ✅ chargé | ✅ VTE | ✅ | ✅ | ✅ 55/55 |
| gtk4  | ✅ | ✅ | ✅ | ✅ | ✅ | ✅ chargé | ✅ VTE | ✅ | ✅ | ✅ 55/55 § |
| qt6   | ✅ | ✅ | ✅ | ✅ | ✅ | ✅ refusé † | ✅ non promis ¶ | ✅ | ✅ | ✅ 54/54 † |
| fltk1 | ✅ | ✅ | ✅ | ✅ | ✅ | ✅ refusé † | ✅ non promis ¶ | ✅ | ✅ | ✅ 54/54 † |
| efl1  | ✅ | ✅ | ✅ | ✅ | ✅ | ✅ refusé † | ✅ non promis ¶ | ✅ | ✅ § | ✅ 54/54 † |
| sdl3  | ✅ | ✅ | ✅ | ✅ | ✅ | ✅ refusé † | ✅ non promis ¶ | ✅ | ✅ | ✅ 54/54 † |
| ncurses ‡ | ✅ | ✅ | ✅ | ✅ | ✅ | ✅ refusé † | ✅ non promis ¶ | — | — | — |

- † `--glade-xml` charge un fichier GtkBuilder : ce n'est possible qu'avec GTK.
  Les autres ports refusent l'option (code 1, message), et l'exemple `glade` y est
  annoncé « réservé » par le banc des exemples, sans être compté.
- ¶ `<terminal>` n'existe (par VTE) que sur gtk3 et gtk4 : la garde y vérifie un
  vrai terminal (PID exporté, fichier témoin écrit par la ligne tapée) ; ailleurs,
  qu'un dialogue qui en contient un s'ouvre et se ferme normalement.
- § Rouges dans la passe complète du 2026-09-24 (une fenêtre pas encore ouverte
  dans le délai de 10 s), sous la charge ordinaire du poste ; verts rejoués seuls,
  sur les mêmes binaires et les mêmes deux cœurs.
- ‡ ncurses est un backend **terminal** : le comportement s'y rejoue en mode
  batch (non-tty / `SERMO_NCURSES_BATCH=1`, il exporte sans ouvrir de fenêtre),
  et une garde qui lui est propre vérifie **dans un vrai pseudo-terminal** que le
  mot de passe s'affiche masqué. Les gardes qui cliquent à l'écran ne s'y
  appliquent pas.

### Transverses

| Banc | Résultat |
|---|---|
| Tests unitaires du cœur | ✅ 4/4 : `safe_exec.c` (ses deux copies, `src` et `src-gtk4`), `sermo_input.c` (la limite des `<input>`), `stringman.c` |
| FORTIFY dans la bibliothèque du cœur | ✅ objets fortifiés : 14/15 (neutre ; la lecture de barre n'appelle aucune fonction à fortifier), 7/14 (GLib), 9/15 (GTK 4) |
| SPDX (tout fichier C/C++/lex/yacc suivi) | ✅ cœur, contrat, tests, exemples, sept backends |
| Fonctions interdites | ✅ cœur (3 dossiers), contrat, sept backends |
| Barre de progression hors du thread GTK | ✅ gtk3, gtk4 |
| En-têtes de poste · façade de l'arbre | ✅ |
| Lectures d'`<input>` plafonnées (`garde_lecture_input.sh`) | ✅ 15 ouvertures de fichier en lecture, toutes nommées et hors `<input>` ; un témoin planté est vu |
| `sermoman-mcp` : la documentation servie dit vrai | ✅ 7/7 contrôles contre le code, dont la limite des `<input>` ; exemples documentés joués sur les sept ports, 0 échec |
| Bancs sur les binaires des paquets (`packaging/bancs-sur-paquets.sh`) | ✅ 2.7.3-1 : sept binaires livrés en place, symboles de débogage trouvés ×7 ; 130/132 verts à la 1re passe, sur deux cœurs, en 20 min environ ; les 2 autres (une fenêtre en retard : exemples de gtk4, clic d'efl1) verts rejoués seuls |
| Ancrage Wayland de gtk3 (`garde_layer_shell.sh`) | ✅ lié à libgtk-layer-shell ; sous un sway sans écran, `layer="top"` donne une surface de couche, le témoin une fenêtre ordinaire (niveau non joué en root, où sway refuse de tourner) |

### Ce que la 2.7.3 ferme

Trouvé par la revue de sécurité du 2026-09-17 et fermé en 2.7.3 : `<input>` n'avait
pas de plafond. Chaque `<input>` lit désormais au plus 16 Mio (`SERMO_INPUT_MAX`),
et le dit sur la sortie d'erreur. En posant la limite, trois défauts voisins, tous
mesurés sur les paquets 2.7.2 : la barre de progression de gtk3 et gtk4 grossissait
sans fin sur une commande rapide (1 086 Mo en 3 s), et tombait quand
`SERMO_ALLOWED_CMDS` refusait sa commande ; leur `<edit>` lisait un fichier d'un
seul tenant (un fichier creux de 8 Go le faisait tomber). Sur les binaires 2.7.2,
`garde_input_sans_fin.sh` voit l'absence de limite et les deux défauts de la
barre ; sur l'arbre 2.7.2, `garde_lecture_input.sh` refuse l'`open()` de l'`<edit>`.

### Ce que la 2.7.1 ferme

Trouvés en préparant les paquets Debian et fermés en 2.7.1 : `<terminal>` (gtk3,
gtk4) et l'ancrage Wayland (gtk3), qui n'étaient plus compilés depuis la 2.0.0 ;
`<input>` sur les ports sans GTK (directive brute envoyée au shell, `<input file>`
non lu, commande exécutée deux fois, selon le port et le widget) ; la table sans
`<label>` (plantage de sdl3). Chacun a son banc, rouge sur les binaires de la
2.7.0. Restent ouverts : un faux « syntax error » à l'arrêt avec `--program`
(efl1, qt6), CET (ci-dessous), et les écarts à l'étalon relevés en réparant
`<input>` — voir [TODO.md](TODO.md).

### Ce que « durcissement » mesure ici

`tests/garde_durcissement.sh` lit huit propriétés sur chaque binaire : PIE,
RELRO, BIND_NOW, pile non exécutable, protection de pile, FORTIFY, et les notes
CET (IBT, SHSTK). ⚠️ **Les notes CET sont forcées à l'édition de liens**
(`-Wl,-z,ibt -Wl,-z,shstk`) : les objets de démarrage de la glibc de Debian
testing n'en portent pas, et ni la glibc ni les bibliothèques des toolkits non
plus (mesuré au `readelf` le 2026-09-16). Sur ce système, CET ne peut donc pas
agir à l'exécution — déduit de l'absence des notes, pas mesuré en exécution.

### Comment vérifier un seul backend

```sh
BIN=sermo-backend-qt6/_build/qt6sermo
TIMEOUT=5 bash tests/xml/run_tests.sh "$BIN"            # 55 PASS
bash tests/comportement/run.sh "$BIN"                   # 53 au vert (étalon gtk3, headless)
bash tests/garde_clic_widgets.sh "$BIN"                 # 17 widgets, aucun plantage
bash tests/run_examples.sh "$BIN"                       # chaque exemple ouvre sa fenêtre
bash tests/garde_glade.sh "$BIN"                        # chargé (gtk3, gtk4) ou refusé net
bash tests/garde_include.sh "$BIN"                      # --include chargé, quel que soit le chemin
bash tests/garde_input_sans_fin.sh "$BIN"               # yes et /dev/zero s'arrêtent à la limite
```

Binaires : `sermo-backend-<t>/_build/<t>sermo`.

---

## Ce qui est acquis (Phase 1 + 7e backend)

| Point | État | Preuve |
|---|:--:|---|
| Cœur unifié sans toolkit (variante neutre) | ✅ | `libsermocore.a` sans dépendance graphique ; 5 backends neutres (qt6, fltk1, efl1, sdl3, ncurses) |
| Trois familles sur un cœur (neutre + GLib + GTK4) | ✅ | qt6/fltk1/efl1/sdl3/ncurses (neutre), gtk3 (GLib), gtk4 (variante dédiée) sur `libsermocore` |
| Consommation par pkg-config | ✅ | `sermocore.pc` installé ; backends via `pkg_check_modules` |
| Packaging séparé, dépendances disjointes | ✅ | `debian/` (debhelper) → **13 paquets** (cœur + 7 backends + `sermo-gtkdialog` + 4 paquets de transition 1.x) et 7 `-dbgsym` ; dépendances calculées par `dh_shlibdeps`. `packaging/construire-paquets.sh` sur la 2.7.1, dans des conteneurs `debian:testing` vierges (2026-09-17) : construction source et binaires, tests de construction verts ; lintian sans erreur ni avertissement (14 exceptions justifiées) ; installation, `--version` ×7, alternative, dialogue en terminal, purge sans reste ; passage 1.x → 2.7.1-1 par `apt upgrade`. Aucun chemin du poste dans les paquets (mesuré sur la 2.7.0-1) |
| 7e backend : ncurses (terminal) | ✅ | `sermo-backend-ncurses` livré, priorité `update-alternatives` 5 ; XML 55/55 et comportement 53/53 headless |
| Alias `sermo` + commande `gtkdialog` | ✅ | `sermo` via `update-alternatives` ; commande `gtkdialog` via le paquet séparé `sermo-gtkdialog` (lien → gtk3sermo ; Conflicts/Replaces/Provides pour éviter le conflit avec gtk3dialog/BunsenLabs) |
| Durcissement conservé | ✅ | `tests/garde_durcissement.sh` + `tests/garde_fortify_coeur.sh` ; CET : notes seulement (voir plus haut) |
| Thème système suivi | ⚠️ 5/6 GUI | `sermo_desktop_is_dark()` partagé. Le 2026-09-17, rendu avec `SERMO_DARK=0` puis `SERMO_DARK=1` : gtk3, gtk4, qt6, fltk1 et sdl3 passent du clair au sombre ; **efl1 rend la même image sombre dans les deux cas** ([TODO.md](TODO.md)). ncurses a son propre réglage (`SERMO_NCURSES_THEME`) |
| Rendu PNG (`--render-png`) | ⚠️ 5/6 GUI | Le 2026-09-17, sur les binaires des paquets : gtk3, gtk4, fltk1 (sous xvfb), efl1, sdl3 (hors écran) rendent la fenêtre, à l'identique d'un essai à l'autre. **qt6** rendait une image de 2×2 pixels quand la fenêtre contient une liste déroulante (2 essais sur 6), et ignorait `default-width`/`default-height` — fermé le 2026-09-20 : 6 essais sur 6 à la taille demandée, pour la liste déroulante comme pour le formulaire de showcase. Il faut un affichage (xvfb suffit) ou une plateforme hors écran. ncurses = terminal : aperçu texte par `--print-ir`. Mesuré depuis le 2026-09-20 par `tests/garde_taille_fenetre.sh`, sur chaque port graphique |

---

## Écarts — trouvés et fermés le 2026-09-09

- **Bug efl1 calendar (date −1 jour)** — ✅ corrigé : date canonique stockée hors
  round-trip `time_t` bogué d'Elementary ; efl1 repassé 24/24.
- **Bug gtk4 (3 écarts d'assemblage)** — ✅ corrigé : énum `WIDGET_*` divergent
  entre l'étalon et gtk4sermo. `GTK4SRC` placé en tête du chemin d'inclusion ;
  gtk4 repassé 24/24. (`password`→0xBC était créé comme `spinner`,
  `filechooser`→0xB4 comme `calendar`, `infobar`→0xB8 comme `linkbutton`.)
- **Bug qt6 togglebutton (mort au clic)** — ✅ corrigé : un togglebutton nu
  passait par le chemin « bouton sans action → `action_exitprogram` » et
  **quittait au clic** au lieu de basculer. Corrigé dans
  `sermo-backend-qt6/src/widget_button.cpp` (togglebutton = bouton *checkable*,
  pas d'action de sortie par défaut, valeur = état actif — sémantique de l'étalon
  gtk3 `widget_button.c:347,413`). `tests/garde_clic_widgets.sh` sur qt6 :
  17 widgets, aucun plantage. Non-régression : comportement qt6 24/24, XML 55/55.

> Le banc `garde_clic_widgets` (clic réel, pas seulement « la fenêtre s'ouvre »)
> a été rejoué le 2026-09-09 : **qt6, fltk1, efl1, gtk4 verts**. L'explication
> donnée alors pour sdl3 (« pilote GL absent sous Xvfb ») **était fausse** : la
> garde cherchait la fenêtre par un titre que xdotool ne lit pas en UTF-8.
> Réparée le 2026-09-16, elle a mesuré sdl3 et trouvé un vrai défaut — cliquer
> sur `<fontbutton>` fermait le dialogue (sdl3 et ncurses), corrigé en 2.6.8.
> ncurses est en terminal (garde graphique sans objet).

---

## Dette architecturale (Phase 3)

**Fait (étapes 1-2)** : les ponts cœur→backend sont **renommés `qt6_*` →
`sermo_be_*`** (cœur neutre + les 5 backends neutres + gtk3) — le cœur ne nomme
plus un port. La boucle porte le nom `sermo_be_run_loop()` (elle était déjà
déléguée au backend via le shim). Bancs verts après renommage (7/7).

**Dépendance à l'ancienne édition monolithique : retirée.** Le cœur GTK4 (variante
`SERMOCORE_GTK4`) est désormais **vendoré** dans `libsermocore/src-gtk4/` : le
dépôt se construit **seul**, sans arbre externe. gtk4 reste vert (**XML 55/55 ·
comportement 40/40**). Le lexer/parser de qt6 pointe aussi sur `libsermocore/src`
(grammaire unifiée) : plus aucune référence de build à l'ancienne édition.

**Reste** : la **source unique** gtk3+gtk4 — retirer la variante
`SERMOCORE_GTK4` et réconcilier en un seul jeu de sources les cinq fichiers cœur
que GTK 4 fait diverger (`gtk_main`/`gtk_socket`/modèle d'évènements retirés). La
`.so` partagée propre + le cloisonnement IPC attendent une **IR sérialisable**
(chantier ultérieur).

---

## Résumé

sermo remplit son objet : **le même XML, le même comportement, sur sept
toolkits, depuis un cœur unique et durci**, avec des paquets légers et disjoints.
Le 2026-09-24, `ci/bancs.sh` a joué ses 132 bancs sur les binaires des paquets
2.7.3-1, sur deux cœurs : XML 55 et comportement 53 sur les sept backends,
terminal et ancrage Wayland vérifiés, sources sans fin arrêtées à la limite,
documentation servie conforme au code ; 130 verts d'une traite, les 2 autres
(fenêtres en retard sous la charge ordinaire du poste) verts une fois rejoués
seuls.
Les paquets Debian se construisent, s'installent, se purgent et remplacent la 1.x
dans une Debian testing vierge. Reste la dette architecturale de la Phase 3, sans
impact fonctionnel, et les points ouverts du TODO.
