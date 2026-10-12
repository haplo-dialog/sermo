#!/bin/bash
# SPDX-License-Identifier: CC0-1.0
# Boîte    : Administrateur
# Besoin   : créer un groupe, y ajouter ou en retirer un membre
# Enrobe   : groupadd ; gpasswd -a / gpasswd -d ; getent group pour la liste
# Ports    : gtk3sermo conseillé ; ncursessermo par SSH
# Terminal : rien de perdu
# Usage    : ./01-groupes.sh [binaire sermo]
#            SERMO_A_BLANC=0 pour agir pour de vrai (défaut : la commande est affichée)
#            Agir demande les droits root : sans eux, le script le dit et affiche la commande.

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

# Arguments : une ligne par groupe, « nom|GID|membres ».
dialogue_principal() {
	local lignes=("$@") l
	[ ${#lignes[@]} -gt 0 ] || lignes=(
		"adm|4|syslog"
		"audio|29|utilisateur"
		"compta|1003|alice,bruno"
		"projet-web|1004|"
		"sudo|27|utilisateur")
	cat <<XML
<window title="Groupes" default-width="600" default-height="520">
  <vbox>
    <frame label="Groupes existants (GID ≥ 1000 et quelques groupes système)">
      <tree scrollable="true" height-request="160" space-expand="true" space-fill="true">
        <label>Groupe|GID|Membres</label>
        <variable>CHOISI</variable>
XML
	for l in "${lignes[@]}"; do printf '        <item>%s</item>\n' "$(xml "$l")"; done
	cat <<XML
      </tree>
    </frame>
    <frame label="Action">
      <vbox>
        <hbox>
          <radiobutton><label>Ajouter un membre</label><default>true</default><variable>A_AJOUTER</variable></radiobutton>
          <radiobutton><label>Retirer un membre</label><variable>A_RETIRER</variable></radiobutton>
          <radiobutton><label>Créer le groupe</label><variable>A_CREER</variable></radiobutton>
        </hbox>
        <hbox><text><label>Groupe :</label></text>
          <entry space-expand="true" space-fill="true"><variable>GROUPE</variable><default>compta</default></entry></hbox>
        <hbox><text><label>Membre (identifiant) :</label></text>
          <entry space-expand="true" space-fill="true"><variable>MEMBRE</variable><default>utilisateur</default></entry></hbox>
      </vbox>
    </frame>
    <frame label="Pour la création seulement">
      <vbox>
        <checkbox><label>Groupe système (--system, GID sous 1000)</label><variable>SYSTEME</variable></checkbox>
        <hbox><text><label>GID imposé (0 = choisi par groupadd) :</label></text>
          <spinbutton range-min="0" range-max="60000" range-step="1"><variable>GID</variable><default>0</default></spinbutton></hbox>
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

# Nom de groupe ou d'utilisateur à la Debian (voir NAME_REGEX dans adduser.conf).
nom_valide() { [[ $1 =~ ^[a-z_][a-z0-9_-]{0,31}$ ]]; }

suite() {
	# La sortie standard de sermo part vers /dev/null : écrire sur la sortie d'erreur.
	exec >&2
	case "$1" in
	principal)
		[ "$EXIT" = appliquer ] || exit 0
		nom_valide "$GROUPE" || { echo "Nom de groupe refusé : $GROUPE" >&2; exit 1; }
		if [ "$A_CREER" = true ]; then
			getent group -- "$GROUPE" >/dev/null && { echo "Le groupe $GROUPE existe déjà." >&2; exit 1; }
			[[ $GID =~ ^[0-9]+$ ]] && [ "$GID" -le 60000 ] || { echo "GID refusé : $GID" >&2; exit 1; }
			cmd=(groupadd)
			[ "$SYSTEME" = true ] && cmd+=(--system)
			[ "$GID" -gt 0 ] && cmd+=(--gid "$GID")
			agir "${cmd[@]}" -- "$GROUPE"
			exit
		fi
		getent group -- "$GROUPE" >/dev/null || { echo "Groupe inconnu : $GROUPE" >&2; exit 1; }
		nom_valide "$MEMBRE" || { echo "Identifiant refusé : $MEMBRE" >&2; exit 1; }
		getent passwd -- "$MEMBRE" >/dev/null || { echo "Utilisateur inconnu : $MEMBRE" >&2; exit 1; }
		if [ "$A_RETIRER" = true ]; then
			agir gpasswd --delete "$MEMBRE" "$GROUPE"
		else
			agir gpasswd --add "$MEMBRE" "$GROUPE"
		fi
		echo "La personne doit rouvrir sa session pour que le changement compte."
		;;
	esac
}

main() {
	if [ "${1:-}" = --suite ]; then suite "$2"; exit; fi
	[ -n "${1:-}" ] && SERMO=$1
	for c in groupadd gpasswd getent; do
		command -v "$c" >/dev/null || PATH=$PATH:/usr/sbin:/sbin
		command -v "$c" >/dev/null || { echo "$c est absent : installez le paquet passwd." >&2; exit 1; }
	done
	# Groupes humains (GID ≥ 1000, hors nogroup) et quelques groupes système courants.
	mapfile -t lignes < <(getent group | awk -F: '($3 >= 1000 && $3 < 65534) || $1 ~ /^(adm|sudo|audio|video|plugdev|netdev|lpadmin)$/ { print $1 "|" $3 "|" $4 }')
	export DIALOGUE; DIALOGUE=$(dialogue_principal "${lignes[@]}")
	"$SERMO" --program=DIALOGUE --do='"$SERMO_SCRIPT" --suite principal' >/dev/null
}

[ "${BASH_SOURCE[0]}" = "$0" ] && main "$@"
