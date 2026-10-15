#!/bin/bash
# SPDX-License-Identifier: GPL-2.0-or-later
# tests/garde_efl1_sans_fenetre.sh — efl1 sort proprement quand EFL ne peut créer
# aucune fenêtre.
#
# POURQUOI CE BANC EXISTE
#
# Quand EFL ne crée pas la fenêtre (écran injoignable, ou moteur opengl_x11 en
# échec sous Xvfb chargé : 2 à 3 lancements sur 100, mesuré le 2026-10-12),
# efl1 continuait avec un parent NULL et le cœur s'arrêtait net :
# « ASSERT FAILED: widget != NULL », abandon, code 134. Un script appelant ne
# savait pas pourquoi.
#
# Ce banc pointe DISPLAY sur un écran qui n'existe pas (le déclencheur sûr du
# même chemin) et exige une sortie nette : code 1, une ligne qui dit pourquoi,
# ni abandon ni assertion.
#
# Usage : garde_efl1_sans_fenetre.sh <chemin/vers/efl1sermo>
# Codes : 0 = conforme · 1 = écart · 2 = usage/outil

set -u
BIN="${1:-}"
[[ -x "$BIN" ]] || { echo "usage: $0 <chemin/vers/efl1sermo>" >&2; exit 2; }
command -v timeout >/dev/null || { echo "outil manquant : timeout" >&2; exit 2; }

# Un numéro d'écran libre : ni socket X, ni verrou.
n=987
while [ -e "/tmp/.X11-unix/X$n" ] || [ -e "/tmp/.X$n-lock" ]; do n=$((n + 1)); done

DIALOG='<window><vbox><checkbox><label>x</label><variable>V</variable></checkbox></vbox></window>'
sortie=$(env -u WAYLAND_DISPLAY -u ELM_ENGINE -u ELM_DISPLAY DISPLAY=":$n" DIALOG="$DIALOG" \
         timeout 30 "$BIN" --program=DIALOG 2>&1)
rc=$?

echecs=0
[ "$rc" -eq 1 ] || { echo "ÉCHEC : code $rc au lieu de 1 (134 = abandon, 124 = figé)"; echecs=1; }
case "$sortie" in *"ASSERT FAILED"*) echo "ÉCHEC : le cœur s'est arrêté sur une assertion"; echecs=1 ;; esac
case "$sortie" in *"impossible de créer la fenêtre"*) ;; *) echo "ÉCHEC : aucune ligne ne dit pourquoi le programme s'arrête"; echecs=1 ;; esac
[ "$echecs" -eq 0 ] || { printf '%s\n' "$sortie" | tail -5 | sed 's/^/  │ /'; exit 1; }
echo "garde_efl1_sans_fenetre : OK — écran :$n injoignable : sortie nette en code 1, la raison est dite."
