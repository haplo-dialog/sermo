# TODO — sermo

État : Phase 1 (modularisation) terminée — sept backends sur `libsermocore`,
empaquetage séparé prouvé. Reste ci-dessous.

## Court terme

Trouvé le 2026-09-16 par les bancs réparés (`ci/bancs.sh`), **ouvert** :

- [ ] **Faux « syntax error » à l'arrêt, avec `--program`** : efl1 arrêté par
      SIGTERM (et il sort alors en code 1), qt6 privé de son serveur X,
      écrivent `Error in line N, near token '<window>': syntax error` — le cœur
      relance l'analyse d'un texte déjà consommé. Rien avec `--file` ; rien sur
      gtk3, gtk4, fltk1, sdl3.
- [x] **CET sans effet à l'exécution** : les notes IBT/SHSTK sont forcées au
      lien (`-Wl,-z,ibt -Wl,-z,shstk`), mais ni la glibc ni les bibliothèques des
      toolkits de Debian testing n'en portent (mesuré au `readelf`). Dit dans
      SECURITY.md le 2026-09-17 ; rien de plus à faire côté sermo tant que la
      distribution ne porte pas ces notes.

Trouvé le 2026-09-17 en préparant les paquets Debian, **ouvert** — écarts à
l'étalon gtk3sermo mesurés (ou lus, quand c'est dit) pendant la réparation de
`<input>`, laissés hors de la 2.7.1 :

