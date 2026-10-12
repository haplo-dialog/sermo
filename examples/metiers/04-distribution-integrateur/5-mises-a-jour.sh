#!/bin/bash
# SPDX-License-Identifier: CC0-1.0
# Métier   : Intégrateur de distribution
# Besoin   : régler la fréquence des mises à jour et leur installation automatique, ou voir celles en attente
# Enrobe   : unattended-upgrades (fichiers de /etc/apt/apt.conf.d), apt list, apt-get
# Ports    : gtk3sermo conseillé ; ncursessermo par SSH
# Terminal : rien de perdu
# Usage    : ./5-mises-a-jour.sh [binaire sermo]
#            SERMO_A_BLANC=0 pour agir pour de vrai (défaut : la commande est affichée)
#            L'écriture dans /etc et apt-get demandent les droits d'administration : lancer alors en root.

export SERMO="${SERMO:-sermo}"
export SERMO_A_BLANC="${SERMO_A_BLANC:-1}"
# Le script lui-même, pour --do : le shell de sermo lit la variable, sans
# citation à refaire (printf %q écrit $'…' en locale C, que dash lit de travers).
export SERMO_SCRIPT="$0"

PERIODIQUE=/etc/apt/apt.conf.d/20auto-upgrades
REGLAGES=/etc/apt/apt.conf.d/52haplo-unattended-upgrades

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
<window title="Mises à jour" default-width="540">
  <vbox>
    <frame label="Rechercher les mises à jour">
      <vbox>
        <radiobutton><label>Chaque jour (conseillé)</label><variable>FREQ_JOUR</variable><default>true</default></radiobutton>
        <radiobutton><label>Chaque semaine</label><variable>FREQ_SEMAINE</variable></radiobutton>
        <radiobutton><label>Jamais seul : je les lance moi-même</label><variable>FREQ_MANUEL</variable></radiobutton>
      </vbox>
    </frame>
    <frame label="Installation automatique">
      <vbox>
        <checkbox><label>Installer seul les mises à jour trouvées</label><variable>AUTO</variable><default>true</default></checkbox>
        <hbox><text><label>Lesquelles :</label></text>
          <comboboxtext space-expand="true" space-fill="true"><variable>ORIGINE</variable><default>Sécurité seulement</default>
            <item>Sécurité seulement</item><item>Toutes celles de Debian</item>
          </comboboxtext></hbox>
        <hbox>
          <checkbox><label>Redémarrer seul si nécessaire, à</label><variable>REDEMARRER</variable></checkbox>
          <spinbutton range-min="0" range-max="23" range-step="1"><variable>HEURE</variable><default>3</default></spinbutton>
          <text><label>h</label></text>
        </hbox>
        <hbox><text><label>Compte rendu par courriel à :</label></text>
          <entry space-expand="true" space-fill="true"><variable>COURRIEL</variable><default>root</default></entry></hbox>
      </vbox>
    </frame>
    <hbox>
      <button cancel></button>
      <button><label>Voir les mises à jour en attente</label><action>exit:voir</action></button>
      <button><label>Enregistrer</label><action>exit:enregistrer</action></button>
    </hbox>
  </vbox>
</window>
XML
}

# Argument : une ligne « paquet|version installée|nouvelle version » par paquet.
dialogue_attente() {
	local lignes=${1:-$'openssl|3.5.4-1|3.5.5-1\nlibc6|2.42-3|2.42-4\nfirefox-esr|153.3.0esr-1|153.4.0esr-1'}
	local n=${2:-3} l
	cat <<XML
<window title="Mises à jour en attente" default-width="560" default-height="380">
  <vbox>
    <text><label>$(xml "$n") paquet(s) à mettre à jour, d'après la dernière recherche.</label></text>
    <table scrollable="true" height-request="240">
      <label>Paquet|Installé|Disponible</label>
$(while IFS= read -r l; do [ -n "$l" ] && printf '      <item>%s</item>\n' "$(xml "$l")"; done <<< "$lignes")
    </table>
    <hbox>
      <button cancel></button>
      <button><label>Mettre à jour maintenant</label><action>exit:mettre_a_jour</action></button>
    </hbox>
  </vbox>
</window>
XML
}

