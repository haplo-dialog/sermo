# sermo — Slackware

```sh
PORTS="ncurses html" ./sermo.SlackBuild      # défaut : ncurses html
```

Produit un `.txz` par port : `sermo-backend-<port>-<version>-<arch>-1_SBo.txz`
(et `sermo-core-dev` avec `PORTS="coeur-dev"`), dans `$OUTPUT` (défaut `/tmp`).

Dépendances : `flex`, `bison`, `cmake`, `pkg-config`, et la bibliothèque du
port (gtk+3 + vte, ncurses, wayland-protocols pour lvgl…). Slackware 15.0 n'a ni Qt 6, ni
SDL 3, ni EFL, ni FLTK 1.4 : ces ports demandent -current ou SlackBuilds.org.
Ni gtk-layer-shell : le script construit alors gtk3 sans l'ancrage Wayland
(`-DSERMO_LAYER_SHELL=OFF`) et le dit. Essayé en conteneur : `ncurses` et `html`
(12/10/2026, 2.8.2), puis `gtk3` et `lvgl` (13/10/2026) construits, installés et
lancés ; les pages de manuel ne sont plus qu'une fois, compressées, dans
`/usr/man`.

Pas d'`update-alternatives` sous Slackware : chaque backend livre
`/usr/bin/<port>sermo`.
