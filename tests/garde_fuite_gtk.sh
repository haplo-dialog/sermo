#!/bin/bash
# SPDX-License-Identifier: GPL-2.0-or-later
# tests/garde_fuite_gtk.sh — haplo-dialog — 2026 — GPL-2.0-or-later
#
# Fuites des widgets GTK : GTK rend des COPIES que l'appelant doit libérer
# (gtk_tree_model_get d'une colonne de chaînes, gtk_combo_box_text_get_active_text,
# gtk_text_buffer_get_text). Jusqu'à la 3.3.0, six endroits de gtk3sermo et de
# gtk4sermo les gardaient :
#   - le rendu de l'icône d'une rangée de <tree> : deux chaînes par rangée
#     visible et par redessin (défilement, survol) ;
#   - le tri naturel d'un <tree> : deux chaînes par comparaison ;
#   - une rangée de <list> remplie par une commande : son texte, à chaque refresh ;
#   - le texte actif d'un <comboboxtext>, à chaque refresh ;
#   - le contenu d'un <edit>, à chaque save: ;
#   - les titres d'onglets d'un <notebook>.
# Un tableau de bord rafraîchi chaque seconde y perdait des Mo par jour.
#
# Mesure : un dialogue joue 30 tours (refresh de la liste, de la liste
# déroulante et de l'arbre trié, save: de l'<edit>) sous LeakSanitizer ; on
# compte les octets perdus dont la pile d'allocation passe par l'un de ces
# widgets. Il ne doit y en avoir aucun. Les fuites du toolkit lui-même ne sont
# pas jugées (leur pile ne passe pas par ces fonctions).
# Le tri naturel n'est pas exercé ici (il faut un clic sur un en-tête de
# colonne) : son correctif est de la même forme que les cinq autres.
# TÉMOIN : un programme qui fuit, construit avec les mêmes options, DOIT être
# signalé ; sinon LeakSanitizer est muet et rien n'est vérifié (code 77).
#
# Usage : garde_fuite_gtk.sh <gtk3sermo|gtk4sermo construit sous ASan>
#         (garde_sanitizers.sh les construit dans sermo-backend-*/_build_asan)
# Codes : 0 = aucune fuite · 1 = fuite · 2 = outil absent · 77 = rien vérifié
set -u
export LC_ALL=C
RACINE=$(CDPATH= cd -- "$(dirname -- "$0")/.." && pwd) || exit 2
BIN=${1:-}
[ -n "$BIN" ] && [ -x "$BIN" ] || { echo "IGNORÉ : binaire absent — rien vérifié"; exit 77; }
BIN=$(CDPATH= cd -- "$(dirname -- "$BIN")" && pwd)/$(basename -- "$BIN")
nm "$BIN" 2>/dev/null | grep -q __asan_init || { echo "IGNORÉ : $BIN n'est pas construit sous ASan — rien vérifié"; exit 77; }
command -v xvfb-run >/dev/null 2>&1 || { echo "outil absent : xvfb-run"; exit 2; }
J="$RACINE/_build/fuite-gtk-$(basename -- "$BIN")"; rm -rf "$J"; mkdir -p "$J" || exit 2
# Piles complètes : GLib est construit sans pointeur de cadre, le déroulement
# rapide s'arrêterait à g_malloc et la pile ne nommerait jamais nos fonctions.
export ASAN_OPTIONS="detect_leaks=1:fast_unwind_on_malloc=0:malloc_context_size=30:log_path=$J/asan"
export UBSAN_OPTIONS="print_stacktrace=1:log_path=$J/asan"
export G_SLICE=always-malloc

printf '#include <stdlib.h>\nvoid *volatile p;\nint main(void){p=malloc(64);p=0;return 0;}\n' > "$J/temoin.c"
cc -fsanitize=address,undefined -g -o "$J/temoin" "$J/temoin.c" 2>"$J/temoin-cc.log" || { echo "outil absent : compilateur avec -fsanitize"; exit 2; }
rm -f "$J"/asan.*; "$J/temoin" >/dev/null 2>&1
cat "$J"/asan.* 2>/dev/null | grep -q '^SUMMARY: AddressSanitizer: [0-9]* byte(s) leaked' \
    || { echo "IGNORÉ : le témoin qui fuit n'est pas signalé — rien vérifié"; exit 77; }

