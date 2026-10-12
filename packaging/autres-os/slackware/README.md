# sermo — Slackware

```sh
PORTS="ncurses html" ./sermo.SlackBuild      # défaut : ncurses html
```

Produit un `.txz` par port : `sermo-backend-<port>-<version>-<arch>-1_SBo.txz`
(et `sermo-core-dev` avec `PORTS="coeur-dev"`), dans `$OUTPUT` (défaut `/tmp`).

Dépendances : `flex`, `bison`, `cmake`, `pkg-config`, et la bibliothèque du
port (gtk+3 + vte + gtk-layer-shell, ncurses…). Slackware 15.0 n'a ni Qt 6, ni
SDL 3, ni EFL, ni FLTK 1.4 : ces ports demandent -current ou SlackBuilds.org.
Ni gtk-layer-shell (pour gtk3 : `CMAKE_EXTRA=-DSERMO_LAYER_SHELL=OFF`, non essayé),
ni un wayland-protocols assez récent pour lvgl (1.25 n'a `linux-dmabuf` qu'en
`unstable/`). Essayé le 12/10/2026 sur la 2.8.2 : `ncurses` et `html` construits,
installés et lancés.

Pas d'`update-alternatives` sous Slackware : chaque backend livre
`/usr/bin/<port>sermo`.
