# sermo-backend-gtk3

Backend **GTK 3** de sermo. Binaire : `gtk3sermo`.

- **Cœur** : variante `SERMOCORE_GLIB` de `libsermocore` (vraie GLib/GTK 3, ABI
  native). Construire le cœur avec `-DSERMOCORE_GLIB=ON`, préfixe `_install_gtk`.
- **Spécificités** :
  - Fournit `sermo-backend` et l'alternative `sermo`. La commande de
    compatibilité **`gtkdialog`** est, elle, fournie par un paquet séparé et
    optionnel, **`sermo-gtkdialog`** (dépend de ce backend ; cf. PACKAGING.md).
  - Widget `terminal` via **VTE**.
  - Ancrage **Wayland layer-shell** (attributs `layer`/`edge`/`dist`/`reserve`).
- **Construire** :
  ```sh
  export PKG_CONFIG_PATH=$PWD/../libsermocore/_install_gtk/lib/pkgconfig
  cmake -S . -B _build && make -C _build     # → _build/gtk3sermo
  ```
- **Bancs** : voir [MANUEL_DEVELOPPEUR §6](../MANUEL_DEVELOPPEUR.md).

Documentation produit (langage XML, widgets, architecture) : à la **racine** du
dépôt ([README](../README.md), [MANUEL_UTILISATEUR](../MANUEL_UTILISATEUR.md),
[MANUEL_DEVELOPPEUR](../MANUEL_DEVELOPPEUR.md)). GPL-2.0-or-later.