- [ ] **`auto-refresh`** (surveillance d'un `<input file>`) ne vit que sur gtk3 et
      gtk4 (GFileMonitor de la vraie GLib) : qt6, fltk1, efl1 et sdl3 ne relisent
      pas le fichier. Non documenté dans les manuels ; deux exemples l'emploient.
- [ ] **Sélection initiale de `<list>`, `<tree>`, `<table>`** : chez l'étalon, la
      1re rangée ne ressort que si le widget a le focus clavier (1er widget
      focalisable de la fenêtre) ; les autres ports la sélectionnent toujours. Les
      cas 16-18 et 45-52 placent le widget seul dans sa fenêtre. Après `refresh:`,
      l'étalon ne sélectionne plus rien ; efl1 garde la 1re rangée.
- [ ] **Plusieurs `<input>` sur un widget** : l'étalon les lit toutes, dans
      l'ordre ; les ports sans GTK ne lisent que la première reconnue.
- [ ] **`<item>` et `<input>` mêlés** : l'étalon charge `<input>` d'abord ; qt6,
      fltk1 et efl1 (list, table) mettent les `<item>` d'abord. L'étalon trie aussi
      `<table>` sur sa 1re colonne dès la création.
- [ ] **`<edit>` avec `<default>` et `<input>`** : l'étalon garde `<default>` au
      démarrage ; qt6, fltk1 et efl1 gardent `<input>` (sdl3 et ncurses suivent
      l'étalon).
- [ ] **`<comboboxentry>` exporte toujours ""** sur efl1 (la valeur est rangée
      ailleurs que là où l'export la cherche) ; le cas 23 passe par hasard. FLTK
      fusionne deux éléments de même libellé dans `<comboboxtext>`.
- [ ] **`<input>` brut restant** : `<terminal>` des ports sans GTK (non promis),
      `<pixmap>` d'efl1 (« file:… » passé tel quel), et, lu dans le code de qt6
      [NON VÉRIFIÉ] : `<image>`/`<button>` comparent « file: » en tenant compte de
      la casse alors que le cœur stocke « File: », `<linkbutton>` prend la valeur
      brute pour adresse. qt6 : `<checkbox>` ne suit ni `refresh:` d'un `<input>`
      ni `activate:` (mesuré).
- [ ] **Deuxième fenêtre aux noms de variables déjà pris** (`launch:`) : le cœur
      l'enregistre sous « NOM__W<id> » ; mesuré juste sur gtk3 (chaque `<input>`
      une fois). Sur les ports sans GTK, désormais lus au refresh [NON VÉRIFIÉ] :
      aucun déclencheur sans affichage n'a ouvert la fenêtre sur qt6.
- [ ] **Widgets numériques** de fltk1 : `<input>` lu à la création seulement, pas
      relu par `refresh:`.
- [ ] **`<entry>` sur une seule ligne très longue** (mesuré le 2026-09-24 sur les
      binaires 2.7.3, `<input>yes | tr -d '\n'</input>`, limite 20 000 octets) :
      l'étalon ne garde que les 511 premiers octets (il lit une ligne de 512), gtk4
      aussi ; sdl3 et ncurses lisent tout, puis gardent 1 023 octets ; qt6, fltk1 et
      efl1 gardent tout, jusqu'à la limite des `<input>`.
- [x] **Taille de `<input>` sans plafond** (revue de sécurité du 2026-09-17) :
      corrigé en 2.7.3. Chaque `<input>` lit au plus 16 Mio (`SERMO_INPUT_MAX`),
      avec un avertissement sur la sortie d'erreur ; la barre de progression
      n'est pas plafonnée. Trois défauts voisins corrigés avec : la barre gtk3/gtk4
      qui grossissait sur `yes 50` et tombait sur une commande refusée, et le
      `<edit>` gtk3/gtk4 qui lisait un fichier d'un seul tenant. Bancs :
      `garde_input_sans_fin.sh` (sept ports), `garde_lecture_input.sh`,
      `tests/unit/test_sermo_input.c`.
- [ ] **ncurses sans écran : une barre de progression sans fin bloque.** Le mode
      sans écran lit chaque barre jusqu'au bout AVANT les minuteries
      (`render_ncurses.c`, `run_headless`). Voulu, et sans risque mémoire ; mais
      un `exit` par minuterie n'y part jamais tant qu'une barre lit `yes`.
- [ ] **Petites pertes mémoire à chaque export des variables** (cœur,
      `variables_export_all`, mesuré sous valgrind sur efl1) : quelques octets par
      variable à chaque `refresh:` ou action ; lent, mais sans fin dans une boucle
      de rafraîchissement.
- [ ] **`<list>` et `<table>` rafraîchies en boucle** : chaque `refresh:` AJOUTE ses
      rangées, comme l'étalon ; un script qui rafraîchit sans `clear:` grossit sans
      fin. Comportement hérité de gtkdialog, à documenter.
- [ ] **`<terminal>` hors GTK** : code partiel, non promis — Fl_Terminal (fltk1)
      compilé mais variable vide ; QTermWidget (qt6) absent de la construction ;
      détection `ecore-exe` d'efl1 fausse (Ecore_Exe fait partie d'ecore).

Trouvé le 2026-09-17 en fabriquant les pages des ports du site, **fermé** :

- [x] **`<chooser>` faisait avorter les cinq ports sans GTK** : le cœur y créait le
      sélecteur embarqué par un appel GTK qui rend NULL, puis
      `variables_new_with_widget()` avortait (« ASSERT FAILED: widget != NULL »,
      code 134) — un dialogue qui contenait un `<chooser>` ne s'ouvrait pas du tout
      sur qt6, fltk1, efl1, sdl3 et ncurses. Le tag y devient maintenant le
      `<filechooser>` du port (bouton + dialogue du toolkit), avec la même variable
      et le même `<default>` ; la référence le dit. Cas de comportement **53**,
      rouge avant sur les cinq, vert sur les sept après.
- [x] **`<filechooser>` de fltk1 exportait son libellé d'invite** « (Aucun) » au lieu
      d'une valeur vide quand rien n'était choisi : le filtre de l'export cherchait
      un ancien libellé. Trouvé par le cas 53 ; libellé mis en une seule constante.

Trouvé le 2026-09-17 en faisant l'image du README (binaires des paquets
2.7.1-1), **ouvert** :

