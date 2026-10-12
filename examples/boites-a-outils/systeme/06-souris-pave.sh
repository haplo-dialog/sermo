#!/bin/bash
# SPDX-License-Identifier: CC0-1.0
# Boîte    : Système
# Besoin   : régler la souris ou le pavé tactile : vitesse du pointeur, défilement naturel, tap pour cliquer, gaucher
# Enrobe   : xinput (pilote libinput)
# Ports    : gtk3sermo ou qt6sermo conseillés
# Terminal : sans objet : xinput règle une session X, pas un terminal
# Usage    : ./06-souris-pave.sh [binaire sermo]
#            SERMO_A_BLANC=0 pour agir pour de vrai (défaut : la commande est affichée)
#            Le réglage vaut pour la session en cours. Sous Wayland, xinput ne règle
#            rien : passer par les réglages du compositeur.

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

# Arguments : des lignes « id: nom de l'appareil » (appareils de pointage).
dialogue_principal() {
	local appareils=("$@") l
	[ ${#appareils[@]} -gt 0 ] || appareils=(
		"11: Logitech USB Optical Mouse"
		"12: SynPS/2 Synaptics TouchPad"
		"13: TPPS/2 IBM TrackPoint")
	cat <<XML
<window title="Souris et pavé tactile" default-width="540">
  <vbox>
    <frame label="Appareil">
      <list scrollable="true" height-request="100"><variable>APPAREIL</variable>
XML
	for l in "${appareils[@]}"; do printf '        <item>%s</item>\n' "$(xml "$l")"; done
	cat <<XML
      </list>
    </frame>
    <frame label="Pointeur">
      <vbox>
        <hbox><text><label>Vitesse (-100 lente, 0 normale, 100 rapide) :</label></text>
          <hscale range-min="-100" range-max="100" range-step="10" draw-value="true" space-expand="true" space-fill="true"><variable>VITESSE</variable><default>0</default></hscale></hbox>
        <checkbox><label>Gaucher : inverser les boutons gauche et droit</label><variable>GAUCHER</variable></checkbox>
      </vbox>
    </frame>
    <frame label="Pavé tactile et molette">
      <vbox>
        <checkbox><label>Défilement naturel (le contenu suit le doigt)</label><variable>NATUREL</variable></checkbox>
        <togglebutton><label>Taper pour cliquer</label><variable>TAP</variable><default>true</default></togglebutton>
        <checkbox><label>Ignorer le pavé pendant la frappe</label><variable>FRAPPE</variable><default>true</default></checkbox>
      </vbox>
    </frame>
    <text><label>Un réglage que l'appareil ne connaît pas est sauté (une souris n'a pas de tap).</label></text>
    <hbox>
      <button cancel></button>
      <button><label>Appliquer</label><action>exit:appliquer</action></button>
    </hbox>
  </vbox>
</window>
XML
}

# Appareils de pointage : « id: nom », un par ligne.
appareils() {
	xinput list 2>/dev/null | grep 'slave  pointer' | grep -v 'XTEST' |
		sed -E 's/^[^A-Za-z0-9]*//; s/[[:space:]]+id=([0-9]+).*/\t\1/' |
		awk -F'\t' '{ print $2 ": " $1 }'
}

# Pose une propriété libinput si l'appareil la connaît.
poser() {
	local id=$1 prop=$2; shift 2
	if xinput list-props "$id" 2>/dev/null | grep -q "^[[:space:]]*$prop ("; then
		agir xinput set-prop "$id" "$prop" "$@"
	else
		echo "« $prop » : inconnu de cet appareil, sauté."
	fi
}

suite() {
	# La sortie standard de sermo part vers /dev/null : écrire sur la sortie d'erreur.
	exec >&2
	case "$1" in
	principal)
		[ "$EXIT" = appliquer ] || exit 0
		[ -n "$APPAREIL" ] || { echo "Aucun appareil choisi." >&2; exit 1; }
		grep -qxF -- "$APPAREIL" < <(appareils) || { echo "Appareil inconnu : $APPAREIL" >&2; exit 1; }
		local id=${APPAREIL%%:*}
		[[ $id =~ ^[0-9]+$ ]] || { echo "Identifiant refusé : $id" >&2; exit 1; }
		[[ $VITESSE =~ ^-?[0-9]{1,3}$ ]] && [ "$VITESSE" -ge -100 ] && [ "$VITESSE" -le 100 ] ||
			{ echo "Vitesse hors bornes : $VITESSE" >&2; exit 1; }
		[ "${XDG_SESSION_TYPE:-}" = wayland ] && echo "Session Wayland : xinput ne change rien ici ; passez par les réglages du compositeur."
		# libinput attend -1.00 … 1.00, écrit avec un point quelle que soit la langue.
		local signe="" abs=$VITESSE vitesse
		[ "$VITESSE" -lt 0 ] && { signe=-; abs=$((-VITESSE)); }
		printf -v vitesse '%s%d.%02d' "$signe" "$((abs / 100))" "$((abs % 100))"
		local b
		poser "$id" "libinput Accel Speed" "$vitesse"
		b=0; [ "$GAUCHER" = true ] && b=1; poser "$id" "libinput Left Handed Enabled" "$b"
		b=0; [ "$NATUREL" = true ] && b=1; poser "$id" "libinput Natural Scrolling Enabled" "$b"
		b=0; [ "$TAP" = true ] && b=1; poser "$id" "libinput Tapping Enabled" "$b"
		b=0; [ "$FRAPPE" = true ] && b=1; poser "$id" "libinput Disable While Typing Enabled" "$b"
		;;
	esac
}

main() {
	if [ "${1:-}" = --suite ]; then suite "$2"; exit; fi
	[ -n "${1:-}" ] && SERMO=$1
	command -v xinput >/dev/null || { echo "xinput est absent : installez le paquet xinput." >&2; exit 1; }
	local liste=()
	mapfile -t liste < <(appareils)
	[ ${#liste[@]} -gt 0 ] || { echo "xinput ne voit aucun appareil de pointage (session X ouverte ?)." >&2; exit 1; }
	export DIALOGUE; DIALOGUE=$(dialogue_principal "${liste[@]}")
	"$SERMO" --program=DIALOGUE --do='"$SERMO_SCRIPT" --suite principal' >/dev/null
}

[ "${BASH_SOURCE[0]}" = "$0" ] && main "$@"
