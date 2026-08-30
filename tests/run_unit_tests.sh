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
GREEN='\033[0;32m'; RED='\033[0;31m'; YELLOW='\033[1;33m'; NC='\033[0m'

# ─────────────────────────────────────────────────────────────────────────
# ⛔ CE BANC NE REND PLUS « OK » SANS AVOIR RIEN TESTÉ.
#
# Le chemin des sources était codé en dur sur « _1.1.0 ». Les dossiers ont été
# renommés en _1.1.2 puis _1.1.3 (une montée de version amont les renomme, voir
# VERSIONING.md) : le banc ne trouvait plus safe_exec.c, imprimait SKIP, et
# rendait rc=0. Il était annoncé dans le badge des deux README.
#
# Constaté le 2026-08-30, après des semaines de « comportement 9/9 » affiché
# alors que ZÉRO test tournait. Deux règles en sortent :
#   1. le chemin versionné se RÉSOUT, il ne s'écrit pas ;
#   2. un SKIP total est une ERREUR, pas un succès.
# ─────────────────────────────────────────────────────────────────────────

# Résout le répertoire src d'un port, quelle que soit sa version.
resoudre_src() {
    _p="$1"
    for _d in "$ROOT/$_p/${_p}_"*/src; do
        [ -f "$_d/safe_exec.c" ] && { printf '%s' "$_d"; return 0; }
    done
    return 1
}

run_port() {
    port="$1"
    if ! src=$(resoudre_src "$port"); then
        printf "${YELLOW}SKIP${NC}  %s : aucun %s_*/src/safe_exec.c sous %s\n" "$port" "$port" "$ROOT"
        return 2      # 2 = rien testé, distinct de 1 = test en échec
    fi
    printf "  source : %s\n" "$src"
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
joues=0; fail=0

if [ "$PORT" = all ]; then
    for p in gtk3sermo gtk4sermo; do
        run_port "$p"; rc=$?
        case "$rc" in
            0) joues=$((joues+1)) ;;
            2) : ;;                       # rien teste pour ce port
            *) joues=$((joues+1)); fail=1 ;;
        esac
    done
else
    run_port "$PORT"; rc=$?
    case "$rc" in
        0) joues=1 ;;
        2) : ;;
        *) joues=1; fail=1 ;;
    esac
fi

# ⛔ Le garde-fou qui manquait : zero port teste n'est PAS un succes.
if [ "$joues" -eq 0 ]; then
    printf "${RED}ERREUR : aucun port n'a ete teste.${NC}\n" >&2
    printf "  Construis d'abord un port, ou verifie le nom du dossier versionne.\n" >&2
    exit 2
fi
if [ "$fail" -ne 0 ]; then
    printf "${RED}=== ECHEC : au moins un port en erreur ===${NC}\n"; exit 1
fi
printf "${GREEN}=== %s port(s) teste(s) : comportement du coeur OK ===${NC}\n" "$joues"
