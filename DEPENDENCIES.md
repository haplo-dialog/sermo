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
| sdl3 | `libsdl3-dev` |
| ncurses | `libncurses-dev` (fournit `ncursesw.pc`) |

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
| ncurses | `libncursesw6`, `libtinfo6` |

Toutes incluent `libc6` et les X11/Wayland/fontconfig usuels. La liste complète
est recalculée à chaque construction des paquets (`packaging/construire-paquets.sh`,
dans une Debian testing vierge).

## Vérification / tests

`xvfb` et `xdotool` (gardes graphiques : clic, échappement ; `run_examples.sh`),
`imagemagick` (`import`, captures), `check` (libcheck : tests unitaires),
`python3` (garde du mot de passe ncurses, en pseudo-terminal). Le banc XML, lui,
tourne sans affichage (`--print-ir`).
