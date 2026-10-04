#!/bin/sh
# SPDX-License-Identifier: GPL-2.0-or-later
# garde_fenetres_homonymes.sh — deux fenêtres ouvertes en même temps qui
# déclarent la même variable ne doivent pas se marcher dessus.
#
# Le défaut : la seconde fenêtre reprenait la variable de la première. Le
# pointeur de widget de la première était écrasé EN SILENCE ; à la sortie,
# NOM valait « deux » au lieu de « un ». Mesuré le 2026-10-04 sur le port
# gtk4, dont la copie du cœur n'avait pas le correctif que gtk3 portait déjà.
#
# Attendu : la première fenêtre garde NOM ; la seconde est rangée sous
# NOM__W<id>, et un avertissement le dit.
#
# La seconde fenêtre s'ouvre par l'action d'une barre de progression arrivée à
# 100 : un minuteur n'a pas le droit de faire « launch », et il n'y a personne
# pour cliquer.
#
# Usage : garde_fenetres_homonymes.sh <binaire-sermo>
# Codes : ceux de tests/comportement/run.sh (0 vert · 1 écart · 77 rien vérifié)
set -u
ICI=$(CDPATH= cd -- "$(dirname -- "$0")" && pwd)
BIN="${1:-${SERMO_BIN:-}}"
[ -n "$BIN" ] && [ -x "$BIN" ] || { echo "IGNORÉ : binaire sermo introuvable. Rien vérifié." >&2; exit 77; }
[ -f "$ICI/fenetres/seconde.xml" ] || { echo "IGNORÉ : tests/fenetres/seconde.xml absent. Rien vérifié." >&2; exit 77; }
SECONDE=$(cat "$ICI/fenetres/seconde.xml")
export SECONDE
CAS="$ICI/fenetres/cas" TIMEOUT="${TIMEOUT:-20}" sh "$ICI/comportement/run.sh" "$BIN"
