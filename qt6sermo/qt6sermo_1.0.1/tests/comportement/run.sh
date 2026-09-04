#!/bin/sh
# run.sh — banc de COMPORTEMENT du port qt6, étalonné sur gtk3sermo.
#
# POURQUOI. Le port qt6 passe par le shim qt6-compat.h, qui portait des leurres
# (g_strsplit->NULL, g_path_get_basename->identité, corrigés en M1). Compiler ne
# prouve donc RIEN sur ce qu'un widget RESTITUE. Ce banc joue chaque cas et
# compare la valeur exportée au shell à celle du port de RÉFÉRENCE gtk3sermo —
# la même syntaxe doit rendre la même valeur, c'est la promesse « écrit une
# fois, tourne sur GTK ou Qt ».
#
# LES .attendu SONT L'ÉTALON gtk3sermo. Ils ne sont PAS adaptés à qt6 : ce serait
# consacrer la divergence au lieu de la corriger. Le banc est donc VOLONTAIREMENT
# ROUGE tant que le port qt6 n'a pas atteint la parité (jalon M3). Chaque widget
# réparé le fait passer du rouge au vert. Vert = parité atteinte.
#
# ⛔ Ne JAMAIS « corriger » un .attendu vers ce que qt6 rend aujourd'hui pour
#    faire taire un échec — ce serait enshriner le bug. On répare le port.
#
# Binaire testé : $QT6_BIN si posé, sinon un build voisin, sinon `qt6sermo` du
# PATH. Étalon (optionnel) : gtk3sermo, pour re-dériver un .attendu manquant.
#
# Codes : 0 tout au vert (parité) · 1 au moins un écart ou un blocage ·
#         77 binaire qt6sermo ou xvfb absent — rien n'a été vérifié.

set -u
ICI=$(CDPATH= cd -- "$(dirname -- "$0")" && pwd)
CAS="$ICI/cas"
TIMEOUT="${TIMEOUT:-20}"

# Trouver le binaire qt6sermo à tester.
QB="${QT6_BIN:-}"
if [ -z "$QB" ]; then
    for c in "$ICI/../../build/qt6sermo" "$ICI/../../../../build/qt6sermo"; do
        [ -x "$c" ] && { QB="$c"; break; }
    done
fi
[ -n "$QB" ] || QB=$(command -v qt6sermo 2>/dev/null || true)

if [ -z "$QB" ] || [ ! -x "$QB" ]; then
    echo "IGNORÉ : binaire qt6sermo introuvable (poser QT6_BIN). Rien vérifié." >&2
    exit 77
fi
command -v xvfb-run >/dev/null 2>&1 || {
    echo "IGNORÉ : xvfb-run absent (paquet xvfb). Rien vérifié." >&2; exit 77; }

TMP=$(mktemp -d); trap 'rm -rf "$TMP"' EXIT
echo "Banc comportement qt6 — binaire : $QB"

n=0; ok=0; diff=0; hang=0
for x in "$CAS"/*.xml; do
    [ -f "$x" ] || continue
    nom=$(basename "$x" .xml)
    att="$CAS/$nom.attendu"
    [ -f "$att" ] || { echo "IGNORÉ  $nom : pas de .attendu"; continue; }
    n=$((n + 1))

    MAIN_DIALOG="$(cat "$x")" QT_QPA_PLATFORM=xcb GTK_A11Y=none \
        xvfb-run -a timeout "$TIMEOUT" "$QB" --program=MAIN_DIALOG \
        >"$TMP/brut" 2>"$TMP/err"
    rc=$?

    grep -aE '^[A-Za-z_][A-Za-z0-9_]*=' "$TMP/brut" 2>/dev/null \
        | grep -v '^EXIT=' | LC_ALL=C sort > "$TMP/out"

    if [ "$rc" -eq 124 ]; then
        printf 'BLOQUÉ  %-22s (timeout %ss — le dialogue ne s est pas fermé)\n' "$nom" "$TIMEOUT"
        hang=$((hang + 1)); continue
    fi
    if diff -q "$att" "$TMP/out" >/dev/null 2>&1; then
        printf 'ok      %-22s\n' "$nom"; ok=$((ok + 1))
    else
        printf 'ÉCART   %-22s\n' "$nom"
        diff "$att" "$TMP/out" 2>/dev/null | grep -E '^[<>]' \
            | sed 's/^</  étalon:/; s/^>/  qt6   :/'
        diff=$((diff + 1))
    fi
done

echo
if [ "$n" -eq 0 ]; then
    echo "ÉCHEC : aucun cas joué — le dossier cas/ est vide ou cassé." >&2
    exit 1
fi
printf '%s cas · %s au vert · %s en écart · %s bloqué(s).\n' "$n" "$ok" "$diff" "$hang"
if [ "$ok" -eq "$n" ]; then
    echo "PARITÉ atteinte : qt6 rend les mêmes valeurs que l étalon gtk3sermo."
    exit 0
fi
echo "Port qt6 PAS encore à parité — écarts ci-dessus = liste de travail M3." >&2
exit 1
