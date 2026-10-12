#!/bin/bash
# SPDX-License-Identifier: CC0-1.0
# Boîte    : Système
# Besoin   : choisir la sortie et l'entrée du son, régler leur volume, couper ou rétablir le son
# Enrobe   : pactl (PipeWire ou PulseAudio) ; wpctl (WirePlumber) en repli pour le volume et le muet
# Ports    : gtk3sermo ou qt6sermo conseillés ; ncursessermo possible
# Terminal : rien de perdu
# Usage    : ./02-son.sh [binaire sermo]
#            SERMO_A_BLANC=0 pour agir pour de vrai (défaut : la commande est affichée)
#            Sans pactl, wpctl règle seulement les appareils par défaut (pas de choix d'appareil).

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

# Arguments : volume de sortie (%), sortie muette (true/false), volume d'entrée (%),
# entrée muette, puis des lignes « S:nom » (sorties) et « E:nom » (entrées).
dialogue_principal() {
	local vol_s=${1:-45} muet_s=${2:-false} vol_e=${3:-70} muet_e=${4:-false} l
	shift 4 2>/dev/null || set --
	local sorties=() entrees=()
	for l in "$@"; do
		case "$l" in S:*) sorties+=("${l#S:}") ;; E:*) entrees+=("${l#E:}") ;; esac
	done
	[ ${#sorties[@]} -gt 0 ] || sorties=(alsa_output.pci-0000_00_1f.3.analog-stereo alsa_output.pci-0000_01_00.1.hdmi-stereo bluez_output.00_1A_7D_DA_71_13.1)
	[ ${#entrees[@]} -gt 0 ] || entrees=(alsa_input.pci-0000_00_1f.3.analog-stereo alsa_input.usb-Logitech_Webcam_C270-02.mono-fallback)
	cat <<XML
<window title="Son" default-width="620">
  <vbox>
    <notebook tab-labels="Sortie (haut-parleurs, casque)|Entrée (micro)">
      <vbox>
        <text><label>Sortie par défaut (laisser sans choix pour ne pas la changer) :</label></text>
        <list scrollable="true" height-request="110"><variable>SORTIE</variable>
XML
	for l in "${sorties[@]}"; do printf '          <item>%s</item>\n' "$(xml "$l")"; done
	cat <<XML
        </list>
        <hbox><text><label>Volume (%) :</label></text>
          <hscale range-min="0" range-max="150" range-step="5" draw-value="true" space-expand="true" space-fill="true"><variable>VOL_SORTIE</variable><default>$(xml "$vol_s")</default></hscale></hbox>
        <togglebutton><label>Couper le son de la sortie</label><variable>MUET_SORTIE</variable><default>$(xml "$muet_s")</default></togglebutton>
      </vbox>
      <vbox>
        <text><label>Entrée par défaut (laisser sans choix pour ne pas la changer) :</label></text>
        <list scrollable="true" height-request="110"><variable>ENTREE</variable>
XML
	for l in "${entrees[@]}"; do printf '          <item>%s</item>\n' "$(xml "$l")"; done
	cat <<XML
        </list>
        <hbox><text><label>Volume (%) :</label></text>
          <hscale range-min="0" range-max="150" range-step="5" draw-value="true" space-expand="true" space-fill="true"><variable>VOL_ENTREE</variable><default>$(xml "$vol_e")</default></hscale></hbox>
        <hbox><text><label>Micro coupé :</label></text>
          <switch><variable>MUET_ENTREE</variable><default>$(xml "$muet_e")</default></switch></hbox>
      </vbox>
    </notebook>
    <text><label>Au-delà de 100 %, le son est amplifié et peut saturer.</label></text>
    <hbox>
      <button cancel></button>
      <button><label>Appliquer</label><action>exit:appliquer</action></button>
    </hbox>
  </vbox>
</window>
XML
}

# Volume en % de l'appareil par défaut (« sink » ou « source »), lu par pactl.
volume_pactl() { pactl "get-default-$1" >/dev/null 2>&1 && pactl "get-$1-volume" "@DEFAULT_${1^^}@" 2>/dev/null | grep -oE '[0-9]+%' | head -1 | tr -d %; }
muet_pactl() { pactl "get-$1-mute" "@DEFAULT_${1^^}@" 2>/dev/null | grep -q 'yes' && echo true || echo false; }

# Nom d'appareil PulseAudio/PipeWire : lettres, chiffres, « . _ : - », pas d'option.
nom_valide() { [[ $1 =~ ^[A-Za-z0-9][A-Za-z0-9._:-]*$ ]]; }
pourcent_valide() { [[ $1 =~ ^[0-9]{1,3}$ ]] && [ "$1" -le 150 ]; }

suite() {
	# La sortie standard de sermo part vers /dev/null : écrire sur la sortie d'erreur.
	exec >&2
	case "$1" in
	principal)
		[ "$EXIT" = appliquer ] || exit 0
		pourcent_valide "$VOL_SORTIE" || { echo "Volume de sortie hors bornes : $VOL_SORTIE" >&2; exit 1; }
		pourcent_valide "$VOL_ENTREE" || { echo "Volume d'entrée hors bornes : $VOL_ENTREE" >&2; exit 1; }
		local m_s=0 m_e=0
		[ "$MUET_SORTIE" = true ] && m_s=1
		[ "$MUET_ENTREE" = true ] && m_e=1
		if command -v pactl >/dev/null; then
			if [ -n "$SORTIE" ]; then
				nom_valide "$SORTIE" && grep -qxF -- "$SORTIE" < <(pactl list short sinks | cut -f2) ||
					{ echo "Sortie inconnue : $SORTIE" >&2; exit 1; }
				agir pactl set-default-sink "$SORTIE"
			fi
			if [ -n "$ENTREE" ]; then
				nom_valide "$ENTREE" && grep -qxF -- "$ENTREE" < <(pactl list short sources | cut -f2) ||
					{ echo "Entrée inconnue : $ENTREE" >&2; exit 1; }
				agir pactl set-default-source "$ENTREE"
			fi
			agir pactl set-sink-volume @DEFAULT_SINK@ "$VOL_SORTIE%"
			agir pactl set-sink-mute @DEFAULT_SINK@ "$m_s"
			agir pactl set-source-volume @DEFAULT_SOURCE@ "$VOL_ENTREE%"
			agir pactl set-source-mute @DEFAULT_SOURCE@ "$m_e"
		else
			[ -z "$SORTIE$ENTREE" ] || echo "Sans pactl, le choix de l'appareil est ignoré (seuls volume et muet sont réglés)."
			agir wpctl set-volume @DEFAULT_AUDIO_SINK@ "$VOL_SORTIE%"
			agir wpctl set-mute @DEFAULT_AUDIO_SINK@ "$m_s"
			agir wpctl set-volume @DEFAULT_AUDIO_SOURCE@ "$VOL_ENTREE%"
			agir wpctl set-mute @DEFAULT_AUDIO_SOURCE@ "$m_e"
		fi
		;;
	esac
}

main() {
	if [ "${1:-}" = --suite ]; then suite "$2"; exit; fi
	[ -n "${1:-}" ] && SERMO=$1
	local args=() n
	if command -v pactl >/dev/null; then
		pactl info >/dev/null 2>&1 || { echo "pactl ne joint aucun serveur de son (PipeWire ou PulseAudio)." >&2; exit 1; }
		args=("$(volume_pactl sink)" "$(muet_pactl sink)" "$(volume_pactl source)" "$(muet_pactl source)")
		while read -r n; do args+=("S:$n"); done < <(pactl list short sinks | cut -f2)
		# Les « .monitor » sont des copies de sortie, pas des micros.
		while read -r n; do [[ $n == *.monitor ]] || args+=("E:$n"); done < <(pactl list short sources | cut -f2)
	elif command -v wpctl >/dev/null; then
		args=("" "" "" "" "S:@DEFAULT_AUDIO_SINK@" "E:@DEFAULT_AUDIO_SOURCE@")
	else
		echo "Ni pactl ni wpctl : installez pulseaudio-utils (ou wireplumber)." >&2; exit 1
	fi
	export DIALOGUE; DIALOGUE=$(dialogue_principal "${args[@]}")
	"$SERMO" --program=DIALOGUE --do='"$SERMO_SCRIPT" --suite principal' >/dev/null
}

[ "${BASH_SOURCE[0]}" = "$0" ] && main "$@"
