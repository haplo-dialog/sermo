#!/bin/bash
# SPDX-License-Identifier: GPL-2.0-or-later
# tests/garde_append_edit.sh — haplo-dialog — 2026
#
# append:SOURCE,CIBLE vers un <edit> garde TOUT le texte, accents compris.
# Jusqu'à la 3.2.0 la longueur passée à GTK était un nombre de CARACTÈRES là où
# il attend des OCTETS : « éèàùç-fin » (9 caractères, 14 octets) était coupé au
# milieu d'un caractère, et GTK n'insérait rien.
#
# Ports GTK seulement : append: n'a pas d'effet sur les sept autres (écart
# connu, hors de ce banc).
#
# Usage : garde_append_edit.sh <gtk3sermo|gtk4sermo>
# Codes : 0 = tient · 1 = défaut · 2 = outil ou binaire manquant
set -u
BIN="${1:?usage: garde_append_edit.sh /chemin/vers/binaire}"
[ -x "$BIN" ] || { echo "binaire introuvable : $BIN" >&2; exit 2; }
command -v xvfb-run >/dev/null || { echo "outil manquant : xvfb-run" >&2; exit 2; }
export MAIN_DIALOG='<window><vbox><edit><variable>ED</variable><default>a</default></edit><entry><variable>SRC</variable><default>éèàùç-fin</default></entry><timer visible="false"><variable>T</variable><action>append:SRC,ED</action><action>exit:fin</action></timer></vbox></window>'
v=$(GSK_RENDERER=cairo GTK_A11Y=none xvfb-run -a timeout 25 "$BIN" --program=MAIN_DIALOG 2>/dev/null | grep -a '^ED=')
[ "$v" = 'ED="aéèàùç-fin"' ] && { echo "OK : append: vers <edit> garde le texte accentué entier"; exit 0; }
echo "ECHEC : append: vers <edit> rend [$v] au lieu de ED=\"aéèàùç-fin\""; exit 1
