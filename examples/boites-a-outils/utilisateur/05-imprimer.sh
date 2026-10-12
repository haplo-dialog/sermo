#!/bin/bash
# SPDX-License-Identifier: CC0-1.0
# Boîte    : Utilisateur
# Besoin   : imprimer un fichier en choisissant l'imprimante, le nombre de copies, le recto verso et les pages
# Enrobe   : lp ; lpstat pour lire les imprimantes et la file d'attente
# Ports    : gtk3sermo conseillé ; qt6sermo ; htmlsermo
# Terminal : rien de perdu
# Usage    : ./05-imprimer.sh [binaire sermo]
#            SERMO_A_BLANC=0 pour imprimer pour de vrai (défaut : la commande est affichée)

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

# Une <item> par ligne non vide.
items() {
	local l
	while IFS= read -r l; do [ -n "$l" ] && printf '<item>%s</item>' "$(xml "$l")"; done <<< "$1"
}

# Arguments : imprimantes (une par ligne), imprimante par défaut, fichier proposé,
# puis une ligne par travail en attente « travail|qui|taille|depuis ».
dialogue_principal() {
	[ $# -gt 0 ] || set -- $'Bureau-Laser\nSalon-JetEncre\nPDF' Bureau-Laser \
		/home/utilisateur/Documents/attestation-assurance.pdf \
		"Bureau-Laser-41|utilisateur|125 Ko|lun. 12 oct. 2026 10:02"
	local imprimantes=$1 defaut=$2 fichier=$3 l
	shift 3
	cat <<XML
<window title="Imprimer" default-width="560">
  <vbox>
    <frame label="Quoi et où">
      <vbox>
        <hbox><text><label>Fichier :</label></text>
          <entry space-expand="true" space-fill="true"><variable>FICHIER</variable><default>$(xml "$fichier")</default></entry></hbox>
        <hbox><text><label>Imprimante :</label></text>
          <comboboxtext space-expand="true" space-fill="true"><variable>IMPRIMANTE</variable><default>$(xml "$defaut")</default>$(items "$imprimantes")</comboboxtext></hbox>
        <hbox><text><label>Nombre de copies :</label></text>
          <spinbutton range-min="1" range-max="50" range-step="1"><variable>COPIES</variable><default>1</default></spinbutton></hbox>
      </vbox>
    </frame>
    <frame label="Recto verso">
      <vbox>
        <radiobutton><label>Recto seulement</label><variable>RECTO</variable><default>true</default></radiobutton>
        <radiobutton><label>Recto verso, on tourne les pages comme un livre</label><variable>VERSO_LIVRE</variable></radiobutton>
        <radiobutton><label>Recto verso, on tourne les pages vers le haut (bloc-notes)</label><variable>VERSO_BLOC</variable></radiobutton>
      </vbox>
    </frame>
    <frame label="Pages">
      <vbox>
        <hbox>
          <radiobutton><label>Toutes</label><variable>TOUTES</variable><default>true</default></radiobutton>
          <radiobutton><label>Seulement :</label><variable>CERTAINES</variable></radiobutton>
          <entry><variable>PAGES</variable><default>1-3,5</default></entry>
        </hbox>
        <hbox>
          <togglebutton><label>À l'italienne (paysage)</label><variable>PAYSAGE</variable></togglebutton>
          <switch><variable>ASSEMBLER</variable><default>true</default></switch>
          <text><label>Assembler les copies (1 2 3, 1 2 3)</label></text>
        </hbox>
      </vbox>
    </frame>
    <expander label="En attente d'impression">
      <table height-request="90">
        <label>Travail|Par|Taille|Depuis</label>
XML
	for l in "$@"; do printf '        <item>%s</item>\n' "$(xml "$l")"; done
	cat <<XML
        <variable>FILE</variable>
      </table>
      <variable>VOIR_FILE</variable>
    </expander>
    <hbox>
      <button cancel></button>
      <button><label>Imprimer</label><action>exit:imprimer</action></button>
    </hbox>
  </vbox>
</window>
XML
}

suite() {
	# La sortie standard de sermo part vers /dev/null : écrire sur la sortie d'erreur.
	exec >&2
	case "$1" in
	principal)
		[ "$EXIT" = imprimer ] || exit 0
		[ -f "$FICHIER" ] || { echo "Fichier introuvable : $FICHIER" >&2; exit 1; }
		# L'imprimante doit être l'une de celles que CUPS connaît.
		lpstat -e 2>/dev/null | grep -qxF -- "$IMPRIMANTE" || { echo "Imprimante inconnue : $IMPRIMANTE" >&2; exit 1; }
		[[ $COPIES =~ ^[0-9]+$ ]] && [ "$COPIES" -ge 1 ] && [ "$COPIES" -le 50 ] || { echo "Nombre de copies illisible : $COPIES" >&2; exit 1; }
		local opts=(-d "$IMPRIMANTE" -n "$COPIES")
		if [ "$VERSO_LIVRE" = true ]; then opts+=(-o sides=two-sided-long-edge)
		elif [ "$VERSO_BLOC" = true ]; then opts+=(-o sides=two-sided-short-edge)
		else opts+=(-o sides=one-sided); fi
		if [ "$CERTAINES" = true ]; then
			PAGES=${PAGES// /}
			[[ $PAGES =~ ^[0-9]+(-[0-9]+)?(,[0-9]+(-[0-9]+)?)*$ ]] || { echo "Pages illisibles : « $PAGES » (exemple : 1-3,5)" >&2; exit 1; }
			opts+=(-P "$PAGES")
		fi
		[ "$PAYSAGE" = true ] && opts+=(-o landscape)
		[ "$ASSEMBLER" = true ] && opts+=(-o collate=true)
		agir lp "${opts[@]}" -- "$FICHIER"
		;;
	esac
}

main() {
	if [ "${1:-}" = --suite ]; then suite "$2"; exit; fi
	[ -n "${1:-}" ] && SERMO=$1
	local c
	for c in lp lpstat; do
		command -v "$c" >/dev/null || { echo "$c est absent : il faut le paquet cups-client." >&2; exit 1; }
	done
	local imprimantes defaut travaux=() t qui taille depuis
	imprimantes=$(lpstat -e 2>/dev/null)
	[ -n "$imprimantes" ] || { echo "Aucune imprimante n'est installée. Ajoutez-en une dans les réglages d'impression." >&2; exit 1; }
	defaut=$(lpstat -d 2>/dev/null | sed -n 's/^[^:]*: *//p')
	[ -n "$defaut" ] || defaut=$(head -n1 <<< "$imprimantes")
	while read -r t qui taille depuis; do
		travaux+=("$t|$qui|$taille o|$depuis")
	done < <(lpstat -o 2>/dev/null)
	export DIALOGUE; DIALOGUE=$(dialogue_principal "$imprimantes" "$defaut" "$HOME/Documents/" "${travaux[@]}")
	"$SERMO" --program=DIALOGUE --do='"$SERMO_SCRIPT" --suite principal' >/dev/null
}

[ "${BASH_SOURCE[0]}" = "$0" ] && main "$@"
