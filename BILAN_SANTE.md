# Bilan de santé — sermo (édition modulaire)

## 3.1.0 — mesuré le 18/10/2026

Sur l'arbre de construction de la 3.1.0, mêmes commandes :

- `bash ci/construire.sh` : tout construit — les neuf ports, lvgl léger,
  sermoia-mcp et les huit programmes d'aide ; **0 avertissement** (cc 16.2,
  cmake 4.3.4).
- `bash ci/bancs.sh` complet, passage final de la 3.1.2 (2026-10-19) : 262 bancs, 0 échec.
- Au passage complet du 18/10/2026, un banc était rouge : le
  banc des fonctions interdites sur `sermo-source` (`strcpy` et `strtod` dans
  le code du lecteur son et vidéo). Corrigé (copie bornée par `memcpy`,
  nombres lus en locale C par `strtod_l`), puis rejoué : vert.
- Comportement : **78 cas**, à parité sur les **9 ports sur 9**.
- Paquets : **27** paquets binaires (`debian/control`), dont 20 propres à une
  architecture.
- Exemples : **76 dossiers** dans `examples/`, dont 15 nouveaux en 3.1.0,
  livrés par `sermo-doc`.
- Programmes d'aide : **8** (`sermo-source-modbus`, `-opcua`, `-mqtt`,
  `-mavlink`, `-media` ; `sermo-cible-mqtt`, `-mavlink`, `-media`), installés
  dans `/usr/libexec/sermo` ; chacun a les 8 garanties de durcissement,
  mesurées par `ci/bancs.sh` sur le binaire.

## 3.0.0 — mesuré du 14 au 15/10/2026

Repris de la section « Mesuré » de la 3.0.0 dans [CHANGELOG.md](CHANGELOG.md) :

- Debian (gcc), `ci/bancs.sh` (14/10) : 208 bancs ; sur la ligne réunie, 207
  verts, puis 208 sur 208 en 3.0.0. Le rouge du premier passage revenait par
  moments : lvglsermo s'arrêtait sur une erreur de segmentation quand l'écran
  X ne s'ouvrait pas.
- ASan et UBSan (15/10) : les neuf ports construits sous les deux, banc de
  comportement joué erreurs fatales : aucune erreur, parité sur les neuf.
  Fuites, mesurées sur ncurses et html : environ 31 octets par
  rafraîchissement d'un widget (export des variables) ; corrigé avant la
  publication de la 3.0.0 (banc `tests/garde_fuite_refresh.sh`).
- Poids (binaire sans symboles, comme dans le paquet) : de 343 Kio (efl1) à
  1 167 Kio (sdl3) et 2 399 Kio (lvgl) ; un plafond par port, vérifié par banc.
- Android 15 arm64 (Termux, clang 21) : neuf cibles sur dix construites ; banc
  de comportement 61/61 pour gtk3, gtk4, qt6, sdl3, lvgl, ncurses, html et
  fltk1, sous Xvfb.
- FreeBSD 15.1 et OpenBSD 7.9, en machine virtuelle : ncurses 61/61 ; scripts
  métier et boîtes à outils, 187 dialogues identiques à gtk3sermo.
- Windows : banc de comportement 61/61 sous Wine 10 ; essais sur un vrai
  Windows 11 et un vrai Windows 10 (affichage, accents, commandes, liens).

Les sections suivantes sont historiques : elles décrivent l'état mesuré à
leur date.

## 2.8.2 — mesuré le 2026-10-11

Sur l'arbre de construction de la 2.8.2 :

- `bash ci/construire.sh` : tout construit ; le cœur et les ports touchés, sans
  nouvel avertissement (les anciens de la 2.8.1 reviennent quand leurs fichiers
  se recompilent). lvgl se lie désormais en C++ (ThorVG, pour le SVG).
- `bash ci/bancs.sh` : 177 bancs (le banc des scripts métier et des boîtes à
  outils s'y ajoute), 174 verts au premier passage. Les trois rouges : la garde
  des lectures, qui ne connaissait pas encore la lecture d'une icône à
  recolorer (nommée, plafonnée, verte) ; `efl1 clic widgets`, une fenêtre pas
  apparue à temps sur un poste chargé (vert rejoué seul, 17 widgets) ; le banc
  des scripts, arrêté par la limite de 30 minutes qu'il dépasse désormais (il a
  la sienne, 90 minutes). Après la dernière correction de lvgl,
  `ci/bancs.sh lvgl` rejoué.
- `bash examples/metiers/verifie.sh` : **104 scripts, 187 dialogues, neuf
  ports, 0 écart** avec gtk3sermo.
- Ce que les bancs ne voient pas, les images : la galerie du site a été refaite
  port par port ; elle a montré les écarts de htmlsermo (forme de la fenêtre),
  les icônes absentes (gtk4, qt6, efl1, lvgl) et la page d'onglets de lvgl,
  corrigés (voir CHANGELOG.md).
