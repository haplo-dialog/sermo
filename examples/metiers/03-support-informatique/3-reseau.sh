#!/bin/bash
# SPDX-License-Identifier: CC0-1.0
# Métier   : Support informatique
# Besoin   : voir les connexions réseau d'un poste, en afficher le détail, les activer ou les relancer
# Enrobe   : nmcli
# Ports    : gtk3sermo conseillé ; ncursessermo par SSH
# Terminal : rien de perdu (attention : relancer la connexion qui porte la session SSH la coupe)
# Usage    : ./3-reseau.sh [binaire sermo]
#            SERMO_A_BLANC=0 pour agir pour de vrai (défaut : la commande est affichée)

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

# Arguments : connectivité, Wi-Fi actif (true/false), puis une ligne par
# connexion « nom|type|interface|état ».
dialogue_principal() {
	[ $# -gt 0 ] || set -- full true \
		"Bureau filaire|802-3-ethernet|enp3s0|activated" \
		"Wi-Fi Invités|802-11-wireless||" \
		"VPN siège|vpn||"
	local connectivite=$1 wifi=$2 l; shift 2
	cat <<XML
<window title="Connexions réseau" default-width="600" default-height="440">
  <vbox>
    <text><label>Accès au réseau : $(xml "$connectivite")</label></text>
    <tree scrollable="true" space-expand="true" space-fill="true">
      <label>Connexion|Type|Interface|État</label>
      <variable>CONNEXION</variable>
XML
	for l in "$@"; do printf '      <item>%s</item>\n' "$(xml "$l")"; done
	cat <<XML
    </tree>
    <frame label="Action sur la connexion choisie" space-expand="false" space-fill="false">
      <hbox>
        <radiobutton><label>Détails</label><default>true</default><variable>A_DETAILS</variable></radiobutton>
        <radiobutton><label>Activer</label><variable>A_ACTIVER</variable></radiobutton>
        <radiobutton><label>Désactiver</label><variable>A_DESACTIVER</variable></radiobutton>
        <radiobutton><label>Relancer</label><variable>A_RELANCER</variable></radiobutton>
      </hbox>
    </frame>
    <checkbox><label>Wi-Fi allumé</label><default>$(xml "$wifi")</default><variable>WIFI</variable></checkbox>
    <hbox>
      <button cancel></button>
      <button><label>Appliquer</label><action>exit:appliquer</action></button>
    </hbox>
  </vbox>
</window>
XML
}

# Arguments : fichier du détail, nom de la connexion.
dialogue_details() {
	local fichier=${1:-} nom=${2:-Bureau filaire}
	cat <<XML
<window title="Connexion $(xml "$nom")" default-width="640" default-height="500">
  <vbox>
    <edit editable="false" space-expand="true" space-fill="true">
XML
	if [ -n "$fichier" ]; then
		printf '      <input file>%s</input>\n' "$(xml "$fichier")"
	else
		printf '      <default>%s</default>\n' "IP4.ADDRESS[1]:   192.168.1.42/24 (démonstration)"
	fi
	cat <<XML
    </edit>
    <hbox><button><label>Fermer</label><action>exit:fermer</action></button></hbox>
  </vbox>
</window>
XML
}

dialogue_confirmation() {
	local resume=${1:-désactiver « Bureau filaire »}
	cat <<XML
<window title="Confirmer">
  <vbox>
    <text><label>Le poste va perdre le réseau un moment :</label></text>
    <text><label>$(xml "$resume")</label></text>
    <hbox>
      <button cancel></button>
      <button><label>Confirmer</label><action>exit:confirmer</action></button>
    </hbox>
  </vbox>
</window>
XML
}

suite() {
	exec >&2
	case "$1" in
	principal)
		[ "$EXIT" = appliquer ] || exit 0
		local coupe=()
		[ "$WIFI" = false ] && [ "$WIFI_AVANT" = true ] && coupe+=("éteindre le Wi-Fi")
		if [ "$A_DETAILS" != true ]; then
			[ -n "$CONNEXION" ] || { echo "Aucune connexion choisie." >&2; exit 1; }
			[ "$A_DESACTIVER" = true ] && coupe+=("désactiver « $CONNEXION »")
			[ "$A_RELANCER" = true ] && coupe+=("relancer « $CONNEXION »")
		fi
		if [ ${#coupe[@]} -gt 0 ]; then
			export CONNEXION A_DETAILS A_ACTIVER A_DESACTIVER A_RELANCER WIFI WIFI_AVANT
			local IFS=,
			export DIALOGUE; DIALOGUE=$(dialogue_confirmation "${coupe[*]}")
			"$SERMO" --program=DIALOGUE --do='"$SERMO_SCRIPT" --suite confirmation' >/dev/null
			exit
		fi
		appliquer
		;;
	confirmation)
		[ "$EXIT" = confirmer ] || exit 0
		appliquer
		;;
	esac
}

appliquer() {
	if [ "$WIFI" != "$WIFI_AVANT" ]; then
		if [ "$WIFI" = true ]; then agir nmcli radio wifi on; else agir nmcli radio wifi off; fi
	fi
	if [ "$A_DETAILS" = true ]; then
		[ -n "$CONNEXION" ] || exit 0
		local f; f=$(mktemp "${XDG_RUNTIME_DIR:-$HOME}/sermo-nmcli-XXXXXX") || exit 1
		# Lecture seule : les secrets (mots de passe Wi-Fi) ne sont pas demandés.
		nmcli connection show id "$CONNEXION" > "$f" 2>&1
		export DIALOGUE; DIALOGUE=$(dialogue_details "$f" "$CONNEXION")
		"$SERMO" --program=DIALOGUE >/dev/null
		rm -f -- "$f"
	elif [ "$A_ACTIVER" = true ]; then
		agir nmcli connection up id "$CONNEXION"
	elif [ "$A_DESACTIVER" = true ]; then
		agir nmcli connection down id "$CONNEXION"
	elif [ "$A_RELANCER" = true ]; then
		agir nmcli connection down id "$CONNEXION"
		agir nmcli connection up id "$CONNEXION"
	fi
}

main() {
	if [ "${1:-}" = --suite ]; then suite "$2"; exit; fi
	[ -n "${1:-}" ] && SERMO=$1
	command -v nmcli >/dev/null || { echo "nmcli est absent : ce poste n'utilise pas NetworkManager." >&2; exit 1; }
	local connectivite lignes=() l champs
	connectivite=$(nmcli networking connectivity 2>/dev/null)
	WIFI_AVANT=false
	[ "$(nmcli radio wifi 2>/dev/null)" = enabled ] && WIFI_AVANT=true
	export WIFI_AVANT
	# nmcli -t sépare par « : » et écrit « \: » pour un deux-points dans un nom.
	while IFS= read -r l; do
		l=${l//\\:/$'\x01'}; l=${l//|//}
		IFS=: read -r -a champs <<<"$l"
		l="${champs[0]}|${champs[1]}|${champs[2]:-}|${champs[3]:-}"
		lignes+=("${l//$'\x01'/:}")
	done < <(nmcli -t -f NAME,TYPE,DEVICE,STATE connection show 2>/dev/null)
	[ ${#lignes[@]} -gt 0 ] || { echo "Aucune connexion connue de NetworkManager." >&2; exit 1; }
	export DIALOGUE; DIALOGUE=$(dialogue_principal "${connectivite:-inconnu}" "$WIFI_AVANT" "${lignes[@]}")
	"$SERMO" --program=DIALOGUE --do='"$SERMO_SCRIPT" --suite principal' >/dev/null
}

[ "${BASH_SOURCE[0]}" = "$0" ] && main "$@"
