#!/bin/bash
# SPDX-License-Identifier: CC0-1.0
# Métier   : Serveur sans écran
# Besoin   : voir les paquets à mettre à jour et choisir entre tout mettre à jour ou un seul paquet
# Enrobe   : apt list --upgradable (lecture) ; apt-get upgrade ou apt-get install --only-upgrade
# Ports    : ncursessermo conseillé (par SSH) ; gtk3sermo sur un poste
# Terminal : rien de perdu
# Usage    : ./2-mises-a-jour.sh [binaire sermo]
#            SERMO_A_BLANC=0 pour agir pour de vrai (défaut : la commande est affichée)
#            La liste vient du dernier « apt-get update » ; l'installation demande les droits administrateur.

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

# Arguments : lignes « paquet|version proposée|version installée ».
dialogue_principal() {
	local lignes=("$@")
	[ ${#lignes[@]} -gt 0 ] || lignes=("openssl|3.5.4-1|3.5.3-1" "openssh-server|1:10.2p1-1|1:10.0p1-7" "nginx|1.28.0-3|1.28.0-2")
	cat <<XML
<window title="Mises à jour du serveur" default-width="600">
  <vbox>
    <text><label>$(xml "${#lignes[@]}") paquet(s) peuvent être mis à jour :</label></text>
    <table scrollable="true" height-request="180">
      <label>Paquet|Proposée|Installée</label>
      <variable>PAQUET</variable>
$(for l in "${lignes[@]}"; do printf '      <item>%s</item>\n' "$(xml "$l")"; done)
    </table>
    <frame label="Que faire ?">
      <vbox>
        <radiobutton><label>Tout mettre à jour (apt-get upgrade)</label><variable>TOUT</variable><default>true</default></radiobutton>
        <radiobutton><label>Seulement le paquet sélectionné</label><variable>UN_SEUL</variable></radiobutton>
        <checkbox><label>Simuler d'abord (--simulate)</label><variable>SIMULER</variable><default>true</default></checkbox>
        <checkbox><label>Garder les fichiers de configuration modifiés localement</label><variable>GARDER_CONF</variable><default>true</default></checkbox>
      </vbox>
    </frame>
    <hbox>
      <button cancel></button>
      <button><label>Mettre à jour</label><action>exit:maj</action></button>
    </hbox>
  </vbox>
</window>
XML
}

dialogue_confirmation() {
	local quoi=${1:-tous les paquets}
	cat <<XML
<window title="Confirmer la mise à jour">
  <vbox>
    <text><label>Installer pour de vrai : $(xml "$quoi") ?</label></text>
    <text><label>Un service mis à jour peut redémarrer pendant l'opération.</label></text>
    <hbox>
      <button cancel></button>
      <button><label>Installer</label><action>exit:installer</action></button>
    </hbox>
  </vbox>
</window>
XML
}

# La commande apt-get, un argument par ligne.
commande() {
	echo apt-get
	[ "$1" = simuler ] && echo --simulate
	if [ "$GARDER_CONF" = true ]; then echo -o; echo Dpkg::Options::=--force-confold; fi
	echo --assume-yes
	if [ "$UN_SEUL" = true ]; then echo install; echo --only-upgrade; echo -- ; echo "$PAQUET"
	else echo upgrade; fi
}

suite() {
	# La sortie standard de sermo part vers /dev/null : écrire sur la sortie d'erreur.
	exec >&2
	PAQUET=${PAQUET%%|*}
	case "$1" in
	principal)
		[ "$EXIT" = maj ] || exit 0
		if [ "$UN_SEUL" = true ]; then
			[[ $PAQUET =~ ^[a-z0-9][a-z0-9+.-]+$ ]] || { echo "Choisissez un paquet dans la liste." >&2; exit 1; }
			quoi="le paquet $PAQUET"
		else
			quoi="tous les paquets"
		fi
		if [ "$SIMULER" = true ]; then
			# La simulation ne modifie rien : elle est toujours lancée.
			mapfile -t cmd < <(commande simuler)
			"${cmd[@]}" 2>&1 | grep -E '^(Inst|Conf|Remv) ' | head -40
		fi
		export PAQUET TOUT UN_SEUL GARDER_CONF
		export DIALOGUE; DIALOGUE=$(dialogue_confirmation "$quoi")
		"$SERMO" --program=DIALOGUE --do='"$SERMO_SCRIPT" --suite confirmation' >/dev/null
		;;
	confirmation)
		[ "$EXIT" = installer ] || exit 0
		[ "$(id -u)" -eq 0 ] || echo "Droits administrateur nécessaires : lancez la commande ci-dessous en root."
		mapfile -t cmd < <(commande)
		agir "${cmd[@]}"
		;;
	esac
}

main() {
	if [ "${1:-}" = --suite ]; then suite "$2"; exit; fi
	[ -n "${1:-}" ] && SERMO=$1
	command -v apt >/dev/null && command -v apt-get >/dev/null || { echo "apt est absent : ce script vise Debian et ses dérivées." >&2; exit 1; }
	# Lecture seule : « paquet/suite version arch [upgradable from: ancienne] ».
	mapfile -t lignes < <(LC_ALL=C apt list --upgradable 2>/dev/null |
		awk -F'[ /]' '/upgradable from/ { v=$NF; sub(/\]$/, "", v); print $1 "|" $3 "|" v }')
	[ ${#lignes[@]} -gt 0 ] || { echo "Rien à mettre à jour (selon le dernier apt-get update)."; exit 0; }
	export DIALOGUE; DIALOGUE=$(dialogue_principal "${lignes[@]}")
	"$SERMO" --program=DIALOGUE --do='"$SERMO_SCRIPT" --suite principal' >/dev/null
}

[ "${BASH_SOURCE[0]}" = "$0" ] && main "$@"
