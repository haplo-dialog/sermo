#!/bin/sh
# run_fuzz.sh — Fuzzing du parser XML haplo-dialog (lexer flex + grammaire bison).
# GPL-2.0-or-later
#
# Le parser traite de l'XML non fiable → surface d'attaque. On fuzze en mode
# parse-only headless : <binaire> --file <f> --print-ir (ni widgets ni X).
#
# Usage :
#   ./run_fuzz.sh <port|chemin-binaire> [secondes]
#     ex. ./run_fuzz.sh gtk3sermo 60
#
# - Si afl++ est présent : campagne afl-fuzz (instrumentation recommandée :
#   recompiler le port avec CC=afl-gcc/CXX=afl-g++).
# - Sinon : fuzzer de repli Python (mutation interne), runnable partout/CI.
set -u
SCRIPT_DIR=$(cd "$(dirname "$0")" && pwd)
ROOT=$(cd "$SCRIPT_DIR/../.." && pwd)
ARG="${1:?usage: run_fuzz.sh <port|binaire> [secondes]}"
DUR="${2:-30}"

# Résoudre un nom de port en chemin de binaire si nécessaire.
if [ -x "$ARG" ]; then
    BIN="$ARG"
elif [ -x "$ROOT/$ARG/${ARG}_1.0.0/src/$ARG" ]; then
    BIN="$ROOT/$ARG/${ARG}_1.0.0/src/$ARG"
else
    echo "Binaire introuvable pour « $ARG » (compile d'abord le port, ou donne un chemin)." >&2
    exit 2
fi
CORPUS="$ROOT/tests/xml"

if command -v afl-fuzz >/dev/null 2>&1; then
    echo "afl++ détecté — campagne afl-fuzz (Ctrl-C pour arrêter)."
    echo "  Astuce : pour l'instrumentation, recompiler avec CC=afl-gcc CXX=afl-g++."
    exec afl-fuzz -i "$CORPUS" -o "$SCRIPT_DIR/findings" -- "$BIN" --file @@ --print-ir
else
    echo "afl absent — fuzzer de repli (mutation interne)."
    exec python3 "$SCRIPT_DIR/fuzz.py" "$BIN" "$DUR" "$CORPUS"
fi