- Paquets **2.8.2-2** (boîtes rangées comme gtk3 sur les neuf ports, bouton à
  icône centré), le même jour : `ci/bancs.sh`, 177 bancs, 176 verts. Le rouge,
  lvgl « source sans fin » : une ligne de 20 000 caractères figeait la mise en
  page ; le texte d'un `<edit>` est désormais posé après la construction, puis
  `ci/bancs.sh lvgl` rejoué, 57/57 ; `examples/metiers/verifie.sh`,
  104 scripts, 187 dialogues, neuf ports, 0 écart ; banc XML 55/55 sur chaque
  port. Chiffres relevés le 2026-10-11 sur le commit de la 2.8.2-2.
- Paquets **2.8.2-3** (2026-10-12) : un correctif ncurses (les variables de
  sortie restaient effacées avec l'écran alterné, nouveau crochet facultatif
  `sermo_be_before_export()` du contrat), `debian/copyright` corrigé (ThorVG est
  compilé dans lvglsermo depuis la 2.8.2) et la documentation réalignée. Le
  cœur et le contrat changent : tous les bancs rejoués, `ci/bancs.sh` 177 joués,
  177 verts, 0 en échec (dont `examples/metiers/verifie.sh` sur les neuf ports).
  Le banc ajouté ensuite pour ce correctif (`tests/garde_ncurses_sortie_terminal.sh`,
  178e banc) a été joué seul sur la même construction : vert.

## 2.8.1 — mesuré le 2026-10-10

Mêmes commandes, sur l'arbre de construction de la 2.8.1 :

- `bash ci/construire.sh` : tout construit. Le changement de version recompile
  tout, ce qui montre 11 avertissements anciens, dans des fichiers que la 2.8.1
  ne touche pas (`_POSIX_C_SOURCE` redéfini dans sdl3, lvgl et ncurses ;
  chaîne constante en `gchar *` dans fltk1 ; `gtk_scrolled_window_add_with_viewport`
  déprécié dans le cœur GLib).
- `bash ci/bancs.sh` : 176 bancs, 175 verts au premier passage ; comportement
  61/61 à parité sur les neuf ports. Le rouge, `sdl3 échappement`, est le garde
  qui ouvre une vraie fenêtre et clique dedans : il n'a pas trouvé la fenêtre
  à temps (« aucune ligne VAL= »). Mesuré : le binaire sdl3 de la 2.8.0 rate
  aussi, 2 passes sur 16 (la 2.8.1 : 1 sur 16). `ci/bancs.sh sdl3`, rejoué
  seul : 56 bancs, 0 en échec.
- Rejoué le 2026-10-11, après les corrections de `sermoia-mcp` : 176 bancs, 175
  verts. Le rouge, `liens de langue`, était un lien de `CHANGELOG.en.md` vers ce
  bilan (en français) qui ne le disait pas : corrigé, garde vert rejoué seul.
  `sermoia-mcp` passe ses cinq contrôles (schéma, catalogue, protocole, rendu en
  image sans commande exécutée, délais) ; quatre mutations du serveur le font
  chacune échouer.
- Ce que les bancs ne voyaient pas : les images. La 2.8.1 corrige ce qu'une
  revue des captures du site, port par port contre gtk3sermo, a montré
  (voir CHANGELOG.md).

## 2.8.0 — mesuré le 2026-10-10

Les **deux mêmes commandes** que ci-dessous, sur les **neuf** ports, jouées sur
l'arbre de construction (pas encore sur les binaires des paquets 2.8.0-1) :

```sh
bash ci/construire.sh     # cœur ×3 variantes, neuf backends, sermoia-mcp
bash ci/bancs.sh          # 176 bancs : 176 verts, 0 en échec
```

| Backend | XML (55) | Comportement (61) | Durcissement (8) | Commandes permises · sources sans fin · `--do` · limite de widgets | `--include` | `--glade-xml` | `<terminal>` | Échappement (4) | Clic (17) | Exemples réels |
|---|:--:|:--:|:--:|:--:|:--:|:--:|:--:|:--:|:--:|:--:|
| gtk3 (étalon) | ✅ | ✅ | ✅ | ✅ | ✅ | ✅ chargé | ✅ VTE | ✅ | ✅ | ✅ 55/55 |
| gtk4  | ✅ | ✅ | ✅ | ✅ | ✅ | ✅ chargé | ✅ VTE | ✅ | ✅ | ✅ 55/55 |
| qt6   | ✅ | ✅ | ✅ | ✅ | ✅ | ✅ refusé | ✅ non promis | ✅ | ✅ | ✅ 54/54 |
| fltk1 | ✅ | ✅ | ✅ | ✅ | ✅ | ✅ refusé | ✅ non promis | ✅ | ✅ | ✅ 54/54 |
| efl1  | ✅ | ✅ | ✅ | ✅ | ✅ | ✅ refusé | ✅ non promis | ✅ | ✅ | ✅ 54/54 |
| sdl3  | ✅ | ✅ | ✅ | ✅ | ✅ | ✅ refusé | ✅ non promis | ✅ | ✅ | ✅ 54/54 |
| lvgl  | ✅ | ✅ | ✅ | ✅ | ✅ | ✅ refusé | ✅ non promis | ✅ | ✅ | ✅ 54/54 |
| ncurses | ✅ | ✅ | ✅ | ✅ | ✅ | ✅ refusé | ✅ non promis | — terminal | — | — |
| html  | ✅ | ✅ | ✅ | ✅ | ✅ | ✅ refusé | ✅ non promis | ✅ | — | — |

