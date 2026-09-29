#!/bin/bash
# SPDX-License-Identifier: GPL-2.0-or-later
# tests/garde_input_sans_fin.sh — haplo-dialog — 2026 — GPL-2.0-or-later
#
# 2.7.3 : un <input> branché sur une source sans fin ne doit plus faire tomber
# le dialogue. Mesuré sur les paquets 2.7.2 : gtk3sermo passait 700 Mo en
# 7 secondes sur `yes` (2026-09-17) ; ncursessermo finissait sur une erreur de
# segmentation ; un <edit> de gtk3/gtk4 sur un fichier creux de 8 Go
# s'arrêtait net (GLib-ERROR, « failed to allocate ») ; une barre de
# progression gtk3/gtk4 sur `yes 50` prenait 70 Mo par seconde, et tombait
# (erreur de segmentation) quand SERMO_ALLOWED_CMDS refusait sa commande
# (2026-09-24).
#
# Sur le binaire donné, ce garde vérifie :
#  1. `yes` et /dev/zero s'arrêtent à SERMO_INPUT_MAX : le dialogue s'ouvre et
#     se ferme (code 0), la valeur lue fait la limite pile, un avertissement
#     par source sur la SORTIE D'ERREUR, aucun sur la sortie (qu'un eval lit) ;
#  2. TÉMOIN : la même source, finie et plus longue que la limite, se lit en
#     entier quand SERMO_INPUT_MAX=0. Sans lui, une valeur courte pourrait
#     venir d'ailleurs que de la limite ;
#  3. la barre de progression SUIT une commande sans fin sans grossir : son pic
#     mémoire (VmHWM) avec `yes 50` reste à moins de 60 Mo de celui avec
#     `echo 50; sleep 5` (qui dure, pour être relevé). Elle n'est pas plafonnée : elle ne garde qu'une ligne à la
#     fois, et doit pouvoir suivre une longue copie jusqu'au bout.
#     ncurses sans écran lit chaque barre jusqu'au bout AVANT les minuteries
#     (render_ncurses.c, run_headless) : une barre sans fin y bloque par
#     construction. On l'y arrête après 4 s, et on ne juge que sa mémoire ;
#  4. une barre dont la commande est refusée par SERMO_ALLOWED_CMDS laisse le
#     dialogue vivre et se fermer.
#
# L'environnement sans écran de chaque toolkit est celui du banc de
# comportement (tests/comportement/run.sh).
#
# Usage : garde_input_sans_fin.sh <binaire>
# Codes : 0 = tout tient · 1 = un défaut · 2 = outil ou binaire manquant

set -u
export LC_ALL=C   # ${#v} compte des octets
BIN="${1:?usage: garde_input_sans_fin.sh /chemin/vers/binaire}"
[[ -x "$BIN" ]] || { echo "binaire introuvable : $BIN" >&2; exit 2; }
BIN=$(readlink -f -- "$BIN")
unset DISPLAY WAYLAND_DISPLAY

ENV_PRE=(); XVFB=()
case "$(basename -- "$BIN")" in
    qt6sermo)     ENV_PRE=(QT_QPA_PLATFORM=offscreen) ;;
    sdl3sermo)    ENV_PRE=(SDL_VIDEODRIVER=offscreen) ;;
    efl1sermo)    ENV_PRE=(ELM_ENGINE=buffer) ;;
    ncursessermo) ENV_PRE=(SERMO_NCURSES_BATCH=1) ;;
    *)            ENV_PRE=(GSK_RENDERER=cairo GTK_A11Y=none); XVFB=(xvfb-run -a) ;;
esac
for t in timeout pgrep readlink ${XVFB[0]:-}; do
    command -v "$t" >/dev/null || { echo "outil manquant : $t" >&2; exit 2; }
done

T=$(mktemp -d); trap 'rm -rf "$T"' EXIT
MARQUE="GARDE_SANS_FIN_$$"
echecs=0
ko() { echo "ECHEC : $*"; echecs=$((echecs + 1)); }

# lancer <xml> [VAR=valeur…] : joue le dialogue, arrêté après $DELAI s.
# Remplit $T/out et $T/err, et pose RC (code de sortie ; 124 = arrêté) et PIC
# (pic mémoire du binaire, VmHWM en Kio).
DELAI=40
lancer() {
    local xml=$1 lanceur pid="" v
    shift
    export "$MARQUE=$xml"
    ( cd "$T" && exec env "${ENV_PRE[@]}" "$@" "${XVFB[@]}" timeout "$DELAI" "$BIN" --program="$MARQUE" ) \
        >"$T/out" 2>"$T/err" </dev/null &
    lanceur=$!
    PIC=0
    while kill -0 "$lanceur" 2>/dev/null; do
        [ -n "$pid" ] || pid=$(pgrep -n -f -- "^$BIN --program=$MARQUE\$")
        if [ -n "$pid" ]; then
            v=$(awk '/^VmHWM:/ { print $2 }' "/proc/$pid/status" 2>/dev/null)
            [ -n "$v" ] && [ "$v" -gt "$PIC" ] && PIC=$v
        fi
        sleep 0.1
    done
    wait "$lanceur"; RC=$?
}
valeur() { sed -n "s/^$1=\"\\(.*\\)\"\$/\\1/p" "$T/out" | head -1; }

