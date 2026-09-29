# sermo-backend-efl1

Backend **Enlightenment / Elementary (EFL)** de sermo.
Binaire : `efl1sermo`.

- **Cœur** : variante **neutre** de `libsermocore` (préfixe `_install`).
- **Spécificités** :
  - **Thème sombre par défaut** (décision projet) ; suit sinon le thème système.
  - `default-width/height` traités comme **minimums** (la fenêtre grandit au
    contenu — corrige un rognage du pied de page).
  - `calendar` : la date canonique est gardée **hors** du round-trip `time_t`
    d'`elm_calendar` (cassé à travers les saisons heure d'été/hiver).
  - Parsing numérique **locale-C** (`g_ascii_strtod` réel via `strtod_l`).
- **Construire** :
  ```sh
  export PKG_CONFIG_PATH=$PWD/../libsermocore/_install/lib/pkgconfig
  cmake -S . -B _build && make -C _build      # → _build/efl1sermo
  ```
- **Bancs** : `bash ../tests/comportement/run.sh _build/efl1sermo` (voir [MANUEL_DEVELOPPEUR §6](../MANUEL_DEVELOPPEUR.md)).

Documentation produit : à la **racine** du dépôt. GPL-2.0-or-later.