- **Comportement** : 61 cas, les neuf ports à parité avec l'étalon gtk3sermo.
- **ncurses** joue ses deux gardes propres (mot de passe masqué dans un vrai
  pseudo-terminal, sortie capturée) ; **html** n'ouvre pas de fenêtre : ni clic,
  ni taille de fenêtre, ni exemples sous xdotool.
- **Transverses** : sources (SPDX, fonctions interdites, en-têtes de poste,
  lectures d'`<input>`), façade, fortification du cœur, `sermoman-mcp` (vérité,
  exemples) et `sermoia-mcp` (schéma, exemples, protocole) : tous verts.

Les sections qui suivent sont l'**édition 2.7.4** (sept ports, binaires des
paquets 2.7.4-1) : elles gardent leurs chiffres et leurs renvois.

## État vérifié — 2.7.4, 2026-09-30

> **Ce que valent ces chiffres.** Ce ne sont pas des notes d'auto-évaluation :
> ce sont des **comptes de bancs rejouables**. Chaque ligne ci-dessous se
> reproduit par une commande, à la date indiquée. Ne leur accordez de crédit que
> parce qu'elles se rejouent — et là où un banc n'est **pas** rejouable, c'est
> dit explicitement.

Tout ce qui suit sort de **deux commandes**, jouées sur un clone neuf limité à
deux cœurs (la taille d'un exécuteur partagé de CI). Le 2026-09-30, elles ont
tourné par `packaging/bancs-sur-paquets.sh` (`SERMO_TASKSET=0,1`) : les sept
binaires mesurés sont ceux des paquets 2.7.4-1, pas ceux de l'arbre :

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
| Calendrier, quel que soit le jour (`garde_calendrier_date_du_jour.sh`) | ✅ sept ports : date du jour du binaire fixée (`faketime`) au 31 janvier, au 30 septembre et au 15 juin ; 29 février et fins de mois posés chaque jour. Sur la 2.7.3, gtk4 y est rouge les deux jours pièges |
| Bancs sur les binaires des paquets (`packaging/bancs-sur-paquets.sh`) | ✅ 2.7.4-1 : sept binaires livrés en place, symboles de débogage trouvés ×7 ; 138/139 verts à la 1re passe, sur deux cœurs ; le dernier (clic d'efl1 : la fenêtre du calendrier en retard, pas un plantage) vert rejoué seul, trois fois sur trois |
| CI dans une Debian testing vierge (`.gitlab-ci.yml`, rejouée en conteneur) | ✅ 139/139 le 2026-09-30, sur l'arbre ; la CI de GitLab était rouge sur 13 bancs faute de trois paquets dans `ci/dependances.txt` (voir CHANGELOG 2.7.4) |
| Ancrage Wayland de gtk3 (`garde_layer_shell.sh`) | ✅ lié à libgtk-layer-shell ; sous un sway sans écran, `layer="top"` donne une surface de couche, le témoin une fenêtre ordinaire (niveau non joué en root, où sway refuse de tourner) |

### Ce que la 2.7.4 ferme

Un `<calendar>` de gtk4 posait le mauvais mois selon le jour où le dialogue
s'ouvrait : le 30 septembre, un `<default>` 2000-02-29 rendait 2000-09-29. Vu le
2026-09-30 par la CI de GitLab (le banc de comportement était vert la veille, le
29) ; la garde du calendrier le voit désormais n'importe quel jour.

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

**3.1.0** (voir la section en tête) : le même XML, le même comportement sur
les **neuf ports** (78 cas à parité, mesuré le 18/10/2026), depuis un cœur
unique et durci ; huit programmes d'aide durcis ; 27 paquets binaires ;
`ci/bancs.sh` complet : 262 bancs, 0 échec (passage final de la 3.1.2, 2026-10-19).

**Historique, état du 2026-09-30 (2.7.4)** — gardé tel quel :
sermo remplit son objet : **le même XML, le même comportement, sur sept
toolkits, depuis un cœur unique et durci**, avec des paquets légers et disjoints.
Le 2026-09-30, `ci/bancs.sh` a joué ses 139 bancs sur les binaires des paquets
2.7.4-1, sur deux cœurs : XML 55 et comportement 53 sur les sept backends,
calendrier juste quel que soit le jour, terminal et ancrage Wayland vérifiés,
sources sans fin arrêtées à la limite, documentation servie conforme au code ;
138 verts d'une traite, le dernier (une fenêtre en retard) vert rejoué seul.
Les paquets Debian se construisent, s'installent, se purgent et remplacent la 1.x
dans une Debian testing vierge. Reste la dette architecturale de la Phase 3, sans
impact fonctionnel, et les points ouverts du TODO.
