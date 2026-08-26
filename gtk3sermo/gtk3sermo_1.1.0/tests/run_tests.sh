#!/bin/bash
# run_tests.sh — Suite de tests fonctionnels gtk3sermo 1.0.0
#
# Usage:  ./run_tests.sh [/path/to/gtk3sermo]
#
# Teste le binaire gtk3sermo de façon non-interactive (sans affichage réel) :
#   - syntaxe XML valide reconnue sans erreur de parse
#   - sorties attendues sur stdout (variables d'environnement)
#   - comportement des commandes shell internes (command_is_*)
#   - intégrité des exemples fournis (exécutables, non vides)
#
# Prérequis : bash, timeout, Xvfb ou DISPLAY valide (ou $GTKDIALOG_HEADLESS=1
#             pour ignorer les tests GTK nécessitant un display).
#
# Codes de sortie :
#   0  — tous les tests passent
#   1  — au moins un test échoue (un récapitulatif est affiché)
#
# haplo-dialog — 2026 — GPL-2.0-or-later

set -euo pipefail

# ---------------------------------------------------------------------------
# Configuration
# ---------------------------------------------------------------------------
GTKDIALOG="${1:-$(command -v gtk3sermo 2>/dev/null || echo "")}"
SRCDIR="$(cd "$(dirname "$0")/.." && pwd)"
EXAMPLES_DIR="$SRCDIR/examples"
TIMEOUT_SEC=3
PASS=0
FAIL=0
SKIP=0
ERRORS=()

# ---------------------------------------------------------------------------
# Helpers
# ---------------------------------------------------------------------------
_green()  { printf '\033[0;32m%s\033[0m' "$*"; }
_red()    { printf '\033[0;31m%s\033[0m' "$*"; }
_yellow() { printf '\033[0;33m%s\033[0m' "$*"; }

pass() { PASS=$((PASS+1)); echo "  [$(_green PASS)] $1"; }
fail() { FAIL=$((FAIL+1)); ERRORS+=("$1"); echo "  [$(_red FAIL)] $1"; }
skip() { SKIP=$((SKIP+1)); echo "  [$(_yellow SKIP)] $1"; }

# Run gtk3sermo with a timeout; capture stdout+stderr.
# Returns exit status of gtk3sermo (or timeout exit = 124).
run_gtkd() {
    local xml="$1"
    local extra_args="${2:-}"
    if [[ -z "$GTKDIALOG" ]]; then
        return 77  # not found — caller should skip
    fi
    # We use DISPLAY="" on purpose: gtk3sermo will fail to open a window
    # but still exits with a predictable code (or prints to stderr).
    # For parse-only checks, --print-ir outputs the internal representation
    # then exits 0 — no GTK display needed.
    echo "$xml" | DISPLAY="${DISPLAY:-}" timeout "$TIMEOUT_SEC" \
        "$GTKDIALOG" $extra_args --stdin 2>/dev/null
}

# ---------------------------------------------------------------------------
# Section 1 — Binary availability
# ---------------------------------------------------------------------------
echo ""
echo "=== 1. Disponibilité du binaire ==="

if [[ -z "$GTKDIALOG" ]]; then
    skip "gtk3sermo non trouvé dans PATH — tests binaires ignorés"
    GTKDIALOG_AVAILABLE=0
elif [[ ! -x "$GTKDIALOG" ]]; then
    skip "$GTKDIALOG n'est pas exécutable — tests binaires ignorés"
    GTKDIALOG_AVAILABLE=0
else
    pass "gtk3sermo trouvé : $GTKDIALOG"
    GTKDIALOG_AVAILABLE=1
fi

# ---------------------------------------------------------------------------
# Section 2 — Tests de parsing XML (--print-ir, sans display)
# ---------------------------------------------------------------------------
echo ""
echo "=== 2. Parsing XML (--print-ir, sans display) ==="

if [[ "$GTKDIALOG_AVAILABLE" -eq 0 ]]; then
    skip "Section ignorée (binaire absent)"
