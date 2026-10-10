# sermo — Slackware

```sh
PORTS="gtk3 ncurses html" ./sermo.SlackBuild      # défaut : gtk3 ncurses html
```

Produit un `.txz` par port : `sermo-backend-<port>-<version>-<arch>-1_SBo.txz`
(et `sermo-core-dev` avec `PORTS="coeur-dev"`), dans `$OUTPUT` (défaut `/tmp`).

Dépendances : `flex`, `bison`, `cmake`, `pkg-config`, et la bibliothèque du
port (gtk+3 + vte + gtk-layer-shell, ncurses…). Slackware 15.0 n'a ni Qt 6, ni
SDL 3, ni EFL, ni FLTK 1.4 : ces ports demandent -current ou SlackBuilds.org.

Pas d'`update-alternatives` sous Slackware : chaque backend livre
`/usr/bin/<port>sermo`.
