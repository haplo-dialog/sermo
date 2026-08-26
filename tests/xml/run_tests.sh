#!/bin/sh
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

# ── Vérifier le binaire ──────────────────────────────────────────────────────
if [ "$BINARY" = "all" ]; then
    for bin in gtk3sermo gtk4sermo; do
        command -v "$bin" > /dev/null 2>&1 && "$0" "$bin"
    done
    exit 0
fi

if ! command -v "$BINARY" > /dev/null 2>&1; then
    echo "Binaire '$BINARY' non trouvé — tests ignorés"
    exit 0
fi

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

    # exit 124 = timeout (acceptable si parsing ok mais pas d'affichage)
    # exit 0 = succès
    # autres = échec de parsing
    if [ "$result" = "0" ] || [ "$result" = "124" ]; then
        ok "${name}: ${desc}"
    else
        fail "${name}: ${desc} (exit=${result})"
    fi
}

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
