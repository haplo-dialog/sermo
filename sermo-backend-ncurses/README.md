# sermo-backend-ncurses

Backend **terminal (ncurses)** de sermo. Binaire : `ncursessermo`. C'est le
backend le plus **léger** : il rend l'interface en **mode texte** dans le
terminal, sans serveur d'affichage.

- **Cœur** : variante **neutre** de `libsermocore` (shim `ncurses-compat.h`, sans
  toolkit graphique ; préfixe `_install`).
- **Dépendances** : `ncursesw` uniquement (ni GTK, ni Qt, ni SDL) — le plus petit
  jeu de dépendances des ports qui affichent (html, qui exporte une page, n'en a
  aucune).
- **Spécificités** :
  - Rendu **TUI** (widgets en mode texte) + boucle d'évènements ncurses
    (`render_ncurses.c`).
  - Thème réglable par `SERMO_NCURSES_THEME` (`clair`|`sombre`|`bleu`|`gris`) ; le
    thème clair/sombre global `SERMO_DARK` est aussi respecté.
  - Les **images et icônes** (sans objet en terminal) sont occultées proprement.
  - `SERMO_NCURSES_BATCH` : chemin **headless** (sans écran), utilisé par les
    bancs. Sans cette variable, l'écran s'ouvre dès que l'entrée standard est un
    terminal — même si la sortie est capturée (`eval "$(ncursessermo …)"`) : il
    se dessine alors sur le terminal de contrôle (`/dev/tty`) et seules les
    variables partent sur la sortie standard.
  - `neutral_glist.c` fournit `g_list_append` hors du cœur (le shim n'apporte pas
    de vraie GLib).
- **Construire** :
  ```sh
  export PKG_CONFIG_PATH=$PWD/../libsermocore/_install/lib/pkgconfig
  cmake -S . -B _build && make -C _build      # → _build/ncursessermo
  ```
- **Bancs** : `bash ../tests/comportement/run.sh _build/ncursessermo` (voir [MANUEL_DEVELOPPEUR §6](../MANUEL_DEVELOPPEUR.md)).
  XML **55/55** et comportement **61/61** rejoués en **headless** ; la parité
  des valeurs avec l'étalon vaut donc pour les **neuf** ports.

Documentation produit : à la **racine** du dépôt. GPL-2.0-or-later.