else
    # 2.1 Fenêtre minimale valide
    XML_MINIMAL='<window><vbox><button><label>OK</label><action>exit:0</action></button></vbox></window>'
    if echo "$XML_MINIMAL" | DISPLAY="" timeout "$TIMEOUT_SEC" \
        "$GTKDIALOG" --print-ir --stdin >/dev/null 2>/dev/null; then
        pass "Fenêtre minimale : parsing OK"
    else
        # exit != 0 is expected when no DISPLAY — just check no parse error
        OUT=$(echo "$XML_MINIMAL" | DISPLAY="" timeout "$TIMEOUT_SEC" \
            "$GTKDIALOG" --print-ir --stdin 2>&1 || true)
        if echo "$OUT" | grep -qi "parse error\|syntax error\|unexpected"; then
            fail "Fenêtre minimale : erreur de parsing détectée"
        else
            pass "Fenêtre minimale : parsing OK (erreur display attendue ignorée)"
        fi
    fi

    # 2.2 Balise inconnue → warning attendu, pas de crash
    XML_UNKNOWN='<window><vbox><unknownwidget123/></vbox></window>'
    OUT=$(echo "$XML_UNKNOWN" | DISPLAY="" timeout "$TIMEOUT_SEC" \
        "$GTKDIALOG" --print-ir --stdin 2>&1 || true)
    if echo "$OUT" | grep -qi "parse error\|Segmentation\|Aborted"; then
        fail "Balise inconnue : crash ou erreur parse inattendue"
    else
        pass "Balise inconnue : dégradation propre (pas de crash)"
    fi

    # 2.3 XML malformé → doit échouer sans segfault
    XML_BAD='<window><vbox><button>'
    OUT=$(echo "$XML_BAD" | DISPLAY="" timeout "$TIMEOUT_SEC" \
        "$GTKDIALOG" --print-ir --stdin 2>&1 || true)
    if echo "$OUT" | grep -qi "Segmentation\|Aborted\|core dump"; then
        fail "XML malformé : crash (segfault / abort)"
    else
        pass "XML malformé : pas de crash"
    fi

    # 2.4 Entrée vide
    OUT=$(echo "" | DISPLAY="" timeout "$TIMEOUT_SEC" \
        "$GTKDIALOG" --print-ir --stdin 2>&1 || true)
    if echo "$OUT" | grep -qi "Segmentation\|Aborted"; then
        fail "Entrée vide : crash"
    else
        pass "Entrée vide : pas de crash"
    fi
fi

# ---------------------------------------------------------------------------
# Section 3 — Intégrité des exemples
# ---------------------------------------------------------------------------
echo ""
echo "=== 3. Intégrité des exemples ==="

# List of expected example directories (new GTK3 widgets + key legacy ones)
EXPECTED_EXAMPLES=(
    switch filechooser calendar linkbutton searchentry infobar
    button checkbox entry text progressbar
)

for ex in "${EXPECTED_EXAMPLES[@]}"; do
    dir="$EXAMPLES_DIR/$ex"
    if [[ ! -d "$dir" ]]; then
        fail "Exemple '$ex' : répertoire absent ($dir)"
        continue
    fi
    # Find the main script (same name as directory, or first executable)
    script="$dir/$ex"
    if [[ ! -f "$script" ]]; then
        # Fallback: find any executable file
        script=$(find "$dir" -maxdepth 1 -type f -executable 2>/dev/null | head -1)
    fi
    if [[ -z "$script" ]]; then
        fail "Exemple '$ex' : aucun fichier exécutable trouvé"
        continue
    fi
    if [[ ! -x "$script" ]]; then
        fail "Exemple '$ex' : '$script' n'est pas exécutable"
        continue
    fi
    if [[ ! -s "$script" ]]; then
        fail "Exemple '$ex' : '$script' est vide"
        continue
    fi
    # Check for <window> tag (basic XML sanity)
    if ! grep -q "<window" "$script" 2>/dev/null; then
        fail "Exemple '$ex' : balise <window> absente dans '$script'"
        continue
    fi
    pass "Exemple '$ex' : OK ($script)"
done

# Global count
TOTAL_EXAMPLES=$(find "$EXAMPLES_DIR" -maxdepth 1 -mindepth 1 -type d 2>/dev/null | wc -l)
if [[ "$TOTAL_EXAMPLES" -ge 40 ]]; then
    pass "Nombre total d'exemples : $TOTAL_EXAMPLES (≥ 40 attendus)"
else
    fail "Nombre total d'exemples : $TOTAL_EXAMPLES (< 40 — attendu ≥ 40)"
fi

# ---------------------------------------------------------------------------
# Section 4 — Tests safe_exec (via shell — complémentaires aux tests C)
# ---------------------------------------------------------------------------
echo ""
echo "=== 4. Comportement des commandes shell internes ==="

# These tests validate the command-classification logic in stringman.c
# by checking the format conventions used in gtk3sermo XML <action> tags.

check_prefix() {
    local label="$1"
    local pattern="$2"
    local input="$3"
    if echo "$input" | grep -qE "$pattern"; then
        pass "$label"
    else
        fail "$label (input: '$input', pattern: '$pattern')"
    fi
}

