#!/bin/sh
# run.sh — Banc de COMPORTEMENT haplo-dialog
# sermo — GPL-2.0-or-later
#
# Usage :
#   ./run.sh /chemin/absolu/vers/gtk4sermo          # contre les valeurs attendues
#   ./run.sh --diff /chemin/gtk3sermo /chemin/gtk4sermo   # les deux ports doivent coïncider
#   TIMEOUT=20 ./run.sh …
#
# ─────────────────────────────────────────────────────────────────────────────
# POURQUOI CE BANC EXISTE
#
# tests/xml/run_tests.sh lance --print-ir : il PARSE le XML, imprime la
# représentation interne, et sort. Il ne construit AUCUN widget et n'ouvre
# AUCUNE fenêtre. Ses 55 PASS attestent que le XML est compris — rien d'autre.
#
# Le 2026-08-29, cinq fonctions du port GTK 4 se sont révélées mortes ou
# menteuses alors que la suite XML était verte depuis des mois :
#   <checkbox><default>true</default>  rendait CB="false"
#   border-width                        était ignoré
#   <menuitem checkbox=>                ne rendait aucune variable
#   <filechooser>                       était un bouton, et son <output file:>
#                                       TRONQUAIT le fichier de destination
# Aucune n'était détectable par --print-ir : le XML était parfaitement compris,
# c'est ce que le programme en FAISAIT qui était faux.
#
# Ce banc-ci lance le dialogue POUR DE VRAI, sous Xvfb, et compare les variables
# shell qu'il rend. Un cas se ferme tout seul par un <timer><action>exit:…
#
# ─────────────────────────────────────────────────────────────────────────────
# DEUX MODES, ET POURQUOI LES DEUX
#
#   attendu   chaque cas a un fichier .attendu qui fait foi. Marche avec UN seul
#             port, donc utilisable en intégration continue.
#   --diff    les deux ports doivent rendre EXACTEMENT la même chose. Le port
#             GTK 3 sert d'oracle : c'est lui qui marche. Toute divergence est un
#             défaut du port GTK 4, sans avoir à écrire l'attendu à la main.
#
# Le mode --diff est celui qui aurait attrapé les cinq défauts du 2026-08-29
# sans qu'on ait rien à prévoir.
#
# ─────────────────────────────────────────────────────────────────────────────
# UN Gtk-CRITICAL EST UN ÉCHEC
#
# Les cinq défauts émettaient des Gtk-CRITICAL sur stderr, et personne ne les
# lisait. Ici un CRITICAL ou un WARNING GLib fait ÉCHOUER le cas.
#
# ─────────────────────────────────────────────────────────────────────────────
# ⛔ CE BANC NE SE TAIT JAMAIS
#
# Pas de Xvfb, pas de binaire, aucun cas trouvé : il sort en ERREUR, il ne rend
# pas un succès vide. Un banc muet qui répond « tout va bien » est pire que pas
# de banc — c'est la leçon de tests/xml/run_tests.sh, qui rendait rc=0 avec zéro
# test exécuté quand il ne trouvait pas de binaire.

set -e

ICI="$(cd "$(dirname "$0")" && pwd)"
CAS="$ICI/cas"
TIMEOUT="${TIMEOUT:-25}"
PASS=0; FAIL=0

GREEN='\033[0;32m'; RED='\033[0;31m'; BLUE='\033[0;34m'; NC='\033[0m'
ok()   { printf "${GREEN}PASS${NC}  %s\n" "$*"; PASS=$((PASS+1)); }
ko()   { printf "${RED}FAIL${NC}  %s\n" "$*"; FAIL=$((FAIL+1)); }
info() { printf "${BLUE}────${NC}  %s\n" "$*"; }
mort() { printf "${RED}ERREUR : %s${NC}\n" "$*" >&2; printf "AUCUN test n'a tourné.\n" >&2; exit 2; }

MODE=attendu
if [ "${1:-}" = "--diff" ]; then MODE=diff; shift; fi

command -v xvfb-run >/dev/null 2>&1 || mort "xvfb-run manquant (paquet xvfb) — ce banc a besoin d'un affichage."
[ -d "$CAS" ] || mort "dossier de cas introuvable : $CAS"
n_cas=$(find "$CAS" -maxdepth 1 -name '*.xml' | wc -l)
[ "$n_cas" -gt 0 ] || mort "aucun cas .xml dans $CAS"

