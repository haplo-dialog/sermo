#!/bin/bash
# SPDX-License-Identifier: GPL-2.0-or-later
# run_examples.sh — exécute RÉELLEMENT chaque exemple contre un binaire donné.
#
# Le banc XML (tests/xml) vérifie que les exemples PARSENT ; le banc comportement
# (tests/comportement) compare des valeurs exportées sur 52 cas. Aucun n'OUVRE
# les 56 exemples réels (210 fichiers) (button/, playmusic/, system-tools/, …). Un backend peut
# donc parser et rester vert alors qu'un widget meurt sur un vrai exemple — c'est
# arrivé à gtk4 (<password> injoignable) et à qt6 (<togglebutton> mort au clic).
# Ce banc-ci ouvre vraiment les fenêtres de TOUS les exemples.
#
# Repris du sermo public (gtk3sermo/tests/run_examples.sh), généralisé aux
# binaires des backends sermo.
#
# Usage :  ./run_examples.sh <binaire-sermo-GUI> [repertoire_examples]
#   ex.  ./run_examples.sh ../sermo-backend-qt6/_build/qt6sermo
#
# Verdicts :
#   OK      la fenêtre s'est affichée
#   CRASH   assertion, segfault, ou « Unknown widget type »
#   SYNTAX  erreur d'analyse XML
#   NOWIN   le programme a tourné sans jamais afficher de fenêtre
#
# Backend GUI attendu (xdotool détecte une fenêtre X) ; ncurses est un backend
# terminal, hors de portée de ce banc (utiliser tests/xml + comportement).
set -uo pipefail

BIN="${1:?usage: run_examples.sh <binaire-sermo> [examples/]}"
[[ -x "$BIN" ]] || { echo "binaire introuvable ou non exécutable : $BIN" >&2; exit 2; }
BIN="$(readlink -f "$BIN")"
EXAMPLES="${2:-$(cd "$(dirname "$0")/.." && pwd)/examples}"
[[ -d "$EXAMPLES" ]] || { echo "répertoire d'exemples introuvable : $EXAMPLES" >&2; exit 2; }
# ⚠️ Toujours ABSOLU : la sonde fait « cd » dans chaque exemple puis relance le
# script par son chemin — relatif, il devient introuvable et les exemples
# sortaient « aucune fenêtre » alors que tout marchait.
EXAMPLES="$(readlink -f "$EXAMPLES")"

for t in xvfb-run xdotool timeout; do
    command -v "$t" >/dev/null || { echo "outil manquant : $t" >&2; exit 2; }
done

# Les exemples codent en dur le nom du binaire (GTKDIALOG=gtkdialog, gtk3sermo…).
# On détourne tous ces noms — y compris ceux des backends sermo — vers le binaire
# à tester, via le PATH.
SHIM="$(mktemp -d)"; trap 'rm -rf "$SHIM"' EXIT
for n in gtkdialog gtkdialog4 gtksermo sermo \
         gtk3sermo gtk4sermo qt6sermo fltk1sermo efl1sermo sdl3sermo ncursessermo \
         gtk3dialog qt6dialog fltk1dialog efl1dialog sdl3dialog; do
    printf '#!/bin/sh\nexec "%s" "$@"\n' "$BIN" > "$SHIM/$n"; chmod +x "$SHIM/$n"
done
export PATH="$SHIM:$PATH"
# Le nom du port testé, pour les exemples qui en dépendent : les exemples glade
# choisissent leur fichier GtkBuilder (GTK 3 ou GTK 4) d'après GTKDIALOG.
export GTKDIALOG="$(basename "$BIN")"
PORT="${GTKDIALOG%sermo}"
export GSK_RENDERER="${GSK_RENDERER:-cairo}"
# Plateformes offscreen des backends neutres (qt6/efl1/sdl3) : sans display réel,
# le binaire s'ouvre quand même ; xvfb-run leur en donne un de toute façon.
export QT_QPA_PLATFORM="${QT_QPA_PLATFORM:-xcb}"

# Quelques exemples ne sont pas des dialogues autonomes mais des visionneuses :
# sans argument ils affichent leur aide et sortent. Leur en fournir un est la
# seule façon de les exercer.
argument_pour() {
    case "$1" in
        pfontview) find /usr/share/fonts -name '*.ttf' 2>/dev/null | head -1 ;;
        *)         : ;;
    esac
}

OK=0; CRASH=0; SYNTAX=0; NOWIN=0; RESERVE=0; DETAIL=()
# Attente MAXIMALE d'une fenêtre, en secondes (l'écran est interrogé toutes les
# 0,1 s : un exemple qui marche n'attend pas plus). 3 s donnaient des « aucune
# fenêtre » sans défaut dès que le poste était chargé : mesuré le 2026-09-17 sur
# trois passes, chaque fois un exemple différent, vert dans les autres passes.
DELAY="${EXAMPLE_DELAY:-10}"

