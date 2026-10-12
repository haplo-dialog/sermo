#!/bin/bash
# SPDX-License-Identifier: CC0-1.0
# Métier   : Multimédia (vidéo)
# Besoin   : convertir une vidéo (format, définition, qualité, son) après avoir vu ce que contient la source
# Enrobe   : ffprobe (lecture seule) ; ffmpeg
# Ports    : gtk3sermo conseillé ; qt6sermo, efl1sermo ; ncursessermo par SSH sur un serveur d'encodage
# Terminal : rien de perdu
# Usage    : ./1-convertir-video.sh [binaire sermo]
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
	local source=${1:-$HOME/Vidéos/reunion-2026-10-08.mov} sortie=${2:-$HOME/Vidéos/converties}
	cat <<XML
<window title="Convertir une vidéo" default-width="560">
  <vbox>
    <hbox><text><label>Vidéo source :</label></text>
      <entry space-expand="true" space-fill="true"><variable>SOURCE</variable><default>$(xml "$source")</default></entry></hbox>
    <frame label="Image">
      <vbox>
        <hbox><text><label>Format :</label></text>
          <comboboxtext space-expand="true" space-fill="true">
            <variable>FORMAT</variable>
            <default>MP4 (H.264 + AAC), lisible partout</default>
            <item>MP4 (H.264 + AAC), lisible partout</item>
            <item>WebM (VP9 + Opus), pour le web</item>
            <item>MKV (H.265 + AAC), fichier plus léger</item>
          </comboboxtext></hbox>
        <hbox><text><label>Définition :</label></text>
          <comboboxtext space-expand="true" space-fill="true">
            <variable>DEFINITION</variable>
            <default>720p (1280 de large)</default>
            <item>Identique à la source</item>
            <item>1080p (1920 de large)</item>
            <item>720p (1280 de large)</item>
            <item>480p (854 de large)</item>
          </comboboxtext></hbox>
        <text xalign="0"><label>Qualité (CRF : petit = meilleur et plus lourd ; 23 est un bon réglage)</label></text>
        <hscale range-min="16" range-max="36" range-step="1" draw-value="true">
          <variable>CRF</variable><default>23</default>
        </hscale>
      </vbox>
    </frame>
    <frame label="Son">
      <vbox>
        <radiobutton><label>Réencoder (compatible avec le format choisi)</label><variable>SON_REENCODER</variable><default>true</default></radiobutton>
        <radiobutton><label>Copier tel quel (rapide, si le format l'accepte)</label><variable>SON_COPIER</variable></radiobutton>
        <radiobutton><label>Supprimer le son</label><variable>SON_SUPPRIMER</variable></radiobutton>
      </vbox>
    </frame>
    <hbox><text><label>Dossier de sortie :</label></text>
      <entry space-expand="true" space-fill="true"><variable>SORTIE</variable><default>$(xml "$sortie")</default></entry></hbox>
    <hbox>
      <button cancel></button>
      <button><label>Analyser la source</label><action>exit:analyser</action></button>
    </hbox>
  </vbox>
</window>
XML
}

# Ce que contient la source, la commande prévue, et l'accord pour écraser si besoin.
# $1 description de la source, $2 commande, $3 fichier de sortie, $4 « existe » si déjà là.
dialogue_confirmation() {
	local infos=${1:-Durée 00:42:17 · 1920x1080 · h264 · son aac} commande=${2:-ffmpeg -i reunion.mov -c:v libx264 -crf 23 reunion.mp4} cible=${3:-$HOME/Vidéos/converties/reunion-2026-10-08.mp4} existe=${4:-}
	cat <<XML
<window title="Avant de convertir" default-width="560">
  <vbox>
    <text xalign="0"><label>Source : $(xml "$infos")</label></text>
    <text xalign="0"><label>Résultat : $(xml "$cible")</label></text>
    <frame label="Commande prévue">
      <vbox>
        <edit editable="false" wrap-mode="char" height-request="80"><variable>COMMANDE</variable><default>"$(xml "$commande")"</default></edit>
      </vbox>
    </frame>
XML
	[ -n "$existe" ] && cat <<XML
    <checkbox><label>Le fichier résultat existe déjà : le remplacer</label><variable>ECRASER</variable></checkbox>
XML
	cat <<XML
    <hbox>
      <button cancel></button>
      <button><label>Convertir</label><action>exit:convertir</action></button>
    </hbox>
  </vbox>
</window>
XML
}

