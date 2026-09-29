#!/bin/sh
# SPDX-License-Identifier: GPL-2.0-or-later
# run_tests.sh — Suite de régression XML haplo-dialog
# sermo 1.1.0 — GPL-2.0-or-later
#
# Usage :
#   ./run_tests.sh gtk3sermo          # tester avec gtk3sermo
#   ./run_tests.sh all            # tester avec tous les binaires disponibles
#   TIMEOUT=5 ./run_tests.sh gtk3sermo
#
# Principe : chaque fichier XML est analysé avec --print-ir, qui parse le XML,
# imprime la représentation interne, puis sort (exit 0) SANS construire de
# widgets ni ouvrir de fenêtre. Si le binaire s'arrête proprement, le test
# passe. Ce mode ne nécessite PAS d'affichage graphique (CI headless OK).

set -e

SCRIPT_DIR="$(cd "$(dirname "$0")" && pwd)"
BINARY="${1:-gtk3sermo}"
TIMEOUT="${TIMEOUT:-3}"
PASS=0
FAIL=0
SKIP=0

# ── Couleurs ────────────────────────────────────────────────────────────────
GREEN='\033[0;32m'; RED='\033[0;31m'; YELLOW='\033[1;33m'
BLUE='\033[0;34m'; NC='\033[0m'

ok()   { printf "${GREEN}PASS${NC}  %s\n" "$*"; PASS=$((PASS+1)); }
fail() { printf "${RED}FAIL${NC}  %s\n" "$*"; FAIL=$((FAIL+1)); }
skip() { printf "${YELLOW}SKIP${NC}  %s\n" "$*"; SKIP=$((SKIP+1)); }
info() { printf "${BLUE}────${NC}  %s\n" "$*"; }

# ── Trouver le binaire ───────────────────────────────────────────────────────
# Un CHEMIN est pris tel quel. Un NOM (gtk3sermo…) n'est cherché QUE dans l'arbre
# construit (sermo-backend-<port>/_build/), JAMAIS dans le PATH : sur un poste où
# la 1.x est installée, c'est elle qui aurait été testée à la place du binaire
# qu'on vient de construire — et le banc aurait dit vert sur le mauvais programme.
# Pour tester un binaire installé, donner son chemin (ex. /usr/bin/gtk3sermo).
RACINE="$(cd "$SCRIPT_DIR/../.." && pwd)"
PORTS="gtk3 gtk4 qt6 fltk1 efl1 sdl3 ncurses"

trouver_binaire() {
    case "$1" in */*) [ -x "$1" ] && { echo "$1"; return 0; }; return 1 ;; esac
    port=${1%sermo}
    c="$RACINE/sermo-backend-$port/_build/$1"
    [ -x "$c" ] && { echo "$c"; return 0; }
    return 1
}

if [ "$BINARY" = "all" ]; then
    # Les SEPT ports. Un port absent est un échec, sauf SERMO_PORTS_OPTIONNELS=1 :
    # « tous les ports disponibles » avait laissé passer une suite qui n'en testait
    # que deux.
    absents=0
    for port in $PORTS; do
        if chemin=$(trouver_binaire "${port}sermo"); then
            "$0" "$chemin" || exit $?
        else
            echo "Binaire absent : sermo-backend-$port/_build/${port}sermo" >&2
            absents=$((absents+1))
        fi
    done
    if [ "$absents" -gt 0 ] && [ "${SERMO_PORTS_OPTIONNELS:-0}" != 1 ]; then
        echo "ÉCHEC : $absents port(s) non construit(s) — AUCUN test n'a tourné pour eux." >&2
        echo "(SERMO_PORTS_OPTIONNELS=1 pour accepter un arbre partiel, en connaissance de cause.)" >&2
        exit 1
    fi
    exit 0
fi

if ! CHEMIN=$(trouver_binaire "$BINARY"); then
    echo "Binaire '$BINARY' introuvable dans l'arbre construit (sermo-backend-<port>/_build/)." >&2
    echo "Donner un chemin pour tester un autre binaire. AUCUN test n'a tourné." >&2
    exit 2
fi
BINARY="$CHEMIN"

BIN_VERSION=$("$BINARY" --version 2>/dev/null | head -1 || echo "inconnu")
info "Suite XML haplo-dialog — ${BINARY} (${BIN_VERSION})"
info "$(ls "$SCRIPT_DIR"/*.xml 2>/dev/null | wc -l) cas de test"
printf "\n"

# ── Fonction de test ─────────────────────────────────────────────────────────
run_test() {
    xml_file="$1"
    desc="$2"
    expected_exit="${3:-0}"
    name="$(basename $xml_file .xml)"

    # Export la variable DIALOG avec le contenu du XML
    DIALOG=$(cat "$xml_file")
    export DIALOG

    # Exécuter avec timeout, sans affichage graphique (DISPLAY=)
    result=0
    if command -v timeout > /dev/null 2>&1; then
        DISPLAY="" timeout "$TIMEOUT" "$BINARY" --program DIALOG --print-ir \
            > /dev/null 2>&1 || result=$?
    else
        DISPLAY="" "$BINARY" --program DIALOG --print-ir \
            > /dev/null 2>&1 || result=$?
    fi

    # exit 0 = l'analyse a abouti.
    # exit 124 = dépassement de temps : --print-ir analyse puis SORT, il ne doit
    #            jamais bloquer. Un blocage est un défaut, pas une réussite — la
    #            version précédente le comptait vert.
    # autres = échec de l'analyse.
    if [ "$result" = "0" ]; then
        ok "${name}: ${desc}"
    elif [ "$result" = "124" ]; then
        fail "${name}: ${desc} (dépassement de temps : ${TIMEOUT} s)"
    else
        fail "${name}: ${desc} (exit=${result})"
    fi
}

# ── Témoin : le banc sait-il voir un XML FAUX ? ──────────────────────────────
# Si un document mal formé passait, tous les PASS ci-dessous ne prouveraient rien.
DIALOG='<window><vbox><button ok></vbox></window>'
export DIALOG
temoin=0
DISPLAY="" timeout "$TIMEOUT" "$BINARY" --program DIALOG --print-ir > /dev/null 2>&1 || temoin=$?
if [ "$temoin" = "0" ] || [ "$temoin" = "124" ]; then
    printf "${RED}ÉCHEC DU TÉMOIN${NC} — un XML mal formé n'est pas refusé (exit=%s) : le banc ne prouverait rien.\n" "$temoin"
    exit 1
fi
info "Témoin : un XML mal formé est bien refusé (exit=${temoin})"

# ── Cas de test ───────────────────────────────────────────────────────────────
for xml in "$SCRIPT_DIR"/*.xml; do
    [ -f "$xml" ] || continue
    name="$(basename $xml .xml)"
    desc=$(grep "^<!-- DESC:" "$xml" 2>/dev/null | sed 's/<!-- DESC: //;s/ -->//' || echo "$name")
    run_test "$xml" "$desc"
done

# ── Résumé ────────────────────────────────────────────────────────────────────
printf "\n"
info "Résultats pour ${BINARY} : ${PASS} PASS | ${FAIL} FAIL | ${SKIP} SKIP"

if [ "$FAIL" -gt 0 ]; then
    printf "${RED}ÉCHEC — ${FAIL} test(s) en erreur${NC}\n"
    exit 1
else
    printf "${GREEN}SUCCÈS — tous les tests passent${NC}\n"
    exit 0
fi
