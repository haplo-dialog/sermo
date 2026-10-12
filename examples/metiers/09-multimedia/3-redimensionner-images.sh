#!/bin/bash
# SPDX-License-Identifier: CC0-1.0
# Métier   : Multimédia (photo, communication, site web)
# Besoin   : réduire un dossier de photos pour le web ou l'envoi (taille, qualité, métadonnées), en copie ou sur place
# Enrobe   : convert (copies, ImageMagick) ; mogrify (sur place, avec confirmation)
# Ports    : gtk3sermo conseillé ; qt6sermo, sdl3sermo ; ncursessermo par SSH
# Terminal : rien de perdu
# Usage    : ./3-redimensionner-images.sh [binaire sermo]
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
	local dossier=${1:-$HOME/Images/fete-du-quartier-2026}
	cat <<XML
<window title="Redimensionner des images" default-width="540">
  <vbox>
    <hbox><text><label>Dossier des photos :</label></text>
      <entry space-expand="true" space-fill="true"><variable>DOSSIER</variable><default>$(xml "$dossier")</default></entry></hbox>
    <frame label="Taille">
      <vbox>
        <hbox><text><label>Plus grand côté (pixels) :</label></text>
          <spinbutton range-min="320" range-max="6000" range-step="160"><variable>COTE</variable><default>1600</default></spinbutton></hbox>
        <checkbox><label>Ne jamais agrandir une image plus petite</label><variable>SANS_AGRANDIR</variable><default>true</default></checkbox>
        <text xalign="0"><label>Qualité JPEG / WebP :</label></text>
        <hscale range-min="50" range-max="100" range-step="1" draw-value="true"><variable>QUALITE</variable><default>82</default></hscale>
      </vbox>
    </frame>
    <frame label="Résultat">
      <vbox>
        <hbox><text><label>Format :</label></text>
          <comboboxtext space-expand="true" space-fill="true">
            <variable>FORMAT</variable>
            <default>Garder le format</default>
            <item>Garder le format</item>
            <item>JPEG</item>
            <item>WebP</item>
          </comboboxtext></hbox>
        <checkbox><label>Retirer les métadonnées (position GPS, appareil)</label><variable>NETTOYER</variable><default>true</default></checkbox>
        <radiobutton><label>Écrire des copies dans un sous-dossier :</label><variable>EN_COPIE</variable><default>true</default></radiobutton>
        <entry><variable>SOUS_DOSSIER</variable><default>web</default></entry>
        <radiobutton><label>Remplacer les originaux (irréversible)</label><variable>SUR_PLACE</variable></radiobutton>
      </vbox>
    </frame>
    <hbox>
      <button cancel></button>
      <button><label>Redimensionner</label><action>exit:redimensionner</action></button>
    </hbox>
  </vbox>
</window>
XML
}

dialogue_confirmation() {
	local nombre=${1:-84} dossier=${2:-$HOME/Images/fete-du-quartier-2026}
	cat <<XML
<window title="Remplacer les originaux ?">
  <vbox>
    <text><label>$(xml "$nombre") photo(s) de $(xml "$dossier") vont être réduites SUR PLACE.</label></text>
    <text><label>Les originaux en pleine taille seront perdus. Avez-vous une copie ?</label></text>
    <hbox>
      <button cancel></button>
      <button><label>Remplacer les originaux</label><action>exit:remplacer</action></button>
    </hbox>
  </vbox>
</window>
XML
}

# Photos du dossier (sans descendre dans les sous-dossiers).
photos() { find "$DOSSIER" -maxdepth 1 -type f \( -iname '*.jpg' -o -iname '*.jpeg' -o -iname '*.png' -o -iname '*.webp' -o -iname '*.tif' -o -iname '*.tiff' \) | sort; }

# Options ImageMagick communes, une par ligne.
options() {
	local geo="${COTE%.*}x${COTE%.*}"
	[ "$SANS_AGRANDIR" = true ] && geo+=">"
	printf '%s\n' -auto-orient -resize "$geo" -quality "${QUALITE%.*}"
	[ "$NETTOYER" = true ] && echo -strip
	return 0
}

ext_cible() {
	case "$FORMAT" in JPEG) echo jpg ;; WebP) echo webp ;; *) echo "${1##*.}" ;; esac
}

suite() {
	# La sortie standard de sermo part vers /dev/null (les lignes NOM=…) et
	# l'étape --do en hérite : écrire sur la sortie d'erreur, qui reste visible.
	exec >&2
	# ImageMagick ne connaît pas « -- » : un chemin qui commence par « - » serait une option.
	case "$DOSSIER" in -*) DOSSIER=./$DOSSIER ;; esac
	case "$1" in
	principal)
		[ "$EXIT" = redimensionner ] || exit 0
		[ -d "$DOSSIER" ] || { echo "Dossier introuvable : $DOSSIER" >&2; exit 1; }
		mapfile -t liste < <(photos)
		[ ${#liste[@]} -gt 0 ] || { echo "Aucune photo (jpg, png, webp, tif) dans $DOSSIER." >&2; exit 1; }
		mapfile -t opts < <(options)
		if [ "$EN_COPIE" = true ]; then
			case "$SOUS_DOSSIER" in ''|*/*|.|..) echo "Nom de sous-dossier refusé : « $SOUS_DOSSIER »." >&2; exit 1 ;; esac
			dest="$DOSSIER/$SOUS_DOSSIER"
			[ -d "$dest" ] || agir mkdir -p -- "$dest"
			for f in "${liste[@]}"; do
				b=${f##*/}
				agir convert "$f" "${opts[@]}" "$dest/${b%.*}.$(ext_cible "$b")"
			done
			echo "${#liste[@]} photo(s) traitée(s) vers $dest."
			exit
		fi
		export DOSSIER COTE SANS_AGRANDIR QUALITE FORMAT NETTOYER
		export DIALOGUE; DIALOGUE=$(dialogue_confirmation "${#liste[@]}" "$DOSSIER")
		"$SERMO" --program=DIALOGUE --do='"$SERMO_SCRIPT" --suite confirmation' >/dev/null
		;;
	confirmation)
		[ "$EXIT" = remplacer ] || exit 0
		mapfile -t liste < <(photos)
		mapfile -t opts < <(options)
		fmt=()
		case "$FORMAT" in JPEG) fmt=(-format jpg) ;; WebP) fmt=(-format webp) ;; esac
		# Avec -format, mogrify écrit un nouveau fichier à côté au lieu d'écraser.
		agir mogrify "${opts[@]}" "${fmt[@]}" "${liste[@]}"
		;;
	esac
}

main() {
	if [ "${1:-}" = --suite ]; then suite "$2"; exit; fi
	[ -n "${1:-}" ] && SERMO=$1
	for c in convert mogrify; do
		command -v "$c" >/dev/null || { echo "$c est absent : installez le paquet imagemagick." >&2; exit 1; }
	done
	export DIALOGUE; DIALOGUE=$(dialogue_principal)
	"$SERMO" --program=DIALOGUE --do='"$SERMO_SCRIPT" --suite principal' >/dev/null
}

[ "${BASH_SOURCE[0]}" = "$0" ] && main "$@"
