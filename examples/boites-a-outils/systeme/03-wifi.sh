#!/bin/bash
# SPDX-License-Identifier: CC0-1.0
# Boîte    : Système
# Besoin   : se connecter à un réseau Wi-Fi, revenir sur un réseau connu, en oublier un, allumer ou éteindre le Wi-Fi
# Enrobe   : nmcli (NetworkManager)
# Ports    : gtk3sermo ou qt6sermo conseillés ; ncursessermo par SSH (attention : couper le Wi-Fi coupe la session)
# Terminal : rien de perdu
# Usage    : ./03-wifi.sh [binaire sermo]
#            SERMO_A_BLANC=0 pour agir pour de vrai (défaut : la commande est affichée)
#            La clé Wi-Fi passe à nmcli par un descripteur (passwd-file), jamais sur la
#            ligne de commande ; NetworkManager la garde avec le profil créé.

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

# Arguments : Wi-Fi allumé (true/false), puis des lignes « V:ssid|signal|sécurité|utilisé »
# (réseaux visibles) et « C:nom » (profils connus).
dialogue_principal() {
	local allume=${1:-true} l
	shift || true
	local visibles=() connus=()
	for l in "$@"; do
		case "$l" in V:*) visibles+=("${l#V:}") ;; C:*) connus+=("${l#C:}") ;; esac
	done
	[ ${#visibles[@]} -gt 0 ] || visibles=(
		"Maison-5G|92|WPA2|oui"
		"Maison|80|WPA2|"
		"Mediatheque-Publique|54|ouvert|"
		"Voisin-Box|31|WPA1 WPA2|")
	[ ${#connus[@]} -gt 0 ] || connus=(Maison-5G Maison Gare-Wifi-Gratuit Hotel-du-Port)
	cat <<XML
<window title="Wi-Fi" default-width="600">
  <vbox>
    <hbox><text><label>Wi-Fi allumé :</label></text>
      <switch><variable>WIFI_ALLUME</variable><default>$(xml "$allume")</default></switch></hbox>
    <notebook tab-labels="Réseaux visibles|Réseaux connus">
      <vbox>
        <table scrollable="true" height-request="150">
          <label>Réseau (SSID)|Signal (%)|Sécurité|Connecté</label>
          <variable>VISIBLE</variable>
XML
	for l in "${visibles[@]}"; do printf '          <item>%s</item>\n' "$(xml "$l")"; done
	cat <<XML
        </table>
        <frame label="Se connecter au réseau choisi (ou à un réseau masqué)">
          <vbox>
            <hbox><text><label>Réseau masqué (sinon laisser vide) :</label></text>
              <entry><variable>MASQUE</variable></entry></hbox>
            <hbox><text><label>Clé Wi-Fi (vide si réseau ouvert) :</label></text>
              <password space-expand="true" space-fill="true"><variable>CLE</variable></password></hbox>
            <checkbox><label>Profil visible de moi seul (sinon, de tous les comptes du poste)</label><variable>PRIVE</variable><default>true</default></checkbox>
          </vbox>
        </frame>
        <hbox>
          <button cancel></button>
          <button><label>Se connecter</label><action>exit:nouveau</action></button>
        </hbox>
      </vbox>
      <vbox>
        <list scrollable="true" height-request="150"><variable>CONNU</variable>
XML
	for l in "${connus[@]}"; do printf '          <item>%s</item>\n' "$(xml "$l")"; done
	cat <<XML
        </list>
        <hbox>
          <radiobutton><label>Se connecter</label><default>true</default><variable>A_CONNECTER</variable></radiobutton>
          <radiobutton><label>Oublier ce réseau</label><variable>A_OUBLIER</variable></radiobutton>
        </hbox>
        <hbox>
          <button cancel></button>
          <button><label>Appliquer</label><action>exit:connu</action></button>
        </hbox>
      </vbox>
    </notebook>
    <hbox>
      <button><label>Seulement allumer / éteindre</label><action>exit:radio</action></button>
    </hbox>
  </vbox>
</window>
XML
}

dialogue_confirmation() {
	local nom=${1:-Hotel-du-Port}
	cat <<XML
<window title="Oublier un réseau Wi-Fi ?">
  <vbox>
    <text><label>Le profil « $(xml "$nom") » sera supprimé, avec sa clé.</label></text>
    <text><label>Pour revenir sur ce réseau, il faudra retaper la clé.</label></text>
    <hbox>
      <button cancel></button>
      <button><label>Oublier</label><action>exit:oublier</action></button>
    </hbox>
  </vbox>
</window>
XML
}

# Profils Wi-Fi connus, un par ligne (le type n'a pas de « : », le nom vient en dernier).
profils_wifi() { nmcli -t -e no -f TYPE,NAME connection show 2>/dev/null | sed -n 's/^802-11-wireless://p'; }

# Nom de réseau : 1 à 32 octets imprimables, pas d'option déguisée.
ssid_valide() { [ -n "$1" ] && [ "$(printf '%s' "$1" | wc -c)" -le 32 ] && [[ $1 != -* && $1 != *[[:cntrl:]]* ]]; }

# Allume ou éteint selon l'interrupteur, si l'état change.
radio() {
	local voulu=off; [ "$WIFI_ALLUME" = true ] && voulu=on
	local actuel; actuel=$(nmcli radio wifi 2>/dev/null)
	if [ "$voulu" = on ] && [ "$actuel" != enabled ]; then agir nmcli radio wifi on
	elif [ "$voulu" = off ] && [ "$actuel" = enabled ]; then agir nmcli radio wifi off
	fi
}

suite() {
	# La sortie standard de sermo part vers /dev/null : écrire sur la sortie d'erreur.
	exec >&2
	case "$1" in
	principal)
		case "$EXIT" in
		radio) radio ;;
		nouveau)
			radio
			local ssid=${MASQUE:-${VISIBLE%%|*}} cache=no
			[ -n "$MASQUE" ] && cache=yes
			ssid_valide "$ssid" || { echo "Réseau refusé ou non choisi : $ssid" >&2; exit 1; }
			if [ "$cache" = no ]; then
				grep -qxF -- "$ssid" < <(nmcli -t -e no -f SSID device wifi list --rescan no 2>/dev/null) ||
					{ echo "Réseau non visible : $ssid" >&2; exit 1; }
			fi
			if grep -qxF -- "$ssid" < <(profils_wifi); then
				echo "Un profil « $ssid » existe déjà : on l'active (onglet Réseaux connus pour l'oublier)."
				agir nmcli connection up id "$ssid"
				exit
			fi
			# Profil privé : réservé au compte courant (nom de compte vérifié).
			local droits=()
			if [ "$PRIVE" = true ]; then
				[[ ${USER:-} =~ ^[a-z_][a-z0-9_-]*$ ]] || { echo "Nom de compte inattendu : ${USER:-}" >&2; exit 1; }
				droits=(connection.permissions "user:$USER")
			fi
			if [ -z "$CLE" ]; then
				agir nmcli connection add type wifi con-name "$ssid" ssid "$ssid" wifi.hidden "$cache" "${droits[@]}"
				agir nmcli connection up id "$ssid"
			else
				[ ${#CLE} -ge 8 ] && [ ${#CLE} -le 63 ] || { echo "Une clé WPA compte de 8 à 63 caractères." >&2; exit 1; }
				[[ $CLE != *[[:cntrl:]]* ]] || { echo "Clé refusée : caractère de contrôle." >&2; exit 1; }
				agir nmcli connection add type wifi con-name "$ssid" ssid "$ssid" wifi.hidden "$cache" \
					wifi-sec.key-mgmt wpa-psk "${droits[@]}"
				# La clé arrive sur le descripteur 3 : ni dans ps, ni à l'écran.
				agir nmcli connection up id "$ssid" passwd-file /dev/fd/3 3<<<"802-11-wireless-security.psk:$CLE"
			fi
			;;
		connu)
			radio
			[ -n "$CONNU" ] || { echo "Aucun réseau connu choisi." >&2; exit 1; }
			grep -qxF -- "$CONNU" < <(profils_wifi) || { echo "Profil inconnu : $CONNU" >&2; exit 1; }
			if [ "$A_OUBLIER" = true ]; then
				# La clé ne doit pas suivre jusqu'au second dialogue.
				export CONNU; unset CLE
				export DIALOGUE; DIALOGUE=$(dialogue_confirmation "$CONNU")
				"$SERMO" --program=DIALOGUE --do='"$SERMO_SCRIPT" --suite oubli' >/dev/null
			else
				agir nmcli connection up id "$CONNU"
			fi
			;;
		*) exit 0 ;;
		esac
		;;
	oubli)
		[ "$EXIT" = oublier ] || exit 0
		grep -qxF -- "$CONNU" < <(profils_wifi) || { echo "Profil inconnu : $CONNU" >&2; exit 1; }
		agir nmcli connection delete id "$CONNU"
		;;
	esac
}

