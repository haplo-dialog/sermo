#!/bin/bash
# SPDX-License-Identifier: GPL-2.0-or-later
# tests/garde_fuite_refresh.sh — haplo-dialog — 2026 — GPL-2.0-or-later
#
# Fuite par action : chaque action qui exporte les variables (commande,
# refresh) lit chaque widget par widget_get_text_value(), qui rend une chaîne
# ALLOUÉE (contrat, widgets.h). Le cœur ne la libérait pas : ~31 octets perdus
# par widget et par action (mesuré le 15/10, R4), des Mo par jour pour un
# tableau de bord rafraîchi chaque seconde.
#
# Mesure : un dialogue joue 51 tours (refresh + commande) sous LeakSanitizer ;
# on compte les octets perdus dont l'allocation part de l'export des variables
# (_variables_export, print_variables, append_fromto_variable). Il doit n'y en
# avoir aucun. Port sans toolkit (ncurses en mode batch, ou html) : pas de
# bruit de toolkit. Les fuites de l'analyseur XML (fixes, liées à la taille du
# fichier) ne sont pas jugées ici.
# TÉMOIN : un programme qui fuit, construit avec les mêmes options, DOIT être
# signalé ; sinon LeakSanitizer est muet et rien n'est vérifié (code 77).
#
# Usage : garde_fuite_refresh.sh <ncursessermo|htmlsermo construit sous ASan>
#         (garde_sanitizers.sh les construit dans sermo-backend-*/_build_asan)
# Codes : 0 = aucune fuite d'export · 1 = fuite · 2 = outil absent · 77 = rien vérifié
set -u
export LC_ALL=C
RACINE=$(CDPATH= cd -- "$(dirname -- "$0")/.." && pwd) || exit 2
BIN=${1:-}
[ -n "$BIN" ] && [ -x "$BIN" ] || { echo "IGNORÉ : binaire absent — rien vérifié"; exit 77; }
BIN=$(CDPATH= cd -- "$(dirname -- "$BIN")" && pwd)/$(basename -- "$BIN")
nm "$BIN" 2>/dev/null | grep -q __asan_init || { echo "IGNORÉ : $BIN n'est pas construit sous ASan — rien vérifié"; exit 77; }
J="$RACINE/_build/fuite-refresh"; rm -rf "$J"; mkdir -p "$J" || exit 2
unset DISPLAY WAYLAND_DISPLAY
# En build combiné, ASan écrit dans le log_path d'UBSAN_OPTIONS : les deux au même endroit.
export ASAN_OPTIONS="detect_leaks=1:log_path=$J/asan"
export UBSAN_OPTIONS="print_stacktrace=1:log_path=$J/asan"
export SERMO_NCURSES_BATCH=1 SERMO_HTML_OUT=/dev/null

printf '#include <stdlib.h>\nvoid *volatile p;\nint main(void){p=malloc(64);p=0;return 0;}\n' > "$J/temoin.c"
cc -fsanitize=address,undefined -g -o "$J/temoin" "$J/temoin.c" 2>"$J/temoin-cc.log" || { echo "outil absent : compilateur avec -fsanitize"; exit 2; }
rm -f "$J"/asan.*; "$J/temoin" >/dev/null 2>&1
cat "$J"/asan.* 2>/dev/null | grep -q '^SUMMARY: AddressSanitizer: [0-9]* byte(s) leaked' \
    || { echo "IGNORÉ : le témoin qui fuit n'est pas signalé — rien vérifié"; exit 77; }

{
    echo '<window><vbox>'
    echo '<entry><variable>E</variable><default>abcdefghijklmnop</default></entry>'
    echo '<text><variable>T</variable><label>texte</label></text>'
    echo '<timer visible="false"><variable>_R</variable>'
    for _ in $(seq 51); do echo '<action>refresh:E</action><action>refresh:T</action><action>true</action>'; done
    echo '<action>exit:fin</action></timer></vbox></window>'
} > "$J/tours.xml"
rm -f "$J"/asan.*
( cd "$J" && timeout 60 "$BIN" -f tours.xml </dev/null >"$J/sortie.txt" 2>"$J/erreur.txt" )
grep -q '^E="abcdefghijklmnop"' "$J/sortie.txt" || { echo "ÉCHEC : le dialogue n'est pas allé au bout (voir $J)"; exit 1; }
cat "$J"/asan.* > "$J/rapport.txt" 2>/dev/null || : > "$J/rapport.txt"
# Octets des blocs dont le 1er appelant (#1) est l'export des variables.
perdus=$(awk '/^(Direct|Indirect) leak/{b=$4; f=1; next}
              f && /#1 /{ if ($0 ~ /_variables_export|print_variables|append_fromto_variable/) t += b; f = 0 }
              END{print t+0}' "$J/rapport.txt")
echo "perdus par l'export des variables, 51 tours : $perdus octets"
[ "$perdus" = 0 ] || { echo "ÉCHEC : fuite à l'export des variables (voir $J/rapport.txt)"; exit 1; }
echo "OK : aucune fuite à l'export des variables"