# Écrit un fichier de configuration par agir ; son contenu est toujours montré.
ecrire() {
	echo "Contenu prévu pour $1 :"; printf '%s\n' "$2" | sed 's/^/    /'
	printf '%s\n' "$2" | agir tee -- "$1"
}

suite() {
	# La sortie standard de sermo part vers /dev/null : écrire sur la sortie d'erreur.
	exec >&2
	case "$1" in
	principal)
		case "$EXIT" in
		enregistrer)
			local jours=1 auto=0
			[ "$FREQ_SEMAINE" = true ] && jours=7
			[ "$FREQ_MANUEL" = true ] && jours=0
			[ "$AUTO" = true ] && [ "$jours" != 0 ] && auto=$jours
			[[ "$HEURE" =~ ^[0-9]+$ ]] || HEURE=3
			[[ "$COURRIEL" =~ ^[A-Za-z0-9._%+@-]*$ ]] || { echo "Adresse de courriel refusée : $COURRIEL"; exit 1; }
			[ "$(id -u)" = 0 ] || echo "Droits d'administration nécessaires : lancez en root pour écrire dans /etc/apt."
			if [ ! -x /usr/bin/unattended-upgrade ] && [ "$auto" != 0 ]; then
				echo "unattended-upgrades n'est pas installé : il le sera d'abord."
				agir apt-get install -y unattended-upgrades
			fi
			ecrire "$PERIODIQUE" "APT::Periodic::Update-Package-Lists \"$jours\";
APT::Periodic::Unattended-Upgrade \"$auto\";
APT::Periodic::AutocleanInterval \"7\";"
			local origines='"origin=Debian,codename=${distro_codename}-security,label=Debian-Security";
	"origin=Debian,codename=${distro_codename},label=Debian-Security";'
			[ "$ORIGINE" = "Toutes celles de Debian" ] &&
				origines=$origines$'\n\t"origin=Debian,codename=${distro_codename},label=Debian";'
			local reboot=false; [ "$REDEMARRER" = true ] && reboot=true
			ecrire "$REGLAGES" "// Écrit par 5-mises-a-jour.sh ; remplace la liste des origines de 50unattended-upgrades.
#clear Unattended-Upgrade::Origins-Pattern;
Unattended-Upgrade::Origins-Pattern {
	$origines
};
Unattended-Upgrade::Automatic-Reboot \"$reboot\";
Unattended-Upgrade::Automatic-Reboot-Time \"$(printf '%02d:00' "$((10#$HEURE))")\";
Unattended-Upgrade::Mail \"$COURRIEL\";"
			;;
		voir)
			# Lecture seule : les listes connues d'apt, sans les rafraîchir.
			local lignes
			lignes=$(apt list --upgradable 2>/dev/null | sed -n 's#^\([^/]*\)/[^ ]* \([^ ]*\) [^ ]* \[[^:]*: \([^]]*\)\]$#\1|\3|\2#p')
			export DIALOGUE; DIALOGUE=$(dialogue_attente "$lignes" "$(grep -c . <<< "$lignes")")
			"$SERMO" --program=DIALOGUE --do='"$SERMO_SCRIPT" --suite attente' >/dev/null
			;;
		esac
		;;
	attente)
		[ "$EXIT" = mettre_a_jour ] || exit 0
		[ "$(id -u)" = 0 ] || echo "Droits d'administration nécessaires : lancez en root les commandes ci-dessous."
		agir apt-get update
		agir apt-get upgrade -y
		;;
	esac
}

main() {
	if [ "${1:-}" = --suite ]; then suite "$2"; exit; fi
	[ -n "${1:-}" ] && SERMO=$1
	for c in apt apt-get tee; do
		command -v "$c" >/dev/null || { echo "$c est absent : ce poste n'est pas une Debian." >&2; exit 1; }
	done
	export DIALOGUE; DIALOGUE=$(dialogue_principal)
	"$SERMO" --program=DIALOGUE --do='"$SERMO_SCRIPT" --suite principal' >/dev/null
}

[ "${BASH_SOURCE[0]}" = "$0" ] && main "$@"