# Lance un script sous un serveur X jetable et dit si une fenêtre est apparue.
#
# On se place dans le répertoire de l'exemple : les applications multi-fichiers
# sourcent leurs fonctions et lisent leurs images en chemin relatif, et ne
# démarrent pas si on les lance d'ailleurs.
#
# 2026-09-16 — trois défauts du banc, mesurés :
#  - la fenêtre était cherchée par son nom, que xdotool ne lit pas quand il est
#    posé en UTF8_STRING : sdl3 sortait « sans fenêtre » sur 54 exemples sur 55.
#    On cherche aussi par classe ;
#  - seul le script de l'exemple était arrêté, pas le programme sermo qu'il
#    avait lancé : celui-ci mourait ensuite de la disparition du serveur X, et
#    qt6 écrivait alors un « syntax error » que le banc attribuait à 50
#    exemples. L'exemple tourne maintenant dans son propre groupe de processus
#    (setsid), arrêté en entier tant que l'affichage existe ;
#  - le verdict lisait aussi ce que le programme écrit en mourant (efl1 et qt6
#    y écrivent un faux « syntax error »). Il porte désormais sur la sortie
#    relevée à l'apparition de la fenêtre, ou juste avant l'arrêt s'il n'en est
#    venu aucune (<journal>.vu) : sur le démarrage, que ce banc mesure.
probe() {
    local script="$1" log="$2" arg="${3:-}"
    xvfb-run -a --server-args="-screen 0 800x600x24" bash -c '
        cd "$(dirname "$0")" || exit 1
        if [ -n "$2" ]; then setsid "$0" "$2" >"$1" 2>&1 & else setsid "$0" >"$1" 2>&1 & fi
        pid=$!
        for i in $(seq 1 '"$DELAY"'0); do
            if [ -n "$(xdotool search --onlyvisible --name . 2>/dev/null)" ] ||
               [ -n "$(xdotool search --onlyvisible --class . 2>/dev/null)" ]; then
                cp "$1" "$1.vu"; echo WINDOW; break
            fi
            kill -0 -- -$pid 2>/dev/null || break
            sleep 0.1
        done
        [ -f "$1.vu" ] || cp "$1" "$1.vu"
        kill -TERM -- -$pid 2>/dev/null
        for i in $(seq 1 20); do kill -0 -- -$pid 2>/dev/null || break; sleep 0.1; done
        kill -KILL -- -$pid 2>/dev/null
        wait $pid 2>/dev/null
    ' "$script" "$log" "$arg" 2>/dev/null
}

echo
echo "  binaire  : $BIN"
echo "  exemples : $EXAMPLES"
echo

for dir in $(find "$EXAMPLES" -maxdepth 1 -mindepth 1 -type d | sort); do
    name="$(basename "$dir")"
    # Point d'entrée : fichier homonyme du répertoire, puis « main », puis un
    # script décrivant une fenêtre (hors fonctions func*), sinon tout exécutable.
    script="$dir/$name"
    if [[ ! -f "$script" ]]; then
        script="$dir/main"
        if [[ ! -f "$script" ]]; then
            script="$(grep -l '<window' "$dir"/* 2>/dev/null | grep -v '/func' | head -1)"
            [[ -n "$script" ]] || script="$(find "$dir" -maxdepth 1 -type f -executable | head -1)"
        fi
    fi
    [[ -n "$script" && -f "$script" ]] || { printf '  %-16s %s\n' "$name" "(aucun script)"; continue; }

    # Un exemple qui ne vaut que pour certains ports le dit dans PORTS (ex. glade :
    # --glade-xml est GTK). Ailleurs il est annoncé « réservé », visiblement, et
    # n'est compté ni réussi ni en échec — la liste est versionnée et relue.
    if [[ -f "$dir/PORTS" ]]; then
        ports_ok="$(grep -v '^[[:space:]]*#' "$dir/PORTS" | tr '\n' ' ')"
        case " $ports_ok " in
            *" $PORT "*) ;;
            *) printf '  %-16s %s\n' "$name" "(réservé à : ${ports_ok% })"
               RESERVE=$((RESERVE+1)); continue ;;
        esac
    fi

    log="$(mktemp)"
    saw="$(probe "$script" "$log" "$(argument_pour "$name")")"
    out="$(cat "$log.vu" 2>/dev/null)"; rm -f "$log" "$log.vu"

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
[[ $RESERVE -gt 0 ]] && printf '  (%s exemple(s) réservé(s) à d’autres ports, non joué(s))\n' "$RESERVE"
echo "  ================================================"
if [[ ${#DETAIL[@]} -gt 0 ]]; then
    echo
    echo "  Détail des échecs :"
    printf '    - %s\n' "${DETAIL[@]}"
fi
echo
[[ $((CRASH+SYNTAX+NOWIN)) -eq 0 ]]
