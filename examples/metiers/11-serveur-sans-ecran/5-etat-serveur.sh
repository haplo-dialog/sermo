#!/bin/bash
# SPDX-License-Identifier: CC0-1.0
# Métier   : Serveur sans écran
# Besoin   : un tableau de bord d'un coup d'œil (durée de marche, charge, mémoire, disques, services en échec) et relancer un service tombé
# Enrobe   : uptime, /proc/loadavg, free, df, systemctl --failed (lecture) ; systemctl restart
# Ports    : ncursessermo conseillé (par SSH) ; gtk3sermo sur un poste
# Terminal : rien de perdu
# Usage    : ./5-etat-serveur.sh [binaire sermo]
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

# Arguments : durée de marche, charge 1 min, charge rapportée aux cœurs (0 à 1),
# mémoire utilisée en %, puis le nombre N de lignes disques, les N lignes
# « montage|taille|utilisé|% », puis les services en échec.
dialogue_principal() {
	local marche=${1:-12 jours, 4 heures, 7 minutes} charge=${2:-0.42} niveau=${3:-0.07} memoire=${4:-38}
	local n=${5:-2}
	shift 5 2>/dev/null || shift $#
	local disques=("${@:1:n}") services=("${@:n+1}")
	[ ${#disques[@]} -gt 0 ] || disques=("/|29G|11G|39%" "/var|98G|71G|76%")
	[ $# -gt 0 ] || services=("nginx.service" "certbot.service")
	cat <<XML
<window title="État du serveur" default-width="560">
  <vbox>
    <notebook tab-labels="Système|Disques|Services">
      <vbox>
        <text><label>Durée de marche : $(xml "$marche")</label></text>
        <text><label>Charge sur 1 minute : $(xml "$charge") (jauge : part des cœurs occupée)</label></text>
        <levelbar range-min="0" range-max="1"><variable>CHARGE</variable><default>$(xml "$niveau")</default></levelbar>
        <text><label>Mémoire utilisée : $(xml "$memoire") %</label></text>
        <hscale range-min="0" range-max="100" range-step="1" draw-value="true" sensitive="false"><variable>MEMOIRE</variable><default>$(xml "$memoire")</default></hscale>
      </vbox>
      <vbox>
        <table scrollable="true" height-request="160">
          <label>Montage|Taille|Utilisé|Occupation</label>
          <variable>DISQUE</variable>
$(for l in "${disques[@]}"; do printf '          <item>%s</item>\n' "$(xml "$l")"; done)
        </table>
      </vbox>
      <vbox>
        <text><label>Services en échec (systemctl --failed) :</label></text>
        <list scrollable="true" height-request="140">
          <variable>SERVICE</variable>
$(for s in "${services[@]}"; do printf '          <item>%s</item>\n' "$(xml "$s")"; done)
        </list>
        <hbox><button><label>Relancer le service choisi</label><action>exit:relancer</action></button></hbox>
      </vbox>
    </notebook>
    <hbox>
      <button><label>Actualiser</label><action>exit:actualiser</action></button>
      <button><label>Fermer</label><action>exit:fermer</action></button>
    </hbox>
  </vbox>
</window>
XML
}

# Recueille l'état (lecture seule) et ouvre le tableau de bord.
tableau() {
	local marche charge coeurs niveau memoire
	marche=$(uptime -p 2>/dev/null)
	read -r charge _ < /proc/loadavg
	coeurs=$(nproc 2>/dev/null || echo 1)
	niveau=$(awk -v c="$charge" -v n="$coeurs" 'BEGIN { v = c / n; if (v > 1) v = 1; printf "%.2f", v }')
	memoire=$(free | awk '/^Mem:/ { printf "%d", ($2 - $7) * 100 / $2 }')
	mapfile -t disques < <(df -h -x tmpfs -x devtmpfs -x efivarfs --output=target,size,used,pcent 2>/dev/null |
		awk 'NR > 1 { print $1 "|" $2 "|" $3 "|" $4 }')
	mapfile -t services < <(systemctl --failed --no-legend --plain 2>/dev/null | awk '{ print $1 }')
	[ ${#services[@]} -gt 0 ] || services=("(aucun service en échec)")
	export DIALOGUE; DIALOGUE=$(dialogue_principal "$marche" "$charge" "$niveau" "${memoire:-0}" \
		"${#disques[@]}" "${disques[@]}" "${services[@]}")
	"$SERMO" --program=DIALOGUE --do='"$SERMO_SCRIPT" --suite principal' >/dev/null
}

suite() {
	# La sortie standard de sermo part vers /dev/null : écrire sur la sortie d'erreur.
	exec >&2
	case "$1" in
	principal)
		case "$EXIT" in
		actualiser) tableau ;;
		relancer)
			[[ $SERVICE =~ ^[A-Za-z0-9@._:-]+\.(service|socket|timer|mount)$ ]] ||
				{ echo "Choisissez un service en échec dans la liste." >&2; exit 1; }
			[ "$(id -u)" -eq 0 ] || echo "Droits administrateur nécessaires : lancez la commande ci-dessous en root."
			agir systemctl restart -- "$SERVICE"
			;;
		esac
		;;
	esac
}

main() {
	if [ "${1:-}" = --suite ]; then suite "$2"; exit; fi
	[ -n "${1:-}" ] && SERMO=$1
	for c in uptime free df systemctl nproc awk; do
		command -v "$c" >/dev/null || { echo "$c est absent : installez procps / coreutils / systemd." >&2; exit 1; }
	done
	tableau
}

[ "${BASH_SOURCE[0]}" = "$0" ] && main "$@"