for i in $(seq 40); do echo "ligne numéro $i"; done > "$J/lignes.txt"
for i in $(seq 40); do echo "gtk-ok|fichier$((41 - i)).txt|$i"; done > "$J/arbre.txt"
printf 'un\ndeux\ntrois\n' > "$J/choix.txt"
{
    echo '<window><vbox>'
    echo '<notebook tab-labels="Listes|Texte"><vbox>'
    echo '<list><variable>L</variable><input>cat lignes.txt</input></list>'
    echo '<comboboxtext><variable>C</variable><input>cat choix.txt</input></comboboxtext>'
    echo '<tree column-sort-function="1|1" sort-column="0" stock-column="0"><variable>A</variable>'
    echo '<label>Nom|Rang</label><height>200</height><input stock-column="0">cat arbre.txt</input></tree>'
    echo '</vbox><vbox>'
    echo '<edit><variable>E</variable><default>contenu de la zone de texte</default>'
    echo '<output file>sauve.txt</output></edit>'
    echo '</vbox></notebook>'
    echo '<timer milliseconds="true" interval="100" visible="false"><variable>_R</variable>'
    echo '<action>clear:L</action><action>refresh:L</action><action>refresh:C</action><action>refresh:A</action><action>save:E</action>'
    echo '<action>sh -c '"'"'n=$(cat tours 2>/dev/null || echo 0); echo $((n + 1)) > tours'"'"'</action>'
    echo '<action condition="command_is_true([ $(cat tours) -ge 30 ] && echo true)">exit:fin</action>'
    echo '</timer></vbox></window>'
} > "$J/tours.xml"
rm -f "$J"/asan.* "$J/tours"
( cd "$J" && xvfb-run -a timeout 120 "$BIN" -f tours.xml </dev/null >"$J/sortie.txt" 2>"$J/erreur.txt" )
# Le compteur de tours fait foi, pas la ligne EXIT : quand LeakSanitizer trouve
# une fuite (celles du toolkit suffisent), il quitte sans vider la sortie.
[ "$(cat "$J/tours" 2>/dev/null)" = 30 ] || { echo "ÉCHEC : le dialogue n'est pas allé au bout (voir $J)"; exit 1; }
[ "$(cat "$J/sauve.txt" 2>/dev/null)" = "contenu de la zone de texte" ] \
    || { echo "ÉCHEC : save: n'a pas écrit l'<edit> (voir $J)"; exit 1; }
cat "$J"/asan.* > "$J/rapport.txt" 2>/dev/null || : > "$J/rapport.txt"
grep -q '^SUMMARY: AddressSanitizer' "$J/rapport.txt" || grep -q 'LeakSanitizer' "$J/rapport.txt" \
    || { echo "IGNORÉ : aucun rapport de LeakSanitizer — rien vérifié"; exit 77; }
# Octets des blocs dont la pile passe par l'un des widgets jugés.
awk '/^(Direct|Indirect) leak/{ if (f && vu) t[vu] += b; b=$4; f=1; vu=""; next }
     f && /^$/ { if (vu) t[vu] += b; f=0; vu=""; next }
     f && match($0, /widget_(tree|list|comboboxtext|edit|notebook)[a-z_]*/) && !vu { vu=substr($0, RSTART, RLENGTH) }
     END{ if (f && vu) t[vu] += b; for (k in t) printf "%d %s\n", t[k], k }' "$J/rapport.txt" | sort -rn > "$J/perdus.txt"
perdus=$(awk '{t += $1} END{print t+0}' "$J/perdus.txt")
echo "perdus par les widgets GTK, 30 tours : $perdus octets"
if [ "$perdus" != 0 ]; then
    sed 's/^/  /' "$J/perdus.txt"
    echo "ÉCHEC : fuite dans un widget GTK (voir $J/rapport.txt)"; exit 1
fi
echo "garde_fuite_gtk : OK ($(basename -- "$BIN"))"
