#!/bin/bash
# SPDX-License-Identifier: GPL-2.0-or-later
# tests/garde_include.sh — --include charge vraiment son fichier.
#
# POURQUOI CETTE GARDE EXISTE
#
# --include=FICHIER fait précéder chaque commande de « . FICHIER; ». Sous /bin/sh
# (dash sur Debian), « . » cherche un nom sans « / » dans le PATH, jamais dans le
# dossier courant : jusqu'à la 2.6.8, --include=fonctions.sh ne chargeait RIEN, et
# une <input> qui appelait une fonction du fichier rendait du vide, sans un mot,
# sur les sept ports. Le cœur rend désormais le chemin absolu et le cite pour le
# shell.
#
# Trois cas : un nom relatif, un chemin absolu, un chemin avec une espace et une
# apostrophe. Ce dernier est passé en DEUXIÈME argument : un premier argument qui
# contient une espace est redécoupé, comportement voulu de gtkdialog pour les
# lignes « #! ».
#
# Témoins : une commande ordinaire doit remplir l'entrée (la garde sait lire une
# valeur), et sans --include la fonction ne doit rien rendre (la garde sait voir
# un fichier non chargé).
#
# Usage : garde_include.sh <chemin-du-binaire>
# Codes : 0 = chargé · 1 = pas chargé, ou témoin raté · 2 = usage/outil

set -u
BIN="${1:-}"
[[ -x "$BIN" ]] || { echo "usage: $0 <chemin-du-binaire>" >&2; exit 2; }
BIN="$(readlink -f "$BIN")"
for t in timeout xvfb-run; do
    command -v "$t" >/dev/null || { echo "outil manquant : $t" >&2; exit 2; }
done

TMP="$(mktemp -d)"; trap 'rm -rf "$TMP"' EXIT
mkdir -p "$TMP/dossier a b'c"
printf 'salut() { echo inclus-ok; }\n' > "$TMP/fonctions.sh"
cp "$TMP/fonctions.sh" "$TMP/dossier a b'c/f'n.sh"

dialogue() {   # dialogue <commande de l'input>
    printf '<window><vbox><entry><variable>E</variable><input>%s</input></entry><timer visible="false"><variable>T</variable><action>exit:fin</action></timer></vbox></window>' "$1"
}

# Lance le dialogue dans l'environnement sans écran du port ; affiche la valeur de E.
lancer() {   # lancer <commande de l'input> [arguments…]
    local entree="$1"; shift
    (
        cd "$TMP" || exit 2
        export MAIN_DIALOG="$(dialogue "$entree")"
        case "$(basename "$BIN")" in
            qt6sermo)     QT_QPA_PLATFORM=offscreen timeout 20 "$BIN" --program=MAIN_DIALOG "$@" ;;
            sdl3sermo)    SDL_VIDEODRIVER=offscreen timeout 20 "$BIN" --program=MAIN_DIALOG "$@" ;;
            efl1sermo)    ELM_ENGINE=buffer timeout 20 "$BIN" --program=MAIN_DIALOG "$@" ;;
            ncursessermo) SERMO_NCURSES_BATCH=1 timeout 20 "$BIN" --program=MAIN_DIALOG "$@" ;;
            *)            GSK_RENDERER=cairo timeout 20 xvfb-run -a "$BIN" --program=MAIN_DIALOG "$@" ;;
        esac </dev/null 2>/dev/null
    ) | sed -n 's/^E="\(.*\)"$/\1/p'
}

echecs=0
verifier() {   # verifier <libellé> <valeur attendue> <commande de l'input> [arguments…]
    local libelle="$1" attendu="$2" obtenu
    shift 2
    obtenu="$(lancer "$@")"
    if [[ "$obtenu" == "$attendu" ]]; then
        echo "  ✔ $libelle"
    else
        echo "  ✘ $libelle : E=\"$obtenu\", attendu \"$attendu\""
        echecs=$((echecs + 1))
    fi
}

echo "garde_include : $BIN"
verifier "témoin : une commande ordinaire remplit l'entrée" "direct" "echo direct"
verifier "témoin : sans --include, la fonction n'existe pas" "" "salut"
verifier "--include avec un nom relatif" "inclus-ok" "salut" --include=fonctions.sh
verifier "--include avec un chemin absolu" "inclus-ok" "salut" "--include=$TMP/fonctions.sh"
verifier "--include avec une espace et une apostrophe" "inclus-ok" "salut" "--include=dossier a b'c/f'n.sh"

if [[ $echecs -gt 0 ]]; then
    echo "ÉCHEC : $echecs cas sur 5."
    exit 1
fi
echo "garde_include : OK — le fichier inclus est chargé, quel que soit son chemin."
