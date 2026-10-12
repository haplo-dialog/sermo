#!/bin/bash
# SPDX-License-Identifier: CC0-1.0
# Métier   : Multimédia (photo, archives, communication)
# Besoin   : renommer les photos d'un dossier d'après leur date de prise de vue, en voyant d'abord les nouveaux noms
# Enrobe   : exiftool (-testname pour l'aperçu, en lecture seule ; -FileName pour renommer)
# Ports    : gtk3sermo conseillé ; qt6sermo, efl1sermo ; ncursessermo par SSH
# Terminal : rien de perdu
# Usage    : ./4-renommer-photos.sh [binaire sermo]
#            SERMO_A_BLANC=0 pour agir pour de vrai (défaut : la commande est affichée)

export SERMO="${SERMO:-sermo}"
export SERMO_A_BLANC="${SERMO_A_BLANC:-1}"
# Le script lui-même, pour --do : le shell de sermo lit la variable, sans
# citation à refaire (printf %q écrit $'…' en locale C, que dash lit de travers).
export SERMO_SCRIPT="$0"

# Échappe une valeur avant de l'insérer dans le XML.
# (« \& » : depuis bash 5.2, un « & » nu dans le remplacement recopie le motif.)
xml() { local s=${1//&/"&amp;"}; s=${s//</"&lt;"}; s=${s//>/"&gt;"}; printf '%s' "${s//\"/"&quot;"}"; }

# Lance une commande, ou l'affiche seulement en mode à blanc.
agir() {
	if [ "$SERMO_A_BLANC" = 1 ]; then
		printf 'à blanc :'; printf ' %q' "$@"; printf '\n'
	else
		"$@"
	fi
}

dialogue_principal() {
	local dossier=${1:-$HOME/Images/import-appareil}
	cat <<XML
<window title="Renommer des photos" default-width="540">
  <vbox>
    <hbox><text><label>Dossier :</label></text>
      <entry space-expand="true" space-fill="true"><variable>DOSSIER</variable><default>$(xml "$dossier")</default></entry></hbox>
    <checkbox><label>Inclure les sous-dossiers</label><variable>RECURSIF</variable></checkbox>
    <frame label="Nouveau nom">
      <vbox>
        <hbox><text><label>Préfixe (facultatif) :</label></text>
          <entry space-expand="true" space-fill="true"><variable>PREFIXE</variable><default>Mariage-Julie-Marc_</default></entry></hbox>
        <text xalign="0"><label>Modèle de date :</label></text>
        <list height-request="100">
          <variable>MODELE</variable>
          <item>2026-10-11_14h05m09</item>
          <item>20261011_140509</item>
          <item>2026-10-11/140509 (un dossier par jour)</item>
        </list>
        <text xalign="0"><label>Deux photos à la même seconde reçoivent « -1 », « -2 »… ; l'extension passe en minuscules.</label></text>
      </vbox>
    </frame>
    <hbox>
      <button cancel></button>
      <button><label>Voir les nouveaux noms</label><action>exit:apercu</action></button>
    </hbox>
  </vbox>
</window>
XML
}

# Aperçu : $1 nombre de photos, puis une ligne « nom actuel|nouveau nom » par argument.
dialogue_apercu() {
	local nombre=${1:-3}
	shift 2>/dev/null || set --
	[ $# -gt 0 ] || set -- "DSC_0412.JPG|Mariage-Julie-Marc_2026-09-19_15h32m07.jpg" "DSC_0413.JPG|Mariage-Julie-Marc_2026-09-19_15h32m07-1.jpg" "IMG_2201.jpeg|Mariage-Julie-Marc_2026-09-19_16h01m44.jpg"
	cat <<XML
<window title="Aperçu du renommage" default-width="620" default-height="360">
  <vbox>
    <text><label>$(xml "$nombre") photo(s) seront renommées ainsi (aperçu sans rien changer) :</label></text>
    <tree scrollable="true" height-request="200">
      <label>Nom actuel|Nouveau nom</label>
      <variable>PHOTO</variable>
XML
	local l
	for l in "$@"; do printf '      <item>%s</item>\n' "$(xml "$l")"; done
	cat <<XML
    </tree>
    <text><label>Le renommage est fait sur place : gardez une copie si vous tenez aux noms d'origine.</label></text>
    <hbox>
      <button cancel></button>
      <button><label>Renommer</label><action>exit:renommer</action></button>
    </hbox>
  </vbox>
</window>
XML
}

# Format strftime d'exiftool (« %%-c » : compteur si doublon, « %%le » : extension en minuscules).
format() {
	local f
	case "$MODELE" in
	2026101*) f="%Y%m%d_%H%M%S" ;;
	*/*)      f="%Y-%m-%d/%H%M%S" ;;
	*)        f="%Y-%m-%d_%Hh%Mm%S" ;;
	esac
	# Un « % » du préfixe serait lu comme un code de date : on le double.
	if [[ "$f" == */* ]]; then
		printf '%s/%s%s%%%%-c.%%%%le' "${f%%/*}" "${PREFIXE//%/%%}" "${f#*/}"
	else
		printf '%s%s%%%%-c.%%%%le' "${PREFIXE//%/%%}" "$f"
	fi
}

