#!/bin/bash
# SPDX-License-Identifier: GPL-2.0-or-later
# tests/garde_sortie_variables_seules.sh — haplo-dialog — 2026
#
# La sortie standard ne porte QUE des lignes NOM="valeur" : le manuel invite à
# la passer à `eval`. Jusqu'à la 3.3.0, sdl3sermo y écrivait un diagnostic
# d'ImGui quand un widget se grisait lui-même pendant son rendu (une minuterie
# ou une case dont l'action est « disable: » de soi-même) :
#   [00013] [imgui-error] In window 'sdl3sermo': Calling EndDisabled() too many times!
# L'état « grisé » était lu deux fois, avant et après l'action.
#
# Mesure : une minuterie se désactive elle-même, une case aussi (activée par
# une seconde minuterie), puis le dialogue sort. Toute ligne de la sortie qui
# n'est pas NOM="valeur" est un échec.
#
# Usage : garde_sortie_variables_seules.sh <binaire d'un port>
# Codes : 0 = tient · 1 = défaut · 2 = outil ou binaire manquant
set -u
BIN="${1:?usage: garde_sortie_variables_seules.sh /chemin/vers/binaire}"
[ -x "$BIN" ] || { echo "binaire introuvable : $BIN" >&2; exit 2; }
command -v xvfb-run >/dev/null || { echo "outil manquant : xvfb-run" >&2; exit 2; }
BIN=$(CDPATH= cd -- "$(dirname -- "$BIN")" && pwd)/$(basename -- "$BIN")
T=$(mktemp -d) || exit 2; trap 'rm -rf "$T"' EXIT
cd "$T" || exit 2
export SERMO_NCURSES_BATCH=1 SERMO_HTML_OUT=/dev/null
export MAIN_DIALOG='<window><vbox>
<checkbox><label>case</label><variable>C</variable><action>disable:C</action></checkbox>
<timer milliseconds="true" interval="200" visible="false"><variable>T</variable><action>disable:T</action></timer>
<timer milliseconds="true" interval="600" visible="false"><variable>A</variable><action>activate:C</action></timer>
<timer interval="2" visible="false"><variable>F</variable><action>exit:fin</action></timer>
</vbox></window>'
GSK_RENDERER=cairo GTK_A11Y=none xvfb-run -a timeout 25 "$BIN" --program=MAIN_DIALOG >sortie 2>err
grep -q '^EXIT="fin"$' sortie || { echo "ECHEC : le dialogue n'est pas sorti par sa minuterie"; exit 1; }
grep -q '^T="false"$' sortie || { echo "ECHEC : la minuterie ne s'est pas désactivée (T=\"false\" attendu)"; exit 1; }
if grep -avE '^[A-Za-z_][A-Za-z0-9_]*="([^"\\$`]|\\.)*"$' sortie > intrus; then
    echo "ECHEC : la sortie standard porte autre chose que des variables :"
    sed 's/^/  /' intrus | head -5
    exit 1
fi
echo "OK : la sortie standard ne porte que des variables ($(wc -l < sortie) lignes)"
