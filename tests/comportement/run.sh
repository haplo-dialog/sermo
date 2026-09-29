#!/bin/sh
# SPDX-License-Identifier: GPL-2.0-or-later
# run.sh — banc de COMPORTEMENT générique, pour N'IMPORTE QUEL backend sermo.
#
# POURQUOI CE RUNNER. Le banc historique ne savait tester que qt6
# (sermo-backend-qt6/tests/comportement/run.sh, variable QT6_BIN). Le « 24/24 »
# des autres backends n'était donc PAS rejouable — un chiffre affirmé, pas
# mesuré. Ce runner joue le MÊME corpus (les .attendu = l'étalon gtk3sermo) sur
# le binaire qu'on lui donne, en posant l'environnement HEADLESS propre à chaque
# toolkit. La promesse « écrit une fois, tourne partout » se vérifie alors sur
# les sept ports, pas sur un seul.
#
# L'ÉTALON reste les valeurs de gtk3sermo. On ne « corrige » jamais un .attendu
# vers ce qu'un backend rend aujourd'hui : on répare le backend.
#
# Usage :  run.sh <binaire-sermo>        (ou SERMO_BIN=<binaire> run.sh)
# Env   :  CAS=<dossier cas>   (défaut : le corpus partagé sous sermo-backend-qt6)
#          TIMEOUT=<s>         (défaut : 20)
# Codes :  0 = parité (tout au vert) · 1 = écart/blocage · 77 = binaire ou
#          outil (xvfb) absent, rien vérifié.

set -u
ICI=$(CDPATH= cd -- "$(dirname -- "$0")" && pwd)
CAS="${CAS:-$ICI/../../sermo-backend-qt6/tests/comportement/cas}"
TIMEOUT="${TIMEOUT:-20}"

BIN="${1:-${SERMO_BIN:-}}"
if [ -z "$BIN" ] || [ ! -x "$BIN" ]; then
    echo "IGNORÉ : binaire sermo introuvable (donner <binaire> ou SERMO_BIN). Rien vérifié." >&2
    exit 77
fi
[ -d "$CAS" ] || { echo "IGNORÉ : corpus '$CAS' absent. Rien vérifié." >&2; exit 77; }
# Chemins absolus : chaque cas tourne dans son propre dossier de travail.
CAS=$(CDPATH= cd -- "$CAS" && pwd)
BIN=$(CDPATH= cd -- "$(dirname -- "$BIN")" && pwd)/$(basename -- "$BIN")

# Environnement HEADLESS, déduit du nom du binaire. Les backends à plateforme
# offscreen (qt6/sdl3/efl1) et le terminal (ncurses en mode batch) tournent SANS
# serveur X ; les toolkits X (gtk3/gtk4/fltk1) passent par xvfb.
base=$(basename "$BIN")
ENV_PRE=""      # variables à exporter devant la commande
USE_XVFB=0
case "$base" in
    qt6sermo)                 ENV_PRE="QT_QPA_PLATFORM=offscreen" ;;
    sdl3sermo|sdl3dialog)     ENV_PRE="SDL_VIDEODRIVER=offscreen" ;;
    efl1sermo|efl1dialog)     ENV_PRE="ELM_ENGINE=buffer" ;;
    ncursessermo)             ENV_PRE="SERMO_NCURSES_BATCH=1" ;;
    *)                        ENV_PRE="GSK_RENDERER=cairo GTK_A11Y=none" ; USE_XVFB=1 ;;
esac
if [ "$USE_XVFB" -eq 1 ]; then
    command -v xvfb-run >/dev/null 2>&1 || {
        echo "IGNORÉ : xvfb-run absent (paquet xvfb) — requis pour $base. Rien vérifié." >&2
        exit 77; }
fi

TMP=$(mktemp -d); trap 'rm -rf "$TMP"' EXIT
echo "Banc comportement — binaire : $BIN"

n=0; ok=0; diff=0; hang=0
for x in "$CAS"/*.xml; do
    [ -f "$x" ] || continue
    nom=$(basename "$x" .xml)
    att="$CAS/$nom.attendu"
    [ -f "$att" ] || { echo "IGNORÉ  $nom : pas de .attendu"; continue; }
    n=$((n + 1))

    # Chaque cas joue dans un dossier de travail NEUF, garni d'une copie de
    # cas/donnees/ : un cas peut lire un fichier par un chemin relatif
    # (<input file>donnees/…) ou compter ce qu'une commande a écrit, sans
    # dépendre du dossier d'où l'on lance le banc ni des cas précédents.
    rm -rf "$TMP/travail"; mkdir "$TMP/travail"
    [ -d "$CAS/donnees" ] && cp -R "$CAS/donnees" "$TMP/travail/"
    if [ "$USE_XVFB" -eq 1 ]; then
        ( cd "$TMP/travail" && MAIN_DIALOG="$(cat "$x")" env $ENV_PRE \
            xvfb-run -a timeout "$TIMEOUT" "$BIN" --program=MAIN_DIALOG ) \
            >"$TMP/brut" 2>"$TMP/err"
        rc=$?
    else
        ( cd "$TMP/travail" && MAIN_DIALOG="$(cat "$x")" env $ENV_PRE \
            timeout "$TIMEOUT" "$BIN" --program=MAIN_DIALOG ) \
            >"$TMP/brut" 2>"$TMP/err" </dev/null
        rc=$?
    fi

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
            | sed 's/^</  étalon:/; s/^>/  rendu :/'
        diff=$((diff + 1))
    fi
done

echo
if [ "$n" -eq 0 ]; then
    echo "ÉCHEC : aucun cas joué — le corpus est vide ou cassé." >&2
    exit 1
fi
printf '%s cas · %s au vert · %s en écart · %s bloqué(s).\n' "$n" "$ok" "$diff" "$hang"
if [ "$ok" -eq "$n" ]; then
    echo "PARITÉ atteinte : $base rend les mêmes valeurs que l étalon gtk3sermo."
    exit 0
fi
exit 1
