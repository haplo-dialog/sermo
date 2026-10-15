#!/bin/bash
# SPDX-License-Identifier: GPL-2.0-or-later
# tests/garde_input_source.sh — haplo-dialog — 2026 — GPL-2.0-or-later
#
# <input modbus> et <input opcua> (libsermocore/include/sermo_source.h) : le
# cœur traduit la balise en une commande qui lance un programme d'aide de
# SERMO_SOURCE_DIR, l'adresse encodée en UN argument sans métacaractère.
#
# Sur le binaire donné, avec de FAUX programmes d'aide (ils rendent l'argument
# reçu ; aucun réseau, aucun automate), ce garde vérifie :
#  1. la valeur arrive dans le widget, l'adresse encodée telle que prévu, en un
#     seul argument, et SANS repli /bin/sh (aucun « syntaxe shell » journalisé) ;
#  2. elle arrive encore avec SERMO_NO_SHELL_FALLBACK=1 (preuve qu'il n'y a pas
#     de shell), et avec SERMO_ALLOWED_CMDS qui nomme les deux programmes ;
#  3. SERMO_ALLOWED_CMDS qui ne les nomme pas : rien n'est lancé, le refus est
#     dit sur la sortie d'erreur, le dialogue se ferme quand même ;
#  4. TÉMOIN : sans programme d'aide dans le dossier, aucune valeur — sans quoi
#     le point 1 pourrait venir d'ailleurs ;
#  5. une adresse vide arrête l'analyse, avec un message.
#
# Usage : garde_input_source.sh <binaire>
# Codes : 0 = tout tient · 1 = un défaut · 2 = outil ou binaire manquant

set -u
export LC_ALL=C
BIN="${1:?usage: garde_input_source.sh /chemin/vers/binaire}"
[[ -x "$BIN" ]] || { echo "binaire introuvable : $BIN" >&2; exit 2; }
BIN=$(readlink -f -- "$BIN")
unset DISPLAY WAYLAND_DISPLAY SERMO_ALLOWED_CMDS SERMO_NO_SHELL_FALLBACK

ENV_PRE=(); XVFB=()
case "$(basename -- "$BIN")" in
    qt6sermo)     ENV_PRE=(QT_QPA_PLATFORM=offscreen) ;;
    sdl3sermo)    ENV_PRE=(SDL_VIDEODRIVER=offscreen) ;;
    efl1sermo)    ENV_PRE=(ELM_ENGINE=buffer) ;;
    ncursessermo) ENV_PRE=(SERMO_NCURSES_BATCH=1) ;;
    htmlsermo)    ENV_PRE=() ;;
    *)            ENV_PRE=(GSK_RENDERER=cairo GTK_A11Y=none); XVFB=(xvfb-run -a) ;;
esac
for t in timeout readlink ${XVFB[0]:-}; do
    command -v "$t" >/dev/null || { echo "outil manquant : $t" >&2; exit 2; }
done

# Le bac vit à côté du binaire (dossier de construction), pas sous /tmp : un
# exécutable posé dans /tmp fait parler les surveillances d'exécution. Son
# chemin ne doit porter que des caractères admis par SERMO_SOURCE_DIR.
T=$(mktemp -d -p "$(dirname -- "$BIN")" banc-input-source.XXXXXX) || exit 2
trap 'rm -rf -- "$T"' EXIT
case "$T" in *[!A-Za-z0-9._:/=,+@-]*) echo "chemin du bac inutilisable pour SERMO_SOURCE_DIR : $T" >&2; exit 2 ;; esac
mkdir "$T/aide" "$T/vide"
for p in modbus opcua; do
    printf '#!/bin/sh\nprintf "TEMOIN-%s:%%s:%%s\\n" "$#" "$1"\n' "$p" > "$T/aide/sermo-source-$p"
    chmod 755 "$T/aide/sermo-source-$p"
done

