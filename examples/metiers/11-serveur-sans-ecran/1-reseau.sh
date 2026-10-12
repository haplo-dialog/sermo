#!/bin/bash
# SPDX-License-Identifier: CC0-1.0
# Métier   : Serveur sans écran
# Besoin   : passer une connexion réseau du serveur en adresse fixe (ou revenir en DHCP) sans se tromper de syntaxe
# Enrobe   : nmcli (connection show en lecture ; connection modify et up pour appliquer)
# Ports    : ncursessermo conseillé (par SSH) ; gtk3sermo sur un poste
# Terminal : rien de perdu
# Usage    : ./1-reseau.sh [binaire sermo]
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

# Arguments : les noms des connexions connues de NetworkManager.
dialogue_principal() {
	local connexions=("$@")
	[ ${#connexions[@]} -gt 0 ] || connexions=("Connexion filaire 1" "lan-serveurs")
	cat <<XML
<window title="Réseau du serveur" default-width="520">
  <vbox>
    <hbox><text><label>Connexion :</label></text>
      <comboboxtext space-expand="true" space-fill="true"><variable>CONNEXION</variable><default>$(xml "${connexions[0]}")</default>
$(for c in "${connexions[@]}"; do printf '        <item>%s</item>\n' "$(xml "$c")"; done)
      </comboboxtext></hbox>
    <frame label="Adressage IPv4">
      <vbox>
        <radiobutton><label>Automatique (DHCP)</label><variable>DHCP</variable></radiobutton>
        <radiobutton><label>Adresse fixe</label><variable>FIXE</variable><default>true</default></radiobutton>
      </vbox>
    </frame>
    <frame label="Adresse fixe">
      <vbox>
        <hbox><text><label>Adresse/préfixe :</label></text>
          <entry space-expand="true" space-fill="true"><variable>ADRESSE</variable><default>192.168.1.20/24</default></entry></hbox>
        <hbox><text><label>Passerelle :</label></text>
          <entry space-expand="true" space-fill="true"><variable>PASSERELLE</variable><default>192.168.1.1</default></entry></hbox>
        <hbox><text><label>DNS (séparés par des espaces) :</label></text>
          <entry space-expand="true" space-fill="true"><variable>DNS</variable><default>192.168.1.1 9.9.9.9</default></entry></hbox>
      </vbox>
    </frame>
    <checkbox><label>Réactiver la connexion tout de suite</label><variable>REACTIVER</variable><default>true</default></checkbox>
    <hbox>
      <button cancel></button>
      <button><label>Préparer</label><action>exit:preparer</action></button>
    </hbox>
  </vbox>
</window>
XML
}

dialogue_confirmation() {
	local resume=${1:-lan-serveurs : 192.168.1.20/24 via 192.168.1.1}
	cat <<XML
<window title="Confirmer le changement d'adresse">
  <vbox>
    <text><label>$(xml "$resume")</label></text>
    <text><label>Si vous êtes connecté par SSH sur cette interface, la session sera coupée.</label></text>
    <text><label>Prévoyez un accès de secours (console, IPMI) avant d'appliquer.</label></text>
    <hbox>
      <button cancel></button>
      <button><label>Appliquer</label><action>exit:appliquer</action></button>
    </hbox>
  </vbox>
</window>
XML
}

ipv4() { [[ $1 =~ ^([0-9]{1,3}\.){3}[0-9]{1,3}$ ]]; }

suite() {
	# La sortie standard de sermo part vers /dev/null : écrire sur la sortie d'erreur.
	exec >&2
	case "$1" in
	principal)
		[ "$EXIT" = preparer ] || exit 0
		[ -n "$CONNEXION" ] || { echo "Choisissez une connexion." >&2; exit 1; }
		if [ "$FIXE" = true ]; then
			[[ $ADRESSE =~ ^([0-9]{1,3}\.){3}[0-9]{1,3}/[0-9]{1,2}$ ]] || { echo "Adresse refusée (attendu : 192.168.1.20/24) : $ADRESSE" >&2; exit 1; }
			[ -z "$PASSERELLE" ] || ipv4 "$PASSERELLE" || { echo "Passerelle refusée : $PASSERELLE" >&2; exit 1; }
			for d in $DNS; do ipv4 "$d" || { echo "DNS refusé : $d" >&2; exit 1; }; done
			resume="$CONNEXION : $ADRESSE via ${PASSERELLE:-aucune passerelle}, DNS ${DNS:-aucun}"
		else
			resume="$CONNEXION : adresse automatique (DHCP)"
		fi
		export CONNEXION DHCP FIXE ADRESSE PASSERELLE DNS REACTIVER
		export DIALOGUE; DIALOGUE=$(dialogue_confirmation "$resume")
		"$SERMO" --program=DIALOGUE --do='"$SERMO_SCRIPT" --suite confirmation' >/dev/null
		;;
	confirmation)
		[ "$EXIT" = appliquer ] || exit 0
		if [ "$FIXE" = true ]; then
			agir nmcli connection modify "$CONNEXION" ipv4.method manual \
				ipv4.addresses "$ADRESSE" ipv4.gateway "$PASSERELLE" ipv4.dns "${DNS// /,}"
		else
			agir nmcli connection modify "$CONNEXION" ipv4.method auto \
				ipv4.addresses "" ipv4.gateway "" ipv4.dns ""
		fi
		if [ "$REACTIVER" = true ]; then agir nmcli connection up "$CONNEXION"; fi
		;;
	esac
}

main() {
	if [ "${1:-}" = --suite ]; then suite "$2"; exit; fi
	[ -n "${1:-}" ] && SERMO=$1
	command -v nmcli >/dev/null || { echo "nmcli est absent : installez le paquet network-manager." >&2; exit 1; }
	# Lecture seule : les connexions connues (nom seul ; « \: » est un deux-points échappé).
	mapfile -t connexions < <(nmcli -t -f NAME connection show 2>/dev/null | sed 's/\\:/:/g')
	[ ${#connexions[@]} -gt 0 ] || { echo "Aucune connexion NetworkManager trouvée." >&2; exit 1; }
	export DIALOGUE; DIALOGUE=$(dialogue_principal "${connexions[@]}")
	"$SERMO" --program=DIALOGUE --do='"$SERMO_SCRIPT" --suite principal' >/dev/null
}

[ "${BASH_SOURCE[0]}" = "$0" ] && main "$@"
