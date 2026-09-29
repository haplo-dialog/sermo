#!/bin/bash
# SPDX-License-Identifier: GPL-2.0-or-later
# tests/garde_layer_shell.sh — gtk3sermo ancre ses fenêtres par wlr-layer-shell
# quand <window> porte layer= ou edge=.
#
# POURQUOI CETTE GARDE EXISTE
#
# De la 2.0 à la 2.7.0, l'ancrage Wayland n'était pas compilé : le
# -DHAVE_LAYER_SHELL ne vivait que dans l'ancien Makefile.am. Les attributs
# layer, edge, dist et reserve étaient ignorés sans un mot, alors que la 1.x
# dépendait de libgtk-layer-shell0 et que le README les annonçait.
#
# Deux niveaux :
#  1. toujours : gtk3sermo est lié à libgtk-layer-shell ;
#  2. si sway est là et que l'on n'est pas root (sway refuse de tourner en
#     root) : un sway sans écran, et deux dialogues. Le témoin, sans attribut,
#     doit être une fenêtre ORDINAIRE (présente dans l'arbre de sway) ; le
#     dialogue layer="top" doit devenir une SURFACE DE COUCHE (absente de
#     l'arbre, annoncée dans le journal de sway). Le témoin prouve que la
#     mesure sait voir une fenêtre : sans lui, « absente de l'arbre » pourrait
#     vouloir dire « rien ne s'est ouvert ».
# Le niveau 2 non joué est dit dans la dernière ligne ; il ne fait pas échouer.
#
# Usage : garde_layer_shell.sh <chemin-du-binaire>
# Codes : 0 = conforme · 1 = écart · 2 = usage/outil

set -u
BIN="${1:-}"
[[ -x "$BIN" ]] || { echo "usage: $0 <chemin-du-binaire>" >&2; exit 2; }
BIN="$(readlink -f "$BIN")"

echo "garde_layer_shell : $BIN"
if [[ "$(basename "$BIN")" != gtk3sermo ]]; then
    echo "garde_layer_shell : sans objet pour $(basename "$BIN") (l'ancrage Wayland est propre à gtk3sermo)."
    exit 0
fi

echecs=0
ok() { echo "  ✔ $1"; }
ko() { echo "  ✘ $1"; echecs=$((echecs + 1)); }

if ldd "$BIN" | grep -q 'libgtk-layer-shell'; then ok "lié à $(ldd "$BIN" | grep -o 'libgtk-layer-shell[^ ]*' | head -1)"
else ko "gtk3sermo n'est pas lié à libgtk-layer-shell : l'ancrage n'est pas compilé"; fi

niveau2=""
if ! command -v sway >/dev/null || ! command -v swaymsg >/dev/null; then
    niveau2="non joué : sway absent"
elif [[ "$(id -u)" -eq 0 ]]; then
    niveau2="non joué : sway refuse de tourner en root"
else
    # Chemin COURT : une socket Unix ne dépasse pas 108 octets.
    R="$(mktemp -d "${XDG_RUNTIME_DIR:-/tmp}/swl.XXXX")"; chmod 700 "$R"; : > "$R/sway.conf"
    env -u DISPLAY -u WAYLAND_DISPLAY -u SWAYSOCK XDG_RUNTIME_DIR="$R" WLR_BACKENDS=headless \
        WLR_RENDERER=pixman WLR_LIBINPUT_NO_DEVICES=1 timeout 120 sway -c "$R/sway.conf" -d > "$R/sway.log" 2>&1 &
    sway_pid=$!
    trap 'kill "$sway_pid" 2>/dev/null; wait "$sway_pid" 2>/dev/null; rm -rf "$R"' EXIT
    for _ in $(seq 50); do ls "$R"/sway-ipc.*.sock >/dev/null 2>&1 && ls "$R"/wayland-* >/dev/null 2>&1 && break; sleep 0.1; done
    sock="$(ls "$R"/sway-ipc.*.sock 2>/dev/null | head -1)"
    wl="$(ls "$R" | grep -m1 '^wayland-[0-9]*$')"
    if [[ -z "$sock" || -z "$wl" ]]; then
        ko "sway sans écran n'a pas démarré : $(tail -2 "$R/sway.log" | tr '\n' ' ')"
    else
        essai() {  # $1 = attributs de <window> ; imprime « fenêtres_ordinaires surface_de_couche »
            local d="<window title=\"garde-couche\" $1><vbox><text><label>couche</label></text><timer milliseconds=\"true\" interval=\"2500\" visible=\"false\"><variable>_T</variable><action>exit:fin</action></timer></vbox></window>"
            local avant; avant=$(wc -l < "$R/sway.log")
            env -u DISPLAY XDG_RUNTIME_DIR="$R" WAYLAND_DISPLAY="$wl" GDK_BACKEND=wayland MAIN_DIALOG="$d" \
                timeout 20 "$BIN" --program=MAIN_DIALOG >/dev/null 2>&1 &
            local p=$!
            sleep 1.2
            local n; n=$(env XDG_RUNTIME_DIR="$R" swaymsg -s "$sock" -t get_tree 2>/dev/null | grep -c '"name": "garde-couche"')
            wait "$p"
            local c; c=$(tail -n +$((avant + 1)) "$R/sway.log" | grep -o 'new layer surface: namespace [^ ]* layer [0-9]*' | head -1)
            echo "$n|${c:-aucune}"
        }
        temoin="$(essai '')"
        couche="$(essai 'layer="top" edge="top" dist="10"')"
        if [[ "$temoin" == "1|aucune" ]]; then ok "témoin sans attribut : fenêtre ordinaire (la mesure voit les fenêtres)"
        else ko "témoin : attendu « 1|aucune », obtenu « $temoin »"; fi
        if [[ "$couche" == "0|new layer surface: namespace "*" layer 2" ]]; then ok "layer=\"top\" : surface de couche (layer 2), hors de l'arbre des fenêtres"
        else ko "layer=\"top\" : attendu une surface de couche layer 2, obtenu « $couche »"; fi
        niveau2="joué sous sway sans écran"
    fi
fi

if [[ $echecs -gt 0 ]]; then
    echo "ÉCHEC : $echecs écart(s)."
    exit 1
fi
echo "garde_layer_shell : OK — lien vérifié ; niveau 2 $niveau2."
