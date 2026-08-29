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

# Joue un cas. Écrit les variables shell triées dans $SORTIE, et l'état dans
# $ETAT : 0 normal, 90 sortie vide, 91 CRITICAL/WARNING émis.
#
# ⛔ Pourquoi passer par des FICHIERS et non par une variable : la première
# version faisait « v=$(joue …) ; rc=$RC ». Or une substitution de commande
# s'exécute dans un SOUS-SHELL — le RC posé dedans ne remonte JAMAIS. Le
# contrôle des Gtk-CRITICAL était donc du code mort, et un cas qui en émettait
# passait quand même. Mesuré le 2026-08-29 : le cas 11-infobar était vert sur
# un binaire qui crachait un CRITICAL. C'est précisément le genre de succès
# muet que ce banc existe pour empêcher.
SORTIE=$(mktemp); ETAT=$(mktemp); ERRS=$(mktemp)
trap 'rm -f "$SORTIE" "$ETAT" "$ERRS"' EXIT INT TERM

joue() {
    MAIN_DIALOG="$(cat "$2")" xvfb-run -a \
        timeout "$TIMEOUT" "$1" --program=MAIN_DIALOG >"$SORTIE.brut" 2>"$ERRS" || true
    grep -E '^[A-Za-z_][A-Za-z0-9_]*=' "$SORTIE.brut" 2>/dev/null \
        | grep -v '^EXIT=' | LC_ALL=C sort > "$SORTIE"
    if grep -qE 'CRITICAL|WARNING \*\*' "$ERRS" 2>/dev/null; then
        printf '91' > "$ETAT"
    elif [ ! -s "$SORTIE" ]; then
        printf '90' > "$ETAT"
    else
        printf '0' > "$ETAT"
    fi
    rm -f "$SORTIE.brut"
}

if [ "$MODE" = diff ]; then
    verifie_binaire "${1:-}"; verifie_binaire "${2:-}"
    A=$1; B=$2
    info "Différentiel : $(basename "$A") (oracle) contre $(basename "$B")"
    for xml in "$CAS"/*.xml; do
        nom=$(basename "$xml" .xml)
        desc=$(grep '^<!-- DESC:' "$xml" 2>/dev/null | sed 's/<!-- DESC: //;s/ -->//' || echo "$nom")
        joue "$A" "$xml"; va=$(cat "$SORTIE"); rca=$(cat "$ETAT")
        joue "$B" "$xml"; vb=$(cat "$SORTIE"); rcb=$(cat "$ETAT")
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
        joue "$BIN" "$xml"; v=$(cat "$SORTIE"); rc=$(cat "$ETAT")
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