- [ ] **efl1 ignore `SERMO_DARK`** : `--render-png` donne la même image sombre
      avec `SERMO_DARK=0` et `SERMO_DARK=1`, alors que gtk3, gtk4, qt6, fltk1 et
      sdl3 basculent. `ELM_PALETTE=light` n'y change rien.
- [ ] **Libellé de `<button cancel>`** : « Cancel » sur gtk3, gtk4 et qt6 (écrit
      en dur en anglais), « Annuler » sur fltk1, efl1, sdl3 et ncurses, quelle que
      soit la langue du système.

Trouvé le même jour, **fermé en 2.7.1** (voir [CHANGELOG.md](CHANGELOG.md)) :

- [x] **Bancs graphiques sensibles à la charge** : `run_examples.sh` attendait une
      fenêtre 3 s ; sur un poste chargé, un exemple différent rougissait à chaque
      passe sans défaut. Attente maximale portée à 10 s (`EXAMPLE_DELAY`), l'écran
      étant interrogé toutes les 0,1 s.

- [x] **`<terminal>` et l'ancrage Wayland absents de la 2.x** : le `-DHAVE_VTE` et le
      `-DHAVE_LAYER_SHELL` ne vivaient que dans les anciens `Makefile.am`. Options
      `SERMOCORE_VTE` et `SERMO_LAYER_SHELL` (actives, absentes = arrêt de la
      construction) ; `tests/garde_terminal.sh`, `tests/garde_layer_shell.sh`.
- [x] **`<input>` sur les ports sans GTK** : directive brute envoyée au shell,
      `<input file>` non lu, commande exécutée deux fois (entry, text, edit,
      comboboxtext, combobox, list, tree, table ; numériques de qt6 ; table de
      gtk4) ; levelbar d'efl1 divisé par 100 ; table sans `<label>` : premier
      `<item>` pris pour en-têtes (gtk4), plantage (sdl3). Cas 43 à 52.

Trouvé le 2026-09-16, **fermé en 2.7.0** (voir [CHANGELOG.md](CHANGELOG.md)) :

- [x] **`<progressbar>` ne vivait que sur gtk3 et gtk4** : qt6 lisait une ligne,
      fltk1/efl1/sdl3/ncurses toute la commande avant d'ouvrir ; la barre
      n'avançait pas et l'action à 100 ne partait jamais. Lecture au fil de
      l'eau dans le cœur (`sermo_progress`) ; cas de comportement 41.
- [x] **`--glade-xml`** supposait libglade, absente pour GTK 3 et 4 : option
      « ignorée », scripts qui avortaient. gtk3sermo et gtk4sermo chargent un
      fichier GtkBuilder ; les autres ports refusent ; `tests/garde_glade.sh`.
- [x] **`--include` relatif** ne chargeait rien sous dash (« . » cherche dans le
      PATH) : chemin rendu absolu et cité ; `tests/garde_include.sh`.
- [x] **`<frame>` à plusieurs enfants** : seul le premier était dessiné sur
      fltk1, efl1, sdl3, ncurses ; cas de comportement 42.

- [x] **Boucher les stubs** (2026-09-13) : `<eventbox>`, `<linkbutton>`,
      `<pulse>` et `<filechooser>` étaient inopérants sur `efl1`, `sdl3` et
      `ncurses` (fichiers `widget_stubs.c`), et `<linkbutton>` / `<pulse>`
      l'étaient AUSSI sur `qt6` et `fltk1` sans que rien ne le signale. Trois
      cas de banc (25-27) mesurent maintenant ces trois tags sur les sept
      ports. Voir [CHANGELOG.md](CHANGELOG.md) 2.1.1.
