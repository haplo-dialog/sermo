#!/bin/bash
# SPDX-License-Identifier: GPL-2.0-or-later
# tests/garde_render_zone.sh — haplo-dialog — 2026 — GPL-2.0-or-later
#
# Un <drawingarea> prend la taille demandée (width-request/height-request,
# et <width>/<height> pour les ports qui lisent le sous-élément).
#
# Trouvé le 2026-10-19 en vérifiant le manuel : efl1sermo lisait la taille
# mais la posait par evas_object_resize ; dans une boîte, c'est la taille
# MINIMALE qui compte, la zone retombait à zéro (fenêtre de 34x36 pour une
# zone de 200x120 comme de 50x30). Aucun banc ne regardait la taille.
#
# Le garde rend la même fenêtre avec une zone de 200x120 puis de 50x30 : la
# première image doit être plus HAUTE d'au moins 50 pixels (la largeur peut
# être imposée par la fenêtre du port). TÉMOIN : les deux images existent.
#
# Usage : garde_render_zone.sh <binaire>
# Codes : 0 = la taille est suivie · 1 = un défaut · 2 = outil ou binaire manquant
set -u
BIN="${1:?usage: garde_render_zone.sh /chemin/vers/binaire}"
[[ -x "$BIN" ]] || { echo "binaire introuvable : $BIN" >&2; exit 2; }
BIN=$(readlink -f -- "$BIN")
for t in xvfb-run identify; do command -v "$t" >/dev/null || { echo "outil manquant : $t" >&2; exit 2; }; done

ENV_PRE=()
case "$(basename -- "$BIN")" in
    qt6sermo)  ENV_PRE=(QT_QPA_PLATFORM=xcb) ;;
    gtk4sermo) ENV_PRE=(GSK_RENDERER=cairo GTK_A11Y=none) ;;
    efl1sermo) ENV_PRE=(ELM_ENGINE=buffer) ;;
    sdl3sermo) ENV_PRE=(SDL_VIDEODRIVER=offscreen) ;;
esac

T=$(mktemp -d); trap 'rm -rf "$T"' EXIT
rendre() {   # rendre <largeur> <hauteur> <fichier.png>
    export GARDE_ZONE="<window><vbox><drawingarea width-request=\"$1\" height-request=\"$2\"><width>$1</width><height>$2</height><variable>ZONE</variable></drawingarea><button ok></button></vbox></window>"
    ( cd "$T" && env "${ENV_PRE[@]}" xvfb-run -a timeout 30 "$BIN" --program=GARDE_ZONE --render-png="$3" ) >/dev/null 2>&1
}
rendre 200 120 "$T/grande.png"
rendre 50 30 "$T/petite.png"
for f in grande petite; do
    [ -s "$T/$f.png" ] || { echo "ECHEC : --render-png n'a pas écrit $f.png"; exit 1; }
done
hg=$(identify -format '%h' "$T/grande.png"); hp=$(identify -format '%h' "$T/petite.png")
case "$hg$hp" in *[!0-9]*|'') echo "ECHEC : mesure impossible ($hg / $hp)"; exit 1 ;; esac
if [ $((hg - hp)) -lt 50 ]; then
    echo "ECHEC : zone de 200x120 → $hg px de haut, de 50x30 → $hp px : la taille demandée n'est pas suivie"
    exit 1
fi
echo "OK : zone de 200x120 → $hg px de haut, de 50x30 → $hp px"
