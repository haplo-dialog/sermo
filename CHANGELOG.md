# Journal des modifications — sermo (édition modulaire)

Format inspiré de [Keep a Changelog](https://keepachangelog.com/fr/).
Versionnage : voir [VERSIONING.md](VERSIONING.md).

La **2.7.3 est la première version 2.x publiée** ; les 2.7.4, 2.7.5, 2.7.7 et 2.8.1 la suivent (la 2.7.6 et la 2.8.0 n'ont été livrées qu'en paquets ; leur contenu est publié avec la version suivante). Les versions 2.0.0 à 2.7.2 sont
restées internes ; elles sont décrites ci-dessous pour garder la trace de ce qui a
changé depuis la 1.x. Pour passer de la 1.x à la 2.x : [MIGRATION.md](MIGRATION.md).

## [2.8.1] — 2026-10-10

Version CORRECTIF. Une revue des captures du site, port par port contre l'étalon
gtk3sermo, a trouvé des widgets mal ou pas dessinés que les bancs ne voyaient
pas : ils lisent les valeurs, pas l'image.

### Sécurité

- **lvgl : `<searchentry>` écrivait au mauvais endroit.** Le texte saisi allait
  dans `state.entry.buf`, qui partage la même mémoire que `state.list.items`
  (un pointeur) : il l'écrasait. Il va désormais dans `state.list.filter`,
  comme sur les autres ports.
- **sermoia-mcp : `sermo_render` exécutait les commandes du XML.** Un `<input>`
  s'exécutait pendant le rendu : demander un aperçu suffisait à lancer une
  commande. Le rendu pose désormais `SERMO_ALLOWED_CMDS` sur une commande qui
  n'existe pas, ce qui interdit tout (repli `sh -c` compris) ; le banc le vérifie
  par un fichier témoin. Seul `sermo_run` exécute, et le dit.

### Amélioré

- **sermoia-mcp**, fonctionnement :
  - délais sur chaque outil, qui tuent tout le groupe de processus (`xvfb-run`
    compris) ; `sermo_run` prend un argument `timeout` ;
  - `sermo_render` montrait du texte au lieu de l'image d'un petit dialogue (les
    PNG de moins de ~800 octets étaient jetés) : l'image est jugée sur son
    en-tête ;
  - `sermo_run` sans affichage signale l'erreur au lieu de « aucune valeur » ;
  - `sermo_check` rend « OK » ou « ERREUR » avec le message de l'analyseur, sans
    le bruit des bibliothèques graphiques ;
  - `sermo_examples` cherche par mots (« formulaire de contact ») dans les noms et
    le contenu, classe par pertinence puis par vote, et donne la carte du
    catalogue sans argument ;
  - `sermo_vote` refuse un sujet hors catalogue ;
  - protocole : `ping`, codes d'erreur JSON-RPC justes, versions 2025-06-18 et
    2025-03-26, champ `instructions`, outils annotés (lecture seule, destructif) ;
  - fichiers temporaires dans un dossier privé.
- **sermoia-mcp**, documentation : README réécrit (chaque outil avec ses
  arguments, ses réponses et ses erreurs ; boucle conseillée ; sécurité ; délais ;
  déclaration dans Claude Code) et version anglaise `README.en.md`. Banc étendu à
  cinq contrôles (rendu en image, témoin de commande, délais) ; quatre mutations
  du code le font chacune échouer.

### Corrigé

- **`<tree>` en colonnes, sous ses en-têtes, sur tous les ports neutres**
  (qt6, fltk1, efl1, sdl3, lvgl, ncurses, html). La ligne brute « a|b|c »
  s'affichait ; fltk1 lisait même le « / » d'un chemin comme un niveau
  d'arborescence. La variable exportée reste la 1re colonne, comme l'étalon.
- **`<statusbar>`** : `<default>` l'emporte sur `<label>`, comme l'étalon. La
  barre restait vide sur les sept ports neutres ; sous qt6, le texte s'écrivait
  ensuite deux fois l'un sur l'autre.
- **`<text>`** montre ce que `<input>` a lu (lvgl, html, sdl3, ncurses), sans
  le saut de ligne final (fltk1 affichait « ^J ») ; la variable garde le texte
  entier.
- **`<expander label="…">`** : l'attribut est lu par les sept ports neutres ;
  sdl3 respecte l'état ouvert ; ncurses dessine le titre, qu'Entrée ouvre et
  ferme.
- **`<levelbar>`** : `range-min` / `range-max` lus (lvgl, html, ncurses, sdl3).
- **`--render-png` sort en code 0** sur fltk1, qt6 et efl1. L'image était
  écrite, mais l'analyseur repartait ensuite et finissait sur « syntax error
  near `<window>` », code 1.
- **lvgl**, dessinés ou redessinés : `<edit>` (invisible), `<calendar>`,
  `<pixmap>` et `<image>` (PNG), `<togglebutton>` (bouton enfoncé, non plus une
  case), `<linkbutton>` (lien souligné), `<menubutton>` (liste déroulante),
  `<stack>` et `<wizard>` (une page à la fois, boutons de l'assistant),
  `<grid>` (rangées), `<overlay>` (couches superposées), `<vscale>` (verticale),
  `<infobar>` (teinte selon `message-type`), `<aspectframe label>`, séparateur
  vertical dans une rangée, bouton radio rond et exclusif, une seule ligne
  choisie dans `<list>`. `width-request` d'une boîte est un minimum, et une
  taille de fenêtre demandée ne descend plus sous le contenu : le formulaire
  480×420 sortait coupé sous ses boutons.
- **html** : `<vscale>` verticale, teinte de `<infobar>`, `<aspectframe label>`.

### Mesuré

- `ci/bancs.sh` : 176 bancs, 175 verts au premier passage ; le rouge (un garde
  qui n'a pas trouvé sa fenêtre à temps, défaut du banc qui touche aussi la
  2.8.0) est vert rejoué seul. Comportement 61/61 à parité sur les neuf ports.
  Détail : [BILAN_SANTE.md](BILAN_SANTE.md).

## [2.8.0] — 2026-10-10

Version MINEURE. Deux ports de plus — neuf en tout —, un second serveur MCP, des
recettes de paquets pour six autres systèmes, et l'histoire de gtkdialog.

### Ajouté

- **`htmlsermo`, le port HTML.** Il écrit le dialogue dans une page HTML autonome
  (CSS embarqué, aucun fichier externe ; sans JavaScript, sauf le mode
  `interactive` qui compose le résultat dans la page) au lieu d'ouvrir une
  fenêtre : `SERMO_HTML_OUT` choisit le fichier (défaut `sermo-output.html`).
  `SERMO_HTML_MODE=serve` sert la page sur la boucle locale et renvoie les valeurs
  saisies, comme une vraie fenêtre de dialogue. Aucune bibliothèque graphique :
  il s'installe sur une machine sans affichage. Paquet `sermo-backend-html` ;
  alternative `sermo` à la priorité 1, il ne la prend que seul installé.
- **`lvglsermo`, le port LVGL.** LVGL v9.6.0, embarqué (absent de Debian), MIT.
  Wayland ou X11 ; framebuffer et entrées evdev sur demande seulement
  (`SERMO_LVGL_FBDEV`, `SERMO_LVGL_EVDEV`). Paquet `sermo-backend-lvgl` ;
  priorité 8.
- **`sermoia-mcp`**, second serveur MCP, à côté de `sermoman-mcp` qui sert le
  manuel : il fait écrire, vérifier et voir un dialogue (`sermo_schema`,
  `sermo_check`, `sermo_run`, `sermo_render`, `sermo_examples`, `sermo_vote`).
  Son schéma est vérifié contre les balises de l'analyseur ; il n'est pas
  empaqueté. Banc : `sermoia-mcp/tests/verifie.sh`.
- **Recettes hors Debian** (`packaging/autres-os/`) : Arch, RPM, Gentoo,
  Slackware, Alpine et NixOS, sur une seule suite de construction
  (`construire-os.sh`).
- **`HISTOIRE.md`** : l'histoire de gtkdialog, de László Pere à aujourd'hui,
  sources à l'appui.

### Corrigé

- **Attributions** : l'ancrage Wayland de `gtk3sermo` a été écrit dans gtkdialog
  chez woof-CE (Puppy Linux), pas chez BunsenLabs ; Thunor a maintenu gtkdialog
  de 2011 à 2013 ; la 0.8.3 date de janvier 2013.
- **SECURITY.md** disait `safe_exec.c` en deux copies : il n'y en a qu'une.

### Mesuré

- `htmlsermo` et `lvglsermo` : banc de comportement 61/61, banc XML 55/55.
  Les gardes qui cliquent ou mesurent une fenêtre ne s'appliquent pas à html.

## [2.7.7] — 2026-10-04

Version CORRECTIF. Une lecture hors tampon dans la barre de progression, et trois
correctifs que le cœur GTK4 n'avait pas encore.

### Sécurité

- **Barre de progression (gtk3, gtk4) : lecture hors tampon.** Le thread qui lit
  l'`<input>` d'une `<progressbar>` retirait le saut de ligne final sans vérifier
  que la ligne n'était pas vide. Une ligne commençant par un octet nul faisait
  lire un octet avant le tampon, et l'écrire si cet octet valait un saut de
  ligne. Ce contenu vient d'une commande, donc de données que l'auteur du script
  ne maîtrise pas toujours. Mesuré sous AddressSanitizer sur les deux ports
  (`stack-buffer-overflow`, `READ of size 1`) ; une garde ferme le défaut.

### Corrigé

- **GTK4 : deux fenêtres ouvertes en même temps avec la même variable.** La
  seconde reprenait la variable de la première et l'écrasait en silence : à la
  sortie, la variable portait la valeur de la mauvaise fenêtre. Elle est
  désormais rangée sous `NOM__W<id>`, avec un avertissement — ce que gtk3 faisait
  déjà.
- **GTK4 : la valeur `<default>` d'un widget n'était pas visible du premier
  `<input>` d'un voisin.** Une commande d'`<input>` qui lisait `$A` trouvait une
  variable vide là où gtk3 trouvait la valeur par défaut de `A`.
- **GTK4 : `g_setenv()` remplace `putenv()`** à l'export d'une variable (fuite
  mémoire, et entrée d'environnement pointant sur une chaîne du tas).

Ces trois correctifs existaient dans `libsermocore/src` ; ils manquaient dans la
copie `libsermocore/src-gtk4`.

### Tests

- `tests/garde_progressbar_ligne_vide.sh` : banc statique avec témoin (sur un
  binaire ordinaire, la lecture d'un octet de pile ne se voit pas).
- `tests/garde_fenetres_homonymes.sh` (gtk3, gtk4) et cas de comportement
  `54-defaut-vu-par-input`, vert sur les sept ports. Les trois bancs sont rouges
  sur l'ancien code.

## [2.7.6] — 2026-10-03

Version CORRECTIF de sécurité. Contournement de la liste blanche
`SERMO_ALLOWED_CMDS` par l'environnement.

### Sécurité

- **Une variable de widget nommée `LD_PRELOAD` franchissait la liste blanche.**
  Un script pouvait nommer une variable `LD_PRELOAD` (ou `LD_LIBRARY_PATH`,
  `LD_AUDIT`…) ; sa valeur était exportée, puis héritée par la commande
  autorisée, qui chargeait alors du code arbitraire. Quand la liste blanche est
  active, toute variable `LD_*` est désormais retirée de l'environnement de la
  commande et `PATH` est figé à `/usr/bin:/bin`. Sans liste blanche, rien ne
  change.

### Interne

- **Source unique, première étape.** L'énumération `WIDGET_*` vit dans un seul
  en-tête (`libsermocore/include/widget_types.h`) au lieu d'être recopiée dans
  chaque port, et cinq fichiers du cœur (`safe_exec`, `stack`, `attributes`,
  `stringman`, `actions`) n'ont plus de copie GTK4 : la variante GTK4 les
  compile depuis `libsermocore/src`. Aucun changement de comportement attendu.

## [2.7.5] — 2026-10-01

Version CORRECTIF de sécurité. Débordement de tas dans `linecutter()`, hérité de
gtkdialog.

### Sécurité

- **`linecutter()` débordait au-delà de 127 séparateurs (CWE-787).** Cette
  fonction découpe les valeurs séparées par `|` : colonnes de `<table>` et
  `<tree>`, `<colorbutton>`, entre autres. Elle réservait 128 cases et ne
  vérifiait jamais cette limite ; une valeur de 127 séparateurs ou plus écrivait
  hors du tableau. Ce contenu peut venir d'une commande ou d'un fichier, donc de
  données que l'auteur du script ne maîtrise pas toujours — c'est ce qui en fait
  un risque de sécurité, pas seulement un défaut. Le tableau s'agrandit
  désormais au besoin. Corrigé dans les **deux** copies du cœur
  (`libsermocore/src` et `libsermocore/src-gtk4`).

### Tests

- Test unitaire de non-régression (`linecutter` sur 200 champs), vérifié **rouge
  sur l'ancien code** sous AddressSanitizer puis vert sur le correctif.
- `run_unit_tests.sh` éprouve désormais `stringman` sur les deux copies du cœur,
  comme il le fait déjà pour `safe_exec`.

## [2.7.4] — 2026-09-30

Version CORRECTIF. Un `<calendar>` de gtk4 pouvait poser le mauvais mois, selon
le jour où le dialogue s'ouvrait.

### Corrigé

- **gtk4 — un `<calendar>` posait le mauvais mois les jours 29 à 31.** Un
  `<default>` 2000-02-29 rendait 2000-09-29 le 30 septembre. Le calendrier part
  de la date du jour, puis reçoit l'année, le mois et le jour, un par un : parti
  du 30 septembre, il passait par le 30 février 2000, que GTK refuse sans rien
  dire, et le mois restait septembre. Selon le jour courant et la date demandée,
  les fins de mois et le 29 février étaient touchés ; la remise à la date du jour
  aussi. Le jour 1, qui existe dans tous les mois, est désormais posé d'abord.
  Les six autres ports étaient justes. Vu le 2026-09-30 par la CI de GitLab : le
  banc de comportement était vert la veille, le 29.

### Ajouté

- **Une garde du calendrier** (`tests/garde_calendrier_date_du_jour.sh`), jouée
  sur les sept ports : elle fixe la date du jour du binaire (`faketime`) au
  31 janvier, au 30 septembre et, en témoin, au 15 juin. Sur la 2.7.3, gtk4 y est
  rouge les deux jours pièges.
- **CI : trois paquets qui manquaient** à `ci/dependances.txt` : `imagemagick`
  (la garde de taille de fenêtre lit la capture avec `identify`),
  `libevas1-engines-x` (seulement « recommandé » : sans lui, EFL ne crée aucune
  fenêtre) et `faketime`. Le poste de travail les avait ; la CI, jamais jouée
  avant la publication, était rouge sur 13 bancs.

## [2.7.3] — 2026-09-24

Version CORRECTIF. Ce que lit un `<input>` n'avait pas de limite : une commande
sans fin (`yes`) ou un fichier sans fin (`/dev/zero`) faisait grossir le
dialogue jusqu'à manquer de mémoire. En posant la limite, trois défauts voisins
sont apparus. Tous ont été mesurés sur les paquets 2.7.2.

### Ajouté

- **Une limite de taille pour chaque `<input>` : 16 Mio.** Commande ou fichier,
  la lecture s'arrête comme sur une fin de fichier, et **un** avertissement part
  sur la sortie d'erreur — jamais sur la sortie, qu'un `eval` lit. La variable
  d'environnement `SERMO_INPUT_MAX` change la limite, en octets ; `0` la
  retire ; une valeur illisible est signalée, et la limite par défaut
  s'applique. Posée dans le cœur (`libsermocore/src/sermo_input.c`), elle vaut
  pour les sept ports. Mesuré le 2026-09-17 sur la 2.7.2 : branché sur `yes`,
  gtk3sermo passait 700 Mo en 7 secondes, et ncurses finissait sur une erreur
  de segmentation.
- **La barre de progression n'est pas plafonnée.** Elle ne garde qu'une ligne à
  la fois, et doit suivre une longue commande jusqu'au bout.

### Corrigé

- **gtk3, gtk4 — la barre de progression grossissait sans fin.** Chaque ligne
  lue partait vers la boucle principale par son propre `g_idle_add()`. Une
  commande rapide (`yes 50`) remplissait cette file plus vite qu'elle ne se
  vidait : 1 086 Mo de plus en 3 secondes (mesuré le 2026-09-24). Une seule
  mise à jour attend désormais. Les passages à 100 restent comptés : les
  actions tournent autant de fois qu'avant.
- **gtk3, gtk4 — une barre dont la commande est refusée faisait tomber le
  dialogue.** Quand `SERMO_ALLOWED_CMDS` refusait sa commande, le fil de
  lecture partait quand même, sur un flux nul : erreur de segmentation (mesuré
  le 2026-09-24). La barre reste vide, le dialogue vit.
- **gtk3, gtk4 — `<edit>` lisait un fichier d'un seul tenant.** Il allouait la
  taille annoncée par `stat()` : un fichier de 8 Go faisait tomber le
  programme (« failed to allocate », mesuré le 2026-09-24). Le tampon n'était
  jamais libéré, et une lecture partielle affichait des octets jamais écrits.
  Il lit maintenant comme sa commande, ligne à ligne, sous la limite.
- **fltk1, efl1 — `<input file>` passait par `g_file_get_contents()`**, qui
  lisait `/dev/zero` sans fin. Il est lu comme dans les autres ports.

### Tests

- `tests/garde_input_sans_fin.sh`, joué sur les sept ports par `ci/bancs.sh` :
  `yes` et `/dev/zero` s'arrêtent à la limite pile et le disent sur la sortie
  d'erreur ; un témoin sans limite lit tout ; la barre sur `yes 50` garde sa
  mémoire ; une barre refusée laisse vivre le dialogue. Lancé sur les binaires
  2.7.2, il voit l'absence de limite et les deux défauts de la barre.
- `tests/garde_lecture_input.sh` : aucune ouverture de fichier en lecture
  n'échappe à la limite, hors d'une liste nommée de lectures qui ne sont pas des
  `<input>` (le script XML lui-même, le thème d'icônes…). Sur l'arbre 2.7.2, il
  refuse 72 lectures, dont l'`open()` du `<edit>` de gtk3 et gtk4.
- `tests/unit/test_sermo_input.c` : la limite pile, un octet de trop, `0`, les
  valeurs illisibles, la reprise sans limite. Sept mutations du code, sept
  échecs vus. `test_safe_exec.c` vérifie en plus que chaque copie de
  `safe_exec.c` plafonne `safe_popen()`.
- `sermoman-mcp/tests/verifie-verite.sh` relie la limite documentée (16 Mio)
  au code.

### Documentation

- `SECURITY.md` : la section « Taille de ce que lit un `<input>` ». L'ancienne
  limite connue « `<input>` n'a pas de plafond » devient : le plafond vaut pour
  chaque `<input>`, pas pour tout le dialogue.
- `SERMO_INPUT_MAX` dans les pages de manuel, les manuels et la documentation
  servie par `sermoman-mcp`.

## [2.7.2] — 2026-09-20

Version CORRECTIF. Ce que montre une fenêtre n'était vérifié par aucun banc :
tous lisent les **valeurs** exportées. Trois défauts de mise en page ont vécu
là, et la galerie du site a publié pendant des semaines une image vide.

S'y ajoutent des corrections de la documentation (le manuel, `SECURITY.md`),
l'icône des fenêtres, et des ports qui ne disaient pas leur nom.

### Corrigé

- **Chaque port dit son nom dans `--version`.** La ligne est imprimée par le
  cœur, compilé une fois : il ne connaissait que son propre nom, et les sept
  binaires répondaient « sermocore version 2.7.1 sermo/libsermocore ». Deux
  symboles **faibles**, remplis par `contract/sermo_port_id.c`, laissent chaque
  port se nommer — un backend qui ne les remplit pas se lie quand même. La
  position des mots ne bouge pas : la version reste le 3e, que des exemples
  livrés lisent par rang.
- **sdl3 — le rendu hors écran ne s'emballe plus.** Sa boucle de chauffe
  redimensionnait la fenêtre à chaque frame, douze fois, quand la boucle vivante
  ne le fait qu'une. Un dialogue demandant 560×420 sortait en 560×1530, vide sur
  les deux tiers.
- **sdl3 — un enfant extensible laisse la place à ses frères.** Dans un `hbox`,
  le premier prenait toute la largeur ; dans une `vbox`, toute la hauteur — le
  bandeau bas et la rangée de boutons étaient dessinés sous le bord de la
  fenêtre.
- **qt6 — le facteur d'étirement n'était noté nulle part.**
  `qt6_layout_register()` existait sans être appelée : la table restait vide,
  les boîtes concluaient qu'aucun enfant n'était extensible et posaient leur
  ressort de calage à droite. Sur `system-tools`, le contenu tenait dans 258 px
  au lieu de 771.
- **qt6 — `--render-png` montre la bonne fenêtre, à la bonne taille.**
  `adjustSize()` écrasait `default-width`/`default-height` (500×320 rendus en
  200×142), et la fenêtre choisie pouvait être une liste déroulante ouverte —
  image de 2×2 pixels, 4 essais sur 6. Rejoué six fois : 6/6 corrects.
- **`examples/system-tools` nommait « gtk3sermo » en dur** dans son pied de
  page, quel que soit le port qui l'affichait.
- **Le manuel se trompait sur `<progressbar>`** : il annonçait « entre 0.0 et
  1.0 » et son exemple écrivait `echo 0.75`, qui rend une barre vide sur tous
  les ports. L'échelle est 0 à 100.
- **`examples/c_embedded/example01.c` n'avait pas d'en-tête de licence**, et le
  banc SPDX ne voyait pas `examples/`.
- **`examples/system-tools/system-tools.sh` codait `gtk3sermo` en dur** à sa
  dernière ligne et ignorait son premier argument, contrairement aux cinq
  exemples de `showcase` : il ne démarrait pas là où un autre port était
  installé. Il prend maintenant le port en argument, ou le premier des sept
  qu'il trouve.
- ⛔ **Les fenêtres n'avaient pas d'icône.** `debian/rules` n'en installait
  aucune : seul qt6 en livrait une, en 32 px, par son propre `CMakeLists`. Les
  six ports graphiques livrent désormais le **logo HD** aux huit tailles du
  thème hicolor (16 à 256 px), engendré depuis `icon/haplo-dialog-hd.svg`.
  Trois fichiers portaient encore un nom mort — `fltk1dialog.png`,
  `efl1dialog.png`, `sdl3dialog.png` — et n'auraient donc rien nommé.
- **Le port ncurses s'annonçait comme le port SDL 3.** Dérivé de lui, il en
  avait gardé les noms : une fenêtre sans titre affichait `sdl3sermo` dans son
  filet, et ses avertissements portaient `[sdl3dialog]`. Mesuré dans un vrai
  pseudo-terminal. Un terminal ne pouvant pas porter d'icône, la marque y est
  maintenant écrite : le filet du haut affiche « HD » avant le titre.
- ⛔ **`--help` renvoyait les sept ports vers `info qt6sermo`.** La chaîne était
  figée dans le cœur ; elle nomme désormais le port qui tourne, celui dont la
  page info est installée.
- **Le manuel enseignait six syntaxes que l'analyseur refuse** — trouvées en
  passant ses 54 blocs XML au `--print-ir` du binaire livré : `<okbutton/>` et
  `<cancelbutton/>` (il faut `<button ok></button>`), `<width-request>` en
  sous-élément (c'est un attribut, ou `<width>`), le balisage Pango non échappé
  dans `<label>`, `<input file="chemin"></input>` (il faut
  `<input file>chemin</input>`), `<menu><label>` (le titre est l'attribut
  `label=`), et `<menuitemseparator/>` auto-fermante. Six squelettes ne tournaient
  pas non plus : un conteneur vide est refusé. **54 blocs sur 54 passent
  maintenant** ; la règle générale — toute balise s'écrit ouverte puis fermée — est
  écrite au §4.3.
- **Le manuel promettait les valeurs « quand l'utilisateur ferme la fenêtre ».**
  Fermée par le gestionnaire de fenêtres, la sortie complète est `EXIT="abort"`,
  sans aucune variable. Le manuel dit désormais ce qui se passe dans les deux cas.
- **`SECURITY.md` ne citait que `<linkbutton>` comme échappatoire aux deux
  garde-fous.** Il en manquait deux, mesurées sur le binaire du paquet :
  `<terminal>` ouvre un `/bin/sh` par VTE, hors de `SERMO_ALLOWED_CMDS` et de
  `SERMO_NO_SHELL_FALLBACK` ; et `--include` fait passer **toute** commande par
  `/bin/sh -c`, donc n'exécute plus rien dès qu'une liste est posée.
- Quatre `safe_exec.h` (fltk1, efl1, sdl3, ncurses) annonçaient encore
  « no /bin/sh -c, no shell injection » deux lignes après avoir décrit le repli.
- Le binaire `sdl3sermo` se nommait `sdl3dialog` dans ses messages
  d'avertissement et dans le titre par défaut de sa fenêtre.
- `sermoman-mcp/data/architecture.md` annonçait **9 paquets** quand le dépôt en
  produit 13 — il contredisait `heritage.md` et `guide.md`, servis par le même
  serveur. Même correction pour `sermo-core-dev`, annoncé dépendant de `libc6`
  alors que son `.deb` ne porte aucun champ `Depends`.

### Changé

- **`VERSION` à la racine devient la seule source du numéro.** Les huit projets
  CMake le lisent ; les `config.h` de ncurses et sdl3 sont engendrés au lieu
  d'être tenus à la main ; les pages de manuel et le manuel Texinfo sont
  substitués à la construction.
- **Règles `install()` pour les six backends qui n'en avaient pas** : binaire,
  page de manuel et icônes s'installent aussi depuis les sources.
- **Manuels utilisateur et développeur traduits en anglais**, et les liens des
  documents anglais qui menaient au français le disent désormais.
- Deux recettes d'intégration continue, une par forge, et un job de paquets qui
  ne publie rien.

### Ajouté

- `tests/garde_taille_fenetre.sh` — le premier banc qui regarde la **fenêtre**,
  et non les valeurs exportées.
- `tests/garde_version.sh`, `tests/garde_identite_port.sh`,
  `tests/garde_liens_langue.sh`.

## [2.7.1] — 2026-09-17 — Le terminal, l'ancrage Wayland et `<input>` tiennent parole

Version CORRECTIF : des fonctions que la documentation promettait, et que la 1.x
avait, manquaient à la 2.x sans un message. Trouvées en préparant les paquets
Debian, en comparant les dépendances des paquets 1.x et 2.x, puis mesurées.

### Corrigé
- ⛔ **`<terminal>` n'existait plus** sur gtk3sermo et gtk4sermo depuis la 2.0.0.
  Le réglage qui compilait VTE ne vivait que dans les anciens `Makefile.am` ; sous
  CMake il n'a jamais été repris (gtk4 le forçait même à 0). Le widget affichait
  une étiquette « requires … libvte » et sa variable restait vide. VTE est de
  nouveau compilé et lié : la variable rend le PID du shell du terminal, et la
  sortie de la commande `<input>` y est tapée comme au clavier. Le cœur décide
  (option `SERMOCORE_VTE`, activée par défaut), l'écrit dans `sermocore.pc`, et les
  backends gtk3 et gtk4 suivent ; VTE absent arrête la construction au lieu de
  produire un binaire sans terminal. Construit sans VTE (option coupée),
  `<terminal>` le dit aussi sur la sortie d'erreur.
- ⛔ **L'ancrage Wayland de gtk3sermo n'était pas compilé** : les attributs
  `layer`, `edge`, `dist` et `reserve` de `<window>` étaient ignorés. gtk-layer-shell
  est de nouveau lié (option `SERMO_LAYER_SHELL`, activée par défaut) : sous un
  compositeur qui parle wlr-layer-shell, la fenêtre devient une surface de couche.
- ⛔ **`<input>` sur les ports sans GTK.** Selon le port, des widgets envoyaient la
  directive brute au shell (« Command:echo : not found »), ne lisaient pas
  `<input file>`, ou exécutaient la commande deux fois — une commande qui écrit
  quelque part le faisait deux fois :
  - `<text>` ne lisait rien sur qt6, fltk1, sdl3 et ncurses ; `<entry>` gardait
    toutes les lignes (fltk1, sdl3, ncurses) ou les collait bout à bout (efl1) ;
    `<input file>` ne lisait rien sur qt6 pour `<entry>`, `<text>`, `<spinbutton>`,
    `<hscale>`, `<vscale>` et `<levelbar>` ; `<levelbar>` d'efl1 divisait la valeur
    lue par 100 (0.4 devenait 0.004) ;
  - `<comboboxtext>`, `<list>`, `<tree>` et `<table>` recevaient la directive brute
    sur plusieurs ports (et `<table>` sur gtk4) ; `<edit>` s'exécutait deux fois sur
    qt6 et fltk1, `<list>` et `<combobox>` deux fois sur fltk1 ; `<text>` perdait
    son saut de ligne final sur efl1 et qt6, `<edit>` sur efl1.
  Partout, `<input>` passe par un seul décodeur (commande ou fichier) et se lit une
  seule fois, au rafraîchissement que le cœur demande juste après la création. Les
  valeurs suivent l'étalon gtk3sermo : `<entry>` garde la première ligne ; `<text>`
  et `<edit>` tout le texte, saut de ligne final compris ; `<comboboxtext>` fait un
  élément par ligne (une ligne vide aussi) et sélectionne le premier ou son
  `<default>` ; `<combobox>` n'implémente pas `<input>`, comme chez gtk3.
- ⛔ **`<table>` sans `<label>`** : sdl3sermo plantait (en-têtes absents lus au
  rendu) ; gtk4sermo prenait le premier `<item>` pour les en-têtes. L'étalon en
  fait une rangée.
- ⛔ **`<chooser>` faisait avorter les cinq ports sans GTK** : le cœur y créait le
  sélecteur embarqué par un appel GTK qui rend NULL, puis l'enregistrement de la
  variable avortait (« ASSERT FAILED: widget != NULL », code 134). Un dialogue qui
  contenait un `<chooser>` ne s'ouvrait donc pas du tout sur qt6, fltk1, efl1, sdl3
  et ncurses. Le tag y devient le `<filechooser>` du port — un bouton qui ouvre le
  dialogue du toolkit —, avec la même variable et le même `<default>` ; gtk3 et gtk4
  gardent le sélecteur dans la fenêtre. La référence et le manuel le disent
  désormais. `<filechooser>` de fltk1, révélé par le même cas : il exportait son
  libellé d'invite « (Aucun) » au lieu d'une valeur vide.
- fltk1 : une ligne de plus de 1 023 octets dans une liste déroulante écrasait la
  pile — FLTK 1.4.4 recopie le libellé dans un tampon fixe. Les lignes venues de
  `<input>` sont bornées. `clear:` puis `refresh:` d'un `<tree>` faisait planter le
  programme.
- sdl3 et ncurses : après `clear:`, `<edit>` gardait la capacité de son ancien
  tampon ; sur sdl3, la saisie qui suit aurait pu écrire au-delà du nouveau
  (défaut lu dans le code, corrigé sans l'avoir déclenché). efl1 : `<edit>`
  perdait ses sauts de ligne (« a\nb » devenait « ab ») ; `<list>` et `<tree>`
  perdaient la mémoire de chaque rangée à chaque `clear:` ou `refresh:` (sous
  valgrind, après correction, plus aucune rangée parmi les pertes).
- Manuel : l'exemple de `<terminal>` (`echo "ls -la" | bash`) aurait tapé le
  RÉSULTAT de `ls` dans le terminal ; celui de `<timer>` (`milliseconds="1000"`)
  prenait un booléen pour une durée.

### Retiré
- Les fichiers autotools morts : `Makefile.am` et `Makefile.in` de gtk3, gtk4, fltk1
  et efl1, `Makefile.am` des données de qt6, et un double mort de `gtk4-compat.c`.
  C'est là que vivaient les réglages perdus ; plus rien ne les lisait.

### Paquets Debian
- `debian/` (debhelper) produit 13 paquets : `sermo-core-dev`, les sept
  `sermo-backend-*`, `sermo-gtkdialog`, et `gtk3sermo`, `gtk4sermo`, `qt6sermo`,
  `gtksermo` devenus paquets de transition depuis la 1.x. Dépendances calculées à
  la construction ; alternative `sermo` avec sa page de manuel et son manuel
  Texinfo ; `debian/copyright` produit depuis l'arbre suivi par git.
- `packaging/construire-paquets.sh` remplace `build-debs.sh` : construction dans
  une Debian testing vierge, lintian, installation, vérification et purge, passage
  depuis la 1.x par un dépôt apt local.
- `sermo-backend-gtk3` dépend désormais de VTE et de gtk-layer-shell,
  `sermo-backend-gtk4` de VTE.
- `libsermocore` : répertoire d'installation réglable (`SERMOCORE_LIBDIR`) ; les
  en-têtes générés par bison et flex ne s'installent plus (ils portaient le chemin
  du dossier de construction).
- `debian/copyright` déclare les métadonnées AppStream de qt6 sous FSFAP et la table
  des kanji de Dear ImGui sous CC BY 4.0 (aussi dans LICENCES) ; l'archive amont
  n'emporte plus `debian/`.

### Bancs
- Banc de comportement : **53 cas**. 53 : `<chooser>` sur les sept ports. 43 : `<entry>` et `<text>` par fichier et par
  commande, une seule exécution ; 44 : `<comboboxtext>`, `<combobox>`, `<edit>` ;
  45 à 50 : `<list>`, `<tree>` et `<table>`, par commande et par fichier, chacun
  seul dans sa fenêtre (chez l'étalon, leur 1re rangée ne ressort qu'avec le
  focus) ; 51 : ligne vide et nombres lus dans un fichier ; 52 : table sans
  `<label>`. Chaque cas joue dans un dossier de travail neuf garni de
  `cas/donnees/`.
- Les écarts à l'étalon mesurés en chemin mais laissés hors de cette version sont
  listés dans le TODO (sélection liée au focus, plusieurs `<input>`, `<item>` et
  `<input>` mêlés, `auto-refresh` hors GTK…).
- `tests/garde_terminal.sh` (sept ports) et `tests/garde_layer_shell.sh` (gtk3 : le
  lien, puis, si sway est là et qu'on n'est pas root, une vraie surface de couche
  sous un sway sans écran, contre un témoin), joués par `ci/bancs.sh`.
- `tests/run_examples.sh` attend une fenêtre jusqu'à 10 s au lieu de 3 : sous une
  charge voisine, un exemple différent rougissait à chaque passe, sans défaut.
- `packaging/bancs-sur-paquets.sh` : `ci/bancs.sh` tel quel, mais sur les binaires
  extraits des paquets `.deb` (sommes SHA-256 comparées) ; chaque binaire doit
  trouver ses symboles de débogage dans son paquet `-dbgsym`.

### Compatibilité
- Construire gtk3 ou gtk4 demande `libvte-2.91-dev` / `libvte-2.91-gtk4-dev`, et
  gtk3 `libgtk-layer-shell-dev` — ou le refus explicite par option.
- `<combobox>` rempli par `<input>` : plus chargé sur qt6, fltk1 et efl1, comme sur
  gtk3 qui ne l'a jamais fait. Utiliser `<comboboxtext>`.
- `<text>` et `<edit>` remplis par `<input>` gardent le saut de ligne final partout.

### Vérifié, pas supposé
- Clone neuf de la 2.7.1 (`087ca97`), limité à deux cœurs : `ci/construire.sh`
  construit les dix projets en 110 s (11 avertissements, les mêmes, fichier par
  fichier, qu'en 2.7.0), `--version` rend 2.7.1 sur les sept binaires et « Glade,
  VTE » sur gtk3 et gtk4 ; `ci/bancs.sh` joue **108 bancs en 18 min 52 s** : 104
  verts d'une traite et 4 rouges — le clic et les exemples réels de gtk4 et de
  fltk1, chaque fois une fenêtre pas encore ouverte dans le délai —, tombés pendant
  des charges lancées à côté sur les mêmes cœurs. Rejoués seuls sur le même clone :
  clic 17/17 et exemples 55/55 (gtk4), clic 17/17 et exemples 54/54 (fltk1).
- Comportement **53/53** sur les sept backends ; XML 55/55 ; `garde_terminal.sh`
  verte sur les sept (vrai terminal sur gtk3 et gtk4 : PID exporté, fichier témoin
  écrit par la ligne tapée) ; `garde_layer_shell.sh` verte, niveau 2 joué sous un
  sway sans écran (témoin : fenêtre ordinaire ; `layer="top"` : surface de couche).
- Rouges sur les binaires de la 2.7.0 : le cas 43 sur qt6, fltk1, efl1, sdl3 et
  ncurses ; les cas 49 et 50 sur gtk4 ; `garde_terminal.sh` sur gtk3 et gtk4 ;
  `garde_layer_shell.sh` (non lié, fenêtre ordinaire) ; la table sans `<label>`
  plantait sdl3 et rendait « 3 » sur gtk4.
- Construit sans VTE ni layer-shell (options coupées) : construction réussie,
  avertissements de CMake, rien de lié, `<terminal>` avertit sur la sortie
  d'erreur.
- efl1 sous valgrind, list et tree rechargés en boucle : aucune lecture ni écriture
  invalide, aucune rangée parmi les pertes.
- FLTK : « stack smashing detected » dès 1 100 caractères avec `Fl_Choice::add` à
  cinq arguments ; 1 023 « / » ou « \ » échappés (2 046 octets en entrée) passent —
  la borne porte sur le libellé déséchappé.
- Paquets, dans des conteneurs `debian:testing` vierges : construction source et
  binaires, tests de construction verts ; lintian sans erreur ni avertissement
  (2 informations, 3 remarques pédantes) ; les 13 paquets installés, `--version`
  2.7.1 sur les sept binaires, `sermo-backend-gtk3` dépend de libvte-2.91-0 et de
  libgtk-layer-shell0 ; purge sans reste ; passage 1.x → 2.7.1-1 par `apt upgrade`.
- Sur les binaires LIVRÉS (`packaging/bancs-sur-paquets.sh`, paquets 2.7.1-1, clone
  de `087ca97`, deux cœurs) : les sept binaires des paquets mis en place (sommes
  comparées), symboles de débogage trouvés pour les sept ; `ci/bancs.sh` : 108 bancs,
  **108 verts** à la première passe. Deux passes de plus, faites pour éprouver le
  script après deux corrections (sortie traduite de `readelf`, `grep -q` sous
  `pipefail`), ont donné 107 puis 106 verts : chaque rouge était un exemple
  différent dont la fenêtre n'était pas encore ouverte au bout de 3 s (gtk4
  `pfontview` ; qt6 `list`, efl1 `togglebutton`), vert dans les autres passes.

## [2.7.0] — 2026-09-16 — Glade revient, et la barre de progression avance partout

Version MINEURE : une capacité (`--glade-xml` sur gtk3 et gtk4) et quatre défauts
corrigés, tous trouvés par les bancs réparés de la 2.6.8.

### Nouveau
- **`--glade-xml` charge un fichier d'interface GtkBuilder** (le format de Glade)
  sur **gtk3sermo** et **gtk4sermo**, chacun dans le format de sa version de GTK.
  La fenêtre est l'objet que `--program` désigne (`MAIN_WINDOW` par défaut) ;
  chaque widget à identifiant devient une variable de ce nom ; un gestionnaire de
  signal est une action sermo (commande, `exit:`…) soumise aux mêmes règles que
  `<action>` ; sur `realize`, sa sortie remplit le widget. L'option supposait
  libglade, qui n'existe ni pour GTK 3 ni pour GTK 4 : elle était compilée
  « ignorée » partout, et tout script qui s'en servait avortait. Le code GtkBuilder
  existait, jamais compilé, avec trois défauts, corrigés : un gestionnaire sur deux
  jamais branché (des accolades manquaient), des variables nommées d'après le TYPE
  du widget, un avortement sur fichier illisible (code 1 et message désormais).
  En GTK 4, les gestionnaires passent par un `GtkBuilderScope`.
- `examples/glade/` : versions GTK 4 des deux exemples (`*-gtk4.ui`) ; les scripts
  choisissent leur fichier d'après `GTKDIALOG`.

### Corrigé
- ⛔ **`<progressbar>` ne vivait que sur gtk3 et gtk4.** qt6 lisait la première
  ligne de la commande `<input>` puis fermait le tube ; fltk1, efl1, sdl3 et ncurses
  la lisaient en entier avant d'ouvrir la fenêtre (3,5 s d'écran vide sur
  `examples/progressbar`). Aucun des cinq ne faisait avancer la barre, et l'action
  prévue à 100 ne partait jamais : un dialogue qui s'en remettait à elle pour se
  fermer restait ouvert. La lecture, écrite une fois dans le cœur
  (`sermo_progress`), suit la commande ligne par ligne sans bloquer la boucle du
  port, comme l'étalon : un nombre fait avancer la barre, un texte devient son
  libellé, la ligne qui vaut 100 déclenche les actions.
- ⛔ **`--include` avec un nom relatif ne chargeait rien.** Chaque commande devient
  « . FICHIER; commande », et sous `/bin/sh` (dash sur Debian) « . » cherche un nom
  sans « / » dans le `PATH`, jamais dans le dossier courant : une `<input>` qui
  appelait une fonction du fichier rendait du vide, sans un mot, sur les sept
  ports. Le chemin devient absolu au lancement, et cité pour le shell : une espace
  ou une apostrophe ne le cassent plus.
- **Un `<frame>` n'affichait que son premier enfant** sur fltk1, efl1, sdl3 et
  ncurses ; les suivants étaient créés mais jamais dessinés (efl1 en laissait
  traîner un morceau hors du cadre). Ils y sont tous, comme chez l'étalon, qui les
  range dans une boîte verticale.
- **Les ports sans GTK refusent `--glade-xml`** en le disant (code 1), au lieu de
  l'ignorer puis d'avorter.

### Bancs
- Banc de comportement : **42 cas**. Le 41e, une barre de progression qui ferme le
  dialogue à 100 ; le 42e, un cadre à plusieurs enfants. Les deux échouent sur les
  binaires de la 2.6.8 ; le 42e aussi sur un sabotage de la seule correction du
  cadre.
- `tests/garde_glade.sh` et `tests/garde_include.sh` (nouveaux), joués par
  `ci/bancs.sh` sur les sept ports ; rouges sur les binaires de la 2.6.8.
- `tests/run_examples.sh` : un exemple réservé à certains ports le dit dans un
  fichier `PORTS` ; ailleurs il est annoncé « réservé », visiblement, et compté ni
  réussi ni en échec. Le banc transmet `GTKDIALOG` aux exemples.

### Compatibilité
- `--glade-xml` sur qt6, fltk1, efl1, sdl3 et ncurses : refusé (code 1). Il n'y a
  jamais fonctionné.
- `--include` : un chemin relatif est résolu depuis le dossier de lancement.

### Vérifié, pas supposé
- Clone neuf de la 2.7.0, limité à deux cœurs : `ci/construire.sh` construit les
  dix projets en 98 s (11 avertissements — celui du `#endif` de glade_support.c
  est parti), `--version` rend 2.7.0 sur les sept binaires ; `ci/bancs.sh` joue
  **100 bancs en 17 min 33 s, aucun en échec**.
- XML 55/55 et comportement **42/42** sur les sept backends ; exemples réels
  55/55 sur gtk3 et gtk4, 54/54 sur qt6, fltk1, efl1 et sdl3 (`glade` réservé) ;
  `garde_glade.sh` et `garde_include.sh` verts sur les sept ; durcissement, liste
  de commandes, `--do`, limite de widgets, échappement, clic et mot de passe
  masqué verts ; FORTIFY du cœur 13/14, 6/13 et 8/14 objets ; tests unitaires
  3/3 ; sermoman-mcp : 6/6 contre le code, 392 exécutions sans échec.
- Les cas 41 et 42 échouent sur les binaires de la 2.6.8 ; le 42 aussi quand on
  sabote la seule correction du cadre (ncurses). `garde_glade.sh` et
  `garde_include.sh` échouent sur les sept binaires de la 2.6.8, pour les raisons
  attendues.
- Sur `examples/progressbar`, qui compte jusqu'à 100 en 3,3 s : fenêtre ouverte
  en 150 ms et fermeture seule en 3,0 à 3,1 s, `EXIT="Ready"`, sur qt6, fltk1,
  efl1, sdl3 et ncurses. Rendu PNG d'un cadre à trois enfants sur fltk1 et efl1 :
  un enfant avant, trois après.

## [2.6.8] — 2026-09-16 — Des bancs qui ne disent plus vert à tort

Version CORRECTIF : un défaut de clic, des bancs qui disaient OK sans avoir
vérifié, et la vérification automatique à la racine du dépôt.

### Corrigé
- **sdl3, ncurses : cliquer sur un `<fontbutton>` fermait le dialogue**, avec
  `EXIT` égal au nom de la police. Le choix de police y est construit comme un
  bouton, et un bouton sans action ferme le dialogue. Côté ncurses, c'est la
  correction de la 2.6.7 (un bouton nu sort enfin) qui avait ouvert la porte.
- La 2.6.7 attribuait l'échec de `garde_clic_widgets.sh` sur sdl3 à l'absence
  d'OpenGL sous Xvfb. **C'était faux** : la garde cherchait la fenêtre par un
  titre que xdotool ne lit pas en UTF-8. Réparée, elle a mesuré sdl3 pour la
  première fois, et trouvé le défaut ci-dessus.

### Bancs
- ⛔ **`run_unit_tests.sh` affichait « OK » sans rien lancer** : il visait des
  dossiers de la 1.x. Il éprouve maintenant les deux copies de `safe_exec.c` du
  cœur et `stringman.c` ; zéro test lancé est un échec.
- ⛔ **`tests/xml/run_tests.sh` pouvait tester la 1.x installée** à la place du
  binaire construit, et comptait un dépassement de temps comme une réussite. Un
  nom n'est plus cherché que dans l'arbre construit ; un XML mal formé sert de
  témoin.
- `run_all.sh` et le mode `all` du banc XML échouent quand un port manque, sauf
  `SERMO_PORTS_OPTIONNELS=1`.
- Le test de qt6 éprouvait une copie de `safe_exec.c` que rien ne liait au
  binaire : il éprouve celle du cœur, et la copie quitte le dépôt.
- `garde_spdx.sh` descend dans les sous-dossiers et lit aussi `.cpp .hpp .l .y` ;
  témoins ajoutés à `garde_spdx`, `garde_fonctions_interdites` et
  `garde_progressbar_thread`. `run_fuzz.sh` visait lui aussi l'ancienne
  disposition.
- `tests/run_examples.sh`, qui ouvre pour de bon chaque exemple, rendait des
  verdicts faux : sdl3 « sans fenêtre » sur 54 exemples (titre UTF-8 illisible
  par xdotool), qt6 en « syntax error » sur 50 (le programme, laissé en vie,
  mourait de la disparition du serveur X en écrivant ce faux message). Recherche
  aussi par classe, arrêt de tout le groupe de processus, verdict sur la sortie
  relevée avant l'arrêt : 54 exemples sur 55 sur ces deux ports.

### Nouveau
- **La vérification automatique, à la racine.** `ci/construire.sh` construit le
  cœur et les sept backends, `ci/bancs.sh` enchaîne tous les bancs, et
  `.gitlab-ci.yml` n'appelle que ces deux scripts, dans une Debian testing munie
  de `ci/dependances.txt`, sous une locale française. Ce que voit la CI se rejoue
  chez soi à l'identique. Un banc rouge, figé, ou qui n'a rien vérifié fait
  échouer l'ensemble. La recette de `sermoman-mcp`, que GitLab ne lisait pas
  (elle n'était pas à la racine), est retirée.
- **`tests/verifie-facade.sh`** contrôle qu'aucun en-tête d'horodatage de poste,
  renvoi à un document de travail interne, marque d'attribution ou chemin
  personnel ne sort : dans l'arbre, dans tout l'historique (identités et
  étiquettes comprises) ou dans des paquets. Un témoin fabriqué doit être vu en
  entier. La CI le lance sur l'arbre, sans contrôler l'identité : une
  contribution garde le nom de son auteur. Avant de publier, le mainteneur lance
  `SERMO_BANC_PUBLICATION=1 bash ci/bancs.sh`.

### Documentation
- Le paquet EFL s'appelle `libefl-all-dev` : `libefl-dev`, écrit dans trois
  documents, n'existe pas dans Debian.
- Les pages de manuel de gtk3sermo et gtk4sermo promettaient que `--glade-xml`
  charge un fichier Glade. L'option est acceptée et **sans effet** : cette édition
  n'a pas de bibliothèque Glade (libglade n'existe ni pour GTK 3 ni pour GTK 4),
  comme le disait déjà `--help`.
- `CONTRIBUTING`, `COMPILE` et le manuel développeur expliquent comment tout
  rejouer comme la CI ; le banc de comportement compte 40 cas, et non 24 ou 22.

### Vérifié, pas supposé
- Clone neuf, limité à deux cœurs (la taille d'un exécuteur partagé de CI) :
  `ci/construire.sh` construit les dix projets en 99 s (12 avertissements, aucune
  erreur), `--version` rend 2.6.8 sur les sept binaires ; `ci/bancs.sh` joue
  86 bancs en 17 minutes.
- XML 55/55 et comportement 40/40 sur les sept backends ; durcissement, liste de
  commandes, `--do` et limite de widgets verts sur les sept ; échappement et clic
  (17 widgets) verts sur les six ports graphiques, sdl3 compris ; mot de passe
  masqué vert sur ncurses ; FORTIFY du cœur 13/13, 6/13 et 8/14 objets ; tests
  unitaires 3/3 ; sermoman-mcp : 6 contrôles sur 6 contre le code, 392
  exécutions sans échec.
- `ci/bancs.sh` éprouvé sur quatre bancs fabriqués : vert, rouge, « rien
  vérifié » (code 77) et figé sont classés comme il faut, et les trois derniers
  le font échouer.
- Restent rouges, pour les raisons dites plus haut : les exemples réels, 54 sur
  55 sur gtk3, gtk4, qt6 et sdl3 (`glade`), 53 sur 55 sur fltk1 et efl1 (`glade`,
  `progressbar`).
- Trouvé en chemin, non corrigé (voir TODO.md) : avec `--program`, efl1 arrêté
  par SIGTERM (il sort alors en code 1) et qt6 privé de son serveur X écrivent un
  faux « syntax error » ; les notes CET sont posées au lien, mais ni la glibc ni
  les toolkits de Debian testing n'en portent.

## [2.6.7] — 2026-09-16 — Des protections promises, rendues réelles

Version CORRECTIF : quatre défauts de sécurité, deux défauts du terminal, et le
ménage qui précède la publication.

### Sécurité
- ⛔ **FORTIFY_SOURCE ne protégeait pas le cœur.** `-D_FORTIFY_SOURCE=3` était bien
  passé, mais le cœur, gtk3, gtk4, fltk1 et efl1 compilaient sans aucun `-O` :
  sans optimisation, FORTIFY ne fait rien. Mesuré : zéro appel `__*_chk` dans les
  trois variantes de `libsermocore.a`. Ils choisissent maintenant le type
  `Release` par défaut, comme qt6, sdl3 et ncurses. `-w`, qui taisait
  l'avertissement de la glibc, quitte notre code.
- ⛔ **`garde_durcissement.sh` répondait OK à tort** : il prenait `__stack_chk_fail`
  pour une preuve de FORTIFY. Il l'exclut, et porte un témoin.
  `garde_fortify_coeur.sh` (nouveau) contrôle la bibliothèque du cœur, que le
  backend optimisé de qt6, sdl3 et ncurses masquait.
- ⛔ **`SERMO_ALLOWED_CMDS` se contournait par un homonyme.** Seul le nom de base
  était comparé : `/tmp/x/ls` passait dès que `ls` était listé. La comparaison
  porte sur la commande telle qu'elle sera lancée : un nom seul, le chemin où le
  `PATH` le trouve, ou un chemin listé tel quel.
- **Les réglages de la 1.x ne sont plus perdus.** `HAPLO_ALLOWED_CMDS` et
  `HAPLO_NO_SHELL_FALLBACK` sont relus quand le nom `SERMO_…` n'est pas posé, avec
  un avertissement.
- **ncurses n'affiche plus le mot de passe** pendant la saisie : une étoile par
  caractère. `garde_ncurses_mot_de_passe.sh` (nouveau) le vérifie dans un vrai
  pseudo-terminal.

### Corrigé
- **ncurses : aucun champ texte ne se remplissait en terminal.** La saisie
  abandonnait au bout de 100 ms, avant toute frappe, et un « q » tapé ensuite
  quittait le programme. Un bouton nu comme `<button ok>` ne fermait pas non plus
  le dialogue.
- **`--version` annonçait 2.0.0** sur les sept ports : le numéro vient désormais du
  `CMakeLists.txt` du cœur.
- La documentation écrit `eval "$(sermo …)"` : sans guillemets, le shell
  développait les jokers de la sortie.

### Documentation et licences
- `LICENCES.md` dit ce que le dépôt contient vraiment : le contrat MIT, Dear ImGui
  (MIT) embarqué dans sdl3, les tests et exemples neufs en CC0, et le fait que
  188 des 210 fichiers d'`examples/` viennent de gtkdialog. Le nom de l'auteur
  d'origine, abîmé dans 88 en-têtes, est rétabli ; `AUTHORS` crédite tout l'amont.
- Les renvois vers des documents de travail non publiés ont quitté le dépôt,
  comme la skill `for-claude/` : `sermoman-mcp` reste le seul outil pour l'IA.

### Compatibilité
- ⚠️ **`SERMO_ALLOWED_CMDS` est plus stricte.** Une commande écrite avec un chemin
  n'est acceptée que si ce chemin est listé, ou si c'est celui où le `PATH` trouve
  un nom listé. Sur un système où le `PATH` trouve `/usr/bin/echo`, lister `echo`
  n'autorise plus `/bin/echo` : listez `/bin/echo`, ou écrivez `echo`.
- Un build `Debug` n'a pas FORTIFY : il n'agit qu'avec l'optimisation.

### Vérifié, pas supposé
- Clone neuf, sans type de build imposé : les dix projets (cœur ×3, sept backends)
  se construisent en `Release`, 12 avertissements en tout, aucune erreur.
- `--version` : `sermocore version 2.6.7` sur les sept binaires.
- `garde_fortify_coeur.sh` : 13 objets fortifiés sur 13 (cœur neutre), 6 sur 13
  (GLib), 8 sur 14 (GTK 4) ; avant : 0 dans les trois.
- Banc XML 55/55 et banc de comportement 40/40 sur les sept backends.
- `garde_durcissement.sh` et `garde_allowed_cmds.sh` verts sur les sept ;
  `garde_ncurses_mot_de_passe.sh` vert. Chacune échoue sur le binaire d'avant ou
  sur un sabotage volontaire : elles mesurent quelque chose.
- Bancs de sermoman-mcp : documentation conforme au code (6/6), 56 exemples sur les
  sept backends (392 exécutions, 0 échec).
- Restent rouges, et l'étaient avant : `garde_clic_widgets.sh` sur sdl3 (aucune
  fenêtre sous Xvfb sans OpenGL) et `garde_fonctions_interdites.sh` lancé hors de
  son périmètre sur `libsermocore/include` (`strtod` voulu de la couche de
  compatibilité).

## [2.6.6] — 2026-09-15 — Cinq cent quatre-vingt-six en-têtes qui dataient

Version CORRECTIF : aucun changement de code. **586 fichiers source** cessent
d'annoncer une version dans leur en-tête.

### Corrigé
- ⛔ **Un en-tête de source portait un numéro de version — donc il mentait.**
  586 fichiers se présentaient sous quinze étiquettes différentes, presque toutes
  mortes : `sermo 2.0.0` (138), `qt6sermo 1.0.0` (102), `fltk1dialog 1.0.0` (82),
  `efl1dialog 1.0.0` (77), `sermo 2.5.0` (65), `gtk4sermo 1.0.0` (24)… Quatre
  portaient même l'ANCIEN nom du produit, celui d'avant le renommage.
  Le numéro n'est pas mis à jour, il est **retiré** : la ligne dit désormais
  `sermo — haplo-dialog <devel@haplo-dialog.fr>`, sans chiffre. Un fichier source
  n'a aucune raison de porter une version : c'est précisément ainsi qu'il se
  périme sans que personne ne le voie — deux fois aujourd'hui, une fois dans un
  exemple, une fois ici.
  ⚠️ Le nettoyage de la 2.1.0 en avait traité 216 et n'avait regardé que les
  NOMS ; les numéros, eux, ont continué de dériver derrière.

### Ce qui n'a pas bougé
- Les lignes de **copyright** et les **SPDX** : intactes, fichier par fichier.
  La provenance et la propriété ne dépendent pas d'un numéro de version.
- Le code : la passe ne touche qu'une ligne de commentaire par fichier.

### Vérifié, pas supposé
- Les trois variantes du cœur et les sept backends recompilent ; comportement
  **7 ports × 40 cas** au vert ; `garde_spdx` verte sur les neuf dossiers source.

## [2.6.5] — 2026-09-15 — Trois défauts empilés sous un seul clic

Version CORRECTIF : sur `fltk1`, ouvrir un `<expander>` brouillait toute la
fenêtre. Trois défauts empilés, tous invisibles au banc.

### Corrigé
- ⛔ **Les voisins n'étaient jamais replacés.** Le rappel de bascule agrandissait
  le groupe de l'expander et s'arrêtait là. Or ce port **épingle** chaque enfant
  d'une boîte à sa hauteur (`Fl_Flex::fixed()`) au moment de la construction : un
  enfant qui grandit ENSUITE chevauche ses voisins au lieu de les pousser.
  Mesuré : l'étiquette du bas remontait par-dessus le titre. La bascule remonte
  désormais la chaîne des boîtes — épingle remise à la hauteur réelle, boîte
  agrandie du même delta (sans quoi `layout()` prend la place aux voisins au lieu
  de l'ajouter), répartition refaite — et la fenêtre suit.
- ⛔ **Le groupe étirait ses propres enfants.** Un `Fl_Group` dont le `resizable`
  est lui-même redimensionne TOUS ses enfants au prorata : le bouton d'en-tête
  grandissait avec le groupe (titre flottant au milieu d'une bande haute) et le
  corps atterrissait par-dessus le widget suivant. La géométrie intérieure n'est
  plus déduite, elle est **écrite** : en-tête à `HEADER_H`, corps juste dessous,
  à la création comme à chaque bascule.
- ⛔ **`@v` n'est pas un symbole FLTK.** Le port l'employait pour la flèche de
  l'état ouvert : FLTK ne dessinait donc **rien**. La rotation s'écrit avec le
  chiffre du pavé numérique — `@2>` pointe vers le bas. L'expander ouvert a de
  nouveau une flèche.

### Vérifié, pas supposé
- Clic réel sous Xvfb, trois captures : replié, ouvert, refermé. Le titre reste,
  le contenu tient dans son cadre, le voisin est poussé vers le bas puis remonte,
  et la fenêtre grandit puis reprend sa taille.
- Comportement **7 ports × 40 cas** au vert ; XML 55/55 sur `fltk1` ;
  `garde_clic_widgets` 17 widgets sans plantage ; durcissement, SPDX et fonctions
  interdites verts.

### Ce que cela met au jour
- ⚠️ **159 fichiers source** de `fltk1` (82) et `efl1` (77) se présentent encore
  dans leur en-tête comme « fltk1dialog 1.0.0 » / « efl1dialog 1.0.0 » : l'ancien
  nom du produit et une version morte. Le nettoyage de la 2.1.0 en avait traité
  216 et laissé ceux-là.

## [2.6.4] — 2026-09-15 — Le titre que le clic effaçait

Version CORRECTIF : deux dettes soldées sur `<expander>` — le titre qu'un clic
effaçait sur `fltk1`, et la documentation qui n'existait pas.

### Corrigé
- ⛔ **`fltk1` effaçait le titre de l'expander au premier clic.** Le rappel de
  bascule réécrivait l'étiquette du bouton d'en-tête avec la SEULE flèche
  (`btn->label("@v  ")`) : un expander nommé « Détails » devenait anonyme dès
  qu'on l'ouvrait, et le restait. Le titre est désormais gardé dans l'état du
  widget et réécrit à chaque bascule, par la même fonction qui le pose à la
  création — une seule façon de composer l'en-tête, donc plus de divergence.
  ⚠️ Aucun banc ne pouvait le voir : le banc de comportement compare des
  VALEURS, pas l'écran. Mesuré au clic réel, sous Xvfb, captures à l'appui.

### Documentation
- **`<expander>` n'était documenté nulle part** dans le manuel utilisateur : ni
  la balise, ni son attribut, ni la valeur qu'elle exporte. Il l'est, avec les
  trois choses qu'un script doit savoir : l'état exporté (`true`/`false`),
  `expanded=` qui ouvre au démarrage (et son absence qui **replie**), et les
  deux écritures possibles du titre.
- ⚠️ **La référence servie par le MCP était fausse sur ce point.** Elle
  affirmait que `<expander><label>…</label>` « ne passe pas l'analyseur ; seul
  `label="…"` fonctionne ». Vérifié aux quatre formes : `<label>` **APRÈS** le
  contenu passe, `label="…"` passe, `<label>` **avant** le contenu est refusé,
  `<expander Titre>` est refusé. Ce n'est pas la forme qui était en cause, c'est
  la PLACE — la grammaire est `<expander>` contenu, puis attributs. Corrigé.

### Ce que cela met au jour
- ⚠️ **`fltk1` brouille la disposition quand l'expander s'ouvre** : le groupe
  grandit (`parent->size()`), mais les widgets voisins ne sont pas replacés — ils
  se chevauchent et le contenu sort du cadre. Vu à la capture, non corrigé ici :
  c'est une question de mise en page du parent, pas d'étiquette.

## [2.6.3] — 2026-09-15 — L'expander qui naissait du mauvais côté

Version CORRECTIF : `<expander>` naissait dans le mauvais état sur les cinq ports
neutres — et les deux moitiés se trompaient en sens inverse.

### Ajouté
- **Un cas au banc de comportement** (40-expander) : corpus 39 → **40 cas**. Il
  joue DEUX expanders, un ouvert et un replié, chacun avec un enfant qui exporte
  une vraie valeur : l'état initial et la survie de l'enfant sont mesurés
  ensemble.

### Corrigé
- ⛔ **`expanded=` n'était lu par aucun port neutre.** L'étalon lit l'ATTRIBUT DE
  BALISE `expanded=` (« true », « yes » ou 1) et **replie par défaut**. Les cinq
  ports se partageaient deux erreurs opposées :
  - `qt6` et `fltk1` repliaient toujours — `<expander expanded="true">` exportait
    « false » ;
  - `efl1`, `sdl3` et `ncurses` ouvraient toujours — un expander sans attribut
    exportait « true » là où l'étalon exporte « false ».
  Ces trois-là lisaient `<default>`, que l'étalon **ignore** pour ce tag : ils
  répondaient à une syntaxe qui n'existe nulle part ailleurs. Ils lisent
  maintenant `expanded=`, avec la règle exacte de l'étalon.
- `fltk1` ouvre désormais aussi sa zone de contenu et pose la flèche vers le bas
  quand l'expander naît ouvert : l'état exporté et l'état à l'écran ne divergent
  pas.

### Vérifié, pas supposé
- Comportement **7 ports × 40 cas** au vert, 0 écart ; XML **55/55** sur les
  sept ; durcissement, SPDX et fonctions interdites verts sur les cinq ports
  touchés.

### Ce que cela met au jour
- ⚠️ **`<expander>` n'est documenté nulle part** dans le manuel utilisateur : ni
  la balise, ni son attribut `expanded=`, ni la valeur qu'elle exporte. Un tag
  livré sur sept ports que la documentation ignore, c'est le même trou
  documentaire sous une autre forme.

## [2.6.2] — 2026-09-15 — Le cadre qui taisait son titre

Version CORRECTIF : `<frame>` exporte enfin son titre sur les cinq ports
neutres. Le défaut est sorti en écrivant les cas de banc qui manquaient aux deux
plus gros conteneurs du langage.

### Ajouté
- **Deux cas au banc de comportement** (38-frame, 39-notebook) : corpus 37 →
  **39 cas**, joués sur les sept ports. `<frame>` et `<notebook>` sont livrés
  partout depuis longtemps et **aucun cas ne les jouait** — exactement l'état
  dans lequel étaient `<flowbox>`, `<overlay>` et `<revealer>` la veille. Les
  deux cas portent des enfants qui exportent de VRAIES valeurs : un conteneur
  n'exporte rien par lui-même, et un `.attendu` vide est un cas mort.

### Corrigé
- ⛔ **`<frame>` n'exportait pas son titre sur cinq ports.** L'étalon exporte
  `gtk_frame_get_label()` ; `qt6`, `fltk1`, `efl1`, `sdl3` et `ncurses` rendaient
  une chaîne vide — `qt6` rendait même `NULL`. Mesuré par le cas 38 : l'étalon
  rend `FR="Options"`, les cinq rendaient `FR=""`. Le titre était pourtant là
  dans chaque port (`QGroupBox::title`, `Fl_Widget::label`,
  `elm_object_text_get`, `node->label`) — il n'était simplement jamais relu.
  ⚠️ `fltk1` portait un commentaire disant « Les frames n'exportent pas de
  valeur » : honnête, et faux comme politique. Même motif que la note d'`efl1`
  sur `<levelbar>`, corrigée en 2.3.1 — un port ne décrète pas seul ce que le
  langage exporte.
- Le cas 38 couvre AUSSI le cadre **sans** titre, qui doit rendre une chaîne
  vide. Sans lui, la correction aurait pu se contenter d'inventer un titre.

### Vérifié, pas supposé
- `<notebook>` était déjà juste sur les sept ports : page courante exportée,
  enfants des DEUX onglets préservés. Le cas 39 le fixe pour qu'il le reste.
- Comportement **7 ports × 39 cas** au vert, 0 écart ; XML **55/55** sur les
  sept ; durcissement, SPDX et fonctions interdites verts sur les cinq ports
  touchés.

## [2.6.1] — 2026-09-15 — Ce que le journal avait oublié

Version CORRECTIF : rien de neuf dans le langage. Un correctif arrivé le 14 et
jamais écrit ici, un numéro de version qui mentait dans dix-neuf endroits, et du
mort sorti du dépôt — une promesse de backend, sept pages de manuel, un kit
d'empaquetage.

### Corrigé
- ⛔ **`execute_action` redéclaré à la main dans six fichiers.** `actions.h` la
  déclare `int execute_action(GtkWidget*, const char*, const char*)` ; quatre
  backends la redéclaraient sur place, avec un autre type de retour et sans les
  `const`. Même symbole, deux signatures : c'est une violation de l'ODR, donc un
  comportement indéfini. Le compilateur n'a crié que sur `qt6` ; le défaut était
  dans `widget_wizard` et `widget_menubutton` de `qt6` et `fltk1`, et dans
  `widget_wizard` de `gtk3` et `gtk4`. Tous incluent désormais l'en-tête — qui
  porte pourtant sa garde `extern "C"`, si bien que rien n'avait jamais empêché
  de l'inclure. `efl1` faisait déjà ainsi : c'est son patron qui est repris.
  Aucun appel n'utilisait la valeur de retour et les arguments passés sont
  compatibles : le comportement observable ne change pas, l'indéfini disparaît.
  ⚠️ **Le correctif est dans l'arbre depuis le 14**, avant même l'entrée 2.6.0 —
  il n'avait simplement jamais été journalisé. Il l'est ici : un correctif qu'on
  ne peut pas retrouver dans le journal n'existe pas pour qui relira.

- ⛔ **Le numéro de version était faux dans dix-neuf endroits**, dont trois qui
  **sortent avec le paquet** : `sermocore.pc` annonçait `Version: 2.0.0` — un
  backend exigeant `sermocore>=2.3` aurait été refusé par un cœur pourtant plus
  récent que sa demande — et `man sermo` comme `info sermo` affichaient 2.0.0 en
  pied de page. Les seize autres sont documentaires : VERSIONING, PACKAGING (des
  noms de `.deb` à copier-coller qui n'existaient pas), le manuel utilisateur, le
  bilan de santé, la documentation du MCP, et les projets CMake de `sermocore`, `qt6`,
  `sdl3` et `ncurses`. Le numéro ne vivait en vérité que dans le script
  d'empaquetage. Les mentions **historiques** de la 2.0.0 restent : « la ligne
  2.x s'est ouverte sur 2.0.0 » est vrai ; ce qui était faux, c'était de
  l'annoncer comme la version courante.

- ⛔ **La seule page de manuel par backend qui soit VRAIMENT installée mentait.**
  `sermo-backend-qt6/src/qt6sermo.1` est posée dans `share/man/man1` par le
  CMakeLists de qt6, et elle s'intitulait `QT6DIALOG` — l'ancien nom du produit —
  en annonçant `qt6sermo 1.0.0`. Corrigée. Dans la foulée, le
  `message(STATUS "qt6sermo 1.0.0 …")` du même CMakeLists suit désormais
  `${PROJECT_VERSION}` : ce numéro-là ne pourra plus dériver.

### Retiré
- **La promesse d'un backend « web ».** La feuille de route (FR et EN), les deux
  README et la documentation du MCP annonçaient un moteur de rendu web à venir.
  La porte reste ouverte — c'est par elle qu'est arrivé le port `ncurses` — mais
  elle ne nomme plus de moteur : le produit n'annonce que les sept ports qu'il
  livre.

- **Sept pages de manuel mortes.** Cinq `.1` (`gtk3sermo`, `gtk4sermo`,
  `fltk1dialog`, `efl1dialog`, `sdl3dialog`) et deux références XML en `.5`
  annonçaient encore `1.0.0` ou `1.1.1`, et trois portaient l'ANCIEN nom du
  produit. Aucune n'était livrée — l'édition modulaire installe `doc/sermo.1`
  pour les sept ports — et quatre d'entre elles figurent dans les `CLEANFILES`
  de leur `Makefile.am` : le build lui-même les tient pour des produits, elles
  n'auraient jamais dû être suivies. Leurs modèles `.in` restent.
- **Le kit d'empaquetage d'un produit disparu** : `sermo-backend-qt6/packaging/`
  (21 fichiers), recettes arch, rpm, gentoo, slackware et debian pour un
  **`qt6sermo 1.0.0` autonome**. L'édition modulaire ne produit plus ce paquet
  mais `sermo-backend-qt6`, par `packaging/build-debs.sh` à la racine. Aucun
  autre backend n'avait son kit, rien ne le référençait, et il embarquait même
  un journal de build (`qt6sermo.debhelper.log`). Qui voudra empaqueter pour une
  autre distribution devra viser les sept backends, pas ce fantôme.

### Documentation
- Le commentaire du lexer qui suit les règles `<flowbox>` / `<overlay>` /
  `<revealer>` décrivait encore ces trois tags comme un trou ouvert sur six
  ports. Ils sont portés depuis la 2.6.0. Il dit maintenant ce qui a eu lieu, et
  garde la leçon : un jeton dans la grammaire sans `case` dans le cœur donne un
  tag qui fait semblant, et sans cas de banc personne ne le voit.

### Vérifié, pas supposé
- Les trois variantes du cœur et les sept backends recompilent ; comportement
  **7 ports × 37 cas** au vert, 0 écart ; XML **55/55** sur les sept ;
  durcissement vert sur les sept binaires ; SPDX et fonctions interdites verts
  sur les dossiers source touchés.

## [2.6.0] — 2026-09-14 — Le flot, la superposition et le rideau

Version MINEURE : **trois balises de plus** — `<flowbox>`, `<overlay>`,
`<revealer>` — sur les sept ports. Aucun changement du contrat cœur↔backend.

### Corrigé — trois tags qui faisaient semblant
Ces trois balises avaient **déjà un jeton** dans la grammaire, et une
implémentation sur le **seul** port gtk4. Sur les six autres, le cœur n'avait
même pas de `case` : le tag s'analysait sans erreur et **ne produisait rien**.
Un script qui les employait obtenait une fenêtre silencieusement amputée.
Aucun cas de banc ne les jouait — personne ne pouvait le voir.

Ils sont désormais portés partout, avec un cas de banc qui le prouve.

### Ajouté
- **`<flowbox>`** : les enfants remplissent une ligne puis passent à la
  suivante. Attributs `min-children-per-line`, `max-children-per-line`,
  `column-spacing`, `row-spacing`, `selection-mode`. Variable : l'index de
  l'enfant sélectionné.
  ⚠️ gtk3 et gtk4 refluent vraiment quand la fenêtre rétrécit (GtkFlowBox) ;
  les autres ports rangent en **N colonnes fixes** — Qt et FLTK n'ont pas de
  disposition en flot, et `elm_gengrid` ne range que des items construits par
  une classe de rappel, pas des widgets quelconques. Seuls gtk3/gtk4 savent
  **sélectionner** : ailleurs la variable reste vide.
- **`<overlay>`** : le premier enfant est le fond, les suivants flottent
  par-dessus — le seul conteneur du langage qui superpose. `QStackedLayout` en
  mode `StackAll` (qt6), `Fl_Group` à géométrie commune (fltk1), `elm_table`
  à cellule unique (efl1), curseur replacé avant chaque couche (sdl3).
  ⚠️ **Un terminal ne superpose pas** : ncurses dessine le fond puis les
  couches l'une sous l'autre, précédées d'un repère. Le contenu reste lisible
  et la dégradation se voit.
- **`<revealer>`** : un enfant qui apparaît et disparaît. Attributs
  `transition`, `duration`, `reveal`. Variable : `true` ou `false`.
  ⚠️ **L'animation n'existe que sur gtk3 et gtk4** ; ailleurs le port montre
  ou cache. L'attribut `transition` est accepté et ignoré — c'est dit dans le
  manuel plutôt que tu.
- **Un cas au banc de comportement** (37) : corpus 36 → **37 cas**, joué sur
  les sept ports.

### Corrigé — deux défauts que le banc ne pouvait pas voir
Un conteneur exporte une chaîne vide : le banc de comportement compare des
VALEURS, il ne regarde pas l'écran. Ces deux-là ne sont sortis qu'à la capture,
en jouant deux `<button>` dans un `<overlay>` sur chaque port.
- ⛔ **`qt6` superposait à l'envers.** `QStackedLayout` en mode `StackAll`
  montre tout, mais **relève l'enfant COURANT** — et `currentIndex` vaut 0 par
  défaut. Le FOND passait donc au-dessus de tout le reste : l'étalon affichait
  « DESSUS (second) », qt6 affichait « FOND (premier enfant) ». Le dernier
  enfant est désormais relevé, comme `GtkOverlay`.
- ⛔ **`sdl3` écrivait sur la SORTIE STANDARD.** Après la pile, le port
  replaçait le curseur sans soumettre d'item : ImGui refuse d'agrandir la
  fenêtre ainsi et se plaignait **à chaque image** — bandeau rouge à l'écran,
  et le texte de l'erreur sur `stdout`, le canal même où sermo écrit
  `VAR="valeur"`. Un `Dummy()` ferme la pile proprement.

## [2.5.0] — 2026-09-14 — Quatre tags de plus

Version MINEURE : **quatre balises nouvelles** — `<toolbar>`, `<stack>`,
`<wizard>`, `<menubutton>` — sur les sept ports.

### Ajouté
- **`<toolbar>`** : une rangée qui se DIT barre d'outils. ⚠️ Pas de
  `GtkToolbar` : GTK 4 l'a **retiré**, GTK 3 le déprécie — une `GtkBox` avec la
  classe CSS « toolbar » donne le même rendu, le même code des deux côtés, et
  aucune API condamnée. Pas d'`elm_toolbar` non plus : il range des ITEMS, pas
  des widgets quelconques.
- **`<stack>`** : N pages, une seule visible — `<notebook>` sans les onglets.
  `page=` choisit l'étape initiale, `switcher="true"` ajoute une rangée de
  boutons. Export : l'index de la page visible.
- **`<wizard>`** : une suite d'étapes avec Précédent / Suivant / Terminer.
  ⚠️ **Composé, pas emprunté** : GTK 4 a retiré `GtkAssistant` et `QWizard` est
  une **fenêtre**, pas un conteneur. Chaque port l'assemble avec SON mécanisme
  de pages. « Terminer » joue l'`<action>` du tag et ne ferme rien de lui-même.
- **`<menubutton>`** : un menu LÀ OÙ ON EST, pas seulement en tête de fenêtre.
  Ses enfants sont des `<menuitem>` ordinaires. Export : le libellé du dernier
  élément choisi.
- **Quatre cas au banc de comportement** (33 à 36) : corpus 32 → **36 cas**.

### Corrigé
- ⛔ **Collision de noms avec le port gtk4.** Il avait DÉJÀ un `widget_stack.c`
  — un `GtkStack` natif qui exporte le NOM de la page — et le nouveau tag l'a
  écrasé avant qu'on s'en aperçoive. Restauré ; le tag commun s'appelle
  désormais `widget_stackpages_*` dans les sept ports.
- ⛔ **Deux règles `<stack>` dans le lexer.** L'ancienne était
  ombragée par la nouvelle — deux règles pour un même motif, c'est la première
  qui gagne. Retirée, avec la raison écrite sur place.
- **`<menubutton>` ne s'affichait pas sur gtk3** : `gtk_container_add()` sur un
  `GtkMenuButton` ne met rien (il a déjà son enfant, la flèche). `set_label`
  est la bonne porte.
- **Le `<wizard>` de gtk3 restait invisible** : une `GtkStack` ne choisit une
  page que parmi les enfants **montrés**, et le cœur ne montre l'arbre qu'à la
  fin. Trouvé en capture, pas au banc — l'export, lui, était juste.

### Ce que cela met au jour, et qui reste ouvert
- ⚠️ **`<stack>` existait déjà**… mais ne marchait que sur gtk4 : sur les six
  autres ports, le cœur neutre n'avait même pas de `case` — le tag s'analysait
  et ne produisait **rien**. **`<flowbox>`, `<overlay>` et `<revealer>` sont
  dans cet état**, aujourd'hui encore : un jeton dans la grammaire, une
  implémentation sur un seul port. Trois tags à porter ou à retirer.

## [2.4.0] — 2026-09-14 — Un nombre, une écriture

Version MINEURE : le **comportement d'export change** pour quatre tags —
`<hscale>`, `<vscale>`, `<spinbutton>` et `<progressbar>` — sur les sept ports.
Aucun tag nouveau. C'est la suite directe de 2.3.1 : la même maladie, le reste
de la famille.

### Changé — ce qu'un script recevait, et ce qu'il reçoit maintenant
- **Le nombre s'écrit avec un POINT, partout et toujours.** L'étalon écrivait
  par `g_snprintf("%.2f")`, qui suit la locale : sous `fr_FR` un `<hscale
  digits="2">` rendait « 2,50 », sous `LC_ALL=C` « 2.50 ». Même binaire, même
  script, deux sorties. Désormais `g_ascii_formatd` — vérifié sous `fr_FR`,
  `C` et `de_DE`.
- **`digits=` décide des décimales, sur les sept ports.** L'étalon l'honorait
  déjà ; les six autres l'ignoraient. Mesuré avant correction sur
  `<hscale digits="2"><default>2.5</default>` : l'étalon rendait « 2,50 » ;
  `qt6`, `efl1`, `sdl3` et `ncurses` rendaient « 2 » ; `fltk1` rendait
  « 2,5 ».
- **`<progressbar>` n'exporte RIEN**, comme l'étalon — c'est un afficheur, pas
  une saisie. Cinq ports rendaient « 50 ».
- **`<spinbutton>` sans `digits=` rend un entier.** `qt6` partait de DEUX
  décimales par défaut (« 4.25 » au lieu de « 4 ») ; les ports neutres
  rendaient « %g », qui ignore l'attribut.

### Corrigé
- **`qt6` perdait la partie décimale à la création** : `QSlider` ne connaît que
  des entiers, et le port posait `setValue((int)valeur)`. L'axe est désormais
  mis à l'échelle de 10^digits — `digits=` a enfin un effet à l'écran aussi.
  ⚠️ L'arrondi suit `nearbyint`, pas « +0.5 » : `printf` arrondit au pair le
  plus proche (« %.0f » de 2.5 rend 2), et l'étalon fait pareil.
- ⛔ **`fltk1` : deux types `SermoScale` distincts.** Le cœur route `<vscale>`
  vers `widget_hscale_create` mais son export vers
  `widget_vscale_envvar_construct` : l'objet est créé dans une unité de
  compilation et relu dans une autre. Avec une classe définie dans le namespace
  anonyme de CHAQUE fichier, le `dynamic_cast` échouait **en silence** et les
  décimales étaient perdues (`<vscale digits="1">` rendait « 4 » pour 3.5). Le
  type vit maintenant dans un en-tête partagé, `sermo_scale.h`.
- **`efl1`** rangeait sa valeur sans décimales : `digits` voyage désormais dans
  les données de l'objet Evas — il doit suivre l'OBJET, puisque création et
  export vivent dans deux fichiers différents.

### Ajouté
- **Un cas au banc de comportement** (32-echelles-decimales) : corpus 31 →
  **32 cas**. Il fixe les six valeurs ensemble, avec et sans `digits=`.

## [2.3.1] — 2026-09-14 — La valeur qui dépendait de la machine

Version CORRECTIVE : aucun tag nouveau. `<levelbar>` et `<drawingarea>`
rendaient une valeur différente d'un port à l'autre — et l'étalon lui-même
changeait de réponse selon la LOCALE de la machine.

### Corrigé
- **L'étalon exportait un nombre à la locale.** `g_strdup_printf("%g", 0.5)`
  écrit « 0,5 » sous `fr_FR` et « 0.5 » sous `LC_ALL=C` : même binaire, même
  script, deux sorties. Un script shell qui compare « 0.5 » ne reconnaissait
  plus rien selon la machine. `g_ascii_formatd` partout — c'est la règle de
  `g_ascii_strtod` appliquée à l'**écriture**. Vérifié sous `fr_FR`, `C` et
  `de_DE` : la sortie ne bouge plus.
- **`<levelbar>` était muet sur quatre ports** (`qt6`, `efl1`, `sdl3`,
  `ncurses`) : les widgets étaient portés, mais le répartiteur d'export ne les
  appelait pas — ils tombaient dans le `default:`. `efl1` portait même une note
  disant que c'était voulu ; c'était honnête et c'était faux comme politique.
- **`<levelbar>` rendait n'importe quoi ailleurs** : `fltk1` tronquait à
  l'entier (« 0 » pour 0.5) *et* lisait la valeur avec `atof`, qui rend ZÉRO
  sous une locale française ; `qt6` exportait la **fraction** interne de sa
  barre (0.005) au lieu de la valeur écrite dans le script ; `efl1` rendait
  « 0.500 » là où l'étalon rend « 0.5 ».
- **`<drawingarea>` bavardait** : `gtk4` rendait ses dimensions (« 40:20 »),
  `sdl3`/`ncurses` rendaient « canvas ». L'étalon rend une chaîne vide.
- ⛔ **`<progressbar>` faisait ABORTER le port fltk1** quand il n'avait aucun
  attribut de balise : `get_tag_attribute()` asserte que `attr` n'est pas NUL.
  Le tag le plus banal qui soit, et le programme s'arrêtait.
- **Seize `atof` de plus dans fltk1** (`hscale`, `vscale`, `spinbutton`,
  `progressbar`, `timer`) : même piège de locale, remplacés par
  `g_ascii_strtod`.

### Changé
- ⚠️ **La garde des fonctions interdites ne regardait pas le C++.** Elle
  parcourait `*.c` et `*.h` seulement — or `qt6` et `fltk1` ont TOUTE leur
  couche widget en `.cpp`. Elle annonçait « aucun appel interdit » sans avoir
  ouvert les fichiers. Elle prend désormais `*.cpp`, `*.cc` et `*.hpp` — et
  elle a immédiatement trouvé les seize `atof` ci-dessus.
- **Un cas au banc de comportement** (31-levelbar-drawingarea) : corpus 30 →
  **31 cas**. Il fixe les deux valeurs ensemble, ce qui garde aussi contre
  l'interversion des deux types corrigée le 2026-09-13.

## [2.3.0] — 2026-09-14 — La poignée

Version MINEURE : **une balise nouvelle**, `<paned>`, rétro-compatible.
Les deux conteneurs prévus sont livrés.

### Ajouté
- **`<paned>`, sur les sept ports.** Deux zones et une poignée que
  l'**utilisateur** déplace : `<hbox>` fige le partage à l'écriture du script,
  `<paned>` le laisse bouger. Attributs : `orientation` (horizontal par défaut
  | vertical), `position` (pixels ou `%`), `resizable`.
  ⚠️ **Exactement deux enfants** ; un troisième est **refusé avec un message**,
  jamais avalé en silence — c'est le défaut qu'avait `<eventbox>`.
- Par port : `GtkPaned` (gtk3, gtk4), `QSplitter` (qt6), `Fl_Tile` (fltk1),
  `elm_panes` (efl1), poignée glissable fabriquée à l'`InvisibleButton` (sdl3),
  frontière déplaçable aux flèches (ncurses).
- **Un cas au banc de comportement** (30-paned) : corpus 29 → **30 cas**.
- `<paned>` rejoint la liste des enfants extensibles des boîtes (gtk3, gtk4,
  efl1) : une poignée sans place à partager ne sert à rien.

### Corrigé
- ⛔ **Le contenu d'un `<edit>` était lu au mauvais endroit de l'union**, sur
  le port `ncurses` : `state.text.content` est un POINTEUR de tas,
  `state.entry.buf` un tableau de 1 024 octets **au même emplacement**. Lire
  `entry.buf` rendait les octets du pointeur puis lisait au-delà jusqu'au
  premier zéro — **débordement de lecture**. Mesuré : `value="��jU"` au lieu du
  texte. Pire encore, la saisie ÉCRIVAIT dans `entry.buf`, écrasant le pointeur
  (fuite, puis `free()` sur un pointeur fabriqué). Lecture et écriture passent
  désormais par `state.text`.

## [2.2.0] — 2026-09-13 — Une grille, enfin

Version MINEURE : **une balise nouvelle**, `<grid>`, rétro-compatible — aucun
script existant ne change de rendu.

### Ajouté
- **`<grid columns="N">`, sur les sept ports.** Les enfants se rangent en flot
  (ordre du document, retour à la ligne tous les N) et les colonnes sont
  **alignées d'une rangée à l'autre**. Jusqu'ici le langage avait 62 types de
  widgets et **deux** conteneurs de disposition : un formulaire s'écrivait en
  empilant des `<hbox>` dans une `<vbox>`, et chaque rangée se largeur-ajustait
  toute seule. Attributs : `columns`, `row-spacing`, `column-spacing`,
  `homogeneous` ; `space-expand`/`space-fill` par enfant.
  ⚠️ `<grid>` n'est pas `<table>` : `<table>` est la liste à colonnes des
  données, héritée de gtkdialog.
- Par port : `gtk_grid_attach` (gtk3, gtk4), `QGridLayout` (qt6), `Fl_Grid`
  (fltk1, nouveau en FLTK 1.4), boîtes alignées à la main (efl1),
  `ImGui::BeginTable` (sdl3), colonnes de largeur égale (ncurses).
- **Deux cas au banc de comportement** (28-grid, 29-grid-flot) : le corpus
  passe de 27 à **29 cas**. Écrits AVANT le code, `.attendu` fabriqués par
  l'étalon gtk3.

### Corrigé
- **fltk1 intervertissait `<levelbar>` et `<drawingarea>`.** Sa copie locale de
  `automaton.h` donnait 0xBE à DRAWINGAREA et 0xBF à LEVELBAR — l'inverse du
  cœur, qui crée les widgets avec SES valeurs. Mesuré : l'étalon rend
  `LB="0,5" DA=""`, fltk1 rendait `LB="" DA="0"`, les deux valeurs croisées.
  Même famille que le bogue des deux énums de la variante GTK4 (corrigé le
  2026-09-09). ⛔ Une copie locale de `automaton.h` ne renumérote JAMAIS.
- **L'installation des en-têtes de la variante GTK4 sautait silencieusement.**
  `file(INSTALL)` compare les HORODATAGES, pas le contenu : `include/` étant
  installé juste avant avec les mêmes noms, un `automaton.h` de `src-gtk4/`
  plus ancien était ignoré — et le backend gtk4 compilait contre l'énum de
  l'étalon. Symptômes observés : `<filechooser>` rendait une date,
  `<password>` rendait « 1 », `<calendar>` rendait vide. Le `CMakeLists.txt`
  du cœur efface désormais nommément les en-têtes que `src-gtk4/` va fournir.

## [2.1.1] — 2026-09-13 — Les tags qui faisaient semblant

Version CORRECTIVE : aucun tag nouveau, aucun changement du langage XML — on
**rend vrai** ce qui était annoncé. Trois backends portaient un fichier
`widget_stubs.c` : le tag était accepté, affichait un avertissement, et ne
faisait rien. Deux autres (`qt6`, `fltk1`) n'avaient pas de fichier de stubs
mais le même trou, **invisible faute de banc**. Trois cas d'épreuve ont été
ajoutés (25-eventbox, 26-linkbutton, 27-pulse) : ils ont mesuré l'écart avant
la correction, ils le gardent après.

### Corrigé
- **`<eventbox>` perdait son contenu.** Sur `efl1`, `sdl3` et `ncurses`, le
  stub ne dépilait pas la pile : l'enfant emballé disparaissait purement et
  simplement (mesuré : le `<text>` d'un eventbox n'apparaissait plus dans la
  sortie). Sur `fltk1`, seul le PREMIER enfant était repris, et il était écrasé
  à 100 × 30 — un `<frame>` emballé sortait de l'écran. Les quatre ports
  enveloppent désormais leur contenu comme une `<vbox>`.
- **`<linkbutton>` n'exportait pas son URI.** Cinq ports sur sept rendaient
  `LB=""` là où l'étalon gtk3 rend l'adresse : trois par stub, `qt6` en
  exportant le LIBELLÉ affiché au lieu de l'URI (et en la lisant dans `<input>`
  au lieu de `<default>`), `fltk1` en ne lisant jamais l'URI et en ne faisant
  rien au clic.
- **`<pulse>` n'exportait pas `pulse`.** Même compte : cinq ports rendaient une
  chaîne vide. `qt6` routait son export vers celui du `<spinner>` ; `fltk1`
  n'affichait qu'une étiquette, sans barre.
- **`<image>` exportait une chaîne vide** sur `efl1`, `sdl3` et `ncurses` : le
  répartiteur d'export court-circuitait le widget réel, pourtant présent.
  L'export rend désormais le **nom d'icône**, comme l'étalon gtk3.

### Ajouté
- **`<filechooser>` choisit vraiment un fichier.** `sdl3` ouvre le **sélecteur
  natif du système** via `SDL_ShowOpenFileDialog` / `SDL_ShowSaveFileDialog` /
  `SDL_ShowOpenFolderDialog` (portail XDG sous Wayland) — API SDL 3.2, aucune
  dépendance ajoutée. `efl1` ouvre `elm_fileselector_button`. `ncurses` ouvre un
  **navigateur de fichiers en terminal** (flèches, `..`, `h` pour les fichiers
  cachés, Échap pour renoncer). Attribut de balise `action` : `open` (défaut),
  `save`, `select-folder`.
- **`<pulse>` s'anime** partout : `elm_progressbar` en mode pulse (efl1),
  `ImGui::ProgressBar` à fraction négative (sdl3), bloc qui va et vient dessiné
  à la main (fltk1, ncurses).
- **`<linkbutton>` ouvre vraiment le lien** : `SDL_OpenURL` (sdl3), `xdg-open`
  lancé **sans shell** (fork + execlp, URI en `argv[1]`) pour `ncurses`, `efl1`
  et `fltk1` — une URL
  de requête contient couramment un `&`, et passer par `/bin/sh` rouvrirait une
  injection. Une URI commençant par `-` est refusée.
- **Trois cas au banc de comportement** (25 à 27) : le corpus passe de 24 à
  **27 cas**, rejoués sur les sept ports.

### Retiré
- Les fichiers `widget_stubs.c` des trois backends qui en portaient un. Le
  créateur de `<menu>`, qui y vivait sans être un stub, rejoint
  `widget_menubar.c`.

## [2.1.0] — 2026-09-12 — La maison mise en ordre

Version MINEURE : ajouts rétro-compatibles, aucun changement du contrat
cœur↔backend ni du langage XML.

### Ajouté
- `contract/` : l'en-tête `sermo-contract.h` (MIT) est **dans le dépôt**.

### Corrigé
- **Un clone ne compilait pas.** Six backends incluaient `sermo-contract.h`
  depuis `../../sermo-contract`, un dépôt voisin absent de tout clone. Corrigé
  en rapatriant l'en-tête ; sa licence MIT est reproduite dans
  `contract/LICENSE.MIT`.
- **`qt6` délesté de 9 198 lignes mortes** : neuf sources du moteur d'amont
  présentes dans son `src/` et **jamais compilées** (le cœur vient de
  `libsermocore`). Il rejoint le niveau de ses voisins.
- **Trois octets NUL** dans des `dialog_state.h` rendaient ces en-têtes binaires
  pour git — aucun diff lisible. Retirés.
- En-têtes de fichiers : 216 se présentaient sous un autre nom que le leur
  (`sdl3dialog 1.0.0`, `SDL3/ImGui` dans le port ncurses…). Trace du chemin de
  fabrication, corrigée.

### Propriété et provenance
- **602 fichiers** portaient une licence sans dire à qui ils appartenaient. Ils
  portent désormais un copyright. **Zéro orphelin** sur les cinq ports non-GTK.
- **Provenance mesurée**, plus seulement affirmée : comparaison fichier par
  fichier contre l'archive d'amont `gtkdialog 0.8.3` (GTK2 uniquement, d'où le
  fait que la couche de rendu des cinq ports non-GTK est nécessairement neuve).
  Table dans `LICENCES.md`.
- `gtk3` a un **second amont** consigné : l'ancrage Wayland `layer-shell` vient
  de la lignée reprise après Pere — copyrights Dima Krasner et Mick Amadio,
  conservés.

## [2.0.0] — 2026-09-09 — Édition modulaire

Première version de l'édition modulaire (non publiée). Elle est faite pour
remplacer la lignée monolithique 1.x. Le bump MAJEUR tient au **changement du
contrat de frontière cœur↔backend** (`sermo_backend.h`) : un backend de la lignée
monolithique ne se lie plus au nouveau cœur. Le **langage XML reste
rétro-compatible** : les scripts gtkdialog / sermo 1.x restent valides.

### Ajouté
- **7e backend `ncurses`** (terminal, le plus léger) : TUI, chemin headless,
  parité comportement 24/24 + XML 55/55 ; réglage `SERMO_NCURSES_THEME`
  (clair|sombre|bleu|gris) et occultation des images/icônes.
- **Thème clair/sombre par backend**, piloté par `SERMO_DARK`
  (`sermo_desktop_is_dark()` dans le cœur).
- Banc `tests/run_examples.sh` : ouvre réellement les 210 exemples contre un
  binaire donné (au-delà du parse et des 24 cas de comportement).

### Modifié
- **Phase 3** : le contrat cœur↔backend est renommé `qt6_*` →
  **`sermo_be_*`** (le cœur ne nomme plus aucun toolkit ; boucle =
  `sermo_be_run_loop`).
- **Version unifiée à 2.0.0** pour le cœur et les sept backends livrés ensemble.

## [1.1.1] — Modularisation (Phase 1)

L'édition modulaire sépare le sermo monolithique en un **cœur** unique
(`libsermocore`) et des **backends de rendu** autonomes, un par toolkit. Le
langage XML et le comportement observable restent identiques à sermo 1.1.x.

### Ajouté

- **`libsermocore`** : cœur syntaxique (lexer/parser Flex/Bison) + exécution
  durcie (`safe_system`/`safe_popen`), variables, actions, automate — **sans
  aucune dépendance graphique** dans la variante neutre.
- **Trois variantes de build du cœur** depuis une source largement unifiée :
  neutre (shim + stubs, familles qt6/fltk1/efl1/sdl3/ncurses), `SERMOCORE_GLIB` (vraie
  GLib/GTK 3), `SERMOCORE_GTK4` (fichiers cœur réécrits de gtk4sermo).
- **Sept backends** consommant le cœur via pkg-config (`sermocore.pc`) :
  `sermo-backend-{gtk3,gtk4,qt6,fltk1,efl1,sdl3,ncurses}`.
- **Contrat de frontière** `sermo_backend.h` : hook
  `sermo_backend_toolkit_init()` remplaçant l'appel `gtk_init` en dur.
- **Packaging séparé** : `sermo-core-dev` (statique + en-têtes + `.pc`, ne dépend
  que de libc6) et un `sermo-backend-<t>` par toolkit, aux dépendances
  **disjointes** (dérivées de `ldd` → `dpkg -S`). Alias `sermo` via
  `update-alternatives` ; commande `gtkdialog` via le paquet séparé
  `sermo-gtkdialog` (lien → gtk3sermo ; Conflicts/Replaces/Provides `gtkdialog,
  gtk3dialog, haplo-dialog` pour éviter le conflit de fichier avec le gtkdialog
  original et le `gtk3dialog` de BunsenLabs).

### Modifié

- Le bridge d'attributs GObject (`try_set_property` + `widget_set_tag_attributes`)
  **sort du cœur** vers chaque backend (`tag_set_property.c`), le cœur ne gardant
  que les helpers indépendants du toolkit.
- `main()` vit désormais dans le cœur (`gtkdialog.c`) et rappelle le hook d'init
  du backend.

### Corrigé

- **efl1** : rognage du pied de page (fenêtre figée) → `default-width/height`
  traités comme **minimums**, la fenêtre grandit au contenu.
- **efl1** : `calendar` renvoyait la date à J-1 (round-trip `time_t` d'`elm_calendar`
  cassé à travers les saisons heure d'été/hiver) → date canonique gardée hors du
  round-trip. efl1 repassé 24/24.
- **gtk4** : 3 écarts de valeur (`05-filechooser`→date, `11-infobar`→type,
  `13-password`→`1`) dus à **deux énums `WIDGET_*` distincts** dans la variante
  `SERMOCORE_GTK4` : le parser généré voyait l'`automaton.h` de l'étalon tandis
  que les fichiers cœur de gtk4sermo voyaient le leur (bloc 0xB4–0xC3 réordonné).
  `GTK4SRC` placé en tête du chemin d'inclusion → un seul énum. gtk4 repassé 24/24.
- **qt6** : icônes de boutons absentes sous Xvfb nu → repli `sermo_icon_lookup`
  quand `QIcon::fromTheme` échoue faute de thème de plateforme.
- **Thème système** : détection partagée `sermo_desktop_is_dark()`
  (`SERMO_DARK` → `GTK_THEME` → `settings.ini` → xsettings ; clair par défaut) ;
  chaque port suit le thème du système en cours. efl1 reste sombre par défaut.

### État des ports (Phase 1)

| Port | Sur `libsermocore` | XML · comportement · garde |
|---|---|---|
| qt6, gtk3, fltk1, efl1, sdl3 | oui | 55/55 · 24/24 · 4/4 |
| gtk4 | oui (variante dédiée) | 55/55 · 24/24 · 4/4 |
| ncurses (terminal) | oui (variante neutre) | 55/55 · 24/24 · source (durcissement/SPDX) |

**Les sept ports à parité complète (24/24)** — six en GUI, ncurses en terminal
(banc comportement rejoué en mode batch headless). gtk4 tourne sur la variante
`SERMOCORE_GTK4` (GTK 4 a retiré `gtk_main`/`gtk_socket`/l'ancien modèle
d'évènements). Deux bugs résiduels corrigés (voir *Corrigé* ci-dessus) :
calendar efl1 à J-1, et 3 écarts gtk4 dus à un mélange d'énums `WIDGET_*`.

### Connu / à venir

- **Part 3** : généraliser les ponts `qt6_*` → `sermo_be_*`, sortir la
  boucle d'évènements côté backend. Cela permettra de remettre gtk4 sur la source
  unifiée, une `.so` partagée propre, puis un cloisonnement de processus (IPC).

---

## Historique amont

Cette édition modulaire reprend la lignée **sermo** (elle-même issue de **gtkdialog**,
GPL-2.0-or-later). L'historique des versions monolithiques de sermo
(1.0.0 → 1.1.x : 43+ widgets, durcissement `safe_system`, échappement de sortie,
unification du lexer/parser sur l'étalon gtk3sermo) est conservé dans les dépôts
sermo d'origine. Cette édition modulaire en repart au niveau fonctionnel 1.1.1.
