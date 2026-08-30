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

# ─────────────────────────────────────────────────────────────────────────
# ⛔ DEUX PIEGES CORRIGES ICI, tous deux SILENCIEUX.
#
# 1. « [ -x "$ARG" ] » est VRAI pour un REPERTOIRE : les repertoires portent le
#    bit x. La commande publiee dans le README, « ./run_fuzz.sh gtk3sermo 60 »,
#    lancee depuis la racine, prenait donc le REPERTOIRE gtk3sermo/ pour un
#    binaire — et le fuzzer annonçait « 0 crash » sans avoir rien fuzze.
#    On exige desormais un FICHIER regulier executable.
#
# 2. Le repli codait « _1.1.0 » en dur. Une montee de version amont renomme le
#    dossier (voir VERSIONING.md) : le repli ne trouvait plus rien. Le chemin
#    se RESOUT.
#
# Constate le 2026-08-30. Meme famille que le defaut de run_unit_tests.sh.
# ─────────────────────────────────────────────────────────────────────────
BIN=""
if [ -f "$ARG" ] && [ -x "$ARG" ]; then
    BIN="$ARG"
else
    if [ -d "$ARG" ]; then
        echo "« $ARG » est un REPERTOIRE, pas un binaire — on ne fuzze pas un dossier." >&2
    fi
    for _c in "$ROOT/$ARG/${ARG}_"*/src/"$ARG"; do
        [ -f "$_c" ] && [ -x "$_c" ] && { BIN="$_c"; break; }
    done
fi
if [ -z "$BIN" ]; then
    echo "Binaire introuvable pour « $ARG »." >&2
    echo "  Donne un CHEMIN vers un fichier executable, ou construis d'abord le port." >&2
    # N'afficher le motif de recherche que si l'argument est un NOM de port :
    # avec un chemin, « $ROOT/$ARG/... » produit une chaine absurde.
    case "$ARG" in
        */*) : ;;
        *)   echo "  Cherche : $ROOT/$ARG/${ARG}_*/src/$ARG" >&2 ;;
    esac
    exit 2
fi
echo "  binaire fuzze : $BIN"

CORPUS="$ROOT/tests/xml"
_n=$(find "$CORPUS" -maxdepth 1 -name '*.xml' 2>/dev/null | wc -l)
if [ "$_n" -eq 0 ]; then
    echo "Corpus vide : aucun .xml dans $CORPUS — le fuzzer n'aurait rien a muter." >&2
    exit 2
fi
echo "  corpus        : $_n fichier(s)"

if command -v afl-fuzz >/dev/null 2>&1; then
    echo "afl++ détecté — campagne afl-fuzz (Ctrl-C pour arrêter)."
    echo "  Astuce : pour l'instrumentation, recompiler avec CC=afl-gcc CXX=afl-g++."
    exec afl-fuzz -i "$CORPUS" -o "$SCRIPT_DIR/findings" -- "$BIN" --file @@ --print-ir
else
    echo "afl absent — fuzzer de repli (mutation interne)."
    exec python3 "$SCRIPT_DIR/fuzz.py" "$BIN" "$DUR" "$CORPUS"
fi
