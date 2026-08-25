#!/bin/sh
# run_unit_tests.sh — Tests de COMPORTEMENT du cœur (unitaires), headless.
# haplo-dialog — GPL-2.0-or-later
#
# Complète tests/xml/run_tests.sh (qui ne teste que le parse) : ici on
# compile et exécute réellement la logique cœur (sécurité safe_exec) contre
# le source de chaque port. « main() pur » : pas de libcheck, pas de X.
#
# Usage :
#   ./run_unit_tests.sh <port>        # un port (ex. gtk3sermo)
#   ./run_unit_tests.sh all           # tous les binaires disponibles
#
# Prérequis : gcc, pkg-config, glib-2.0 dev.

set -u
SCRIPT_DIR=$(cd "$(dirname "$0")" && pwd)
ROOT=$(cd "$SCRIPT_DIR/.." && pwd)
GREEN='\033[0;32m'; RED='\033[0;31m'; NC='\033[0m'

run_port() {
    port="$1"
    src="$ROOT/$port/${port}_1.0.0/src"
    if [ ! -f "$src/safe_exec.c" ]; then
        printf "SKIP  %s (pas de safe_exec.c)\n" "$port"; return 0
    fi
    bin=$(mktemp /tmp/ut_${port}.XXXXXX)
    if gcc -D_GNU_SOURCE $(pkg-config --cflags glib-2.0) -I"$src" \
           "$SCRIPT_DIR/unit/test_safe_exec.c" "$src/safe_exec.c" \
           $(pkg-config --libs glib-2.0) -o "$bin" 2>/tmp/ut_cc.log; then
        printf "──────── %s ────────\n" "$port"
        "$bin" 2>/dev/null; rc=$?
        rm -f "$bin"
        [ "$rc" -eq 0 ] && printf "${GREEN}OK${NC} %s\n\n" "$port" || printf "${RED}ÉCHEC${NC} %s\n\n" "$port"
        return $rc
    else
        printf "${RED}COMPILE KO${NC} %s\n" "$port"; cat /tmp/ut_cc.log; rm -f "$bin"; return 1
    fi
}

PORT="${1:-all}"
if [ "$PORT" = all ]; then
    fail=0
    for p in gtk3sermo gtk4sermo; do
        run_port "$p" || fail=1
    done
    [ "$fail" -eq 0 ] && printf "${GREEN}=== Tous les ports : tests de comportement OK ===${NC}\n"
    exit $fail
else
    run_port "$PORT"
fi
