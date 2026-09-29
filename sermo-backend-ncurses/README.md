# sermo-backend-ncurses

Backend **terminal (ncurses)** de sermo. Binaire : `ncursessermo`. C'est le
backend le plus **léger** : il rend l'interface en **mode texte** dans le
terminal, sans serveur d'affichage.

- **Cœur** : variante **neutre** de `libsermocore` (shim `ncurses-compat.h`, sans
  toolkit graphique ; préfixe `_install`).
- **Dépendances** : `ncursesw` uniquement (ni GTK, ni Qt, ni SDL) — le plus petit
  jeu de dépendances des sept backends.
- **Spécificités** :
  - Rendu **TUI** (widgets en mode texte) + boucle d'évènements ncurses
    (`render_ncurses.c`).
  - Thème réglable par `SERMO_NCURSES_THEME` (`clair`|`sombre`|`bleu`|`gris`) ; le
    thème clair/sombre global `SERMO_DARK` est aussi respecté.
  - Les **images et icônes** (sans objet en terminal) sont occultées proprement.
  - `SERMO_NCURSES_BATCH` : chemin **headless** (sans TTY interactif), utilisé par
    les bancs.
  - `neutral_glist.c` fournit `g_list_append` hors du cœur (le shim n'apporte pas
    de vraie GLib).
- **Construire** :
  ```sh
  export PKG_CONFIG_PATH=$PWD/../libsermocore/_install/lib/pkgconfig
  cmake -S . -B _build && make -C _build      # → _build/ncursessermo
  ```
- **Bancs** : `bash ../tests/comportement/run.sh _build/ncursessermo` (voir [MANUEL_DEVELOPPEUR §6](../MANUEL_DEVELOPPEUR.md)).
  XML **55/55** et comportement **53/53** rejoués en **headless** ; le rendu
  identique aux six backends GUI porte donc sur les **sept** ports.

Documentation produit : à la **racine** du dépôt. GPL-2.0-or-later.