XML='<window title="source"><vbox>
<entry><variable>M</variable><input modbus>tcp://127.0.0.1:5020?esclave=1&registre=1</input></entry>
<entry><variable>O</variable><input opcua>opc.tcp://h:4840?noeud=ns=1;s=a b</input></entry>
<timer interval="1"><variable>T</variable><action>exit:OK</action></timer>
</vbox></window>'
M_ATTENDU='TEMOIN-modbus:1:tcp://127.0.0.1:5020%3Fesclave=1%26registre=1'
O_ATTENDU='TEMOIN-opcua:1:opc.tcp://h:4840%3Fnoeud=ns=1%3Bs=a%20b'

echecs=0
ko() { echo "ECHEC : $*"; echecs=$((echecs + 1)); }

# jouer <xml> [VAR=valeur…] : remplit $T/out et $T/err, pose RC.
jouer() {
    local xml=$1
    shift
    ( cd "$T" && exec env GARDE_SOURCE_XML="$xml" "${ENV_PRE[@]}" "$@" "${XVFB[@]}" \
          timeout 30 "$BIN" --program=GARDE_SOURCE_XML ) >"$T/out" 2>"$T/err" </dev/null
    RC=$?
}
valeur() { sed -n "s/^$1=\"\\(.*\\)\"\$/\\1/p" "$T/out" | head -n 1; }

# 1. Sans contrainte.
jouer "$XML" SERMO_SOURCE_DIR="$T/aide"
[ "$(valeur M)" = "$M_ATTENDU" ] || ko "modbus : M=« $(valeur M) », attendu « $M_ATTENDU » (code $RC)"
[ "$(valeur O)" = "$O_ATTENDU" ] || ko "opcua : O=« $(valeur O) », attendu « $O_ATTENDU » (code $RC)"
grep -q 'syntaxe shell' "$T/err" && ko "le programme d'aide est passé par /bin/sh"

# 2. Sans repli shell possible, puis sous liste blanche qui nomme les programmes.
jouer "$XML" SERMO_SOURCE_DIR="$T/aide" SERMO_NO_SHELL_FALLBACK=1
[ "$(valeur M)" = "$M_ATTENDU" ] || ko "SERMO_NO_SHELL_FALLBACK=1 : M=« $(valeur M) »"
jouer "$XML" SERMO_SOURCE_DIR="$T/aide" SERMO_ALLOWED_CMDS="$T/aide/sermo-source-modbus,$T/aide/sermo-source-opcua"
[ "$(valeur M)" = "$M_ATTENDU" ] || ko "liste blanche qui les nomme : M=« $(valeur M) »"
[ "$(valeur O)" = "$O_ATTENDU" ] || ko "liste blanche qui les nomme : O=« $(valeur O) »"

# 3. Liste blanche qui ne les nomme pas.
jouer "$XML" SERMO_SOURCE_DIR="$T/aide" SERMO_ALLOWED_CMDS=ls
case "$(valeur M)$(valeur O)" in *TEMOIN*) ko "liste blanche sans eux : un programme d'aide a tourné" ;; esac
grep -q 'SERMO_ALLOWED_CMDS' "$T/err" || ko "liste blanche sans eux : refus non dit sur la sortie d'erreur"
[ "$RC" = 124 ] && ko "liste blanche sans eux : le dialogue ne s'est pas fermé"

# 4. Témoin : dossier sans programme d'aide.
jouer "$XML" SERMO_SOURCE_DIR="$T/vide"
case "$(valeur M)$(valeur O)" in *TEMOIN*) ko "témoin : une valeur est venue sans programme d'aide" ;; esac

# 5. Adresse vide.
jouer '<window><vbox><entry><input modbus></input></entry></vbox></window>' SERMO_SOURCE_DIR="$T/aide"
{ [ "$RC" != 0 ] && grep -q 'input modbus' "$T/err"; } || ko "adresse vide : code $RC, message absent"

[ "$echecs" = 0 ] && echo "OK : <input modbus>/<input opcua> sur $(basename -- "$BIN")"
[ "$echecs" = 0 ]
