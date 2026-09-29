# sermo-backend-sdl3

Backend **SDL 3 + Dear ImGui** de sermo. Binaire : `sdl3sermo`.

- **Cœur** : variante **neutre** de `libsermocore` (préfixe `_install`).
- **Spécificités** :
  - Rendu immédiat via **Dear ImGui** (vendorisé, cible `imgui`).
  - Parsing numérique **locale-C** (`g_ascii_strtod` réel via `strtod_l`).
  - Écrit un `imgui.ini` à l'exécution (état de fenêtre ImGui) — ignoré par git.
  - ⚠️ Sous certains WM (xfwm4), lancer `sdl3sermo` **avant** de démarrer le WM
    pour un mapping/décoration corrects (cf. procédure de capture).
- **Construire** :
  ```sh
  export PKG_CONFIG_PATH=$PWD/../libsermocore/_install/lib/pkgconfig
  cmake -S . -B _build && make -C _build      # → _build/sdl3sermo
  ```
- **Bancs** : `bash ../tests/comportement/run.sh _build/sdl3sermo` (voir [MANUEL_DEVELOPPEUR §6](../MANUEL_DEVELOPPEUR.md)).

Documentation produit : à la **racine** du dépôt. GPL-2.0-or-later.
