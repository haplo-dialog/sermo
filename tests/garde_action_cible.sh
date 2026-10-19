#!/bin/bash
# SPDX-License-Identifier: GPL-2.0-or-later
# tests/garde_action_cible.sh — haplo-dialog — 2026 — GPL-2.0-or-later
#
# <action mqtt> et <action mavlink> (sermo_source.h, sermo_cible_commande) :
# une ÉCRITURE ne part que d'un CLIC. Sur le binaire donné, avec un FAUX programme d'écriture (il
# note les arguments reçus et la variable du dialogue ; aucun réseau) :
#  1. refusé À L'ANALYSE (code ≠ 0, message) : <action mqtt> ou <action
#     mavlink> dans un <timer>, une <entry>, une <checkbox> ; le nom
#     sermo-cible- écrit à la main dans un <input> ou une <action> ; action vide ;
#  2. TÉMOIN : le dialogue ouvert puis fermé SANS clic n'écrit rien ;
#  3. un CLIC sur le bouton (qui porte les deux actions) lance CHAQUE
#     programme d'écriture UNE fois, l'adresse encodée en UN argument, sans
#     repli /bin/sh, avec la variable du dialogue (valeur=$V) dans
#     l'environnement.
# Clic : xdotool sous écran virtuel (ports graphiques), touche Entrée dans un
# pseudo-terminal (ncurses). htmlsermo n'a pas de clic à simuler ici, et efl1sermo
# reste une fenêtre NOIRE sous Xvfb où même un bouton ordinaire ne réagit pas
# (témoin mesuré le 2026-10-17, <action>touch …</action> : rien) : pour ces deux,
# seuls les refus (1) sont joués, et le banc le dit. <action mqtt> y produit une
# action ordinaire, jouée par le même code commun (jouer_actions.h) que fltk1,
# sdl3, lvgl et ncurses, où le clic est mesuré.
#
# Usage : garde_action_cible.sh <binaire>
# Codes : 0 = tout tient · 1 = un défaut · 2 = outil ou binaire manquant

set -u
export LC_ALL=C
BIN="${1:?usage: garde_action_cible.sh /chemin/vers/binaire}"
[[ -x "$BIN" ]] || { echo "binaire introuvable : $BIN" >&2; exit 2; }
BIN=$(readlink -f -- "$BIN")
NOM=$(basename -- "$BIN")
unset DISPLAY WAYLAND_DISPLAY SERMO_ALLOWED_CMDS SERMO_NO_SHELL_FALLBACK

T=$(mktemp -d -p "$(dirname -- "$BIN")" banc-action-cible.XXXXXX) || exit 2
trap 'rm -rf -- "$T"' EXIT
case "$T" in *[!A-Za-z0-9._:/=,+@-]*) echo "chemin du bac inutilisable pour SERMO_SOURCE_DIR : $T" >&2; exit 2 ;; esac
mkdir "$T/aide"
# Le faux programme d'écriture : une ligne par appel.
printf '#!/bin/sh\nprintf "APPEL %%s|%%s|V=%%s\\n" "$#" "$1" "${V-ABSENTE}" >> "%s/appels"\n' "$T" >"$T/aide/sermo-cible-mqtt"
chmod 755 "$T/aide/sermo-cible-mqtt"
printf '#!/bin/sh\nprintf "MAVLINK %%s|%%s|V=%%s\\n" "$#" "$1" "${V-ABSENTE}" >> "%s/appels"\n' "$T" >"$T/aide/sermo-cible-mavlink"
chmod 755 "$T/aide/sermo-cible-mavlink"
ATTENDU='APPEL 1|mqtt://127.0.0.1:1883%3Fsujet=maison/salon/consigne%26valeur=%24V|V=21'
ATTENDU_M='MAVLINK 1|tcp://127.0.0.1:5760%3Fparametre=WP_SPD%26valeur=%24V|V=21'

echecs=0
ko() { echo "ECHEC : $*"; echecs=$((echecs + 1)); }
ok() { echo "ok     : $*"; }

ENV_PRE=(); XVFB=()
case "$NOM" in
    qt6sermo)     ENV_PRE=(QT_QPA_PLATFORM=offscreen) ;;
    sdl3sermo)    ENV_PRE=(SDL_VIDEODRIVER=offscreen) ;;
    efl1sermo)    ENV_PRE=(ELM_ENGINE=buffer) ;;
    ncursessermo) ENV_PRE=(SERMO_NCURSES_BATCH=1) ;;
    htmlsermo)    ENV_PRE=() ;;
    *)            ENV_PRE=(GSK_RENDERER=cairo GTK_A11Y=none) ;;
