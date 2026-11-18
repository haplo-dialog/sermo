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
# Le widget rangé sous NOM__W<id> doit être initialisé comme un autre : son
# <default> (NOM__W2="deux") et son <input> (P__W2="lu"). Jusqu'à la 3.3.0,
# gtk3 et gtk4 rafraîchissaient à sa place le widget de la première fenêtre,
# par le nom écrit dans le XML : le champ de la seconde s'ouvrait vide
# (NOM__W2="", mesuré le 2026-11-15). Joué sur les neuf ports.
#
# Cas 02 — une action jouée DANS la seconde fenêtre vise sa propre variable :
# sa commande lit $NOM de sa fenêtre (« deux »), « clear:NOM » vide son champ.
# Jusqu'à la 3.3.0 les deux visaient la première fenêtre (NOM="" et
# NOM__W2="deux" à la sortie, « un » lu par la commande).
#
# Cas 03 — l'auteur nomme lui-même une variable NOM__W2 dans la première
# fenêtre. La copie de la seconde est alors rangée sous NOM__W2_2 ; l'action
# de la seconde doit viser cette copie. Jusqu'à la 3.3.0 elle visait la
# variable de l'auteur (« auteur » lu, relecture externe du 16/11).
#
# Cas 04 — une fenêtre fermée par l'action d'un de ses propres widgets, alors
# que les actions suivantes du même widget visent encore ses widgets (clear,
# refresh) : rien ne plante, la suite est jouée, la première fenêtre continue.
# Ajouté après la relecture externe du 16/11, qui n'avait pas pu examiner ce
# chemin sur les sept ports sans GTK ; joué aussi sous ASan.
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
ACTIONS=$(cat "$ICI/fenetres/actions.xml")
FERME=$(cat "$ICI/fenetres/ferme.xml")
export SECONDE ACTIONS FERME
CAS="$ICI/fenetres/cas" TIMEOUT="${TIMEOUT:-20}" sh "$ICI/comportement/run.sh" "$BIN"
