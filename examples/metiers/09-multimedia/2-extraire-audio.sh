#!/bin/bash
# SPDX-License-Identifier: CC0-1.0
# Métier   : Multimédia (son, podcast, captation)
# Besoin   : extraire la bande son d'une vidéo (format, débit, extrait, volume normalisé) pour un podcast ou une transcription
# Enrobe   : ffmpeg
# Ports    : gtk3sermo conseillé ; qt6sermo, fltk1sermo ; ncursessermo par SSH
# Terminal : rien de perdu
# Usage    : ./2-extraire-audio.sh [binaire sermo]
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
	local source=${1:-$HOME/Vidéos/conference-climat.mp4}
	cat <<XML
<window title="Extraire le son" default-width="540">
  <vbox>
    <hbox><text><label>Vidéo source :</label></text>
      <entry space-expand="true" space-fill="true"><variable>SOURCE</variable><default>$(xml "$source")</default></entry></hbox>
    <hbox>
      <frame label="Format">
        <vbox>
          <radiobutton><label>MP3</label><variable>F_MP3</variable><default>true</default></radiobutton>
          <radiobutton><label>Ogg Vorbis</label><variable>F_OGG</variable></radiobutton>
          <radiobutton><label>FLAC (sans perte)</label><variable>F_FLAC</variable></radiobutton>
          <radiobutton><label>WAV (pour le montage)</label><variable>F_WAV</variable></radiobutton>
        </vbox>
      </frame>
      <frame label="Réglages">
        <vbox>
          <text xalign="0"><label>Débit (MP3 et Ogg) :</label></text>
          <comboboxtext>
            <variable>DEBIT</variable>
            <default>128k</default>
            <item>64k</item>
            <item>96k</item>
            <item>128k</item>
            <item>192k</item>
            <item>256k</item>
          </comboboxtext>
          <checkbox><label>Mono (voix seule)</label><variable>MONO</variable><default>true</default></checkbox>
          <checkbox><label>Normaliser le volume (EBU R128)</label><variable>NORMALISER</variable><default>true</default></checkbox>
        </vbox>
      </frame>
    </hbox>
    <frame label="Extrait">
      <vbox>
        <checkbox><label>Ne garder qu'un extrait</label><variable>EXTRAIT</variable></checkbox>
        <hbox>
          <text><label>Début (hh:mm:ss) :</label></text>
          <entry width-chars="9"><variable>DEBUT</variable><default>00:02:30</default></entry>
          <text><label>Fin :</label></text>
          <entry width-chars="9"><variable>FIN</variable><default>00:47:00</default></entry>
        </hbox>
      </vbox>
    </frame>
    <hbox>
      <button cancel></button>
      <button><label>Extraire</label><action>exit:extraire</action></button>
    </hbox>
  </vbox>
</window>
XML
}

dialogue_ecraser() {
	local fichier=${1:-$HOME/Vidéos/conference-climat.mp3}
	cat <<XML
<window title="Fichier existant">
  <vbox>
    <text><label>$(xml "$fichier") existe déjà.</label></text>
    <text><label>Le remplacer par le nouvel extrait ?</label></text>
    <hbox>
      <button cancel></button>
      <button><label>Remplacer</label><action>exit:remplacer</action></button>
    </hbox>
  </vbox>
</window>
XML
}

ext() {
	if [ "$F_OGG" = true ]; then echo ogg
	elif [ "$F_FLAC" = true ]; then echo flac
	elif [ "$F_WAV" = true ]; then echo wav
	else echo mp3; fi
}

# Même dossier et même nom que la vidéo, extension du format choisi.
cible() {
	local d=. b=${SOURCE##*/}
	case "$SOURCE" in */*) d=${SOURCE%/*} ;; esac
	printf '%s/%s.%s' "$d" "${b%.*}" "$(ext)"
}

# Arguments ffmpeg après l'entrée, un par ligne.
options() {
	echo -vn
	case "$(ext)" in
	mp3)  printf '%s\n' -c:a libmp3lame -b:a "$DEBIT" ;;
	ogg)  printf '%s\n' -c:a libvorbis -b:a "$DEBIT" ;;
	flac) printf '%s\n' -c:a flac ;;
	wav)  printf '%s\n' -c:a pcm_s16le ;;
	esac
	[ "$MONO" = true ] && printf '%s\n' -ac 1
	[ "$NORMALISER" = true ] && printf '%s\n' -af loudnorm=I=-16:TP=-1.5:LRA=11
	return 0
}

extraire() {
	local ecrase=$1 decoupe=()
	[ "$EXTRAIT" = true ] && decoupe=(-ss "$DEBUT" -to "$FIN")
	mapfile -t opts < <(options)
	agir ffmpeg -hide_banner "$ecrase" "${decoupe[@]}" -i "$SOURCE" "${opts[@]}" "$(cible)"
}

suite() {
	# La sortie standard de sermo part vers /dev/null (les lignes NOM=…) et
	# l'étape --do en hérite : écrire sur la sortie d'erreur, qui reste visible.
	exec >&2
	case "$1" in
	principal)
		[ "$EXIT" = extraire ] || exit 0
		[ -f "$SOURCE" ] || { echo "Vidéo introuvable : $SOURCE" >&2; exit 1; }
		if [ "$EXTRAIT" = true ]; then
			for t in "$DEBUT" "$FIN"; do
				[[ "$t" =~ ^[0-9]{1,2}:[0-5][0-9]:[0-5][0-9](\.[0-9]+)?$ ]] || { echo "Temps attendu au format hh:mm:ss : $t" >&2; exit 1; }
			done
		fi
		if [ ! -e "$(cible)" ]; then extraire -n; exit; fi
		export SOURCE F_MP3 F_OGG F_FLAC F_WAV DEBIT MONO NORMALISER EXTRAIT DEBUT FIN
		export DIALOGUE; DIALOGUE=$(dialogue_ecraser "$(cible)")
		"$SERMO" --program=DIALOGUE --do='"$SERMO_SCRIPT" --suite ecraser' >/dev/null
		;;
	ecraser)
		[ "$EXIT" = remplacer ] || exit 0
		extraire -y
		;;
	esac
}

main() {
	if [ "${1:-}" = --suite ]; then suite "$2"; exit; fi
	[ -n "${1:-}" ] && SERMO=$1
	command -v ffmpeg >/dev/null || { echo "ffmpeg est absent : installez le paquet ffmpeg." >&2; exit 1; }
	export DIALOGUE; DIALOGUE=$(dialogue_principal)
	"$SERMO" --program=DIALOGUE --do='"$SERMO_SCRIPT" --suite principal' >/dev/null
}

[ "${BASH_SOURCE[0]}" = "$0" ] && main "$@"
