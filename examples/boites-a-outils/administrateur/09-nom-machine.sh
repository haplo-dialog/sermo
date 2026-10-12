#!/bin/bash
# SPDX-License-Identifier: CC0-1.0
# Boîte    : Administrateur
# Besoin   : renommer la machine, et garder /etc/hosts d'accord avec le nouveau nom
# Enrobe   : hostnamectl (hostname, pretty, chassis, location) ; sed sur la ligne 127.0.1.1 de /etc/hosts
# Ports    : gtk3sermo conseillé ; ncursessermo par SSH
# Terminal : rien de perdu
# Usage    : ./09-nom-machine.sh [binaire sermo]
#            SERMO_A_BLANC=0 pour agir pour de vrai (défaut : la commande est affichée)
#            Renommer demande les droits root : sans eux, le script le dit et affiche la
#            commande. Les sessions ouvertes gardent l'ancien nom jusqu'à leur reconnexion.

export SERMO="${SERMO:-sermo}"
export SERMO_A_BLANC="${SERMO_A_BLANC:-1}"
# Le script lui-même, pour --do : le shell de sermo lit la variable, sans
# citation à refaire (printf %q écrit $'…' en locale C, que dash lit de travers).
export SERMO_SCRIPT="$0"

# Échappe une valeur avant de l'insérer dans le XML.
xml() { local s=${1//&/"&amp;"}; s=${s//</"&lt;"}; s=${s//>/"&gt;"}; printf '%s' "${s//\"/"&quot;"}"; }

# Lance une commande, ou l'affiche seulement en mode à blanc.
# Sans les droits root, ne lance rien : affiche la commande à lancer en root.
agir() {
	if [ "$SERMO_A_BLANC" = 1 ]; then
		printf 'à blanc :'; printf ' %q' "$@"; printf '\n'
	elif [ "$EUID" -ne 0 ]; then
		printf 'Droits root nécessaires. Commande à lancer en root :'; printf ' %q' "$@"; printf '\n'
		return 1
	else
		"$@"
	fi
}

# Arguments : nom actuel, nom affiché actuel, contenu actuel de /etc/hosts.
dialogue_principal() {
	local nom=${1:-poste-compta-03} joli=${2-"Poste compta n° 3"}
	local hosts=${3-"127.0.0.1	localhost
127.0.1.1	poste-compta-03

::1	localhost ip6-localhost ip6-loopback
ff02::1	ip6-allnodes
ff02::2	ip6-allrouters"}
	cat <<XML
<window title="Nom de la machine" default-width="560" default-height="500">
  <vbox>
    <text><label>Nom actuel : $(xml "$nom")</label></text>
    <frame label="Nouveau nom">
      <vbox>
        <hbox><text><label>Nom réseau (minuscules, chiffres, tirets) :</label></text>
          <entry space-expand="true" space-fill="true"><variable>NOM</variable><default>$(xml "$nom")</default></entry></hbox>
        <hbox><text><label>Nom affiché (libre) :</label></text>
          <entry space-expand="true" space-fill="true"><variable>JOLI</variable><default>$(xml "$joli")</default></entry></hbox>
        <hbox><text><label>Type de machine :</label></text>
          <comboboxtext><variable>CHASSIS</variable><default>desktop</default>
            <item>desktop</item><item>laptop</item><item>server</item><item>tablet</item><item>vm</item><item>container</item><item>embedded</item>
          </comboboxtext></hbox>
        <hbox><text><label>Emplacement (ex. Bureau 2, 1er étage) :</label></text>
          <entry space-expand="true" space-fill="true"><variable>LIEU</variable></entry></hbox>
      </vbox>
    </frame>
    <switch><label>Mettre à jour la ligne 127.0.1.1 de /etc/hosts</label><default>true</default><variable>HOSTS</variable></switch>
    <expander label="/etc/hosts aujourd'hui">
      <edit editable="false" height-request="120"><variable>HOSTS_VU</variable><default>$(xml "$hosts")</default></edit>
      <variable>HOSTS_OUVERT</variable>
    </expander>
    <hbox>
      <button cancel></button>
      <button><label>Renommer</label><action>exit:renommer</action></button>
    </hbox>
  </vbox>
</window>
XML
}

suite() {
	# La sortie standard de sermo part vers /dev/null : écrire sur la sortie d'erreur.
	exec >&2
	case "$1" in
	principal)
		[ "$EXIT" = renommer ] || exit 0
		# Nom d'hôte RFC 1123 : 1 à 63 caractères, ni tiret au début ni à la fin.
		[[ $NOM =~ ^[a-z0-9]([a-z0-9-]{0,61}[a-z0-9])?$ ]] || { echo "Nom refusé : $NOM" >&2; exit 1; }
		[[ $JOLI != *[[:cntrl:]]* && ${#JOLI} -le 64 ]] || { echo "Nom affiché refusé." >&2; exit 1; }
		[[ $LIEU != *[[:cntrl:]]* && ${#LIEU} -le 64 ]] || { echo "Emplacement refusé." >&2; exit 1; }
		case "$CHASSIS" in desktop|laptop|server|tablet|vm|container|embedded) ;; *) echo "Type refusé : $CHASSIS" >&2; exit 1 ;; esac
		agir hostnamectl set-hostname --static -- "$NOM" || exit 1
		[ -n "$JOLI" ] && agir hostnamectl set-hostname --pretty -- "$JOLI"
		agir hostnamectl set-chassis -- "$CHASSIS"
		[ -n "$LIEU" ] && agir hostnamectl set-location -- "$LIEU"
		if [ "$HOSTS" = true ]; then
			if grep -q '^127\.0\.1\.1[[:space:]]' /etc/hosts; then
				agir sed -i.avant-renommage -e "s/^127\.0\.1\.1[[:space:]].*/127.0.1.1\t$NOM/" -- /etc/hosts
			else
				agir tee --append -- /etc/hosts <<<"127.0.1.1	$NOM"
			fi
		fi
		echo "Reconnectez les sessions (et SSH) pour voir le nouveau nom dans l'invite."
		;;
	esac
}

main() {
	if [ "${1:-}" = --suite ]; then suite "$2"; exit; fi
	[ -n "${1:-}" ] && SERMO=$1
	command -v hostnamectl >/dev/null || { echo "hostnamectl est absent : installez le paquet systemd." >&2; exit 1; }
	export DIALOGUE; DIALOGUE=$(dialogue_principal "$(hostnamectl hostname 2>/dev/null || hostname)" \
		"$(hostnamectl hostname --pretty 2>/dev/null)" "$(cat /etc/hosts 2>/dev/null)")
	"$SERMO" --program=DIALOGUE --do='"$SERMO_SCRIPT" --suite principal' >/dev/null
}

[ "${BASH_SOURCE[0]}" = "$0" ] && main "$@"
