#!/bin/bash
# SPDX-License-Identifier: GPL-2.0-or-later
# tests/garde_html_valeurs_page.sh — haplo-dialog — 2026 — GPL-2.0-or-later
#
# La page écrite par htmlsermo montre les vraies valeurs de ses barres et de
# ses curseurs, quelle que soit la langue du système.
#
# Deux défauts trouvés le 2026-10-13 par la galerie du site (« Progression
# (doit afficher 65 %) » : barre vide) :
#  1. la page était écrite AVANT que la barre lise sa commande <input> : elle
#     montrait toujours 0 ;
#  2. les nombres de la page passaient par fprintf(« %g »), qui suit la langue :
#     sous fr_FR, value="0,65" et step="0,5" — que les navigateurs refusent.
#
# Ce garde rend une <progressbar> (« echo 65 ») et un <hscale> (2.5, pas de
# 0.5) sous une langue à virgule décimale, puis lit la page : value="0.65"
# pour la barre, value="2.5" et step="0.5" pour le curseur. TÉMOIN : la langue
# à virgule existe vraiment (printf %g y écrit « 0,5 ») ; sans elle, le garde
# ne mesurerait rien et le dit (code 2, sauté).
#
# Usage : garde_html_valeurs_page.sh <htmlsermo>
# Codes : 0 = la page est juste · 1 = un défaut · 2 = outil ou langue manquants

set -u
BIN="${1:?usage: garde_html_valeurs_page.sh /chemin/vers/htmlsermo}"
[[ -x "$BIN" ]] || { echo "binaire introuvable : $BIN" >&2; exit 2; }
BIN=$(readlink -f -- "$BIN")

LANGUE=""
for l in fr_FR.UTF-8 fr_FR.utf8 de_DE.UTF-8 de_DE.utf8; do
    if [ "$(env LC_ALL=$l /usr/bin/printf '%g' 0.5 2>/dev/null)" = "0,5" ]; then LANGUE=$l; break; fi
done
[ -n "$LANGUE" ] || { echo "SAUTÉ : aucune langue à virgule décimale installée (fr_FR, de_DE) — rien de mesuré"; exit 2; }

T=$(mktemp -d); trap 'rm -rf "$T"' EXIT
export GARDE_VALEURS='<window><vbox>
<progressbar><input>echo 65</input><variable>PROG</variable></progressbar>
<hscale range-min="0" range-max="10" range-step="0.5"><default>2.5</default><variable>H</variable></hscale>
<button ok></button></vbox></window>'
( cd "$T" && LC_ALL=$LANGUE SERMO_HTML_OUT="$T/page.html" timeout 30 "$BIN" --program=GARDE_VALEURS ) >/dev/null 2>"$T/err"
[ -s "$T/page.html" ] || { echo "ECHEC : aucune page écrite"; cat "$T/err"; exit 1; }

echecs=0
verifie() {  # verifie <motif> <ce que ça prouve>
    if grep -q -- "$1" "$T/page.html"; then :; else
        echo "ECHEC : $2 — attendu $1, vu : $(grep -o '<progress[^>]*>\|<input type="range"[^>]*>' "$T/page.html" | tr '\n' ' ')"
        echecs=$((echecs + 1))
    fi
}
verifie '<progress class="sermo-progress" value="0.65" max="1">' "la barre montre ce que sa commande a écrit (65 %), avec un point"
verifie 'type="range" disabled value="2.5" min="0" max="10" step="0.5"' "le curseur écrit ses nombres avec un point"

[ "$echecs" -eq 0 ] || exit 1
echo "OK : barre à 0.65 et curseur à 2.5 (pas 0.5) dans la page, sous $LANGUE (témoin : %g y écrit « 0,5 »)"
