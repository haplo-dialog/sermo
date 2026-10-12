#!/bin/bash
# SPDX-License-Identifier: CC0-1.0
# Métier   : Support informatique
# Besoin   : voir les files d'impression, annuler un travail bloqué, imprimer une page de test
# Enrobe   : lpstat, cancel, lp
# Ports    : gtk3sermo conseillé ; ncursessermo par SSH
# Terminal : rien de perdu
# Usage    : ./4-imprimantes.sh [binaire sermo]
#            SERMO_A_BLANC=0 pour agir pour de vrai (défaut : la commande est affichée)

export SERMO="${SERMO:-sermo}"
export SERMO_A_BLANC="${SERMO_A_BLANC:-1}"
# Le script lui-même, pour --do : le shell de sermo lit la variable, sans
# citation à refaire (printf %q écrit $'…' en locale C, que dash lit de travers).
export SERMO_SCRIPT="$0"
PAGE_TEST=/usr/share/cups/data/testprint

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

# Arguments : imprimante par défaut, les imprimantes, « -- », puis les travaux
# « travail|propriétaire|taille|date ».
dialogue_principal() {
	[ $# -gt 0 ] || set -- Laser-Accueil Laser-Accueil Couleur-Etage2 -- \
		"Laser-Accueil-118|cdupont|2,1 Mo|sam. 11 oct. 2026 09:02" \
		"Laser-Accueil-119|mmartin|340 ko|sam. 11 oct. 2026 09:15"
	local defaut=$1 a; shift
	local imprimantes=()
	while [ $# -gt 0 ] && [ "$1" != -- ]; do imprimantes+=("$1"); shift; done
	[ "${1:-}" = -- ] && shift
	cat <<XML
<window title="Imprimantes" default-width="620" default-height="420">
  <vbox>
    <notebook tab-labels="File d'attente|Page de test" space-expand="true" space-fill="true">
      <vbox>
        <table scrollable="true" space-expand="true" space-fill="true">
          <label>Travail|Propriétaire|Taille|Envoyé le</label>
          <variable>TRAVAIL</variable>
XML
	for a in "$@"; do printf '          <item>%s</item>\n' "$(xml "$a")"; done
	cat <<XML
        </table>
        <checkbox><label>Annuler tous mes travaux, sur toutes les imprimantes</label><variable>TOUS</variable></checkbox>
        <hbox><button><label>Annuler</label><action>exit:annuler</action></button></hbox>
      </vbox>
      <vbox>
        <hbox><text><label>Imprimante :</label></text>
          <comboboxtext space-expand="true" space-fill="true">
            <variable>IMPRIMANTE</variable>
            <default>$(xml "$defaut")</default>
XML
	for a in "${imprimantes[@]}"; do printf '            <item>%s</item>\n' "$(xml "$a")"; done
	cat <<XML
          </comboboxtext></hbox>
        <hbox><text><label>Exemplaires :</label></text>
          <spinbutton range-min="1" range-max="5" range-step="1"><variable>COPIES</variable><default>1</default></spinbutton></hbox>
        <checkbox><label>Recto verso</label><variable>RECTO_VERSO</variable></checkbox>
        <hbox><button><label>Imprimer la page de test</label><action>exit:tester</action></button></hbox>
      </vbox>
    </notebook>
    <hbox space-expand="false" space-fill="false"><button><label>Fermer</label><action>exit:fermer</action></button></hbox>
  </vbox>
</window>
XML
}

dialogue_confirmation() {
	local quoi=${1:-le travail Laser-Accueil-118}
	cat <<XML
<window title="Confirmer l'annulation">
  <vbox>
    <text><label>Annuler $(xml "$quoi") ?</label></text>
    <text><label>Le document devra être renvoyé à l'imprimante.</label></text>
    <hbox>
      <button cancel></button>
      <button><label>Annuler l'impression</label><action>exit:confirmer</action></button>
    </hbox>
  </vbox>
</window>
XML
}

suite() {
	exec >&2
	case "$1" in
	principal)
		case "$EXIT" in
		annuler)
			local quoi
			if [ "$TOUS" = true ]; then
				quoi="tous les travaux de $USER"
			else
				[[ "$TRAVAIL" =~ ^[A-Za-z0-9_.@-]+-[0-9]+$ ]] || { echo "Aucun travail choisi." >&2; exit 1; }
				quoi="le travail $TRAVAIL"
			fi
			export TRAVAIL TOUS
			export DIALOGUE; DIALOGUE=$(dialogue_confirmation "$quoi")
			"$SERMO" --program=DIALOGUE --do='"$SERMO_SCRIPT" --suite confirmation' >/dev/null
			;;
		tester)
			[ -n "$IMPRIMANTE" ] || { echo "Aucune imprimante." >&2; exit 1; }
			[ -r "$PAGE_TEST" ] || { echo "Page de test absente ($PAGE_TEST) : installez cups." >&2; exit 1; }
			[[ "$COPIES" =~ ^[1-5]$ ]] || COPIES=1
			local opts=(-d "$IMPRIMANTE" -n "$COPIES" -t "Page de test")
			[ "$RECTO_VERSO" = true ] && opts+=(-o sides=two-sided-long-edge)
			agir lp "${opts[@]}" "$PAGE_TEST"
			;;
		esac
		;;
	confirmation)
		[ "$EXIT" = confirmer ] || exit 0
		if [ "$TOUS" = true ]; then
			agir cancel -u "$USER"
		else
			agir cancel "$TRAVAIL"
		fi
		;;
	esac
}

main() {
	if [ "${1:-}" = --suite ]; then suite "$2"; exit; fi
	[ -n "${1:-}" ] && SERMO=$1
	local c
	for c in lpstat cancel lp; do
		command -v "$c" >/dev/null || { echo "$c est absent : installez le paquet cups-client." >&2; exit 1; }
	done
	local defaut imprimantes=() travaux=() id prop taille date
	defaut=$(LC_ALL=C lpstat -d 2>/dev/null | sed -n 's/^system default destination: //p')
	mapfile -t imprimantes < <(lpstat -e 2>/dev/null)
	[ ${#imprimantes[@]} -gt 0 ] || { echo "Aucune imprimante déclarée sur ce poste (lpstat -e est vide)." >&2; exit 1; }
	while read -r id prop taille date; do
		[ -n "$id" ] && travaux+=("$id|$prop|$taille o|$date")
	done < <(lpstat -o 2>/dev/null)
	export DIALOGUE; DIALOGUE=$(dialogue_principal "${defaut:-${imprimantes[0]}}" "${imprimantes[@]}" -- "${travaux[@]}")
	"$SERMO" --program=DIALOGUE --do='"$SERMO_SCRIPT" --suite principal' >/dev/null
}

[ "${BASH_SOURCE[0]}" = "$0" ] && main "$@"
