#!/bin/bash
# SPDX-License-Identifier: CC0-1.0
# Métier   : Atelier, industrie, laboratoire
# Besoin   : préparer et imprimer les étiquettes de colis d'un lot (référence, quantité, dates), aperçu d'abord
# Enrobe   : lp (impression), lpstat (liste des imprimantes, en lecture)
# Ports    : gtk3sermo conseillé ; lvglsermo sur un écran d'atelier
# Terminal : rien de perdu
# Usage    : ./3-etiquetage.sh [binaire sermo]
#            SERMO_A_BLANC=0 pour agir pour de vrai (défaut : la commande est affichée)

export SERMO="${SERMO:-sermo}"
export SERMO_A_BLANC="${SERMO_A_BLANC:-1}"
# Le script lui-même, pour --do : le shell de sermo lit la variable, sans
# citation à refaire (printf %q écrit $'…' en locale C, que dash lit de travers).
export SERMO_SCRIPT="$0"

# Échappe une valeur avant de l'insérer dans le XML.
xml() { local s=${1//&/"&amp;"}; s=${s//</"&lt;"}; s=${s//>/"&gt;"}; printf '%s' "${s//\"/"&quot;"}"; }

# Lance une commande, ou l'affiche seulement en mode à blanc.
agir() {
	if [ "$SERMO_A_BLANC" = 1 ]; then
		printf 'à blanc :'; printf ' %q' "$@"; printf '\n'
	else
		"$@"
	fi
}

# Arguments : imprimantes (une par ligne), date de fabrication (AAAA-MM-JJ).
dialogue_principal() {
	local imprimantes=${1:-$'Zebra_ZD421\nBureau_Laser'} jour=${2:-2026-10-12} l
	cat <<XML
<window title="Étiquetage des colis" default-width="580">
  <vbox>
    <frame label="Produit">
      <grid columns="2" row-spacing="6" column-spacing="8">
        <text><label>Référence :</label></text>
        <entry space-expand="true" space-fill="true"><variable>REFERENCE</variable><default>AX-2500-H7</default></entry>
        <text><label>Désignation :</label></text>
        <entry space-expand="true" space-fill="true"><variable>DESIGNATION</variable><default>Axe rectifié Ø 25 mm, L 120 mm</default></entry>
        <text><label>Lot :</label></text>
        <entry space-expand="true" space-fill="true"><variable>LOT</variable><default>L2026-1042</default></entry>
      </grid>
    </frame>
    <hbox>
      <frame label="Date de fabrication">
        <calendar><variable>FABRICATION</variable><default>$(xml "$jour")</default></calendar>
      </frame>
      <frame label="Quantités" space-expand="true" space-fill="true">
        <vbox>
          <text><label>Pièces par colis :</label></text>
          <spinbutton range-min="1" range-max="10000" range-step="1"><variable>QUANTITE</variable><default>50</default></spinbutton>
          <text><label>Nombre d'étiquettes :</label></text>
          <spinbutton range-min="1" range-max="500" range-step="1"><variable>NOMBRE</variable><default>4</default></spinbutton>
        </vbox>
      </frame>
    </hbox>
    <frame label="Impression">
      <vbox>
        <hbox>
          <radiobutton><label>Étiquette 100 × 50 mm</label><variable>FORMAT_ETIQUETTE</variable><default>true</default></radiobutton>
          <radiobutton><label>Feuille A4</label><variable>FORMAT_A4</variable></radiobutton>
        </hbox>
        <hbox><text><label>Imprimante :</label></text>
          <comboboxtext space-expand="true" space-fill="true"><variable>IMPRIMANTE</variable>
$(while IFS= read -r l; do [ -n "$l" ] && printf '            <item>%s</item>\n' "$(xml "$l")"; done <<< "$imprimantes")
          </comboboxtext></hbox>
      </vbox>
    </frame>
    <hbox>
      <button cancel></button>
      <button><label>Aperçu</label><action>exit:apercu</action></button>
    </hbox>
  </vbox>
</window>
XML
}

dialogue_apercu() {
	local texte=${1:-$'RÉF   AX-2500-H7\nAxe rectifié Ø 25 mm, L 120 mm\nLOT   L2026-1042\nQTÉ   50 pièces\nFAB   2026-10-12'}
	local resume=${2:-4 étiquettes sur Zebra_ZD421} l
	# Un <text> par ligne : sermo ne garde que la dernière ligne d'un <label> ou d'un <default>.
	cat <<XML
<window title="Aperçu de l'étiquette" default-width="460">
  <vbox>
    <text><label>Aperçu ($(xml "$resume")) :</label></text>
    <frame label="Étiquette">
      <vbox>
$(while IFS= read -r l; do printf '        <text><label>%s</label></text>\n' "$(xml "$l")"; done <<< "$texte")
      </vbox>
    </frame>
    <hbox>
      <button cancel></button>
      <button><label>Imprimer</label><action>exit:imprimer</action></button>
    </hbox>
  </vbox>
</window>
XML
}

# Le texte imprimé, fait des valeurs saisies.
etiquette() {
	printf 'RÉF   %s\n%s\nLOT   %s\nQTÉ   %s pièces\nFAB   %s\n' \
		"$REFERENCE" "$DESIGNATION" "$LOT" "$QUANTITE" "$FABRICATION"
}

suite() {
	# La sortie standard de sermo part vers /dev/null : écrire sur la sortie d'erreur.
	exec >&2
	case "$1" in
	principal)
		[ "$EXIT" = apercu ] || exit 0
		[ -n "${REFERENCE// /}" ] && [ -n "${LOT// /}" ] || { echo "Référence et lot sont obligatoires."; exit 1; }
		[[ "$QUANTITE" =~ ^[0-9]+$ && "$NOMBRE" =~ ^[0-9]+$ ]] || { echo "Quantités illisibles."; exit 1; }
		[ -n "$IMPRIMANTE" ] || { echo "Aucune imprimante choisie."; exit 1; }
		export REFERENCE DESIGNATION LOT QUANTITE FABRICATION NOMBRE IMPRIMANTE FORMAT_A4
		export DIALOGUE; DIALOGUE=$(dialogue_apercu "$(etiquette)" "$NOMBRE étiquette(s) sur $IMPRIMANTE")
		"$SERMO" --program=DIALOGUE --do='"$SERMO_SCRIPT" --suite apercu' >/dev/null
		;;
	apercu)
		[ "$EXIT" = imprimer ] || exit 0
		local media=Custom.100x50mm
		[ "$FORMAT_A4" = true ] && media=A4
		# Le texte part sur l'entrée standard de lp : pas de fichier temporaire.
		etiquette | agir lp -d "$IMPRIMANTE" -n "$NOMBRE" -o media="$media" -o cpi=10 -o lpi=6 -t "Étiquettes $REFERENCE $LOT"
		;;
	esac
}

main() {
	if [ "${1:-}" = --suite ]; then suite "$2"; exit; fi
	[ -n "${1:-}" ] && SERMO=$1
	for c in lp lpstat; do
		command -v "$c" >/dev/null || { echo "$c est absent : installez cups-client." >&2; exit 1; }
	done
	local imprimantes
	imprimantes=$(lpstat -e 2>/dev/null)
	[ -n "$imprimantes" ] || { echo "Aucune imprimante déclarée dans CUPS (lpstat -e est vide)." >&2; exit 1; }
	export DIALOGUE; DIALOGUE=$(dialogue_principal "$imprimantes" "$(date +%F)")
	"$SERMO" --program=DIALOGUE --do='"$SERMO_SCRIPT" --suite principal' >/dev/null
}

[ "${BASH_SOURCE[0]}" = "$0" ] && main "$@"
