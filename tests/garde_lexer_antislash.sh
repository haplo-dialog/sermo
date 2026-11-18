#!/bin/bash
# SPDX-License-Identifier: GPL-2.0-or-later
# tests/garde_lexer_antislash.sh — haplo-dialog — 2026
#
# Une barre oblique inverse dans un attribut entre guillemets ne fait rien
# perdre : title="debut\xfin" reste « debut\xfin ». Jusqu'à la 3.2.0 aucune
# règle du lexer ne prenait ce « \ » : tout le texte qui le précédait
# disparaissait (« xfin »), et le « \ » partait sur la sortie standard, au
# milieu des variables. « \" » reste un guillemet dans la valeur.
#
# Lu dans la page de htmlsermo (le titre y est écrit tel quel).
# Usage : garde_lexer_antislash.sh <htmlsermo>
# Codes : 0 = tient · 1 = défaut · 2 = binaire manquant
set -u
BIN="${1:?usage: garde_lexer_antislash.sh /chemin/vers/htmlsermo}"
[ -x "$BIN" ] || { echo "binaire introuvable : $BIN" >&2; exit 2; }
T=$(mktemp -d) || exit 2; trap 'rm -rf "$T"' EXIT
printf '%s' '<window title="debut\xfin C:\dossier\"cite"><vbox><text><label>t</label></text></vbox></window>' >"$T/d.xml"
SERMO_HTML_OUT="$T/page.html" "$BIN" -f "$T/d.xml" >"$T/sortie" 2>"$T/err"
titre=$(grep -ao '<title>[^<]*</title>' "$T/page.html" 2>/dev/null)
case "$titre" in
    *'debut\xfin C:\dossier'*) ;;
    *) echo "ECHEC : titre rendu [$titre] ; « debut\\xfin C:\\dossier » attendu"; exit 1 ;;
esac
grep -q '^\\' "$T/sortie" && { echo "ECHEC : un « \\ » de l'attribut est parti sur la sortie standard"; exit 1; }
echo "OK : « \\ » dans un attribut entre guillemets : rien n'est perdu, rien ne fuit sur la sortie"