# Fichier résultat : même nom que la source, extension du format choisi.
cible() {
	local ext=mp4 base=${SOURCE##*/}
	case "$FORMAT" in WebM*) ext=webm ;; MKV*) ext=mkv ;; esac
	printf '%s/%s.%s' "$SORTIE" "${base%.*}" "$ext"
}

# Arguments ffmpeg, un par ligne (sans l'entrée ni la sortie).
options() {
	case "$FORMAT" in
	WebM*) printf '%s\n' -c:v libvpx-vp9 -crf "${CRF%.*}" -b:v 0 -row-mt 1 ;;
	MKV*)  printf '%s\n' -c:v libx265 -crf "${CRF%.*}" -preset medium ;;
	*)     printf '%s\n' -c:v libx264 -crf "${CRF%.*}" -preset medium -pix_fmt yuv420p -movflags +faststart ;;
	esac
	case "$DEFINITION" in
	1080p*) printf '%s\n' -vf 'scale=1920:-2' ;;
	720p*)  printf '%s\n' -vf 'scale=1280:-2' ;;
	480p*)  printf '%s\n' -vf 'scale=854:-2' ;;
	esac
	if [ "$SON_SUPPRIMER" = true ]; then echo -an
	elif [ "$SON_COPIER" = true ]; then printf '%s\n' -c:a copy
	elif [[ "$FORMAT" == WebM* ]]; then printf '%s\n' -c:a libopus -b:a 128k
	else printf '%s\n' -c:a aac -b:a 160k
	fi
}

suite() {
	# La sortie standard de sermo part vers /dev/null (les lignes NOM=…) et
	# l'étape --do en hérite : écrire sur la sortie d'erreur, qui reste visible.
	exec >&2
	case "$1" in
	principal)
		[ "$EXIT" = analyser ] || exit 0
		[ -f "$SOURCE" ] || { echo "Vidéo introuvable : $SOURCE" >&2; exit 1; }
		# Lecture seule : durée, définition et codecs de la source.
		infos=$(ffprobe -v error -show_entries format=duration:stream=codec_type,codec_name,width,height \
			-of default=noprint_wrappers=1 -- "$SOURCE" 2>/dev/null |
			awk -F= '$1 == "duration" { d = int($2); printf "Durée %02d:%02d:%02d · ", d / 3600, d % 3600 / 60, d % 60 }
				$1 == "codec_name" { c = $2 } $1 == "width" { w = $2 } $1 == "height" { printf "%sx%s · vidéo %s · ", w, $2, c }
				$1 == "codec_type" && $2 == "audio" { printf "son %s · ", c }')
		mapfile -t opts < <(options)
		c=$(cible); existe=""; [ -e "$c" ] && existe=oui
		export SOURCE FORMAT DEFINITION CRF SON_REENCODER SON_COPIER SON_SUPPRIMER SORTIE
		export DIALOGUE; DIALOGUE=$(dialogue_confirmation "${infos% · }" "ffmpeg -i $(printf '%q' "$SOURCE") ${opts[*]} $(printf '%q' "$c")" "$c" "$existe")
		"$SERMO" --program=DIALOGUE --do='"$SERMO_SCRIPT" --suite confirmation' >/dev/null
		;;
	confirmation)
		[ "$EXIT" = convertir ] || exit 0
		c=$(cible)
		ecrase=-n
		if [ -e "$c" ]; then
			[ "${ECRASER:-false}" = true ] || { echo "$c existe déjà et n'a pas été remplacé." >&2; exit 1; }
			ecrase=-y
		fi
		mapfile -t opts < <(options)
		[ -d "$SORTIE" ] || agir mkdir -p -- "$SORTIE"
		agir ffmpeg -hide_banner "$ecrase" -i "$SOURCE" "${opts[@]}" "$c"
		;;
	esac
}

main() {
	if [ "${1:-}" = --suite ]; then suite "$2"; exit; fi
	[ -n "${1:-}" ] && SERMO=$1
	for c in ffmpeg ffprobe; do
		command -v "$c" >/dev/null || { echo "$c est absent : installez le paquet ffmpeg." >&2; exit 1; }
	done
	export DIALOGUE; DIALOGUE=$(dialogue_principal)
	"$SERMO" --program=DIALOGUE --do='"$SERMO_SCRIPT" --suite principal' >/dev/null
}

[ "${BASH_SOURCE[0]}" = "$0" ] && main "$@"
