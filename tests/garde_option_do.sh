#!/bin/bash
# tests/garde_option_do.sh — haplo-dialog — 2026 — GPL-2.0-or-later
#
# --do etait documentee dans les pages de manuel INSTALLEES et dans le manuel
# info, mais n'existait dans aucun binaire : qui recopiait l'exemple officiel
# obtenait un abandon, pas une erreur d'usage. Ce test verifie les quatre
# proprietes que la documentation promet.
BIN="${1:?usage: garde_option_do.sh /chemin/vers/binaire}"
[[ -x "$BIN" ]] || { echo "binaire introuvable : $BIN" >&2; exit 2; }
for t in xvfb-run timeout; do command -v "$t" >/dev/null || { echo "outil manquant : $t" >&2; exit 2; }; done

DIALOG='<window title="essai-do"><vbox><entry><default>Ada</default><variable>NOM</variable></entry><timer interval="1"><variable>T</variable><action>exit:OK</action></timer><button ok></button></vbox></window>'
export DIALOG

# 1 — l'option est annoncee par --help
"$BIN" --help 2>&1 | grep -q -- '--do=' || { echo "ECHEC : --do absente de --help"; exit 1; }

# 2 — la commande tourne, et voit les valeurs des widgets
out=$(timeout 25 xvfb-run -a "$BIN" --program=DIALOG --do='echo "SALUT $NOM"' 2>/dev/null)
grep -q '^SALUT Ada$' <<<"$out" || {
    echo "ECHEC : la commande --do n'a pas tourne, ou n'a pas vu \$NOM. Sortie :"
    sed 's/^/    /' <<<"$out"; exit 1; }

# 3 — l'ordre : la sortie de la commande vient APRES les lignes du dialogue,
#     sinon un script qui fait eval "$(...)" recoit les lignes melangees.
n_exit=$(grep -n '^EXIT=' <<<"$out" | head -1 | cut -d: -f1)
n_cmd=$(grep -n '^SALUT Ada$' <<<"$out" | head -1 | cut -d: -f1)
if [[ -z "$n_exit" || -z "$n_cmd" || "$n_cmd" -lt "$n_exit" ]]; then
    echo "ECHEC : sortie de --do avant celle du dialogue (tampon non vide). Sortie :"
    sed 's/^/    /' <<<"$out"; exit 1
fi

# 4 — le garde-fou du modele de securite mord aussi sur --do
err=$(HAPLO_NO_SHELL_FALLBACK=1 timeout 25 xvfb-run -a "$BIN" --program=DIALOG --do='echo "SALUT $NOM"' 2>&1 >/dev/null)
grep -qi 'fallback refused' <<<"$err" || {
    echo "ECHEC : HAPLO_NO_SHELL_FALLBACK=1 n'a pas refuse le repli shell de --do"; exit 1; }

echo "OK : --do annoncee, executee, ordonnee, et soumise au refus fail-closed"
