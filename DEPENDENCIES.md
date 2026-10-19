# Dépendances — sermo

[English](DEPENDENCIES.en.md)

## Cœur `libsermocore`

**Compilation :** `flex`, `bison`, `cmake` (≥ 3.16), `gcc`, `pkg-config`.
La variante neutre n'a **aucune** dépendance graphique. La variante `SERMOCORE_GLIB`
ajoute `libgtk-3-dev` (ABI GLib) et `libvte-2.91-dev` ; `SERMOCORE_GTK4` ajoute
`libgtk-4-dev` et `libvte-2.91-gtk4-dev`. VTE fait exister le widget `<terminal>` :
absent, il **arrête** la construction, sauf refus explicite (`-DSERMOCORE_VTE=OFF`,
et `<terminal>` n'affiche alors qu'un avertissement).

**Exécution :** le paquet `sermo-core-dev` n'a **aucune** dépendance — le `.deb`
ne porte pas de champ `Depends` (bibliothèque statique, rien à charger à
l'exécution).

## Backends — `-dev` de compilation

| Backend | Paquets `-dev` |
|---|---|
| gtk3 | `libgtk-3-dev`, `libvte-2.91-dev` (`<terminal>`), `libgtk-layer-shell-dev` (ancrage Wayland ; sans lui la construction s'arrête, sauf `-DSERMO_LAYER_SHELL=OFF`) |
| gtk4 | `libgtk-4-dev`, `libvte-2.91-gtk4-dev` (`<terminal>`) |
| qt6 | `qt6-base-dev` |
| fltk1 | `libfltk1.4-dev` |
| efl1 | `libefl-all-dev` (Enlightenment/Elementary) |
| sdl3 | `libsdl3-dev`, `libgl-dev` (OpenGL, le rendu ImGui), `libgdk-pixbuf-2.0-dev` (chargement des images) |
| lvgl | `libx11-dev`, `libpng-dev`, `libwayland-dev`, `libxkbcommon-dev`, `wayland-protocols`, et un compilateur C++ depuis la 2.8.2 (ThorVG, le moteur SVG embarqué dans LVGL) ; LVGL lui-même est embarqué dans `sermo-backend-lvgl/tiers/lvgl/` |
| ncurses | `libncurses-dev` (fournit `ncursesw.pc`) |
| html | aucun |

Pour construire les paquets (`debian/rules`), il faut en plus `texinfo` :
`makeinfo` y produit le manuel `info`.

## Backends — dépendances d'exécution

Elles sont **dérivées du binaire** par `dh_shlibdeps` à la construction du
paquet (voir [PACKAGING.md](PACKAGING.md)), donc toujours exactes et
**disjointes** : chaque backend ne tire que sa pile graphique. Bibliothèque principale par backend :

| Backend | Toolkit runtime (principale) |
|---|---|
| gtk3 | `libgtk-3-0t64` (+ pango, cairo, gdk-pixbuf…), `libvte-2.91-0`, `libgtk-layer-shell0` |
| gtk4 | `libgtk-4-1` (+ graphene, gstreamer…), `libvte-2.91-gtk4-0` |
| qt6 | `libqt6core6t64`, `libqt6gui6`, `libqt6widgets6` |
| fltk1 | `libfltk1.4`, `libfltk-images1.4` |
| efl1 | `libelementary1`, `libevas1`, `libecore*`, `libedje1`… |
| sdl3 | `libsdl3-0` |
| lvgl | `libx11-6`, `libwayland-client0`, `libwayland-cursor0`, `libxkbcommon0`, `libpng16-16t64`, `libstdc++6` (LVGL est lié dans le binaire ; ThorVG est en C++) |
| ncurses | `libncursesw6`, `libtinfo6` |
| html | aucune, hors `libc6` |

Toutes incluent `libc6` et les X11/Wayland/fontconfig usuels. La liste complète
est recalculée à chaque construction des paquets (`packaging/construire-paquets.sh`,
dans une Debian testing vierge).

## Serveurs MCP

| Paquet | Compilation | Exécution |
|---|---|---|
| `sermoia-mcp` | `libglib2.0-dev`, `libjson-glib-dev` | GLib, json-glib, et au moins un port de sermo (`sermo-backend-qt6` recommandé : il rend les images sans affichage ; gtk3, gtk4, fltk1 et lvgl demandent `xvfb`) |
| `sermoman-mcp` | rien (Python, bibliothèque standard seule) | `python3` |

## Programmes d'aide (`sermo-source/`)

Ils servent `<input modbus>`, `<input opcua>`, `<input mqtt>`,
`<input mavlink>`, `<input media>`, `<action mqtt>`, `<action mavlink>` et
`<action media>` ; on les installe à part (voir [PACKAGING.md](PACKAGING.md)).

| Programmes | Compilation | Exécution |
|---|---|---|
| `sermo-source-modbus` | `libmodbus-dev` | libmodbus (`libmodbus5`) |
| `sermo-source-opcua` | `libopen62541-1.4-dev` | open62541 (`libopen62541-1.4`) |
| `sermo-source-mqtt`, `sermo-cible-mqtt` | `libmosquitto-dev` | libmosquitto (`libmosquitto1`) |
| `sermo-source-mavlink`, `sermo-cible-mavlink` | rien : les en-têtes C de MAVLink sont embarqués (`sermo-source/tiers/mavlink`) | aucune, hors `libc6` |
| `sermo-source-media`, `sermo-cible-media` | rien | `sermo-cible-media` exige le programme `mpv` et recommande `bubblewrap` (la cage de mpv, obligatoire pour une source réseau) ; `sermo-source-media` recommande `sermo-cible-media` |

Sans sa bibliothèque, un programme n'est pas construit (CMake prévient), sauf
avec `-DSERMO_SOURCE_EXIGER=ON` qui arrête la construction (voir
[COMPILE.md](COMPILE.md)). Les dépendances d'exécution des paquets viennent de
`dh_shlibdeps`, comme pour les backends.

## Vérification / tests

`xvfb` et `xdotool` (gardes graphiques : clic, échappement ; `run_examples.sh`),
`imagemagick` (`import`, captures), `check` (libcheck : tests unitaires),
`python3` (garde du mot de passe ncurses, en pseudo-terminal). Le banc XML, lui,
tourne sans affichage (`--print-ir`).

La liste complète des paquets qu'installe la CI, outils de test compris (les
serveurs d'essai `mosquitto` et `python3-pymodbus`, `mpv`, `bubblewrap` et
`ffmpeg` pour le banc du lecteur…), est [`ci/dependances.txt`](ci/dependances.txt).
