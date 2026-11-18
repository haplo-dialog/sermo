#!/bin/bash
# SPDX-License-Identifier: GPL-2.0-or-later
# tests/garde_render_sans_commande.sh — haplo-dialog — 2026 — GPL-2.0-or-later
#
# --render-png n'exécute aucune commande du dialogue.
#
# Rendre une image sert d'aperçu, souvent d'un XML qu'on n'a pas écrit (éditeur,
# CI, assistant). Jusqu'à la 3.3.0, --render-png exécutait pourtant les <input>
# du dialogue : seul le serveur sermoia-mcp s'en gardait, en posant lui-même
# SERMO_ALLOWED_CMDS. Signalé par un audit externe de la 3.2.0 : un
# <input>touch temoin</input> rendu en PNG créait le fichier.
#
# Mesure, sur le binaire donné :
#  1. trois commandes — lancée directement (<entry>), passant par le shell
#     (« ; »), et celle d'une <progressbar> — ne laissent AUCUN fichier ; l'image
#     est écrite quand même, et le refus est dit sur la sortie d'erreur ;
#  2. TÉMOIN : avec SERMO_RENDER_EXEC=1, les mêmes commandes s'exécutent. Sans
#     ce témoin, un port qui ne lit jamais ses <input> passerait le point 1.
#  3. gtk3 et gtk4 seulement : un <terminal> ne lance rien non plus. VTE lance
#     sa commande lui-même, sans passer par safe_system : jusqu'à la 3.3.0 le
#     terminal d'un dialogue rendu en PNG exécutait ses argvN (relecture
#     externe du 16/11). Même témoin avec SERMO_RENDER_EXEC=1.
#
# Usage : garde_render_sans_commande.sh <binaire d'un port à fenêtre>
# Codes : 0 = tient · 1 = défaut · 2 = outil ou binaire manquant
set -u
BIN="${1:?usage: garde_render_sans_commande.sh /chemin/vers/binaire}"
[[ -x "$BIN" ]] || { echo "binaire introuvable : $BIN" >&2; exit 2; }
BIN=$(readlink -f -- "$BIN")
command -v xvfb-run >/dev/null || { echo "outil manquant : xvfb-run" >&2; exit 2; }

ENV_PRE=()
case "$(basename -- "$BIN")" in
    qt6sermo)  ENV_PRE=(QT_QPA_PLATFORM=xcb) ;;
    gtk4sermo) ENV_PRE=(GSK_RENDERER=cairo GTK_A11Y=none) ;;
    efl1sermo) ENV_PRE=(ELM_ENGINE=buffer) ;;
    sdl3sermo) ENV_PRE=(SDL_VIDEODRIVER=offscreen) ;;
esac
unset SERMO_ALLOWED_CMDS HAPLO_ALLOWED_CMDS SERMO_RENDER_EXEC SERMO_NO_SHELL_FALLBACK

T=$(mktemp -d); trap 'rm -rf "$T"' EXIT
export GARDE_RENDU='<window><vbox>
<entry><variable>A</variable><input>touch direct</input></entry>
<entry><variable>B</variable><input>touch shell; true</input></entry>
<progressbar><variable>P</variable><input>sh -c "touch barre; echo 50"</input></progressbar>
<button ok></button></vbox></window>'
TERMINAL=0
case "$(basename -- "$BIN")" in gtk3sermo|gtk4sermo) TERMINAL=1 ;; esac
[ "$TERMINAL" = 1 ] && GARDE_RENDU=${GARDE_RENDU/<button ok>/<terminal argv0=\"/bin/sh\" argv1=\"-c\" argv2=\"touch terminal\"><variable>T</variable></terminal>
<button ok>}
attendre() {   # le shell du terminal est un autre processus : lui laisser 3 s
    for _ in $(seq 1 30); do [ -e "$1" ] && return 0; sleep 0.1; done; return 1
}
rendre() {   # rendre <dossier> [VAR=valeur]
    mkdir "$T/$1"
    ( cd "$T/$1" && env "${ENV_PRE[@]}" "${@:2}" xvfb-run -a timeout 40 "$BIN" --program=GARDE_RENDU --render-png="$T/$1/image.png" ) >"$T/$1/sortie" 2>"$T/$1/erreur"
}
echecs=0
ko() { echo "ECHEC : $*"; echecs=$((echecs + 1)); }

rendre sans
[ -s "$T/sans/image.png" ] || ko "--render-png n'a pas écrit l'image"
for f in direct shell barre; do
    [ -e "$T/sans/$f" ] && ko "--render-png a exécuté une commande du dialogue (fichier « $f » créé)"
done
grep -q "render-png n'execute aucune commande" "$T/sans/erreur" \
    || ko "le refus n'est pas dit sur la sortie d'erreur"
if [ "$TERMINAL" = 1 ]; then
    attendre "$T/sans/terminal" && ko "--render-png a lancé la commande du <terminal> (fichier « terminal » créé)"
    grep -q "<terminal> : commande refusee" "$T/sans/erreur" \
        || ko "le refus du <terminal> n'est pas dit sur la sortie d'erreur"
fi

rendre avec SERMO_RENDER_EXEC=1
[ -s "$T/avec/image.png" ] || ko "TÉMOIN : avec SERMO_RENDER_EXEC=1, pas d'image"
for f in direct shell barre; do
    [ -e "$T/avec/$f" ] || ko "TÉMOIN : avec SERMO_RENDER_EXEC=1, « $f » n'est pas créé — le point 1 ne mesure rien"
done

if [ "$TERMINAL" = 1 ]; then
    attendre "$T/avec/terminal" || ko "TÉMOIN : avec SERMO_RENDER_EXEC=1, le <terminal> ne lance rien — le point 3 ne mesure rien"
fi

[ "$echecs" -eq 0 ] || exit 1
formes="3 formes"; [ "$TERMINAL" = 1 ] && formes="3 formes et un <terminal>"
echo "OK : --render-png n'exécute aucune commande ($formes), l'image est écrite, le refus est dit ; avec SERMO_RENDER_EXEC=1 elles s'exécutent"
