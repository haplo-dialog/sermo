#!/bin/bash
# SPDX-License-Identifier: CC0-1.0
# Métier   : Intégrateur de distribution
# Besoin   : au premier démarrage, régler la langue, le clavier et le fuseau horaire
# Enrobe   : localectl, timedatectl
# Ports    : gtk3sermo conseillé ; ncursessermo sur une console sans bureau
# Terminal : rien de perdu
# Usage    : ./1-premier-demarrage.sh [binaire sermo]
#            SERMO_A_BLANC=0 pour agir pour de vrai (défaut : la commande est affichée)
#            localectl et timedatectl demandent eux-mêmes les droits (polkit).

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

dialogue_principal() {
	local langue=${1:-fr_FR.UTF-8} fuseau=${2:-Europe/Paris} message=${3:-}
	cat <<XML
<window title="Premier démarrage" default-width="560">
  <vbox>
    <text><label>Bienvenue. Réglez la langue, le clavier et l'heure de ce poste.</label></text>
$([ -n "$message" ] && printf '    <text><label>%s</label></text>' "$(xml "$message")")
    <frame label="Langue du système">
      <hbox>
        <text><label>Langue (locale) :</label></text>
        <comboboxtext space-expand="true" space-fill="true">
          <variable>LANGUE</variable>
          <default>$(xml "$langue")</default>
          <item>fr_FR.UTF-8</item>
          <item>fr_BE.UTF-8</item>
          <item>fr_CH.UTF-8</item>
          <item>fr_CA.UTF-8</item>
          <item>en_US.UTF-8</item>
          <item>en_GB.UTF-8</item>
          <item>de_DE.UTF-8</item>
          <item>es_ES.UTF-8</item>
        </comboboxtext>
      </hbox>
    </frame>
    <frame label="Disposition du clavier">
      <vbox>
        <radiobutton><label>Français AZERTY (fr)</label><variable>CLAVIER_FR</variable><default>true</default></radiobutton>
        <radiobutton><label>Français AZERTY, variante latin9 (fr latin9)</label><variable>CLAVIER_LATIN9</variable></radiobutton>
        <radiobutton><label>Belge (be)</label><variable>CLAVIER_BE</variable></radiobutton>
        <radiobutton><label>Suisse romand (ch fr)</label><variable>CLAVIER_CH</variable></radiobutton>
        <radiobutton><label>Canadien français (ca fr)</label><variable>CLAVIER_CA</variable></radiobutton>
        <radiobutton><label>Anglais américain QWERTY (us)</label><variable>CLAVIER_US</variable></radiobutton>
      </vbox>
    </frame>
    <frame label="Date et heure">
      <vbox>
        <text><label>Fuseau horaire :</label></text>
        <list scrollable="true" height-request="130">
          <variable>FUSEAU</variable>
          <default>$(xml "$fuseau")</default>
          <item>Europe/Paris</item>
          <item>Europe/Brussels</item>
          <item>Europe/Zurich</item>
          <item>Europe/Luxembourg</item>
          <item>America/Montreal</item>
          <item>America/Guadeloupe</item>
          <item>America/Martinique</item>
          <item>America/Cayenne</item>
          <item>Indian/Reunion</item>
          <item>Indian/Mayotte</item>
          <item>Pacific/Noumea</item>
          <item>Pacific/Tahiti</item>
          <item>UTC</item>
        </list>
        <checkbox><label>Synchroniser l'heure par le réseau (NTP)</label><variable>NTP</variable><default>true</default></checkbox>
      </vbox>
    </frame>
    <hbox>
      <button cancel></button>
      <button><label>Appliquer</label><action>exit:appliquer</action></button>
    </hbox>
  </vbox>
</window>
XML
}

# Disposition et variante X11 choisies : « disposition variante » (variante vide possible).
clavier() {
	if   [ "$CLAVIER_LATIN9" = true ]; then echo "fr latin9"
	elif [ "$CLAVIER_BE" = true ];     then echo "be "
	elif [ "$CLAVIER_CH" = true ];     then echo "ch fr"
	elif [ "$CLAVIER_CA" = true ];     then echo "ca fr"
	elif [ "$CLAVIER_US" = true ];     then echo "us "
	else                                    echo "fr "
	fi
}

suite() {
	# La sortie standard de sermo part vers /dev/null : écrire sur la sortie d'erreur.
	exec >&2
	case "$1" in
	principal)
		[ "$EXIT" = appliquer ] || exit 0
		if [ -z "$FUSEAU" ] || [ ! -e "/usr/share/zoneinfo/$FUSEAU" ]; then
			export DIALOGUE; DIALOGUE=$(dialogue_principal "$LANGUE" Europe/Paris "Choisissez un fuseau horaire dans la liste.")
			"$SERMO" --program=DIALOGUE --do='"$SERMO_SCRIPT" --suite principal' >/dev/null
			exit
		fi
		# La locale doit être générée avant de devenir celle du système (lecture seule).
		local nom=${LANGUE%%.*}
		if ! locale -a 2>/dev/null | grep -qix "$nom\.utf-\?8"; then
			echo "La locale $LANGUE n'est pas générée sur ce poste."
			echo "Générez-la d'abord (droits d'administration) : dpkg-reconfigure locales"
		fi
		local disposition variante
		read -r disposition variante <<< "$(clavier)"
		agir localectl set-locale "LANG=$LANGUE"
		agir localectl set-x11-keymap "$disposition" "" "$variante"
		agir timedatectl set-timezone "$FUSEAU"
		if [ "$NTP" = true ]; then agir timedatectl set-ntp true; else agir timedatectl set-ntp false; fi
		echo "La langue s'applique à la prochaine ouverture de session."
		;;
	esac
}

main() {
	if [ "${1:-}" = --suite ]; then suite "$2"; exit; fi
	[ -n "${1:-}" ] && SERMO=$1
	for c in localectl timedatectl; do
		command -v "$c" >/dev/null || { echo "$c est absent : ce poste n'utilise pas systemd." >&2; exit 1; }
	done
	export DIALOGUE; DIALOGUE=$(dialogue_principal)
	"$SERMO" --program=DIALOGUE --do='"$SERMO_SCRIPT" --suite principal' >/dev/null
}

[ "${BASH_SOURCE[0]}" = "$0" ] && main "$@"
