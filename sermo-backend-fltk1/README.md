# sermo-backend-fltk1

Backend **FLTK 1.4** de sermo. Binaire : `fltk1sermo`. Langage : **C++**.

- **Cœur** : variante **neutre** de `libsermocore` (préfixe `_install`).
- **Spécificités** :
  - `widget_*.cpp` (C++) ; installation légère — ne dépend **que** de FLTK
    (ni GTK ni Qt).
  - Parsing numérique **locale-C** (`g_ascii_strtod` réel via `strtod_l`).
- **Construire** :
  ```sh
  export PKG_CONFIG_PATH=$PWD/../libsermocore/_install/lib/pkgconfig
  cmake -S . -B _build && make -C _build      # → _build/fltk1sermo
  ```
- **Bancs** : voir [MANUEL_DEVELOPPEUR §6](../MANUEL_DEVELOPPEUR.md)
  (`bash ../tests/comportement/run.sh _build/fltk1sermo`).

Documentation produit : à la **racine** du dépôt. GPL-2.0-or-later.