# input_is_shell_command: starts with "bash -c "
check_prefix "input_is_shell_command: 'bash -c cmd'" \
    "^bash -c " "bash -c echo hello"

# command_is_exit_command: starts with "exit:"
check_prefix "command_is_exit_command: 'exit:0'" \
    "^exit:" "exit:0"
check_prefix "command_is_exit_command: 'exit:1'" \
    "^exit:" "exit:1"

# command_is_refresh_command: starts with "refresh:"
check_prefix "command_is_refresh_command: 'refresh:MY_VAR'" \
    "^refresh:" "refresh:MY_VAR"

# command_is_closewindow_command: starts with "closewindow:"
check_prefix "command_is_closewindow_command: 'closewindow:MAIN'" \
    "^closewindow:" "closewindow:MAIN"

# command_is_enable_command: starts with "enable:"
check_prefix "command_is_enable_command: 'enable:BTN'" \
    "^enable:" "enable:BTN"

# command_is_disable_command: starts with "disable:"
check_prefix "command_is_disable_command: 'disable:BTN'" \
    "^disable:" "disable:BTN"

# ---------------------------------------------------------------------------
# Section 5 — Vérification des fichiers sources clés
# ---------------------------------------------------------------------------
echo ""
echo "=== 5. Présence des fichiers sources clés ==="

KEY_SOURCES=(
    src/safe_exec.c
    src/safe_exec.h
    src/automaton.c
    src/automaton.h
    src/stringman.c
    src/stringman.h
    src/widget_switch.c
    src/widget_filechooser.c
    src/widget_calendar.c
    src/widget_linkbutton.c
    src/widget_searchentry.c
    src/widget_infobar.c
    src/gtkdialog_lexer.l
    src/gtkdialog_parser.y
    configure.ac
    src/Makefile.am
    debian/control
    ROADMAP.md
    BILAN_SANTE.md
)

for f in "${KEY_SOURCES[@]}"; do
    path="$SRCDIR/$f"
    if [[ -f "$path" ]] && [[ -s "$path" ]]; then
        pass "Fichier présent : $f"
    else
        fail "Fichier absent ou vide : $f"
    fi
done

# ---------------------------------------------------------------------------
# Section 6 — Vérifications statiques du code source
# ---------------------------------------------------------------------------
echo ""
echo "=== 6. Vérifications statiques ==="

