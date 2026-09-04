#!/bin/bash
# run_examples.sh — exécute RÉELLEMENT chaque exemple contre un binaire donné.
#
# Le banc historique (run_tests.sh) vérifie que les exemples EXISTENT ; il ne
# les lance jamais. Un port peut donc compiler, passer run_tests.sh, et mourir
# sur le premier script réel — c'est arrivé à gtk4sermo (widget <password>
# injoignable, 6 widgets non câblés). Ce banc-ci ouvre vraiment les fenêtres.
#
# Usage :  ./run_examples.sh /chemin/vers/gtk3sermo [repertoire_examples]
#
# Verdicts :
#   OK      la fenêtre s'est affichée
#   CRASH   assertion, segfault, ou « Unknown widget type »
#   SYNTAX  erreur d'analyse XML
#   NOWIN   le programme a tourné sans jamais afficher de fenêtre
#
# haplo-dialog — 2026 — GPL-2.0-or-later
set -uo pipefail

BIN="${1:?usage: run_examples.sh /chemin/vers/binaire [examples/]}"
[[ -x "$BIN" ]] || { echo "binaire introuvable ou non exécutable : $BIN" >&2; exit 2; }
BIN="$(readlink -f "$BIN")"
EXAMPLES="${2:-$(cd "$(dirname "$0")/.." && pwd)/examples}"
[[ -d "$EXAMPLES" ]] || { echo "répertoire d'exemples introuvable : $EXAMPLES" >&2; exit 2; }
# ⚠️ Toujours ABSOLU : la sonde fait « cd » dans chaque exemple puis relance le
# script par son chemin — relatif, il devient introuvable et les 32 exemples
# sortaient « aucune fenêtre » alors que tout marchait (vécu le 2026-09-03).
EXAMPLES="$(readlink -f "$EXAMPLES")"

for t in xvfb-run xdotool timeout; do
    command -v "$t" >/dev/null || { echo "outil manquant : $t" >&2; exit 2; }
done

# Les exemples codent en dur GTKDIALOG=gtkdialog (ou gtkdialog4, gtk3sermo…).
# On détourne tous ces noms vers le binaire à tester, via le PATH.
SHIM="$(mktemp -d)"; trap 'rm -rf "$SHIM"' EXIT
for n in gtkdialog gtkdialog4 gtk3sermo gtk4sermo gtksermo qt6sermo; do
    printf '#!/bin/sh\nexec "%s" "$@"\n' "$BIN" > "$SHIM/$n"; chmod +x "$SHIM/$n"
done
export PATH="$SHIM:$PATH"
export GSK_RENDERER="${GSK_RENDERER:-cairo}"

# Quelques exemples ne sont pas des dialogues autonomes mais des visionneuses :
# sans argument ils affichent leur aide et sortent, ce qui est le comportement
# juste. Leur en fournir un est la seule façon de les exercer.
argument_pour() {
    case "$1" in
        pfontview) find /usr/share/fonts -name '*.ttf' 2>/dev/null | head -1 ;;
        *)         : ;;
    esac
}

OK=0; CRASH=0; SYNTAX=0; NOWIN=0; DETAIL=()
DELAY="${EXAMPLE_DELAY:-3}"

# Lance un script sous un serveur X jetable et dit si une fenêtre est apparue.
probe() {
    local script="$1" log="$2"
    # On se place dans le répertoire de l'exemple : les applications
    # multi-fichiers sourcent leurs fonctions et lisent leurs images en
    # chemin relatif, et ne démarrent pas si on les lance d'ailleurs.
    local arg="${3:-}"
    xvfb-run -a --server-args="-screen 0 800x600x24" bash -c '
        cd "$(dirname "$0")" || exit 1
        if [ -n "$2" ]; then "$0" "$2" >"$1" 2>&1 & else "$0" >"$1" 2>&1 & fi
        pid=$!
        for i in $(seq 1 '"$DELAY"'0); do
            [ "$(xdotool search --onlyvisible --name . 2>/dev/null | wc -l)" -gt 0 ] && { echo WINDOW; break; }
            kill -0 $pid 2>/dev/null || break
            sleep 0.1
        done
        kill -TERM $pid 2>/dev/null; wait $pid 2>/dev/null
    ' "$script" "$log" "$arg" 2>/dev/null
}

echo
echo "  binaire  : $BIN"
echo "  exemples : $EXAMPLES"
echo

for dir in $(find "$EXAMPLES" -maxdepth 1 -mindepth 1 -type d | sort); do
    name="$(basename "$dir")"
    # Choix du point d'entrée. Les applications multi-fichiers (playmusic,
    # pfeme, pfontview) nomment le leur « main » et l'entourent d'une vingtaine
    # de fonctions auxiliaires « func* », qui ne s'exécutent pas seules :
    # prendre le premier exécutable venu revenait à lancer une fonction, jamais
    # le programme. On cherche donc, dans l'ordre : le fichier homonyme du
    # répertoire, puis « main », puis un script qui décrit une fenêtre sans être
    # une fonction, et seulement en dernier n'importe quel exécutable.
    script="$dir/$name"
    if [[ ! -f "$script" ]]; then
        script="$dir/main"
        if [[ ! -f "$script" ]]; then
            script="$(grep -l '<window' "$dir"/* 2>/dev/null | grep -v '/func' | head -1)"
            [[ -n "$script" ]] || script="$(find "$dir" -maxdepth 1 -type f -executable | head -1)"
        fi
    fi
    [[ -n "$script" && -f "$script" ]] || { printf '  %-16s %s\n' "$name" "(aucun script)"; continue; }

    log="$(mktemp)"
    saw="$(probe "$script" "$log" "$(argument_pour "$name")")"
    out="$(cat "$log")"; rm -f "$log"

    if   grep -qiE 'assertion failed|Bail out|Segmentation|Unknown widget type' <<<"$out"; then
        v=CRASH;  CRASH=$((CRASH+1)); DETAIL+=("$name : $(grep -oiE 'Unknown widget type|assertion failed[^)]*|Segmentation[a-z ]*' <<<"$out" | head -1)")
    elif grep -qiE 'syntax error|parse error' <<<"$out"; then
        v=SYNTAX; SYNTAX=$((SYNTAX+1)); DETAIL+=("$name : $(grep -oiE '(syntax|parse) error.*' <<<"$out" | head -1)")
    elif [[ "$saw" == WINDOW ]]; then
        v=OK;     OK=$((OK+1))
    else
        v=NOWIN;  NOWIN=$((NOWIN+1)); DETAIL+=("$name : aucune fenêtre en ${DELAY}s")
    fi
    printf '  %-16s %s\n' "$name" "$v"
done

TOTAL=$((OK+CRASH+SYNTAX+NOWIN))
echo
echo "  ================================================"
printf '  %s exemples : %s OK · %s CRASH · %s SYNTAX · %s sans fenêtre\n' \
       "$TOTAL" "$OK" "$CRASH" "$SYNTAX" "$NOWIN"
echo "  ================================================"
if [[ ${#DETAIL[@]} -gt 0 ]]; then
    echo
    echo "  Détail des échecs :"
    printf '    - %s\n' "${DETAIL[@]}"
fi
echo
[[ $((CRASH+SYNTAX+NOWIN)) -eq 0 ]]
