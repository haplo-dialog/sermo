#!/bin/bash
# SPDX-License-Identifier: CC0-1.0
# Métier   : Support informatique
# Besoin   : relever l'état d'un poste (système, matériel, mémoire, disques, réseau) en texte à copier
# Enrobe   : uname, lscpu, free, df, lsblk, ip, dpkg-query
# Ports    : gtk3sermo conseillé ; ncursessermo par SSH
# Terminal : rien de perdu
# Usage    : ./1-releve-poste.sh [binaire sermo]
#            SERMO_A_BLANC=0 pour agir pour de vrai (défaut : la commande est affichée)
#            Le relevé ne fait que lire ; seul son enregistrement passe par agir.

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
	cat <<XML
<window title="Relevé du poste" default-width="460">
  <vbox>
    <frame label="Pour qui">
      <vbox>
        <hbox><text><label>Demandeur :</label></text>
          <entry space-expand="true" space-fill="true"><variable>DEMANDEUR</variable></entry></hbox>
        <hbox><text><label>N° de ticket :</label></text>
          <entry space-expand="true" space-fill="true"><variable>TICKET</variable></entry></hbox>
      </vbox>
    </frame>
    <frame label="Ce que le relevé contient">
      <vbox>
        <checkbox><label>Système (distribution, noyau, durée de marche)</label><default>true</default><variable>R_SYSTEME</variable></checkbox>
        <checkbox><label>Processeur</label><default>true</default><variable>R_MATERIEL</variable></checkbox>
        <checkbox><label>Mémoire</label><default>true</default><variable>R_MEMOIRE</variable></checkbox>
        <checkbox><label>Disques et partitions</label><default>true</default><variable>R_DISQUES</variable></checkbox>
        <checkbox><label>Réseau (adresses, passerelle)</label><default>true</default><variable>R_RESEAU</variable></checkbox>
        <checkbox><label>Versions de quelques logiciels clés</label><variable>R_LOGICIELS</variable></checkbox>
      </vbox>
    </frame>
    <hbox>
      <button cancel></button>
      <button><label>Faire le relevé</label><action>exit:relever</action></button>
    </hbox>
  </vbox>
</window>
XML
}

# Arguments : fichier du relevé, nom de fichier proposé pour l'enregistrer.
dialogue_releve() {
	local fichier=${1:-} sortie=${2:-$HOME/releve-poste.txt}
	cat <<XML
<window title="Relevé" default-width="700" default-height="540">
  <vbox>
    <text><label>Vous pouvez compléter le texte avant de l'enregistrer ou de le copier.</label></text>
    <edit space-expand="true" space-fill="true">
      <variable>RELEVE</variable>
XML
	if [ -n "$fichier" ]; then
		printf '      <input file>%s</input>\n' "$(xml "$fichier")"
	else
		printf '      <default>%s</default>\n' "Poste : poste-accueil · Debian GNU/Linux forky · noyau 7.1.13 (démonstration)"
	fi
	cat <<XML
    </edit>
    <hbox><text><label>Enregistrer sous :</label></text>
      <entry space-expand="true" space-fill="true"><variable>FICHIER</variable><default>$(xml "$sortie")</default></entry></hbox>
    <hbox>
      <button><label>Fermer</label><action>exit:fermer</action></button>
      <button><label>Enregistrer</label><action>exit:enregistrer</action></button>
    </hbox>
  </vbox>
</window>
XML
}

# Le relevé lui-même : uniquement des commandes qui lisent.
releve() {
	echo "Relevé du poste $(hostname) — $(date '+%Y-%m-%d %H:%M')"
	[ -n "$DEMANDEUR" ] && echo "Demandeur : $DEMANDEUR"
	[ -n "$TICKET" ] && echo "Ticket : $TICKET"
	if [ "$R_SYSTEME" = true ]; then
		echo; echo "== Système"
		( . /etc/os-release 2>/dev/null && echo "Distribution : ${PRETTY_NAME:-inconnue}" )
		echo "Noyau : $(uname -srm)"
		echo "En marche depuis : $(uptime -p 2>/dev/null)"
	fi
	if [ "$R_MATERIEL" = true ]; then
		echo; echo "== Processeur"
		LC_ALL=C lscpu 2>/dev/null | grep -E '^(Model name|CPU\(s\)|Architecture):' || echo "(lscpu indisponible)"
	fi
	if [ "$R_MEMOIRE" = true ]; then
		echo; echo "== Mémoire"
		free -h 2>/dev/null
	fi
	if [ "$R_DISQUES" = true ]; then
		echo; echo "== Disques"
		lsblk -o NAME,SIZE,TYPE,FSTYPE,MOUNTPOINTS 2>/dev/null
		echo
		df -h -x tmpfs -x devtmpfs -x squashfs -x overlay -x efivarfs 2>/dev/null
	fi
	if [ "$R_RESEAU" = true ]; then
		echo; echo "== Réseau"
		ip -brief address 2>/dev/null
		ip route show default 2>/dev/null
	fi
	if [ "$R_LOGICIELS" = true ] && command -v dpkg-query >/dev/null; then
		echo; echo "== Logiciels"
		dpkg-query -W -f='${Package} ${Version}\n' firefox-esr thunderbird libreoffice-core cups network-manager 2>/dev/null
	fi
}

suite() {
	exec >&2
	case "$1" in
	principal)
		[ "$EXIT" = relever ] || exit 0
		local f; f=$(mktemp "${XDG_RUNTIME_DIR:-$HOME}/sermo-releve-XXXXXX") || exit 1
		releve > "$f" 2>&1
		local sortie
		sortie="$HOME/releve-$(hostname)-$(date +%Y%m%d-%H%M).txt"
		export DIALOGUE; DIALOGUE=$(dialogue_releve "$f" "$sortie")
		"$SERMO" --program=DIALOGUE --do='"$SERMO_SCRIPT" --suite releve' >/dev/null
		rm -f -- "$f"
		;;
	releve)
		[ "$EXIT" = enregistrer ] || exit 0
		[ -n "$FICHIER" ] || { echo "Nom de fichier vide." >&2; exit 1; }
		[ -e "$FICHIER" ] && { echo "Existe déjà, rien n'est écrasé : $FICHIER" >&2; exit 1; }
		agir install -m 600 /dev/stdin "$FICHIER" <<<"$RELEVE"
		;;
	esac
}

main() {
	if [ "${1:-}" = --suite ]; then suite "$2"; exit; fi
	[ -n "${1:-}" ] && SERMO=$1
	local c
	for c in uname free df lsblk; do
		command -v "$c" >/dev/null || { echo "$c est absent : relevé impossible." >&2; exit 1; }
	done
	export DIALOGUE; DIALOGUE=$(dialogue_principal)
	"$SERMO" --program=DIALOGUE --do='"$SERMO_SCRIPT" --suite principal' >/dev/null
}

[ "${BASH_SOURCE[0]}" = "$0" ] && main "$@"