esac
# Les ports qui ouvrent l'écran avant de lire le XML (GTK, FLTK, LVGL…) : sous
# écran virtuel, sinon « cannot open display » arrête tout avant l'analyse.
case "$NOM" in ncursessermo|htmlsermo|qt6sermo|sdl3sermo|efl1sermo) ;; *) XVFB=(xvfb-run -a) ;; esac
# Pour le CLIC, une vraie fenêtre X sous l'écran virtuel (pas les modes sans
# écran ci-dessus) ; efl1 : moteur logiciel (voir garde_clic_widgets.sh).
CLIC_ENV=(GSK_RENDERER=cairo GTK_A11Y=none)
[ "$NOM" = efl1sermo ] && CLIC_ENV+=(ELM_ENGINE=software_x11)

# 1. Refus à l'analyse (sans écran : l'analyse précède tout affichage).
refus() {  # refus LIBELLÉ XML MOTIF
    GARDE_X="$2" env "${ENV_PRE[@]}" SERMO_SOURCE_DIR="$T/aide" "${XVFB[@]}" timeout 20 "$BIN" --program=GARDE_X \
        >/dev/null 2>"$T/err" </dev/null
    local rc=$?
    if [ $rc != 0 ] && [ $rc != 124 ] && grep -q -- "$3" "$T/err" && [ ! -e "$T/appels" ]; then
        ok "refusé à l'analyse : $1"
    else
        ko "$1 : code $rc, $(head -c 200 "$T/err")"
    fi
}
A='<action mqtt>mqtt://127.0.0.1?sujet=a/b&amp;valeur=on</action>'
refus "<action mqtt> dans un <timer>" "<window><vbox><timer interval=\"1\">$A</timer></vbox></window>" "only allowed in a <button>"
refus "<action mqtt> dans une <entry>" "<window><vbox><entry><variable>E</variable>$A</entry></vbox></window>" "only allowed in a <button>"
refus "<action mqtt> dans une <checkbox>" "<window><vbox><checkbox><label>c</label><variable>C</variable>$A</checkbox></vbox></window>" "only allowed in a <button>"
refus "sermo-cible- à la main dans <input>" "<window><vbox><entry><variable>E</variable><input>$T/aide/sermo-cible-mqtt x</input></entry></vbox></window>" "names a sermo-cible-"
refus "sermo-cible- à la main dans <action>" "<window><vbox><button><label>b</label><action>$T/aide/sermo-cible-mqtt x</action></button></vbox></window>" "names a sermo-cible-"
refus "<action mqtt> vide" "<window><vbox><button><label>b</label><action mqtt></action></button></vbox></window>" "action mqtt> element is empty"
M='<action mavlink>tcp://127.0.0.1:5760?ordre=rtl</action>'
refus "<action mavlink> dans un <timer>" "<window><vbox><timer interval=\"1\">$M</timer></vbox></window>" "only allowed in a <button>"
refus "<action mavlink> dans une <entry>" "<window><vbox><entry><variable>E</variable>$M</entry></vbox></window>" "only allowed in a <button>"
refus "<action mavlink> dans une <checkbox>" "<window><vbox><checkbox><label>c</label><variable>C</variable>$M</checkbox></vbox></window>" "only allowed in a <button>"
refus "<action mavlink> vide" "<window><vbox><button><label>b</label><action mavlink></action></button></vbox></window>" "action mavlink> element is empty"
refus "sermo-cible-mavlink à la main dans <action>" "<window><vbox><button><label>b</label><action>$T/aide/sermo-cible-mavlink x</action></button></vbox></window>" "names a sermo-cible-"

# Le dialogue de clic : le bouton EN PREMIER (le clic vise le coin haut-gauche).
XML='<window title="CLIC"><vbox>
<button><label>ECRIRE ECRIRE ECRIRE</label><action mqtt>mqtt://127.0.0.1:1883?sujet=maison/salon/consigne&amp;valeur=$V</action><action mavlink>tcp://127.0.0.1:5760?parametre=WP_SPD&amp;valeur=$V</action></button>
<entry><variable>V</variable><default>21</default></entry>
</vbox></window>'
printf '%s\n' "$XML" >"$T/clic.xml"

