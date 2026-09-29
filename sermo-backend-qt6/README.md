# sermo-backend-qt6

Backend **Qt 6** de sermo. Binaire : `qt6sermo`. Langage : **C++**.

- **Cœur** : variante **neutre** de `libsermocore` (shim `sermocore-shim.h`, sans
  toolkit). Préfixe `_install`.
- **Spécificités** :
  - `widget_*.cpp` (C++), pont d'opérations neutre **`sermo_be_*`** (rename
    `qt6_*`→`sermo_be_*` effectué ; boucle = `sermo_be_run_loop()`). Les
    `qt6_*` restants sont des helpers internes au backend, pas le contrat.
  - `QApplication` reçoit un argv **filtré** (sermo possède sa propre CLI) — les
    options `--help`/`--do`… sont traitées par le cœur, pas par Qt.
  - Parsing numérique **locale-C** (`g_ascii_strtod`→`strtod_l`) ; `g_strtod`
    garde la sémantique GLib. Repli d'icônes `sermo_icon_lookup`.
- **Construire** :
  ```sh
  export PKG_CONFIG_PATH=$PWD/../libsermocore/_install/lib/pkgconfig
  cmake -S . -B _build && make -C _build      # → _build/qt6sermo
  ```
- **Bancs partagés** : le banc de comportement (`tests/comportement/`) et
  `tests/run_examples.sh` vivent ici et servent **tous** les backends
  (`bash ../tests/comportement/run.sh <binaire>`).

Documentation produit : à la **racine** du dépôt. GPL-2.0-or-later.