verifie_binaire() {
    [ -n "${1:-}" ] || mort "usage : run.sh [--diff] <binaire> [binaire2]"
    [ -x "$1" ] || mort "binaire introuvable ou non exécutable : $1"
    # ⚠️ On n'accepte QUE des chemins : run_tests.sh, lui, prend le binaire du
    # PATH en priorité et valide alors le paquet INSTALLÉ au lieu de la
    # construction en cours. Piège rencontré le 2026-08-29.
    case "$1" in */*) : ;; *) mort "donne un CHEMIN, pas un nom : « $1 » irait chercher dans le PATH." ;; esac
}

# Joue un cas et rend sur stdout les variables shell, triées.
# Rend 90 si le programme n'a pas fini, 91 s'il a émis un CRITICAL/WARNING.
joue() {
    _bin=$1; _xml=$2; _err=$(mktemp); _out=$(mktemp)
    MAIN_DIALOG="$(cat "$_xml")" xvfb-run -a \
        timeout "$TIMEOUT" "$_bin" --program=MAIN_DIALOG >"$_out" 2>"$_err" || true
    _rc=0
    if grep -qE 'CRITICAL|WARNING \*\*' "$_err" 2>/dev/null; then _rc=91; fi
    # Les variables shell : lignes NOM="valeur". On trie pour ne pas dependre de
    # l'ordre d'evaluation, et on retire EXIT qui ne dit rien du comportement.
    grep -E '^[A-Za-z_][A-Za-z0-9_]*=' "$_out" 2>/dev/null | grep -v '^EXIT=' | LC_ALL=C sort
    if [ ! -s "$_out" ] && [ "$_rc" = 0 ]; then _rc=90; fi
    printf '%s' "$_rc" > "$_err.rc"
    RC=$(cat "$_err.rc"); rm -f "$_err" "$_out" "$_err.rc"
    return 0
}

if [ "$MODE" = diff ]; then
    verifie_binaire "${1:-}"; verifie_binaire "${2:-}"
    A=$1; B=$2
    info "Différentiel : $(basename "$A") (oracle) contre $(basename "$B")"
    for xml in "$CAS"/*.xml; do
        nom=$(basename "$xml" .xml)
        desc=$(grep '^<!-- DESC:' "$xml" 2>/dev/null | sed 's/<!-- DESC: //;s/ -->//' || echo "$nom")
        va=$(joue "$A" "$xml"); rca=$RC
        vb=$(joue "$B" "$xml"); rcb=$RC
        if [ "$rca" = 91 ]; then ko "$nom — $(basename "$A") a émis un CRITICAL"; continue; fi
        if [ "$rcb" = 91 ]; then ko "$nom — $(basename "$B") a émis un CRITICAL"; continue; fi
        if [ "$rca" = 90 ] || [ "$rcb" = 90 ]; then ko "$nom — sortie vide (le dialogue ne s'est pas fermé ?)"; continue; fi
        if [ "$va" = "$vb" ]; then ok "$nom : $desc"
        else
            ko "$nom : $desc"
            printf '        oracle  : %s\n' "$(printf '%s' "$va" | tr '\n' ' ')"
            printf '        testé   : %s\n' "$(printf '%s' "$vb" | tr '\n' ' ')"
        fi
    done
else
    verifie_binaire "${1:-}"
    BIN=$1
    info "Attendu : $(basename "$BIN")"
    for xml in "$CAS"/*.xml; do
        nom=$(basename "$xml" .xml)
        att="$CAS/$nom.attendu"
        desc=$(grep '^<!-- DESC:' "$xml" 2>/dev/null | sed 's/<!-- DESC: //;s/ -->//' || echo "$nom")
        [ -f "$att" ] || { ko "$nom — fichier .attendu manquant"; continue; }
        v=$(joue "$BIN" "$xml"); rc=$RC
        if [ "$rc" = 91 ]; then ko "$nom — Gtk-CRITICAL émis"; continue; fi
        if [ "$rc" = 90 ]; then ko "$nom — sortie vide (le dialogue ne s'est pas fermé ?)"; continue; fi
        if [ "$v" = "$(cat "$att")" ]; then ok "$nom : $desc"
        else
            ko "$nom : $desc"
            printf '        attendu : %s\n' "$(cat "$att" | tr '\n' ' ')"
            printf '        obtenu  : %s\n' "$(printf '%s' "$v" | tr '\n' ' ')"
        fi
    done
fi

printf '\n'
info "Comportement : ${PASS} PASS | ${FAIL} FAIL  (sur ${n_cas} cas)"
[ "$((PASS+FAIL))" -gt 0 ] || mort "aucun cas n'a été évalué"
if [ "$FAIL" -gt 0 ]; then printf "${RED}ÉCHEC — ${FAIL} cas en erreur${NC}\n"; exit 1; fi
printf "${GREEN}SUCCÈS — le comportement est conforme${NC}\n"