FERMER='<timer interval="1"><variable>T</variable><action>exit:fin</action></timer>'

# ── 1. Deux sources sans fin, limite 20000 octets ──────────────────────────
LIM=20000
lancer "<window><vbox>
<edit><variable>OUI</variable><input>yes | tr -d '\\n'</input></edit>
<edit><variable>ZERO</variable><input file>/dev/zero</input></edit>
$FERMER</vbox></window>" SERMO_INPUT_MAX=$LIM
oui=$(valeur OUI)
if [ "$RC" -ne 0 ]; then
    ko "sources sans fin : le dialogue ne se ferme pas proprement (code $RC ; 124 = figé, 139 = segmentation)"
else
    [ "${#oui}" -eq "$LIM" ] || ko "sources sans fin : OUI fait ${#oui} octets, $LIM attendus"
    grep -q '^ZERO=""$' "$T/out" || ko "sources sans fin : ZERO absent ou non vide"
    grep -q "« yes .*lecture arrêtée à $LIM octets" "$T/err" || ko "sources sans fin : pas d'avertissement pour \`yes\` sur la sortie d'erreur"
    grep -q "« /dev/zero » : lecture arrêtée à $LIM octets" "$T/err" || ko "sources sans fin : pas d'avertissement pour /dev/zero sur la sortie d'erreur"
    grep -q 'lecture arrêtée' "$T/out" && ko "sources sans fin : l'avertissement est parti sur la SORTIE"
fi

# ── 2. Témoin : la même source finie, sans limite, se lit en entier ────────
lancer "<window><vbox>
<edit><variable>OUI</variable><input>yes | tr -d '\\n' | head -c 25000</input></edit>
$FERMER</vbox></window>" SERMO_INPUT_MAX=0
oui=$(valeur OUI)
[ "$RC" -eq 0 ] && [ "${#oui}" -eq 25000 ] \
    || ko "TÉMOIN : sans limite, 25000 octets attendus, ${#oui} lus (code $RC) — le cas 1 ne mesure rien"
grep -q 'lecture arrêtée' "$T/err" && ko "TÉMOIN : un avertissement alors que SERMO_INPUT_MAX=0"

# ── 3. La barre de progression suit une commande sans fin sans grossir ──────
barre() { printf '<window><vbox><progressbar><variable>PB</variable><input>%s</input></progressbar><timer interval="3"><variable>T</variable><action>exit:fin</action></timer></vbox></window>' "$1"; }
lancer "$(barre 'echo 50; sleep 5')"
rc_fin=$RC; pic_fin=$PIC
rc_attendu=0
if [ "$(basename -- "$BIN")" = ncursessermo ]; then
    DELAI=4; rc_attendu=124   # bloque par construction, voir l'en-tête
fi
lancer "$(barre 'yes 50')"
rc_sans_fin=$RC; pic_sans_fin=$PIC
DELAI=40
if [ "$rc_fin" -ne 0 ] || [ "$rc_sans_fin" -ne "$rc_attendu" ]; then
    ko "barre : le dialogue ne se ferme pas comme attendu (echo 50 : code $rc_fin ; yes 50 : code $rc_sans_fin, $rc_attendu attendu)"
elif [ "$pic_fin" -le 0 ] || [ "$pic_sans_fin" -le 0 ]; then
    ko "barre : pic mémoire illisible ($pic_fin / $pic_sans_fin Kio) — le processus n'a pas été vu"
elif [ $((pic_sans_fin - pic_fin)) -ge 61440 ]; then
    ko "barre : \`yes 50\` fait grossir le processus de $(( (pic_sans_fin - pic_fin) / 1024 )) Mo en 3 s (echo 50 : $((pic_fin / 1024)) Mo)"
fi
grep -q 'lecture arrêtée' "$T/err" && ko "barre : la barre a été plafonnée — elle doit suivre sa commande jusqu'au bout"

# ── 4. Une barre dont la commande est refusée ───────────────────────────────
lancer "$(barre 'yes 50')" SERMO_ALLOWED_CMDS=true
[ "$RC" -eq 0 ] || ko "barre refusée : le dialogue tombe (code $RC ; 139 = segmentation)"
grep -q 'SERMO_ALLOWED_CMDS' "$T/err" || ko "barre refusée : le refus n'est pas dit sur la sortie d'erreur"

[ "$echecs" -eq 0 ] || exit 1
echo "OK : \`yes\` et /dev/zero s'arrêtent à $LIM octets et le disent sur la sortie d'erreur (témoin sans limite : 25000) ; barre sur \`yes 50\` : $((pic_sans_fin / 1024)) Mo contre $((pic_fin / 1024)) Mo sur \`echo 50\` ; barre refusée : le dialogue vit"
