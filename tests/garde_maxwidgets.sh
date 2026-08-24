#!/bin/bash
# tests/garde_maxwidgets.sh — haplo-dialog — 2026 — GPL-2.0-or-later
#
# _sum() recopie les widgets d'un conteneur dans des tableaux de taille FIXE
# (MAXWIDGETS). Sans garde-fou, un conteneur qui a plus d'enfants directs que
# cette limite ecrit au-dela (CWE-787) — en silence, la plupart du temps.
#
# Le test verifie les DEUX cotes de la borne :
#   sous la limite  -> la fenetre s'ouvre normalement
#   au-dessus       -> refus net et nomme, pas une corruption silencieuse
BIN="${1:?usage: garde_maxwidgets.sh /chemin/vers/binaire}"
[[ -x "$BIN" ]] || { echo "binaire introuvable : $BIN" >&2; exit 2; }
for t in xvfb-run timeout; do command -v "$t" >/dev/null || { echo "outil manquant : $t" >&2; exit 2; }; done

MAX=$(grep -hoE '#define[[:space:]]+MAXWIDGETS[[:space:]]+[0-9]+' \
        "$(dirname "$BIN")"/stack.h 2>/dev/null | grep -oE '[0-9]+$')
MAX="${MAX:-256}"

fenetre() { printf '<window><vbox>'; for ((i=0;i<$1;i++)); do printf '<text><label>x</label></text>'; done; printf '</vbox></window>'; }

# --- sous la limite : doit s'ouvrir ---
DLG="$(fenetre $((MAX/4)))" ; export DLG
timeout 12 xvfb-run -a "$BIN" --program=DLG </dev/null >/dev/null 2>&1
rc=$?
if [[ $rc -ne 124 && $rc -ne 0 ]]; then
    echo "ECHEC : $((MAX/4)) enfants devraient s'afficher, rc=$rc"; exit 1
fi

# --- au-dessus : doit refuser, et le dire ---
DLG="$(fenetre $((MAX+44)))" ; export DLG
sortie=$(timeout 12 xvfb-run -a "$BIN" --program=DLG </dev/null 2>&1)
rc=$?
if [[ $rc -eq 124 || $rc -eq 0 ]]; then
    echo "ECHEC : $((MAX+44)) enfants ont ete acceptes (limite $MAX) — le garde-fou ne mord pas"; exit 1
fi
if ! grep -qi 'too many sibling widgets' <<<"$sortie"; then
    echo "ECHEC : refus sans le message attendu (rc=$rc). Sortie :"; sed 's/^/    /' <<<"$sortie" | tail -3; exit 1
fi
echo "OK : sous $MAX la fenetre s'ouvre, au-dela le refus est net et nomme"
