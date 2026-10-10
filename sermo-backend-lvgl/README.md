# sermo-backend-lvgl — `lvglsermo`

Port de sermo sur [LVGL](https://lvgl.io) v9.6.0, la bibliothèque graphique des
écrans sans bureau (bornes, embarqué, framebuffer). Même cœur neutre que les
ports fltk1, efl1, sdl3 et ncurses : le cœur lit le dialogue, tient les
variables et joue les actions ; ce port dessine et renvoie au cœur ce que
l'utilisateur change.

- **LVGL est embarqué** dans `tiers/lvgl/` (il n'est pas dans Debian). Version,
  commit et empreinte : `tiers/lvgl/PROVENANCE`. Aucune retouche de ses fichiers.
- **Réglage** : `lv_conf.h` — le modèle d'origine (`tiers/lvgl/lv_conf_template.h`)
  avec quelques lignes changées, chacune expliquée en tête du fichier.
- **Affichage** : Wayland si `WAYLAND_DISPLAY` est posé, sinon X11 (l'ordre de
  GTK) ; `SERMO_LVGL_X11=1` force X11. **Framebuffer** (console, borne, sans
  serveur d'affichage) **sur demande seulement** : `SERMO_LVGL_FBDEV=/dev/fb0`,
  entrées par `SERMO_LVGL_EVDEV=/dev/input/eventN` ou `auto`. Jamais choisi
  tout seul : un script lancé sans écran (cron, ssh) dessinerait sur la
  console. Il faut les droits du groupe `video` (écran) et `input` (entrées).
- **Police** : une TTF du système (DejaVu Sans) pour les accents ; les symboles
  de LVGL viennent de ses Montserrat embarquées.
- `SERMO_LVGL_BATCH=1`, ou pas de `DISPLAY` : chemin **sans écran** (exporte sans
  ouvrir de fenêtre), comme `SERMO_NCURSES_BATCH` pour le port terminal.
- **Dialogues** : `<filechooser>` ouvre un navigateur de dossiers (dossiers
  d'abord, « .. » pour remonter, noms cachés masqués), `<colorbutton>` trois
  glissières rouge, vert, bleu avec aperçu, `<fontbutton>` une famille (Sans,
  Serif, Monospace) et une taille. Ils s'ouvrent DANS la fenêtre : une fenêtre
  qui en contient un fait au moins 380×340.
- **Onglets et tableau** : `<notebook>` en vrais onglets (`tab-labels`,
  `tab-pos="left"`), `<table>` en tableau à en-têtes. `<tree>` reste une liste
  simple (le cœur neutre le range ainsi).
- **Clic** : le pilote X11 de LVGL relit la souris toutes les 33 ms et ne garde
  que l'état du moment. Un clic plus bref (appui et relâche dans le même
  relevé, comme `xdotool click`) est perdu ; un clic humain ne l'est pas. Les
  essais automatiques doivent tenir l'appui (`mousedown`, ~0,1 s, `mouseup`).

## Construire

Dépendances en plus du cœur : `libx11-dev`, `libpng-dev`, `libwayland-dev`,
`libxkbcommon-dev`, `wayland-protocols` (les en-têtes de protocole Wayland que
LVGL attend sont engendrés à la construction par `wayland-scanner`).

```sh
ci/construire.sh lvgl          # → sermo-backend-lvgl/_build/lvglsermo
```

Bancs : `ci/bancs.sh lvgl` (XML 55/55, comportement 61/61, taille de fenêtre,
clic sur 17 widgets, exemples réels ; mesurés le 2026-10-10).