main() {
	if [ "${1:-}" = --suite ]; then suite "$2"; exit; fi
	[ -n "${1:-}" ] && SERMO=$1
	command -v nmcli >/dev/null || { echo "nmcli est absent : installez le paquet network-manager." >&2; exit 1; }
	nmcli -t general status >/dev/null 2>&1 || { echo "NetworkManager ne répond pas : le service tourne-t-il ?" >&2; exit 1; }
	local args=(false) u sig secu ssid n
	[ "$(nmcli radio wifi 2>/dev/null)" = enabled ] && args=(true)
	# -e no : pas d'échappement ; le SSID vient en dernier, il peut contenir « : ».
	while IFS=: read -r u sig secu ssid; do
		[ -n "$ssid" ] || continue
		[ -n "$secu" ] || secu=ouvert
		[ "$u" = '*' ] && u=oui || u=""
		args+=("V:${ssid//|/ }|$sig|$secu|$u")
	done < <(nmcli -t -e no -f IN-USE,SIGNAL,SECURITY,SSID device wifi list --rescan no 2>/dev/null | awk -F: '!vu[$4]++')
	while read -r n; do args+=("C:$n"); done < <(profils_wifi)
	export DIALOGUE; DIALOGUE=$(dialogue_principal "${args[@]}")
	"$SERMO" --program=DIALOGUE --do='"$SERMO_SCRIPT" --suite principal' >/dev/null
}

[ "${BASH_SOURCE[0]}" = "$0" ] && main "$@"
