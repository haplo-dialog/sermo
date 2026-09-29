# sermo-backend-gtk4

Backend **GTK 4** de sermo. Binaire : `gtk4sermo`.

- **Cœur** : variante **`SERMOCORE_GTK4`** de `libsermocore` — fichiers cœur
  réécrits pour GTK 4, **vendorés dans `libsermocore/src-gtk4/`** (aucune
  dépendance externe ; GTK 4 a retiré `gtk_main`/`gtk_socket`/l'ancien modèle
  d'évènements). Construire le cœur avec `-DSERMOCORE_GTK4=ON`, préfixe
  `_install_gtk4`.
- **⚠️ Footgun énum `WIDGET_*`** : l'ordre du bloc 0xB4–0xC3 d'`automaton.h`
  diffère entre l'étalon et la variante GTK4. Le lexer/parser généré et les
  fichiers cœur de `src-gtk4/` doivent voir **le même** `automaton.h` → le CMake
  du cœur met `${GTK4SRC}` (= `src-gtk4/`) **en tête** du chemin d'inclusion
  (`BEFORE`). Ne pas retirer.
- **Construire** :
  ```sh
  export PKG_CONFIG_PATH=$PWD/../libsermocore/_install_gtk4/lib/pkgconfig
  cmake -S . -B _build && make -C _build     # → _build/gtk4sermo
  ```
- **Bancs** : voir [MANUEL_DEVELOPPEUR §6](../MANUEL_DEVELOPPEUR.md).

Documentation produit : à la **racine** du dépôt. GPL-2.0-or-later.
