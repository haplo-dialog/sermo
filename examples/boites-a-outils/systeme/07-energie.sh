#!/bin/bash
# SPDX-License-Identifier: CC0-1.0
# Boîte    : Système
# Besoin   : choisir au bout de combien de minutes l'écran s'assombrit, se met en veille puis s'éteint
# Enrobe   : xfconf-query (gestionnaire d'énergie XFCE, durable) ; xset dpms (session X en cours)
# Ports    : gtk3sermo ou qt6sermo conseillés
# Terminal : sans objet : ces réglages visent l'écran d'une session graphique
# Usage    : ./07-energie.sh [binaire sermo]
#            SERMO_A_BLANC=0 pour agir pour de vrai (défaut : la commande est affichée)
#            Hors XFCE, choisir « Session en cours » (xset) ; sous Wayland, passer par
#            les réglages du compositeur.

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

CANAL=xfce4-power-manager
P=/xfce4-power-manager

# Arguments : DPMS actif (true/false), minutes avant écran noir, avant veille,
# avant extinction, XFCE présent (true/false), résumé de l'état actuel.
dialogue_principal() {
	local actif=${1:-true} noir=${2:-10} veille=${3:-15} eteint=${4:-30} xfce=${5:-true} \
		etat=${6:-"XFCE : écran noir 10 min, veille 15 min, extinction 30 min."}
	local cible_xfce=$xfce cible_x=true
	[ "$xfce" = true ] && cible_x=false
	cat <<XML
<window title="Énergie de l'écran" default-width="520">
  <vbox>
    <frame label="Où régler">
      <vbox>
        <radiobutton><label>XFCE, durablement (gestionnaire d'énergie)</label><default>$(xml "$cible_xfce")</default><variable>CIBLE_XFCE</variable></radiobutton>
        <radiobutton><label>Session en cours seulement (xset, perdu à la déconnexion)</label><default>$(xml "$cible_x")</default><variable>CIBLE_X</variable></radiobutton>
      </vbox>
    </frame>
    <frame label="Sur secteur (0 = jamais)">
      <vbox>
        <hbox><text><label>Gestion d'énergie de l'écran (DPMS) :</label></text>
          <switch><variable>DPMS</variable><default>$(xml "$actif")</default></switch></hbox>
        <hbox><text><label>Écran noir après (min) :</label></text>
          <spinbutton range-min="0" range-max="180" range-step="1"><variable>NOIR</variable><default>$(xml "$noir")</default></spinbutton></hbox>
        <hbox><text><label>Mise en veille après (min) :</label></text>
          <spinbutton range-min="0" range-max="180" range-step="1"><variable>VEILLE</variable><default>$(xml "$veille")</default></spinbutton></hbox>
        <hbox><text><label>Extinction après (min) :</label></text>
          <spinbutton range-min="0" range-max="180" range-step="1"><variable>ETEINT</variable><default>$(xml "$eteint")</default></spinbutton></hbox>
      </vbox>
    </frame>
    <checkbox><label>Mêmes délais sur batterie (XFCE)</label><variable>BATTERIE</variable><default>true</default></checkbox>
    <statusbar><variable>ETAT</variable><default>$(xml "$etat")</default></statusbar>
    <hbox>
      <button cancel></button>
      <button><label>Appliquer</label><action>exit:appliquer</action></button>
    </hbox>
  </vbox>
</window>
XML
}

minutes_valides() { [[ $1 =~ ^[0-9]{1,3}$ ]] && [ "$1" -le 180 ]; }

# Pose une propriété XFCE : la crée (avec son type) si elle n'existe pas encore.
poser_xfce() {
	local prop=$1 type=$2 valeur=$3
	if xfconf-query -c "$CANAL" -p "$prop" >/dev/null 2>&1; then
		agir xfconf-query -c "$CANAL" -p "$prop" -s "$valeur"
	else
		agir xfconf-query -c "$CANAL" -p "$prop" -n -t "$type" -s "$valeur"
	fi
}

suite() {
	# La sortie standard de sermo part vers /dev/null : écrire sur la sortie d'erreur.
	exec >&2
	case "$1" in
	principal)
		[ "$EXIT" = appliquer ] || exit 0
		minutes_valides "$NOIR" && minutes_valides "$VEILLE" && minutes_valides "$ETEINT" ||
			{ echo "Délais hors bornes (0 à 180 minutes)." >&2; exit 1; }
		NOIR=$((10#$NOIR)); VEILLE=$((10#$VEILLE)); ETEINT=$((10#$ETEINT))
		if [ "$VEILLE" -gt 0 ] && [ "$ETEINT" -gt 0 ] && [ "$ETEINT" -lt "$VEILLE" ]; then
			echo "L'extinction doit venir après la mise en veille." >&2; exit 1
		fi
		local actif=false; [ "$DPMS" = true ] && actif=true
		if [ "$CIBLE_XFCE" = true ]; then
			command -v xfconf-query >/dev/null || { echo "xfconf-query est absent : ce n'est pas un bureau XFCE." >&2; exit 1; }
			poser_xfce "$P/dpms-enabled" bool "$actif"
			poser_xfce "$P/blank-on-ac" int "$NOIR"
			poser_xfce "$P/dpms-on-ac-sleep" uint "$VEILLE"
			poser_xfce "$P/dpms-on-ac-off" uint "$ETEINT"
			if [ "$BATTERIE" = true ]; then
				poser_xfce "$P/blank-on-battery" int "$NOIR"
				poser_xfce "$P/dpms-on-battery-sleep" uint "$VEILLE"
				poser_xfce "$P/dpms-on-battery-off" uint "$ETEINT"
			fi
		else
			[ "${XDG_SESSION_TYPE:-}" = wayland ] && echo "Session Wayland : xset ne change rien ici ; passez par les réglages du compositeur."
			if [ "$NOIR" -gt 0 ]; then agir xset s "$((NOIR * 60))"; else agir xset s off; fi
			if [ "$actif" = true ]; then
				# xset dpms : veille, suspension, extinction, en secondes (0 = jamais).
				agir xset dpms "$((VEILLE * 60))" "$((VEILLE * 60))" "$((ETEINT * 60))"
				agir xset +dpms
			else
				agir xset -dpms
			fi
		fi
		;;
	esac
}

main() {
	if [ "${1:-}" = --suite ]; then suite "$2"; exit; fi
	[ -n "${1:-}" ] && SERMO=$1
	command -v xset >/dev/null || command -v xfconf-query >/dev/null ||
		{ echo "Ni xset ni xfconf-query : installez x11-xserver-utils (ou xfconf sous XFCE)." >&2; exit 1; }
	local actif=false noir=0 veille=0 eteint=0 xfce=false etat v
	if command -v xfconf-query >/dev/null && xfconf-query -c "$CANAL" -l >/dev/null 2>&1; then
		xfce=true
		v=$(xfconf-query -c "$CANAL" -p "$P/dpms-enabled" 2>/dev/null); [ "${v:-true}" = true ] && actif=true
		noir=$(xfconf-query -c "$CANAL" -p "$P/blank-on-ac" 2>/dev/null)
		veille=$(xfconf-query -c "$CANAL" -p "$P/dpms-on-ac-sleep" 2>/dev/null)
		eteint=$(xfconf-query -c "$CANAL" -p "$P/dpms-on-ac-off" 2>/dev/null)
		etat="XFCE : écran noir ${noir:-?} min, veille ${veille:-?} min, extinction ${eteint:-?} min."
	elif command -v xset >/dev/null; then
		# xset q : « Standby: 600 Suspend: 0 Off: 1800 », « DPMS is Enabled », « timeout: 600 ».
		read -r veille eteint < <(xset q 2>/dev/null | awk '/Standby:/ { print int($2 / 60), int($6 / 60) }')
		noir=$(xset q 2>/dev/null | awk '/timeout:/ { print int($2 / 60) }')
		xset q 2>/dev/null | grep -q 'DPMS is Enabled' && actif=true
		etat="Session X : écran noir ${noir:-?} min, veille ${veille:-?} min, extinction ${eteint:-?} min."
	fi
	minutes_valides "$noir" || noir=10
	minutes_valides "$veille" || veille=15
	minutes_valides "$eteint" || eteint=30
	export DIALOGUE; DIALOGUE=$(dialogue_principal "$actif" "$noir" "$veille" "$eteint" "$xfce" "$etat")
	"$SERMO" --program=DIALOGUE --do='"$SERMO_SCRIPT" --suite principal' >/dev/null
}

[ "${BASH_SOURCE[0]}" = "$0" ] && main "$@"