# clic_x CLIQUER(0|1) : ouvre le dialogue sous Xvfb, clique (ou pas), ferme.
clic_x() {
    env "${CLIC_ENV[@]}" SERMO_SOURCE_DIR="$T/aide" xvfb-run -a -s '-screen 0 800x600x24' timeout 60 sh -c '
        for j in $(seq 1 50); do xdpyinfo >/dev/null 2>&1 && break; sleep 0.1; done
        "$1" --file="$2" >"$3/sortie" 2>"$3/err" &
        pid=$!
        for i in $(seq 1 100); do
            WID=$(xdotool search --onlyvisible --name CLIC 2>/dev/null | head -1)
            [ -n "$WID" ] || WID=$(xdotool search --onlyvisible --class "$(basename "$1")" 2>/dev/null | head -1)
            [ -n "$WID" ] && break
            kill -0 $pid 2>/dev/null || break
            sleep 0.2
        done
        [ -z "$WID" ] && { kill $pid 2>/dev/null; echo NOWIN; exit 0; }
        H0=""; n=0
        for k in $(seq 1 30); do
            H1=$(xwininfo -id $WID 2>/dev/null | awk "/Height:/{print \$2}")
            if [ -n "$H1" ] && [ "$H1" = "$H0" ]; then n=$((n + 1)); else n=0; fi
            [ $n -ge 2 ] && break
            H0=$H1; sleep 0.3
        done
        if [ "$4" = 1 ]; then
            eval $(xdotool getwindowgeometry --shell $WID)
            xdotool mousemove $((X + 40)) $((Y + 22)) click 1
        fi
        sleep 1.5
        kill $pid 2>/dev/null; wait $pid 2>/dev/null; echo FAIT' _ "$BIN" "$T/clic.xml" "$T" "$1" 2>/dev/null
}
# clic_tty APPUYER(0|1) : ncurses dans un pseudo-terminal ; Entrée sur le
# premier widget (le bouton a le focus au départ).
clic_tty() {
    local touche=""; [ "$1" = 1 ] && touche=$'\r'
    ( sleep 1.5; printf '%s' "$touche"; sleep 1.5; printf '\033' ) | \
        SERMO_SOURCE_DIR="$T/aide" TERM=xterm timeout 20 script -qec "$BIN --file=$T/clic.xml" /dev/null >/dev/null 2>&1
    echo FAIT
}

case "$NOM" in
    htmlsermo)
        echo "note   : htmlsermo — pas de clic simulé ici (page web) ; seuls les refus à l'analyse sont joués" ;;
    efl1sermo)
        echo "note   : efl1sermo — fenêtre noire sous Xvfb, clic non mesurable (témoin ordinaire inerte aussi) ; seuls les refus sont joués" ;;
    *)
        if [ "$NOM" = ncursessermo ]; then jouer=clic_tty; else jouer=clic_x; fi
        for t in xvfb-run xdotool xwininfo xdpyinfo script; do
            command -v "$t" >/dev/null || { echo "outil manquant : $t" >&2; exit 2; }
        done
        # 2. Témoin : ouvert, fermé, sans clic.
        : >"$T/appels"
        r=$($jouer 0)
        [ "$r" = NOWIN ] && ko "témoin : aucune fenêtre (mesure impossible)"
        [ -s "$T/appels" ] && ko "sans clic, le programme d'écriture a été lancé : $(cat "$T/appels")" \
            || ok "sans clic : rien d'écrit"
        # 3. Clic.
        : >"$T/appels"
        r=$($jouer 1)
        if [ "$r" = NOWIN ]; then
            ko "clic : aucune fenêtre (mesure impossible)"
        else
            n=$(grep -c '^APPEL' "$T/appels")
            l=$(grep '^APPEL' "$T/appels" | head -1)
            if [ "$n" = 1 ] && [ "$l" = "$ATTENDU" ]; then
                ok "clic : écriture lancée une fois, adresse en un argument, valeur du dialogue (V=21)"
            else
                ko "clic : $n appel(s), reçu « $l », attendu « $ATTENDU »"
            fi
            n=$(grep -c '^MAVLINK' "$T/appels")
            l=$(grep '^MAVLINK' "$T/appels" | head -1)
            if [ "$n" = 1 ] && [ "$l" = "$ATTENDU_M" ]; then
                ok "clic : écriture drone lancée une fois, adresse en un argument (V=21)"
            else
                ko "clic (mavlink) : $n appel(s), reçu « $l », attendu « $ATTENDU_M »"
            fi
            grep -q 'syntaxe shell' "$T/err" 2>/dev/null && ko "le programme d'écriture est passé par /bin/sh"
        fi ;;
esac

[ "$echecs" = 0 ] && echo "OK : <action mqtt> et <action mavlink> sur $NOM"
[ "$echecs" = 0 ]