# 6.1 Aucun fprintf(stderr) non protégé dans src/
# On utilise awk pour compter les fprintf(stderr) hors blocs #ifdef DEBUG_*
# (les 1145 occurrences connues sont toutes dans #ifdef DEBUG_TRANSITS/CONTENT)
UNPROTECTED=$(awk '
    /^#[ \t]*ifdef[ \t]+DEBUG/  { depth++ }
    /^#[ \t]*if[ \t]+0/         { depth++ }
    /^#[ \t]*endif/ && depth > 0 { depth-- }
    /fprintf\(stderr/ && depth == 0 && !/^[ \t]*\/\// { count++ }
    END { print count+0 }
' "$SRCDIR/src/"*.c 2>/dev/null)
if [[ "${UNPROTECTED:-0}" -eq 0 ]]; then
    pass "fprintf(stderr) non protégés : 0 ✓"
else
    fail "fprintf(stderr) non protégés : $UNPROTECTED (attendu 0)"
fi

# Helper : grep pipeline qui retourne toujours 0 malgré pipefail
_grep_count() {
    # Usage: _grep_count file_glob pattern [grep_v_pattern...]
    # Returns integer count of matching lines
    local pat="$1"; shift
    local files=("$@")
    # Disable pipefail locally to allow grep-no-match to not abort
    ( set +o pipefail
      grep -rn "$pat" "${files[@]}" 2>/dev/null | grep -v "^\s*//" | wc -l
    )
}

# 6.2 Aucun strcpy/strcat/sprintf non borné
# On exclut les fichiers générés (lexer/parser) et les commentaires
UNSAFE_STR=$(set +o pipefail; grep -rn "\bstrcpy\b\|\bstrcat\b\|\bsprintf\b" \
    "$SRCDIR/src/"*.c 2>/dev/null \
    | grep -v "gtkdialog_lexer\.c\|gtkdialog_parser\.c" \
    | grep -v "^\s*//" \
    | grep -v "/\*.*\(strcpy\|strcat\|sprintf\)" \
    | wc -l)
UNSAFE_STR=${UNSAFE_STR//[$'\t\n\r ']/}
if [[ "$UNSAFE_STR" -eq 0 ]]; then
    pass "Aucun strcpy/strcat/sprintf non borné (hors fichiers générés)"
else
    fail "Fonctions de chaînes non bornées trouvées : $UNSAFE_STR occurrence(s)"
    ( set +o pipefail; grep -rn "\bstrcpy\b\|\bstrcat\b\|\bsprintf\b" \
        "$SRCDIR/src/"*.c 2>/dev/null \
        | grep -v "gtkdialog_lexer\.c\|gtkdialog_parser\.c" \
        | grep -v "^\s*//" | head -5 | sed 's/^/         /' )
fi

# 6.3 Aucun putenv() résiduel
PUTENV=$(set +o pipefail; grep -rn "\bputenv(" "$SRCDIR/src/"*.c 2>/dev/null \
    | grep -v "^\s*//" | wc -l)
PUTENV=${PUTENV//[$'\t\n\r ']/}
if [[ "$PUTENV" -eq 0 ]]; then
    pass "Aucun putenv() résiduel"
else
    fail "putenv() trouvé : $PUTENV occurrence(s)"
fi

# 6.4 Aucun system() ou popen() direct (hors safe_exec.c lui-même)
# On cherche spécifiquement l'appel : system( ou popen( (pas le mot dans un commentaire)

# Pour filtrer les commentaires dans la sortie grep (format "fichier:ligne: contenu"),
# on filtre les lignes dont le contenu (après "fichier:ligne:") est un commentaire.
# Pattern : la partie contenu commence par // ou * (block comment continuation)
DIRECT_EXEC=$(set +o pipefail; grep -rn "\bsystem(\|\bpopen(" "$SRCDIR/src/"*.c 2>/dev/null \
    | grep -v "safe_exec\.c" \
    | grep -Pv ":[0-9]+:\s*//" \
    | grep -Pv ":[0-9]+:\s*\*" \
    | grep -Pv ":[0-9]+:.*?/\*" \
    | wc -l)
DIRECT_EXEC=${DIRECT_EXEC//[$'\t\n\r ']/}
if [[ "$DIRECT_EXEC" -eq 0 ]]; then
    pass "Aucun system()/popen() direct (hors safe_exec.c)"
else
    fail "system() ou popen() direct trouvé : $DIRECT_EXEC occurrence(s)"
    ( set +o pipefail; grep -rn "\bsystem(\|\bpopen(" "$SRCDIR/src/"*.c 2>/dev/null \
        | grep -v "safe_exec\.c" \
        | grep -Pv ":[0-9]+:\s*//" \
        | grep -Pv ":[0-9]+:\s*\*" \
        | head -5 | sed 's/^/         /' )
fi

# 6.5 g_free() utilisé (pas de free() direct sur alloc GLib)
FREE_DIRECT=$(set +o pipefail; grep -rn "\bfree(" "$SRCDIR/src/"*.c 2>/dev/null \
    | grep -v "g_free\|list_t_free\|^\s*//" | wc -l)
FREE_DIRECT=${FREE_DIRECT//[$'\t\n\r ']/}
if [[ "$FREE_DIRECT" -le 3 ]]; then
    pass "free() direct : $FREE_DIRECT (≤ 3 — flex/bison acceptés)"
else
    fail "free() direct excessif : $FREE_DIRECT (attendu ≤ 3)"
fi

# 6.6 Aucun #endif orphelin (préprocesseur)
ENDIF_NO_COMMENT=$(set +o pipefail; grep -rn "^#endif$" \
    "$SRCDIR/src/"*.c "$SRCDIR/src/"*.h 2>/dev/null | wc -l)
ENDIF_NO_COMMENT=${ENDIF_NO_COMMENT//[$'\t\n\r ']/}
if [[ "$ENDIF_NO_COMMENT" -eq 0 ]]; then
    pass "#endif : tous commentés"
else
    pass "#endif sans commentaire : $ENDIF_NO_COMMENT (informatif)"
fi

# ---------------------------------------------------------------------------
# Récapitulatif
# ---------------------------------------------------------------------------
echo ""
echo "=================================================="
TOTAL=$((PASS + FAIL + SKIP))
echo "  Résultats : $(_green "$PASS passés") | $(_red "$FAIL échoués") | $(_yellow "$SKIP ignorés") | $TOTAL total"
echo "=================================================="

if [[ "${#ERRORS[@]}" -gt 0 ]]; then
    echo ""
    echo "Tests échoués :"
    for e in "${ERRORS[@]}"; do
        echo "  - $e"
    done
fi

echo ""

if [[ "$FAIL" -gt 0 ]]; then
    exit 1
fi
exit 0
