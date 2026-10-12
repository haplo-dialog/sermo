#!/bin/bash
# SPDX-License-Identifier: CC0-1.0
# Métier   : Multimédia (tutoriels vidéo, formation, support)
# Besoin   : enregistrer l'écran ou une zone (images par seconde, son, souris, durée) dans un fichier vidéo
# Enrobe   : ffmpeg -f x11grab (et -f pulse pour le son) ; xdpyinfo (taille de l'écran, lecture seule)
# Ports    : gtk3sermo conseillé ; qt6sermo, fltk1sermo
# Terminal : sans serveur X il n'y a rien à filmer : le script affiche la commande, à lancer sur le poste graphique
# Usage    : ./5-capture-ecran.sh [binaire sermo]
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

# $1 largeur, $2 hauteur de l'écran, $3 affichage X, $4 fichier proposé.
dialogue_principal() {
	local l=${1:-1920} h=${2:-1080} ecran=${3:-:0} fichier=${4:-$HOME/Vidéos/capture-2026-10-12_1030.mkv}
	cat <<XML
<window title="Capture d'écran vidéo" default-width="560">
  <vbox>
    <text xalign="0"><label>Écran $(xml "$ecran") : $(xml "$l") x $(xml "$h") pixels</label></text>
    <frame label="Zone filmée">
      <vbox>
        <radiobutton><label>Tout l'écran</label><variable>TOUT</variable><default>true</default></radiobutton>
        <radiobutton><label>Une zone :</label><variable>ZONE</variable></radiobutton>
        <hbox>
          <text><label>x</label></text>
          <spinbutton range-min="0" range-max="$(xml "$l")" range-step="10"><variable>X</variable><default>0</default></spinbutton>
          <text><label>y</label></text>
          <spinbutton range-min="0" range-max="$(xml "$h")" range-step="10"><variable>Y</variable><default>0</default></spinbutton>
          <text><label>largeur</label></text>
          <spinbutton range-min="64" range-max="$(xml "$l")" range-step="10"><variable>LARGEUR</variable><default>1280</default></spinbutton>
          <text><label>hauteur</label></text>
          <spinbutton range-min="64" range-max="$(xml "$h")" range-step="10"><variable>HAUTEUR</variable><default>720</default></spinbutton>
        </hbox>
      </vbox>
    </frame>
    <frame label="Réglages">
      <vbox>
        <text xalign="0"><label>Images par seconde :</label></text>
        <hscale range-min="5" range-max="60" range-step="5" draw-value="true"><variable>IPS</variable><default>30</default></hscale>
        <hbox><text><label>Durée maximale (minutes, 0 = jusqu'à « q ») :</label></text>
          <spinbutton range-min="0" range-max="240" range-step="1"><variable>DUREE</variable><default>0</default></spinbutton></hbox>
        <hbox><text><label>Encodage :</label></text>
          <comboboxtext space-expand="true" space-fill="true">
            <variable>ENCODAGE</variable>
            <default>Équilibré</default>
            <item>Rapide (machine modeste, fichier lourd)</item>
            <item>Équilibré</item>
            <item>Compact (fichier léger, processeur sollicité)</item>
          </comboboxtext></hbox>
        <checkbox><label>Enregistrer le son du poste (PulseAudio / PipeWire)</label><variable>SON</variable><default>true</default></checkbox>
        <checkbox><label>Montrer le pointeur de la souris</label><variable>SOURIS</variable><default>true</default></checkbox>
      </vbox>
    </frame>
    <hbox><text><label>Fichier :</label></text>
      <entry space-expand="true" space-fill="true"><variable>FICHIER</variable><default>$(xml "$fichier")</default></entry></hbox>
    <hbox>
      <button cancel></button>
      <button><label>Préparer l'enregistrement</label><action>exit:enregistrer</action></button>
    </hbox>
  </vbox>
</window>
XML
}

suite() {
	# La sortie standard de sermo part vers /dev/null (les lignes NOM=…) et
	# l'étape --do en hérite : écrire sur la sortie d'erreur, qui reste visible.
	exec >&2
	case "$1" in
	principal)
		[ "$EXIT" = enregistrer ] || exit 0
		[ -e "$FICHIER" ] && { echo "$FICHIER existe déjà : choisissez un autre nom." >&2; exit 1; }
		if [ "$TOUT" = true ]; then
			taille="${ECRAN_L}x${ECRAN_H}"; origine="$ECRAN"
		else
			# x11grab refuse une zone qui déborde de l'écran.
			if [ $(( ${X%.*} + ${LARGEUR%.*} )) -gt "$ECRAN_L" ] || [ $(( ${Y%.*} + ${HAUTEUR%.*} )) -gt "$ECRAN_H" ]; then
				echo "La zone dépasse de l'écran ($ECRAN_L x $ECRAN_H)." >&2; exit 1
			fi
			# Largeur et hauteur paires : exigé par H.264 en 4:2:0.
			taille="$(( ${LARGEUR%.*} / 2 * 2 ))x$(( ${HAUTEUR%.*} / 2 * 2 ))"
			origine="$ECRAN+${X%.*},${Y%.*}"
		fi
		preset=veryfast
		case "$ENCODAGE" in Rapide*) preset=ultrafast ;; Compact*) preset=slow ;; esac
		souris=0; [ "$SOURIS" = true ] && souris=1
		son=(); [ "$SON" = true ] && son=(-f pulse -i default -c:a aac -b:a 128k)
		duree=(); [ "${DUREE%.*}" -gt 0 ] && duree=(-t $(( ${DUREE%.*} * 60 )))
		[ -d "${FICHIER%/*}" ] || agir mkdir -p -- "${FICHIER%/*}"
		echo "Pendant l'enregistrement, touche « q » dans le terminal pour arrêter."
		agir ffmpeg -hide_banner -n -f x11grab -draw_mouse "$souris" -framerate "${IPS%.*}" -video_size "$taille" -i "$origine" \
			"${son[@]}" -c:v libx264 -preset "$preset" -crf 23 -pix_fmt yuv420p "${duree[@]}" "$FICHIER"
		;;
	esac
}

main() {
	if [ "${1:-}" = --suite ]; then suite "$2"; exit; fi
	[ -n "${1:-}" ] && SERMO=$1
	command -v ffmpeg >/dev/null || { echo "ffmpeg est absent : installez le paquet ffmpeg." >&2; exit 1; }
	command -v xdpyinfo >/dev/null || { echo "xdpyinfo est absent : installez le paquet x11-utils." >&2; exit 1; }
	[ -n "${DISPLAY:-}" ] || { echo "Pas d'affichage X (DISPLAY vide) : rien à filmer." >&2; exit 1; }
	[ -n "${WAYLAND_DISPLAY:-}" ] && echo "Session Wayland : x11grab ne voit que les fenêtres XWayland." >&2
	local taille
	taille=$(xdpyinfo 2>/dev/null | awk '/dimensions:/ { print $2; exit }')
	[ -n "$taille" ] || { echo "Taille de l'écran illisible (xdpyinfo)." >&2; exit 1; }
	export ECRAN=$DISPLAY ECRAN_L=${taille%x*} ECRAN_H=${taille#*x}
	export DIALOGUE; DIALOGUE=$(dialogue_principal "$ECRAN_L" "$ECRAN_H" "$ECRAN" "$HOME/Vidéos/capture-$(date +%F_%H%M).mkv")
	"$SERMO" --program=DIALOGUE --do='"$SERMO_SCRIPT" --suite principal' >/dev/null
}

[ "${BASH_SOURCE[0]}" = "$0" ] && main "$@"
