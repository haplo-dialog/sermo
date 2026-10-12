#!/bin/bash
# SPDX-License-Identifier: CC0-1.0
# Boîte    : Serveur
# Besoin   : voir les prisons fail2ban et les adresses bannies, débannir une adresse légitime (ou en bannir une à la main)
# Enrobe   : fail2ban-client status (lecture) ; fail2ban-client set <prison> unbanip / banip ; fail2ban-client unban
# Ports    : ncursessermo conseillé (par SSH) ; gtk3sermo sur un poste
# Terminal : rien de perdu (les listes montrent six lignes à la fois)
# Usage    : ./05-fail2ban.sh [binaire sermo]
#            SERMO_A_BLANC=0 pour agir pour de vrai (défaut : la commande est affichée)
#            fail2ban-client demande les droits root, même pour lire : sans eux, rien n'est lu et les commandes sont affichées.

export SERMO="${SERMO:-sermo}"
export SERMO_A_BLANC="${SERMO_A_BLANC:-1}"
# Le script lui-même, pour --do : le shell de sermo lit la variable, sans
# citation à refaire (printf %q écrit $'…' en locale C, que dash lit de travers).
export SERMO_SCRIPT="$0"

# Échappe une valeur avant de l'insérer dans le XML.
xml() { local s=${1//&/"&amp;"}; s=${s//</"&lt;"}; s=${s//>/"&gt;"}; printf '%s' "${s//\"/"&quot;"}"; }

# Une ligne de texte par <item>, chacune échappée.
items() { local l; while IFS= read -r l; do [ -n "$l" ] && printf '<item>%s</item>\n' "$(xml "$l")"; done <<<"$1"; }

# Lance une commande, ou l'affiche seulement en mode à blanc.
agir() {
	if [ "$SERMO_A_BLANC" = 1 ]; then
		printf 'à blanc :'; printf ' %q' "$@"; printf '\n'
	else
		"$@"
	fi
}

# Sans les droits root, on ne peut qu'afficher les commandes.
exiger_root() {
	if [ "$SERMO_A_BLANC" != 1 ] && [ "$EUID" -ne 0 ]; then
		echo "fail2ban-client demande les droits root : relancez ce script en root."
		echo "Voici la commande qu'il lancerait :"
		SERMO_A_BLANC=1
	fi
}

dialogue_principal() {
	local prisons=${1:-"sshd
nginx-http-auth
postfix-sasl
recidive"}
	cat <<XML
<window title="fail2ban : prisons" default-width="460">
  <vbox>
    <text><label>Prisons actives (fail2ban-client status) :</label></text>
    <list height-request="150"><variable>PRISON</variable>
$(items "$prisons")
    </list>
    <text><label>Choisissez une prison pour voir ses adresses bannies.</label></text>
    <hbox>
      <button cancel></button>
      <button><label>Voir les bannis</label><action>exit:voir</action></button>
    </hbox>
  </vbox>
</window>
XML
}

dialogue_bannis() {
	local prison=${1:-sshd} resume=${2:-3 bannies en ce moment, 41 depuis le démarrage} bannis=${3:-"203.0.113.17|sshd
198.51.100.4|sshd
2001:db8::42|sshd"}
	cat <<XML
<window title="fail2ban : $(xml "$prison")" default-width="520">
  <vbox>
    <text><label>Prison $(xml "$prison") : $(xml "$resume").</label></text>
    <tree scrollable="true" height-request="140" exported-column="0">
      <label>Adresse|Prison</label>
      <variable>CHOISIE</variable>
$(items "$bannis")
    </tree>
    <hbox><text><label>Adresse (vide = la ligne choisie) :</label></text>
      <entry space-expand="true" space-fill="true"><variable>ADRESSE</variable></entry></hbox>
    <vbox>
      <radiobutton><label>Débannir de cette prison</label><default>true</default><variable>A_DEBANNIR</variable></radiobutton>
      <radiobutton><label>Débannir de toutes les prisons</label><variable>A_PARTOUT</variable></radiobutton>
      <radiobutton><label>Bannir dans cette prison</label><variable>A_BANNIR</variable></radiobutton>
    </vbox>
    <hbox>
      <button cancel></button>
      <button><label>Appliquer</label><action>exit:appliquer</action></button>
    </hbox>
  </vbox>
</window>
XML
}

dialogue_bannir() {
	local ip=${1:-203.0.113.17} prison=${2:-sshd}
	cat <<XML
<window title="Bannir une adresse">
  <vbox>
    <text><label>L'adresse $(xml "$ip") sera bannie de la prison $(xml "$prison").</label></text>
    <text><label>Vérifiez que ce n'est pas la vôtre, ni celle d'un poste d'administration :</label></text>
    <text><label>bannie, elle ne pourra plus joindre ce serveur.</label></text>
    <hbox>
      <button cancel></button>
      <button><label>Bannir</label><action>exit:bannir</action></button>
    </hbox>
  </vbox>
</window>
XML
}

# Noms des prisons, un par ligne (lecture seule).
prisons() {
	fail2ban-client status 2>/dev/null | sed -n 's/.*Jail list:[[:space:]]*//p' | tr -d ' \t' | tr ',' '\n'
}

suite() {
	# La sortie standard de sermo part vers /dev/null : écrire sur la sortie d'erreur.
	exec >&2
	[[ $PRISON =~ ^[A-Za-z0-9_.-]{1,64}$ ]] || { [ "$EXIT" = voir ] && echo "Choisissez d'abord une prison."; exit 0; }
	case "$1" in
	principal)
		[ "$EXIT" = voir ] || exit 0
		local etat actuel total ips
		etat=$(fail2ban-client status "$PRISON" 2>/dev/null)
		actuel=$(sed -n 's/.*Currently banned:[[:space:]]*//p' <<<"$etat")
		total=$(sed -n 's/.*Total banned:[[:space:]]*//p' <<<"$etat")
		ips=$(sed -n 's/.*Banned IP list:[[:space:]]*//p' <<<"$etat" | tr -s ' \t' '\n' | sed "/^\$/d; s/\$/|$PRISON/")
		[ -n "$etat" ] || echo "Lecture impossible (il faut les droits root) : la liste sera vide."
		export PRISON
		export DIALOGUE; DIALOGUE=$(dialogue_bannis "$PRISON" "${actuel:-?} bannies en ce moment, ${total:-?} depuis le démarrage" "${ips:-(aucune)|$PRISON}")
		"$SERMO" --program=DIALOGUE --do='"$SERMO_SCRIPT" --suite bannis' >/dev/null
		;;
	bannis)
		[ "$EXIT" = appliquer ] || exit 0
		local ip=${ADRESSE:-$CHOISIE}
		ip=${ip//[[:space:]]/}
		# IPv4 ou IPv6 (chiffres, a-f, points, deux-points), avec un masque /NN au plus ;
		# jamais de tiret en tête : fail2ban-client ne connaît pas « -- » après la sous-commande.
		[[ $ip =~ ^[0-9A-Fa-f:.]{2,45}(/[0-9]{1,3})?$ && $ip == *[0-9]* ]] || { echo "Adresse refusée : ${ip:-vide}"; exit 1; }
		if [ "$A_BANNIR" = true ]; then
			# Bannir sa propre adresse coupe l'accès : refusée, et pas de réseau entier.
			local moi=${SSH_CLIENT%% *}
			[[ $ip != */* ]] || { echo "Bannir un réseau entier (/NN) est refusé ici : une adresse à la fois."; exit 1; }
			if [ -n "$moi" ] && [ "${ip,,}" = "${moi,,}" ]; then
				echo "Refus : $ip est l'adresse d'où vous êtes connecté (SSH_CLIENT)."; exit 1
			fi
			export BAN_IP=$ip PRISON
			export DIALOGUE; DIALOGUE=$(dialogue_bannir "$ip" "$PRISON")
			"$SERMO" --program=DIALOGUE --do='"$SERMO_SCRIPT" --suite bannir' >/dev/null
			exit
		fi
		exiger_root
		if [ "$A_PARTOUT" = true ]; then
			agir fail2ban-client unban "$ip"
		else
			agir fail2ban-client set "$PRISON" unbanip "$ip"
		fi
		;;
	bannir)
		[ "$EXIT" = bannir ] || exit 0
		local moi=${SSH_CLIENT%% *}
		[[ $BAN_IP =~ ^[0-9A-Fa-f:.]{2,45}$ && $BAN_IP == *[0-9]* ]] || exit 1
		if [ -n "$moi" ] && [ "${BAN_IP,,}" = "${moi,,}" ]; then echo "Refus : $BAN_IP est votre adresse."; exit 1; fi
		exiger_root
		agir fail2ban-client set "$PRISON" banip "$BAN_IP"
		;;
	esac
}

main() {
	if [ "${1:-}" = --suite ]; then suite "$2"; exit; fi
	[ -n "${1:-}" ] && SERMO=$1
	command -v fail2ban-client >/dev/null || [ -x /usr/bin/fail2ban-client ] ||
		{ echo "fail2ban est absent : installez le paquet fail2ban." >&2; exit 1; }
	local liste
	liste=$(prisons)
	[ -n "$liste" ] || { echo "Aucune prison lue : fail2ban est arrêté, ou il faut les droits root." >&2; liste="sshd"; }
	export DIALOGUE; DIALOGUE=$(dialogue_principal "$liste")
	"$SERMO" --program=DIALOGUE --do='"$SERMO_SCRIPT" --suite principal' >/dev/null
}

[ "${BASH_SOURCE[0]}" = "$0" ] && main "$@"