- [x] **gtk4 — 3 écarts de valeur** (`05-filechooser`, `11-infobar`,
      `13-password`) : corrigés le 2026-09-09. Cause = **deux énums `WIDGET_*`**
      dans la variante `SERMOCORE_GTK4` (parser généré vs fichiers cœur de
      gtk4sermo voyant chacun un `automaton.h` différent, bloc 0xB4–0xC3
      réordonné). `GTK4SRC` mis en tête du chemin d'inclusion. gtk4 = 24/24.
- [x] **efl1 — calendar à J-1** : corrigé (date canonique hors round-trip
      `time_t` d'`elm_calendar`).
- [x] Intégrer **gtk4** dans `packaging/build-debs.sh` — fait le
      2026-09-09 ; `build-debs.sh` produit 9 `.deb`, dépendances disjointes.

## Moyen terme — Part 3 (architecture propre)

- [x] Renommer les ponts `qt6_*` → `sermo_be_*` (cœur neutre + 5 backends
      neutres) : le cœur ne nomme plus un port. Boucle = `sermo_be_run_loop()`.
- [x] **Dépendance à l'ancienne édition monolithique retirée** : le cœur GTK4 (`SERMOCORE_GTK4`)
      est **vendoré** dans `libsermocore/src-gtk4/` — le dépôt se construit seul
      (gtk4 : XML 55/55 · comportement 24/24). qt6 pointe aussi sur
      `libsermocore/src` pour le lexer/parser.
- [ ] **Source unique** gtk3+gtk4 (supprimer `SERMOCORE_GTK4`) : cinq fichiers
      cœur divergent (GTK 4 a retiré `gtk_main`/`gtk_socket`/le modèle d'évts) ;
      les réconcilier reste le vrai travail.
- [ ] Formaliser une **IR** consommée par le backend → permettre une `.so`
      partagée propre (le cœur n'appellerait plus `widget_*_create`).

## Langage

- [x] **`<grid>`** (livré le 2026-09-13) : sept ports,
      bancs 7 × 29 au vert, captures à l'appui. Voir CHANGELOG 2.2.0.
- [x] **`<paned>`** (livré le 2026-09-14) : sept ports, bancs
      7 × 30 au vert, poignée vérifiée à l'écran (souris sur les six GUI,
      flèches en terminal). Voir CHANGELOG 2.3.0.

## Exports numériques — la même famille, pas encore traitée

- [x] `<levelbar>` et `<drawingarea>` alignés sur les sept ports le 2026-09-14
      (CHANGELOG 2.3.1), écriture rendue insensible à la locale.
- [x] **La famille numérique est alignée** (2026-09-14, CHANGELOG 2.4.0) :
      `<hscale>`, `<vscale>`, `<spinbutton>`, `<progressbar>` rendent la même
      valeur sur les sept ports, `digits=` est honoré partout, et le nombre
      s'écrit avec un POINT quelle que soit la locale (vérifié fr_FR, C, de_DE).
      Cas de banc 32.

## Langage

- [x] **`<toolbar>`, `<stack>`, `<wizard>`, `<menubutton>`** livrés le
      2026-09-14 sur les sept ports (CHANGELOG 2.5.0). Bancs 7 × 36 au vert.
- [x] **`<flowbox>`, `<overlay>`, `<revealer>` portés** le 2026-09-14 sur les
      sept ports (CHANGELOG 2.6.0) : ils avaient un jeton dans la grammaire et
      une implémentation sur le seul gtk4 ; sur les six autres le tag
      s'analysait et ne produisait RIEN. Cas de banc 37.

## Long terme

- [ ] Cloisonnement de processus (IPC) : le shell confiné au cœur, backend séparé.
- [ ] Backends supplémentaires possibles via le contrat de frontière.

## Toujours vrai (garde-fous)

- Ne jamais régresser ce que joue **`ci/bancs.sh`** (XML 55 · comportement 52 ·
  gardes · exemples réels).
- Ne jamais retirer un drapeau de durcissement (voir [SECURITY.md](SECURITY.md)).