# Arguments exiftool communs : date de création, remplacée par la prise de vue quand elle existe.
# $1 vaut « testname » (aperçu) ou « filename » (renommage).
arguments() {
	printf '%s\n' "-$1<CreateDate" "-$1<DateTimeOriginal" -d "$(format)"
	[ "$RECURSIF" = true ] && echo -r
	printf '%s\n' -ext jpg -ext jpeg -ext heic -ext png -ext cr2 -ext nef -ext dng "$DOSSIER"
}

suite() {
	# La sortie standard de sermo part vers /dev/null (les lignes NOM=…) et
	# l'étape --do en hérite : écrire sur la sortie d'erreur, qui reste visible.
	exec >&2
	# exiftool ne connaît pas « -- » : un chemin qui commence par « - » serait une option.
	case "$DOSSIER" in -*) DOSSIER=./$DOSSIER ;; esac
	case "$1" in
	principal)
		[ "$EXIT" = apercu ] || exit 0
		[ -d "$DOSSIER" ] || { echo "Dossier introuvable : $DOSSIER" >&2; exit 1; }
		[ -n "$MODELE" ] || { echo "Choisissez un modèle de date dans la liste." >&2; exit 1; }
		case "$PREFIXE" in */*) echo "Le préfixe ne peut pas contenir « / »." >&2; exit 1 ;; esac
		mapfile -t args < <(arguments testname)
		# -testname ne renomme rien : il affiche « 'ancien' --> 'nouveau' ».
		mapfile -t lignes < <(exiftool "${args[@]}" 2>/dev/null | sed -n "s|^'\(.*\)' --> '\(.*\)'\$|\1\|\2|p" | head -200)
		lignes=("${lignes[@]//"$DOSSIER/"/}")
		[ ${#lignes[@]} -gt 0 ] || { echo "Aucune photo datée à renommer dans $DOSSIER." >&2; exit 1; }
		export DOSSIER RECURSIF PREFIXE MODELE
		export DIALOGUE; DIALOGUE=$(dialogue_apercu "${#lignes[@]}" "${lignes[@]}")
		"$SERMO" --program=DIALOGUE --do='"$SERMO_SCRIPT" --suite apercu' >/dev/null
		;;
	apercu)
		[ "$EXIT" = renommer ] || exit 0
		mapfile -t args < <(arguments filename)
		agir exiftool "${args[@]}"
		;;
	esac
}

main() {
	if [ "${1:-}" = --suite ]; then suite "$2"; exit; fi
	[ -n "${1:-}" ] && SERMO=$1
	command -v exiftool >/dev/null || { echo "exiftool est absent : installez le paquet libimage-exiftool-perl." >&2; exit 1; }
	export DIALOGUE; DIALOGUE=$(dialogue_principal)
	"$SERMO" --program=DIALOGUE --do='"$SERMO_SCRIPT" --suite principal' >/dev/null
}

[ "${BASH_SOURCE[0]}" = "$0" ] && main "$@"
