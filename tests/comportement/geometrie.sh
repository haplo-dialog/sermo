#!/bin/sh
# geometrie.sh — ce que run.sh ne peut PAS voir : la mise en page.
# sermo — GPL-2.0-or-later
#
# Usage : ./geometrie.sh /chemin/absolu/vers/le/binaire
#
# run.sh compare les VARIABLES rendues. Certains attributs ne produisent aucune
# variable : ils changent la taille ou l'espacement. border-width en est le cas
# type — il a été ignoré pendant toute la vie du port GTK 4 sans qu'aucun test
# ne bronche, parce qu'aucun test ne regardait une taille.
#
# Principe : la même fenêtre, avec border-width 0 puis N. Si l'attribut est
# appliqué, la fenêtre grandit de 2N dans chaque dimension. Si les deux tailles
# sont identiques, l'attribut est mort.
#
# ⛔ Ne se tait jamais : sans Xvfb, sans xdotool, sans binaire, ou si la fenêtre
# n'est pas trouvée, il sort en ERREUR — jamais un succès vide.

set -e
BIN="${1:-}"
GREEN='\033[0;32m'; RED='\033[0;31m'; BLUE='\033[0;34m'; NC='\033[0m'
mort() { printf "${RED}ERREUR : %s${NC}\n" "$1" >&2; printf "AUCUNE mesure n'a ete faite.\n" >&2; exit 2; }

[ -n "$BIN" ] || mort "usage : geometrie.sh <chemin du binaire>"
[ -x "$BIN" ] || mort "binaire introuvable ou non executable : $BIN"
case "$BIN" in */*) : ;; *) mort "donne un CHEMIN, pas un nom : « $BIN » irait chercher dans le PATH." ;; esac
command -v Xvfb   >/dev/null 2>&1 || mort "Xvfb manquant (paquet xvfb)"
command -v xdotool >/dev/null 2>&1 || mort "xdotool manquant (paquet xdotool)"

DISP=":${DISPLAY_NUM:-96}"
Xvfb "$DISP" -screen 0 1280x1024x24 >/dev/null 2>&1 &
XPID=$!
trap 'kill $XPID 2>/dev/null' EXIT
sleep 2

taille() {
    _n=$1
    _x="<window title=\"geo$_n\" border-width=\"$_n\"><vbox><text><label>MESURE</label></text></vbox></window>"
    MAIN_DIALOG="$_x" DISPLAY=$DISP timeout 15 "$BIN" --program=MAIN_DIALOG >/dev/null 2>&1 &
    _p=$!
    _g=""
    _i=0
    while [ "$_i" -lt 8 ]; do
        sleep 1
        _g=$(DISPLAY=$DISP xdotool search --name "geo$_n" getwindowgeometry 2>/dev/null \
             | grep -oE 'Geometry: [0-9]+x[0-9]+' | awk '{print $2}')
        [ -n "$_g" ] && break
        _i=$((_i+1))
    done
    kill $_p 2>/dev/null; wait $_p 2>/dev/null || true
    [ -n "$_g" ] || mort "fenetre « geo$_n » jamais apparue — le binaire s'est-il lance ?"
    printf '%s' "$_g"
}

printf "${BLUE}────${NC}  Geometrie : %s\n" "$(basename "$BIN")"
T0=$(taille 0);  printf '  border-width=0   %s\n' "$T0"
T20=$(taille 20); printf '  border-width=20  %s\n' "$T20"
T40=$(taille 40); printf '  border-width=40  %s\n' "$T40"

L0=${T0%x*};  H0=${T0#*x}
L20=${T20%x*}; H20=${T20#*x}
L40=${T40%x*}; H40=${T40#*x}

echec=0
if [ "$T0" = "$T20" ] || [ "$T20" = "$T40" ]; then
    printf "${RED}FAIL${NC}  la taille ne bouge pas : border-width est IGNORE\n"; echec=1
fi
# 0 -> 40 doit ajouter 80 px dans chaque dimension.
dl=$((L40 - L0)); dh=$((H40 - H0))
if [ "$dl" -lt 70 ] || [ "$dh" -lt 70 ]; then
    printf "${RED}FAIL${NC}  de 0 a 40 : +%s x +%s px, on attend ~+80 dans chaque dimension\n" "$dl" "$dh"; echec=1
fi

# ── 2. <pixmap> : l'icone prise dans le THEME suit-elle la taille demandee ? ──
# gtk_icon_theme_load_icon a disparu en GTK 4 ; un stub rendait NULL, donc le
# widget tombait toujours sur l'icone cassee, quel que soit le theme installe.
# Aucune variable shell ne le trahit : seule la TAILLE le montre.
taille_icone() {
    _s=$1
    _x="<window title=\"pix$_s\"><vbox><pixmap theme-icon-size=\"$_s\"><input file icon=\"folder\"></input></pixmap></vbox></window>"
    MAIN_DIALOG="$_x" DISPLAY=$DISP timeout 15 "$BIN" --program=MAIN_DIALOG >/dev/null 2>&1 &
    _p=$!
    _g=""; _i=0
    while [ "$_i" -lt 8 ]; do
        sleep 1
        _g=$(DISPLAY=$DISP xdotool search --name "pix$_s" getwindowgeometry 2>/dev/null \
             | grep -oE 'Geometry: [0-9]+x[0-9]+' | awk '{print $2}')
        [ -n "$_g" ] && break
        _i=$((_i+1))
    done
    kill $_p 2>/dev/null; wait $_p 2>/dev/null || true
    [ -n "$_g" ] || mort "fenetre « pix$_s » jamais apparue"
    printf '%s' "$_g"
}

P16=$(taille_icone 16); printf '  pixmap theme-icon-size=16  %s\n' "$P16"
P64=$(taille_icone 64); printf '  pixmap theme-icon-size=64  %s\n' "$P64"
if [ "$P16" = "$P64" ]; then
    printf "${RED}FAIL${NC}  la taille ne suit pas : l'icone de theme n'est PAS chargee\n"; echec=1
fi

printf '\n'
if [ "$echec" -gt 0 ]; then printf "${RED}ECHEC — la geometrie n'est pas conforme${NC}\n"; exit 1; fi
printf "${GREEN}SUCCES — border-width +%s x +%s pour N=40, et l'icone de theme suit sa taille${NC}\n" "$dl" "$dh"
