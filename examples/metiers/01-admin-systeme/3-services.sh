#!/bin/bash
# SPDX-License-Identifier: CC0-1.0
# Métier   : Administrateur système
# Besoin   : voir l'état d'un service systemd, le démarrer, l'arrêter ou le redémarrer
# Enrobe   : systemctl
# Ports    : gtk3sermo conseillé ; ncursessermo par SSH
# Terminal : rien de perdu
# Usage    : ./3-services.sh [binaire sermo]
#            SERMO_A_BLANC=0 pour agir pour de vrai (défaut : la commande est affichée)
#            En vrai, systemctl demande lui-même l'autorisation (polkit) s'il le faut.

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

# Arguments : une ligne par service, « unité|actif|état|description ».
dialogue_principal() {
	local lignes=("$@") l
	[ ${#lignes[@]} -gt 0 ] || lignes=(
		"cron.service|active|running|Regular background program processing daemon"
		"cups.service|active|running|CUPS Scheduler"
		"ssh.service|inactive|dead|OpenBSD Secure Shell server"
		"systemd-timesyncd.service|active|running|Network Time Synchronization")
	cat <<XML
<window title="Services systemd" default-width="680" default-height="480">
  <vbox>
    <text><label>Choisissez un service, puis l'action.</label></text>
    <tree scrollable="true" space-expand="true" space-fill="true">
      <label>Service|Actif|État|Description</label>
      <variable>SERVICE</variable>
XML
	for l in "${lignes[@]}"; do printf '      <item>%s</item>\n' "$(xml "$l")"; done
	cat <<XML
    </tree>
    <frame label="Action" space-expand="false" space-fill="false">
      <vbox>
        <hbox>
          <radiobutton><label>Voir l'état</label><default>true</default><variable>A_ETAT</variable></radiobutton>
          <radiobutton><label>Démarrer</label><variable>A_DEMARRER</variable></radiobutton>
          <radiobutton><label>Arrêter</label><variable>A_ARRETER</variable></radiobutton>
          <radiobutton><label>Redémarrer</label><variable>A_REDEMARRER</variable></radiobutton>
        </hbox>
        <checkbox><label>Aussi au démarrage de la machine (enable avec Démarrer, disable avec Arrêter)</label><variable>PERMANENT</variable></checkbox>
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

# Arguments : fichier qui contient la sortie de « systemctl status », nom du service.
dialogue_etat() {
	local fichier=${1:-} service=${2:-cron.service}
	cat <<XML
<window title="État de $(xml "$service")" default-width="680" default-height="460">
  <vbox>
    <edit editable="false" space-expand="true" space-fill="true">
XML
	if [ -n "$fichier" ]; then
		printf '      <input file>%s</input>\n' "$(xml "$fichier")"
	else
		printf '      <default>%s</default>\n' "● cron.service - Regular background program processing daemon (démonstration)"
	fi
	cat <<XML
    </edit>
    <hbox>
      <button><label>Fermer</label><action>exit:fermer</action></button>
    </hbox>
  </vbox>
</window>
XML
}

dialogue_confirmation() {
	local action=${1:-arrêter} service=${2:-cups.service}
	cat <<XML
<window title="Confirmer">
  <vbox>
    <text><label>Vraiment $(xml "$action") le service $(xml "$service") ?</label></text>
    <text><label>Les utilisateurs de ce service seront coupés.</label></text>
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
		[ -n "$SERVICE" ] || { echo "Aucun service choisi." >&2; exit 1; }
		# Noms d'unités systemd : lettres, chiffres et « @ . _ : - \ ».
		[[ "$SERVICE" =~ ^[A-Za-z0-9@._:\\-]+$ && "$SERVICE" != -* ]] ||
			{ echo "Nom de service refusé : $SERVICE" >&2; exit 1; }
		if [ "$A_ETAT" = true ]; then
			local f; f=$(mktemp "${XDG_RUNTIME_DIR:-$HOME}/sermo-etat-XXXXXX") || exit 1
			# Lecture seule : systemctl status ne change rien.
			systemctl status --no-pager --full --lines=30 -- "$SERVICE" > "$f" 2>&1
			export DIALOGUE; DIALOGUE=$(dialogue_etat "$f" "$SERVICE")
			"$SERMO" --program=DIALOGUE >/dev/null
			rm -f -- "$f"
		elif [ "$A_DEMARRER" = true ]; then
			if [ "$PERMANENT" = true ]; then
				agir systemctl enable --now -- "$SERVICE"
			else
				agir systemctl start -- "$SERVICE"
			fi
		elif [ "$A_ARRETER" = true ] || [ "$A_REDEMARRER" = true ]; then
			local verbe=redémarrer; [ "$A_ARRETER" = true ] && verbe=arrêter
			export SERVICE A_ARRETER A_REDEMARRER PERMANENT
			export DIALOGUE; DIALOGUE=$(dialogue_confirmation "$verbe" "$SERVICE")
			"$SERMO" --program=DIALOGUE --do='"$SERMO_SCRIPT" --suite confirmation' >/dev/null
		fi
		;;
	confirmation)
		[ "$EXIT" = confirmer ] || exit 0
		if [ "$A_ARRETER" = true ]; then
			if [ "$PERMANENT" = true ]; then
				agir systemctl disable --now -- "$SERVICE"
			else
				agir systemctl stop -- "$SERVICE"
			fi
		else
			agir systemctl restart -- "$SERVICE"
		fi
		;;
	esac
}

main() {
	if [ "${1:-}" = --suite ]; then suite "$2"; exit; fi
	[ -n "${1:-}" ] && SERMO=$1
	command -v systemctl >/dev/null || { echo "systemctl est absent : cette machine n'utilise pas systemd." >&2; exit 1; }
	local lignes=() u charge actif etat desc
	while read -r u charge actif etat desc; do
		[ -n "$u" ] || continue
		lignes+=("$u|$actif|$etat|${desc//|//}")
	done < <(systemctl list-units --type=service --all --no-legend --plain --no-pager 2>/dev/null)
	[ ${#lignes[@]} -gt 0 ] || { echo "systemctl ne liste aucun service." >&2; exit 1; }
	export DIALOGUE; DIALOGUE=$(dialogue_principal "${lignes[@]}")
	"$SERMO" --program=DIALOGUE --do='"$SERMO_SCRIPT" --suite principal' >/dev/null
}

[ "${BASH_SOURCE[0]}" = "$0" ] && main "$@"
