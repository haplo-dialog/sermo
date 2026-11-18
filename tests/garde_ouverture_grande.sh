#!/bin/bash
# SPDX-License-Identifier: GPL-2.0-or-later
# tests/garde_ouverture_grande.sh — haplo-dialog — 2026
#
# Un grand dialogue s'ouvre en un temps raisonnable. Deux coûts quadratiques
# ou répétés, mesurés avant la 3.3.0 :
#   - champs : chaque widget créé réexportait les variables de tous les
#     autres. 2000 <entry> : 7,5 s sur gtk3sermo (0,4 s pour 500). L'export
#     n'a plus lieu que pour un widget qui porte un <input>. Désormais 0,7 s.
#   - icônes : chaque recherche relisait index.theme du thème, de ses parents,
#     d'Adwaita et de hicolor. 400 boutons à icône : 12 s sur htmlsermo.
#     Les thèmes lus et les réponses sont gardés. Désormais 0,25 s.
# Seuil : 5 s pour chacun (large devant la mesure, en dessous de l'ancienne).
#
# Usage : garde_ouverture_grande.sh <binaire d'un port> [champs|icones]
# Codes : 0 = tient · 1 = défaut · 2 = outil ou binaire manquant
set -u
BIN="${1:?usage: garde_ouverture_grande.sh /chemin/vers/binaire [champs|icones]}"
QUOI="${2:-champs}"
[ -x "$BIN" ] || { echo "binaire introuvable : $BIN" >&2; exit 2; }
command -v xvfb-run >/dev/null || { echo "outil manquant : xvfb-run" >&2; exit 2; }
BIN=$(CDPATH= cd -- "$(dirname -- "$BIN")" && pwd)/$(basename -- "$BIN")
T=$(mktemp -d) || exit 2; trap 'rm -rf "$T"' EXIT
cd "$T" || exit 2
export SERMO_NCURSES_BATCH=1 SERMO_HTML_OUT=/dev/null
FIN='<timer milliseconds="true" interval="50" visible="false"><variable>_F</variable><action>exit:fin</action></timer>'
{
    echo '<window><hbox>'
    case "$QUOI" in
    champs)
        for g in $(seq 20); do
            echo '<vbox>'
            for i in $(seq 100); do echo "<entry><variable>E${g}_$i</variable><default>valeur $i</default></entry>"; done
            echo '</vbox>'
        done ;;
    icones)
        set -- document-open document-save edit-copy edit-paste icone-qui-n-existe-pas go-next go-previous dialog-ok
        for g in $(seq 4); do
            echo '<vbox>'
            for i in $(seq 100); do
                eval "nom=\${$(( (i % 8) + 1 ))}"
                echo "<button><input file icon=\"$nom\"></input><label>b$i</label></button>"
            done
            echo '</vbox>'
        done ;;
    *) echo "mesure inconnue : $QUOI" >&2; exit 2 ;;
    esac
    echo "$FIN</hbox></window>"
} > grand.xml
debut=$(date +%s%N)
GSK_RENDERER=cairo GTK_A11Y=none xvfb-run -a timeout 60 "$BIN" -f grand.xml </dev/null >sortie 2>err
ms=$(( ($(date +%s%N) - debut) / 1000000 ))
grep -q '^EXIT="fin"' sortie || { echo "ECHEC : le dialogue ($QUOI) ne s'est pas ouvert puis fermé en 60 s"; exit 1; }
[ "$ms" -le 5000 ] || { echo "ECHEC : $QUOI : ouverture et fermeture en $ms ms (5000 au plus)"; exit 1; }
echo "OK : $QUOI : grand dialogue ouvert et fermé en $ms ms"
