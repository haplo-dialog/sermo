# Changelog - haplo-dialog

Toutes les modifications notables de ce projet sont documentées ici.  
Format : [Keep a Changelog](https://keepachangelog.com/fr/1.0.0/)  
Versionning : [Semantic Versioning](https://semver.org/lang/fr/) à partir de 1.0.0.

---

## [Unreleased] - v1.1.0 (en cours)

### Ajouté
- **Ancrage Wayland de `<window>` (2026-08-20)** : trois attributs, `layer` (`background`/`bottom`/`top`/`overlay`), `edge` (les 4 bords, les 4 coins, et 6 formes « bande » qui arriment deux bords opposés) et `dist` (marge 0-200 px, 20 par défaut), transforment le dialogue en surface *wlr-layer-shell* : barre, dock, widget de bureau. Portage du fork gtk3dialog de BunsenLabs (GPL-2.0+), avec deux corrections par rapport à l'original : la marge, lue dans un type non signé puis contrôlée par un `< 0` qui ne peut jamais se déclencher, est désormais analysée par `strtol()`, refuse une fin de chaîne parasite et est bornée à [0, 200] ; et la détection de Wayland, qui lisait la variable d'environnement `GDK_BACKEND` et continuait quand elle n'était pas définie (le cas normal sous X11), interroge maintenant la bibliothèque elle-même (`gtk_layer_is_supported()`). Hors Wayland, ou sur un compositeur sans le protocole (GNOME), les trois attributs sont ignorés et une fenêtre ordinaire s'ouvre. Dépendance optionnelle `gtk-layer-shell ≥ 0.8.0` ; `./configure --without-layer-shell` s'en passe. Documenté dans `haplo-dialog-xml(5)`, exemple dans `examples/layer-shell`. **Ancrage éprouvé sous sway 1.12 (wlroots 0.20) et mesuré au pixel** : barre `topstride` collée en haut sur toute la largeur, dock `bottom` `dist="24"` à exactement 24 px du bord, bande `background` intégralement recouverte par une fenêtre ordinaire, `dist="0"` et `dist="60"` séparés de 60 px exactement. Non couvert : le multi-écran, le matériel réel, et les compositeurs hors wlroots comme Hyprland.
- **Compatibilité ascendante gtkdialog (2026-06-06)** : `make install` pose un **symlink `gtkdialog` → `gtk3dialog`** (et `gtkdialog.1` → `gtk3dialog.1`) via le hook autotools ; un dialogue d'époque (`export MAIN_DIALOG='<window …>'; gtkdialog --program=MAIN_DIALOG`) parse, s'exécute et rend sa sortie au format historique (`VAR="valeur"`). La cohérence du symlink est répercutée dans chaque recette de paquet (Debian `.links`, RPM `%files`, etc.).
- `detect_terminal()` / `detect_editor()`, auto-détection de l'environnement graphique (xfce4-terminal, konsole, gnome-terminal, mousepad, kate, gedit…)
- Suite XML étendue à **53 cas de test** (étiquettes et valeurs par défaut vides, searchentry, levelbar, drawingarea, colorbutton, fontbutton, aspectframe, tree, table, menubar, statusbar, togglebutton, timer, edit, list, separators, infobar types, notebook 3 pages, actions REFRESH/ENABLE/DISABLE/SHOW/HIDE/CLEAR, formulaire complexe)
- `AUTHORS` et `NEWS` à la racine (standard GNU)

### Modifié
- `LOGO_TMP` utilise un nom fixe par UID (`/tmp/haplo-logo-UID.png`), évite la fuite en cas de `SIGKILL`
- ALLOWED_CMDS élargi : pacman, dnf, zypper, emerge, slackpkg, xbps-install, xbps-query, apk
- Licence **uniformisée à GPL-2.0-or-later** sur tout le dépôt (en-têtes source, packaging, `LICENCES.md`, `CONTRIBUTING.md`), l'essai GPL-3.0+ a été annulé, conformément à la clause « either version 2 … any later version » des sources et à l'amont gtkdialog

### Corrigé
- **Élément à contenu textuel vide (2026-08-20)** : `<label></label>`, ou une étiquette faite uniquement d'espaces, cassait l'analyse avec un « syntax error » pointant la balise fermante, alors que poser un espaceur en écrivant `<text><label>   </label></text>` est le réflexe naturel. `<label>` et `<default>` acceptent désormais le vide et valent `""` (comme `<item>` le faisait déjà) ; `<sensitive>`, `<width>`, `<height>`, `<input>`, `<output>`, `<variable>` et `<action>` restent une erreur, mais avec un message qui nomme la cause : « the <variable> element is empty; it requires a name. »
- **Troncature des noms de widgets auto-générés (2026-06-06)** : `g_snprintf(name, sizeof(name), …)` où `name` est un `char*`, `sizeof` valait donc 8 octets, tronquant les noms, corrigé en passant la taille du tampon à 64 ; restaure le comportement gtkdialog d'origine
- **Lecture de variable non initialisée (2026-06-06)** : `instruction inst;` dont `inst.ival` était lu → `instruction inst = {0};`
- **Compilation propre (2026-06-06)** : 0 erreur ; les warnings résiduels relèvent d'idiomes amont gtkdialog et de 2 conflits shift/reduce bison intouchables

---

## Révisions d'empaquetage de la 1.0.0

Le **logiciel** reste en version 1.0.0. Les révisions -2 et -3 ne changent que le
paquet Debian et livrent un binaire identique ; la -4 fait exception et embarque
l'ancrage Wayland décrit plus haut.

### [1.0.0-4] - 2026-08-20
- Premier paquet embarquant l'**ancrage Wayland** (`layer`/`edge`/`dist` sur
  `<window>`, voir la section 1.1.0 ci-dessus). Contrairement aux deux révisions
  précédentes, le binaire n'est **pas** identique au précédent.
- Nouvelle dépendance optionnelle `libgtk-layer-shell0` (build :
  `libgtk-layer-shell-dev ≥ 0.8.0`).
- Ancrage éprouvé sous sway 1.12 (wlroots 0.20) et mesuré au pixel : barre, dock,
  couche `background` et marge `dist` conformes.

### [1.0.0-3] - 2026-08-16
- Identité du mainteneur du paquet : le champ `Maintainer` portait un nom
  personnel et un domaine privé, visibles par `apt show gtk3dialog` chez chaque
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
- **gtk3dialog** : port de référence (backend GTK 3), fournissant l'alias rétro-compatible `gtkdialog`
- **43 widgets** implémentés
- Nouveaux widgets : `<switch>`, `<password>`, `<searchentry>`, `<calendar>`, `<infobar>`, `<levelbar>`, `<spinner>`, `<aspectframe>`, `<drawingarea>`
- `safe_system()` / `safe_popen()`, remplacement sécurisé de `system()` / `popen()`
- Durcissement : `FORTIFY_SOURCE=3`, PIE, Full RELRO, NX stack, stack canary (`-fstack-protector-strong`), CFI (`-fcf-protection`)
- Manpage roff `gtk3dialog(1)`
- `haplo-dialog-xml(5)`, manpage de référence de la syntaxe XML
- Documentation Texinfo (`.texi` → `.info`)
- Site web de documentation (HTML statique)
- Recettes d'empaquetage : `.deb` (Debian, seule construite par le projet), `.rpm` (Fedora/SUSE), `PKGBUILD` (Arch), `.ebuild` (Gentoo), `.SlackBuild` (Slackware)
- Scripts d'exemple couvrant tous les widgets (`examples/`, 53 répertoires)
- Suite de régression XML (`tests/xml/`)
- `SECURITY.md`, `.clang-format`, `.editorconfig`
- `CONTRIBUTING.md`, `CODE_OF_CONDUCT.md`, `ROADMAP.md`

### Modifié
- Renommage du binaire : `gtkdialog` → `gtk3dialog`
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
- Structure initiale du port gtk3dialog
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

