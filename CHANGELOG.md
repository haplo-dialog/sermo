# Changelog - haplo-dialog

Toutes les modifications notables de ce projet sont documentées ici.  
Format : [Keep a Changelog](https://keepachangelog.com/fr/1.0.0/)  
Versionning : [Semantic Versioning](https://semver.org/lang/fr/) à partir de 1.0.0.

---

## [Unreleased] - v1.1.0 (en cours)

### Ajouté
- **`--do=CMD` (2026-08-24)** : l'option existait dans les pages de manuel installées et dans le manuel info depuis toujours, sans exister dans aucun binaire — qui recopiait l'exemple officiel obtenait un abandon. Elle exécute la commande après la fermeture de la fenêtre, valeurs des widgets exportées dans l'environnement, via `safe_system()` : `exec()` direct hors métacaractères, repli `/bin/sh -c` journalisé sinon, refus net sous `HAPLO_NO_SHELL_FALLBACK=1`. Branchée par `atexit()`, parce que deux sorties directes ne repassent pas par la fin de `main()`.
- **`HAPLO_ALLOWED_CMDS` (2026-08-24)** : borne facultative des commandes lançables (`HAPLO_ALLOWED_CMDS=ls,cat,date`). **Éteinte par défaut**, à dessein : le langage sert à lancer des commandes et 14 des exemples livrés appellent `bash` ou `sh`. Elle vise celui qui *déploie* un dialogue dans un contexte moins fiable. Le nom comparé est le nom de base, et tant qu'elle est posée le repli shell est refusé — sinon `sh -c` la contournerait.
- **Bancs de non-régression (2026-08-22 → 25)** : `tests/run_examples.sh` lance réellement les exemples sous Xvfb au lieu de vérifier qu'ils existent ; `garde_progressbar_thread.sh`, `garde_maxwidgets.sh`, `garde_option_do.sh`, `garde_allowed_cmds.sh`, `garde_durcissement.sh`, `garde_spdx.sh` et `garde_fonctions_interdites.sh` tiennent chacun une promesse du dépôt. Tous branchés dans la CI, sur les deux ports.
- **`garde_fonctions_interdites.sh` (2026-08-25)** : `SECURITY.md` affirme qu'aucune fonction interdite n'est appelée dans `src/`. Une affirmation pareille est vraie le jour où on l'écrit et plus personne ne la revérifie — c'est ainsi que trois `strtod()` ont survécu quatre mois. Le banc la rejoue à chaque poussée sur les deux `src/`, pour `strcpy`, `strcat`, `sprintf`, `vsprintf`, `gets`, `system`, `popen`, `atof` et `strtod`. Deux pièges traités : les **commentaires** sont retirés avant la recherche (`safe_exec.c` cite légitimement `system()` et `popen()` pour dire qu'il les remplace), et une frontière de mot est exigée **à gauche**, faute de quoi `safe_system` compterait pour `system` et `g_ascii_strtod` pour `strtod`. Éprouvé par six sabotages, dont un appel réel, un appel en commentaire simple, un en commentaire multiligne, et un répertoire vide qui doit échouer au lieu de passer en silence.
- **Port `gtk4sermo` publié (2026-08-21)** : second backend, même grammaire, quatre balises de plus (`flowbox`, `overlay`, `revealer`, `stack`). La CI le construit et l'éprouve désormais comme l'autre.
- **Ancrage Wayland de `<window>` (2026-08-20)** : quatre attributs, `layer` (`background`/`bottom`/`top`/`overlay`), `edge` (les 4 bords, les 4 coins, et 6 formes « bande » qui arriment deux bords opposés), `dist` (marge 0-200 px, 20 par défaut) et `reserve` (`yes`/`no`, `no` par défaut : la surface flotte au-dessus des fenêtres ordinaires, ou bien le compositeur lui réserve sa place et dispose les autres à côté), transforment le dialogue en surface *wlr-layer-shell* : barre, dock, widget de bureau. Portage du fork gtk3sermo de BunsenLabs (GPL-2.0+), avec deux corrections par rapport à l'original : la marge, lue dans un type non signé puis contrôlée par un `< 0` qui ne peut jamais se déclencher, est désormais analysée par `strtol()`, refuse une fin de chaîne parasite et est bornée à [0, 200] ; et la détection de Wayland, qui lisait la variable d'environnement `GDK_BACKEND` et continuait quand elle n'était pas définie (le cas normal sous X11), interroge maintenant la bibliothèque elle-même (`gtk_layer_is_supported()`). Hors Wayland, ou sur un compositeur sans le protocole (GNOME), les quatre attributs sont ignorés et une fenêtre ordinaire s'ouvre. Dépendance optionnelle `gtk-layer-shell ≥ 0.8.0` ; `./configure --without-layer-shell` s'en passe. Documenté dans `haplo-dialog-xml(5)`, exemple dans `examples/layer-shell`. **Ancrage éprouvé sous sway 1.12 (wlroots 0.20) et mesuré au pixel** : barre `topstride` collée en haut sur toute la largeur, dock `bottom` `dist="24"` à exactement 24 px du bord, bande `background` intégralement recouverte par une fenêtre ordinaire, `dist="0"` et `dist="60"` séparés de 60 px exactement. `reserve="yes"` mesuré par `swaymsg` : une barre de 48 px repousse la fenêtre ordinaire à `y=48` avec `dist="0"`, et à `y=88` avec `dist="20"`, la marge comptant des deux côtés ; `reserve="no"` et l'absence d'attribut laissent la fenêtre occuper toute la sortie. Non couvert : le multi-écran, le matériel réel, et les compositeurs hors wlroots comme Hyprland.
- **Compatibilité ascendante gtkdialog (2026-06-06)** : `make install` pose un **symlink `gtkdialog` → `gtk3sermo`** (et `gtkdialog.1` → `gtk3sermo.1`) via le hook autotools ; un dialogue d'époque (`export MAIN_DIALOG='<window …>'; gtkdialog --program=MAIN_DIALOG`) parse, s'exécute et rend sa sortie au format historique (`VAR="valeur"`). La cohérence du symlink est répercutée dans chaque recette de paquet (Debian `.links`, RPM `%files`, etc.).
- **`detect_terminal()` / `detect_editor()` (2026-08-24)**, dans `examples/system-tools` : auto-détection du terminal et de l'éditeur graphiques installés (`x-terminal-emulator` en premier, puis xfce4-terminal, gnome-terminal, konsole, mate-terminal, lxterminal, xterm ; mousepad, gedit, kate, pluma, gnome-text-editor, xed, leafpad). La détection existait déjà en boucle en ligne ; cette entrée la nommait par des fonctions qui n'existaient pas — elles existent maintenant, et rendent le premier outil trouvé.
- Suite XML étendue à **55 cas de test** (ancrage Wayland, étiquettes et valeurs par défaut vides, searchentry, levelbar, drawingarea, colorbutton, fontbutton, aspectframe, tree, table, menubar, statusbar, togglebutton, timer, edit, list, separators, infobar types, notebook 3 pages, actions REFRESH/ENABLE/DISABLE/SHOW/HIDE/CLEAR, formulaire complexe)
- `AUTHORS` et `NEWS` à la racine (standard GNU)

### Modifié
- `LOGO_TMP` utilise un nom fixe par UID (`/tmp/haplo-logo-UID.png`), évite la fuite en cas de `SIGKILL`
- **`ALLOWED_CMDS` élargi (2026-08-24)**, dans `examples/system-tools` : `dnf`, `zypper`, `pacman`, `emerge`, `slackpkg`, `xbps-install`, `xbps-query`, `apk` s'ajoutent à la famille `apt`, pour que l'aide en ligne de l'exemple réponde ailleurs que sur Debian. La liste continue de refuser tout le reste (`rm` reste refusé, vérifié).
- Licence **uniformisée à GPL-2.0-or-later** sur tout le dépôt (en-têtes source, packaging, `LICENCES.md`, `CONTRIBUTING.md`), l'essai GPL-3.0+ a été annulé, conformément à la clause « either version 2 … any later version » des sources et à l'amont gtkdialog

### Corrigé
- **Les deux ports plantaient dès qu'on s'en servait (2026-08-22)** : une fenêtre contenant une `<list>` mourait à l'ouverture (segfault) — le rappel de `row-selected` n'attendait que deux paramètres sur les trois que GTK passe ; et tout nombre à virgule tuait le programme en locale française (abandon) — `atof()` lit `0.5` comme `0` sous `fr_FR`. 52 appels à `atof()` remplacés par `g_ascii_strtod`. La passe était incomplète : voir l'entrée du 2026-08-25 ci-dessous. Mesuré en locale `fr_FR` : les exemples passent de 26/54 à 55/55 pour gtk3sermo, et 58/58 pour gtk4sermo.
- **La barre de progression faisait tourner GTK depuis son thread de lecture (2026-08-23)** : `gtk_main_iteration_do()` appelé hors du thread principal, sous un `gdk_threads_enter()` qui ne protège plus rien depuis GTK 3.6. Segfault ou abandon environ une fois sur dix. Le thread ne fait plus que lire son tuyau ; tout ce qui touche GTK repart vers la boucle principale par `g_idle_add()`.
- **`make check` ne compilait pas sous `--enable-unit-tests` (2026-08-25)** : l'option est documentée dans `COMPILE.md` et dans la check-list de release, et elle échouait à l'édition de liens avec une trentaine de « undefined reference ». `test_stringman.c` était écrit contre une vingtaine de fonctions `command_is_*()` / `command_get_*()` qui n'existent dans aucun des deux binaires : leurs définitions sont enfermées dans un bloc `/* Redundant: Not being used */` de `stringman.c`, hérité de l'amont gtkdialog, ouvert ligne 76 et refermé ligne 303. Le fichier de test l'avait même écrit noir sur blanc — « ces fonctions existent dans stringman.c » — sans que ce soit vrai. Il porte désormais sur l'API qui les a remplacées (la table de préfixes, lue par `command_get_prefix()` et `command_prefix_get_type()`), plus `strnatcmp`, `input_is_shell_command`, `linecutter` et `str_default_name` : 15 vérifications, dont l'alignement de la table sur l'énumération `CommandType` et le cas d'une commande shell contenant un deux-points. Éprouvé par deux sabotages, tous deux attrapés et nommés. **La CI rejoue maintenant `make check` à chaque poussée** : son absence est précisément ce qui a laissé la casse passer inaperçue.
- **`linecutter()` mélangeait deux allocateurs (2026-08-25)** : les champs après le premier étaient alloués par `strdup()` (malloc) et libérés par `g_free()`. Sans conséquence visible sur glibc, mais formellement indéfini. Passés à `g_strdup()`. Au passage, le commentaire d'origine affirmait que la fonction « ne détruit pas la chaîne d'origine, elle en fait des copies » : c'est faux deux fois — elle écrit des `\0` dedans, et `line[0]` **est** la chaîne d'origine, que `list_t_free()` libère ensuite. Le contrat d'appartenance réel est maintenant écrit au-dessus de la fonction ; s'y tromper, c'est passer un littéral et faire libérer une adresse que GLib n'a jamais allouée.
- **Trois `strtod()` avaient survécu à la passe sur les locales (2026-08-25)** : la campagne du 2026-08-22 avait bien retiré les 52 `atof()`, mais laissé trois `strtod()` par port, qui souffrent exactement du même mal — c'est la locale qui décide où se trouve la virgule. Mesuré sous `fr_FR.UTF-8` : `strtod("0.9")` rend **0,000**, quand `g_ascii_strtod("0.9")` rend **0,900**. Les trois sites : un attribut XML flottant dans `tag_attributes.c` (par exemple `xalign="0.9"`), et deux colonnes numériques de `<tree>` dans `widget_tree.c`. Autrement dit, un nombre écrit avec un point dans un fichier XML valait zéro **en silence** sur toute machine française. Corrigés dans les deux ports, et désormais interdits par un banc.
- **Sûreté mémoire du port GTK 4 (2026-08-24)** : `strncpy` dans un tampon non mis à zéro (un nom de variable de 512 caractères ou plus n'était pas terminé), recopie des widgets d'un conteneur sans borne `MAXWIDGETS` (300 enfants directs étaient acceptés en silence), et `action_append()` qui recopiait son premier paramètre depuis le début de la chaîne. Le port GTK 3 avait déjà les trois garde-fous ; les deux ports sont alignés.
- **Filtrage de l'environnement enfant, port GTK 4 (2026-08-24)** : `_build_child_env()` manquait entièrement — 60 lignes d'écart avec le port GTK 3 — alors que `SECURITY.md` annonçait le filtrage pour le produit.
- **Élément à contenu textuel vide (2026-08-20)** : `<label></label>`, ou une étiquette faite uniquement d'espaces, cassait l'analyse avec un « syntax error » pointant la balise fermante, alors que poser un espaceur en écrivant `<text><label>   </label></text>` est le réflexe naturel. `<label>` et `<default>` acceptent désormais le vide et valent `""` (comme `<item>` le faisait déjà) ; `<sensitive>`, `<width>`, `<height>`, `<input>`, `<output>`, `<variable>` et `<action>` restent une erreur, mais avec un message qui nomme la cause : « the <variable> element is empty; it requires a name. »
- **Troncature des noms de widgets auto-générés (2026-06-06)** : `g_snprintf(name, sizeof(name), …)` où `name` est un `char*`, `sizeof` valait donc 8 octets, tronquant les noms, corrigé en passant la taille du tampon à 64 ; restaure le comportement gtkdialog d'origine
- **Lecture de variable non initialisée (2026-06-06)** : `instruction inst;` dont `inst.ival` était lu → `instruction inst = {0};`
- **Compilation propre (2026-06-06)** : 0 erreur ; les warnings résiduels relèvent d'idiomes amont gtkdialog et de 2 conflits shift/reduce bison intouchables

---

## Révisions d'empaquetage de la 1.0.0

Le **logiciel** reste en version 1.0.0. Les révisions -2 et -3 ne changent que le
paquet Debian et livrent un binaire identique ; à partir de la -4 elles changent
aussi le binaire.

Les deux ports ont chacun son `debian/changelog`, qui fait foi pour lui, et leurs
révisions peuvent diverger : un correctif ne touche pas toujours les deux. État au
2026-08-24 : **gtk3sermo 1.0.0-10**, **gtk4sermo 1.0.0-11**.

### [1.0.0-11] - 2026-08-22 — gtk4sermo seulement
- Page de manuel : la section SEE ALSO renvoyait vers `fltk1d`, `efl1d`, `qt6d` et
  d'autres ports jamais publiés.

### [1.0.0-10] - 2026-08-22 — les deux ports
- Menus GTK 4 : `<menuitem>` rendait un `GtkPopoverMenu` autonome, empilé dans la
  fenêtre au lieu d'être attaché à sa barre.

### [1.0.0-9] - 2026-08-21 — les deux ports
- **Renommage.** Le paquet s'appelait `gtk3dialog`, nom déjà porté par un paquet de
  BunsenLabs, puis `haplo-dialog` — qui est le nom du produit, pas celui d'un port.
  Les ports s'appellent désormais `gtk3sermo` et `gtk4sermo`, et le lien de
  compatibilité `gtkdialog` vit dans un paquet séparé, `gtksermo`, en conflit
  assumé. Les deux ports s'installent donc à côté de n'importe quelle autre
  implémentation.

### [1.0.0-5] - 2026-08-20
- **Analyseur** : une étiquette vide est acceptée là où le vide est une valeur
  (`<label>`, `<default>`, `<item>` sous sa forme attribuée) ; ailleurs, l'erreur
  nomme enfin sa cause au lieu de désigner la balise fermante. Voir la section
  1.1.0 ci-dessus.
- **Ancrage Wayland éprouvé** sous sway 1.12 (wlroots 0.20), sortie headless
  1280x720, mesuré au pixel : barre `topstride` pleine largeur, dock `bottom` à
  exactement `dist` px du bord, bande `background` intégralement recouverte par
  une fenêtre ordinaire, `dist="0"` et `dist="60"` séparés de 60 px. Le paquet
  -4 est antérieur à cette vérification. Non couvert : le multi-écran, le
  matériel réel, et les compositeurs hors wlroots comme Hyprland.
- Une barre ne réserve **pas** d'espace : la zone exclusive reste à zéro, donc
  une surface ancrée flotte au-dessus des fenêtres ordinaires au lieu de les
  repousser.
- Comme la -4, cette révision change le binaire.

### [1.0.0-4] - 2026-08-20
- Premier paquet embarquant l'**ancrage Wayland** (`layer`/`edge`/`dist` sur
  `<window>`, voir la section 1.1.0 ci-dessus). Contrairement aux deux révisions
  précédentes, le binaire n'est **pas** identique au précédent.
- Nouvelle dépendance optionnelle `libgtk-layer-shell0` (build :
  `libgtk-layer-shell-dev ≥ 0.8.0`).
- Construit avant que l'ancrage soit éprouvé sur un compositeur réel ; c'est la
  -5 qui apporte cette vérification.

### [1.0.0-3] - 2026-08-16
- Identité du mainteneur du paquet : le champ `Maintainer` portait un nom
  personnel et un domaine privé, visibles par `apt show gtk3sermo` chez chaque
  utilisateur. Il porte désormais l'identité du projet.
- `Vcs-Git` et `Vcs-Browser` pointent sur le dépôt réel ; ils désignaient
  auparavant une forge qui n'a jamais existé.
- Aucun changement fonctionnel.

### [1.0.0-2] - 2026-08-11
- Première construction propre du paquet, après la passe de sécurité
  d'avant-publication (voir la 1.0.0 ci-dessous) : `lintian` sans signalement,
  durcissement PIE / RELRO / stack-protector / FORTIFY vérifié.

---

## [1.0.0] - 2026-05-29

Première version publique stable, refonte complète de gtkdialog 0.8.3.

### Ajouté
- **gtk3sermo** : port de référence (backend GTK 3), fournissant l'alias rétro-compatible `gtkdialog`
- **43 widgets** implémentés
- Nouveaux widgets : `<switch>`, `<password>`, `<searchentry>`, `<calendar>`, `<infobar>`, `<levelbar>`, `<spinner>`, `<aspectframe>`, `<drawingarea>`
- `safe_system()` / `safe_popen()`, remplacement sécurisé de `system()` / `popen()`
- Durcissement : `FORTIFY_SOURCE=3`, PIE, Full RELRO, NX stack, stack canary (`-fstack-protector-strong`), CFI (`-fcf-protection`)
- Manpage roff `gtk3sermo(1)`
- `haplo-dialog-xml(5)`, manpage de référence de la syntaxe XML
- Documentation Texinfo (`.texi` → `.info`)
- Site web de documentation (HTML statique)
- Recettes d'empaquetage : `.deb` (Debian, seule construite par le projet), `.rpm` (Fedora/SUSE), `PKGBUILD` (Arch), `.ebuild` (Gentoo), `.SlackBuild` (Slackware)
- Scripts d'exemple couvrant tous les widgets (`examples/`, 53 répertoires)
- Suite de régression XML (`tests/xml/`)
- `SECURITY.md`, `.clang-format`, `.editorconfig`
- `CONTRIBUTING.md`, `CODE_OF_CONDUCT.md`, `ROADMAP.md`

### Modifié
- Renommage du binaire : `gtkdialog` → `gtk3sermo`
- `gtkdialog_parser.y` / `gtkdialog_lexer.l`, conservés
- Élévation via `pkexec` (PolicyKit), `sudo` GUI supprimé
- `fclose()` sur tout `FILE*` issu de `safe_popen()`, `pclose()` banni

### Supprimé
- Dépendances GTK2 résiduelles
- `system()` et `popen()` directs dans les widgets
- Code mort et commentaires parasites

### Sécurité
- CVE-like : aucune vulnérabilité connue au 2026-05-29
- Durcissement `safe_exec.c`, exécution sans shell quand possible, repli `/bin/sh -c` journalisé et désactivable (`HAPLO_NO_SHELL_FALLBACK`)
- Variables exportées par l'environnement, jamais évaluées par l'outil

---

## [0.9.0-haplo1] - 2026-03-01 (interne)

Version de travail initiale, portage depuis gtkdialog 0.8.3.

### Ajouté
- Structure initiale du port gtk3sermo
- Migration du core (parser XML, variables, actions, signaux, pile)
- Implémentation initiale des widgets
- Système de build : autotools

---

## [gtkdialog-0.8.3] - 2013-xx-xx (upstream original)

Dernière version de l'upstream original par László Pere et Thunor.  
Référence : https://github.com/01micko/gtkdialog

---

*haplo-dialog est un fork de gtkdialog 0.8.3 (GPL-2.0+),  
modernisé et étendu par le projet haplo-dialog (GPL-2.0-or-later).*

---

