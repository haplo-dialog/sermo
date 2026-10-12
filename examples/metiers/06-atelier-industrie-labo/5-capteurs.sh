#!/bin/bash
# SPDX-License-Identifier: CC0-1.0
# Métier   : Atelier, industrie, laboratoire
# Besoin   : relever les températures des capteurs de la machine, les comparer à un seuil, garder le relevé
# Enrobe   : lecture de /sys/class/thermal (et /sys/class/hwmon à défaut) ; tee -a (relevés CSV)
# Ports    : gtk3sermo conseillé ; lvglsermo sur un écran d'atelier
# Terminal : rien de perdu (la jauge devient une barre de caractères)
# Usage    : ./5-capteurs.sh [binaire sermo]
#            SERMO_A_BLANC=0 pour agir pour de vrai (défaut : la commande est affichée)
#            ATELIER_DONNEES : dossier des relevés (défaut ~/atelier)

export SERMO="${SERMO:-sermo}"
export SERMO_A_BLANC="${SERMO_A_BLANC:-1}"
# Le script lui-même, pour --do : le shell de sermo lit la variable, sans
# citation à refaire (printf %q écrit $'…' en locale C, que dash lit de travers).
export SERMO_SCRIPT="$0"
export ATELIER_DONNEES="${ATELIER_DONNEES:-$HOME/atelier}"
RELEVES="$ATELIER_DONNEES/temperatures.csv"
ENTETE='horodatage;capteur;temperature_c;seuil_c;alerte'

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

# Argument : une ligne « nom du capteur|température en °C » par capteur ; seuil d'alerte.
dialogue_principal() {
	local capteurs=${1:-$'x86_pkg_temp|52.0\nacpitz|41.5\nnvme Composite|38.9'} seuil=${2:-80}
	local i=0 nom t
	cat <<XML
<window title="Températures" default-width="560">
  <vbox>
    <frame label="Capteurs (°C, jauge de 0 à 110)">
      <grid columns="3" row-spacing="6" column-spacing="10">
$(while IFS='|' read -r nom t; do
	[ -n "$nom" ] || continue
	printf '        <text><label>%s</label></text>\n' "$(xml "$nom")"
	printf '        <levelbar range-min="0" range-max="110" digits="1" space-expand="true" space-fill="true"><variable>TEMP_%d</variable><default>%s</default></levelbar>\n' "$i" "$(xml "$t")"
	printf '        <text><label>%s °C</label></text>\n' "$(xml "$t")"
	i=$((i + 1))
done <<< "$capteurs")
      </grid>
    </frame>
    <frame label="Alerte">
      <vbox>
        <text><label>Seuil d'alerte (°C) :</label></text>
        <hscale range-min="40" range-max="105" range-step="1" draw-value="true"><variable>SEUIL</variable><default>$(xml "$seuil")</default></hscale>
        <checkbox><label>N'enregistrer que les capteurs au-dessus du seuil</label><variable>SEULEMENT_ALERTES</variable></checkbox>
      </vbox>
    </frame>
    <hbox>
      <button><label>Fermer</label><action>exit:fermer</action></button>
      <button><label>Actualiser</label><action>exit:actualiser</action></button>
      <button><label>Enregistrer le relevé</label><action>exit:enregistrer</action></button>
    </hbox>
  </vbox>
</window>
XML
}

# Millidegrés → degrés avec une décimale (arrondi).
degres() { local m=$1 s=""; [ "$m" -lt 0 ] && { s=-; m=$((-m)); }; m=$(( (m + 50) / 100 )); printf '%s%d.%d' "$s" $((m / 10)) $((m % 10)); }

# Lecture seule : « nom|°C » par capteur. /sys/class/thermal d'abord, hwmon à défaut
# (sur bien des PC à processeur AMD, thermal n'expose aucune zone).
capteurs() {
	local z t nom h e
	for z in /sys/class/thermal/thermal_zone*; do
		[ -r "$z/temp" ] || continue
		t=$(cat "$z/temp" 2>/dev/null) && [[ "$t" =~ ^-?[0-9]+$ ]] || continue
		printf '%s|%s\n' "$(cat "$z/type" 2>/dev/null || basename "$z")" "$(degres "$t")"
	done
	[ -n "$(compgen -G '/sys/class/thermal/thermal_zone*/temp')" ] && return
	for e in /sys/class/hwmon/hwmon*/temp*_input; do
		[ -r "$e" ] || continue
		t=$(cat "$e" 2>/dev/null) && [[ "$t" =~ ^-?[0-9]+$ ]] || continue
		h=$(dirname "$e"); nom=$(cat "$h/name" 2>/dev/null)
		[ -r "${e%_input}_label" ] && nom="$nom $(cat "${e%_input}_label")"
		printf '%s|%s\n' "$nom" "$(degres "$t")"
	done
}

afficher() {
	export CAPTEURS; CAPTEURS=$(capteurs)
	[ -n "$CAPTEURS" ] || { echo "Aucun capteur de température lisible dans /sys/class/thermal ni /sys/class/hwmon." >&2; exit 1; }
	export DIALOGUE; DIALOGUE=$(dialogue_principal "$CAPTEURS" "${1:-80}")
	"$SERMO" --program=DIALOGUE --do='"$SERMO_SCRIPT" --suite principal' >/dev/null
}

suite() {
	# La sortie standard de sermo part vers /dev/null : écrire sur la sortie d'erreur.
	exec >&2
	case "$1" in
	principal)
		[[ "$SEUIL" =~ ^[0-9]+$ ]] || SEUIL=80
		case "$EXIT" in
		actualiser)
			afficher "$SEUIL"
			;;
		enregistrer)
			# Les valeurs enregistrées sont celles affichées (rendues par les jauges).
			local i=0 nom t v alerte lignes="" maintenant
			maintenant=$(date '+%F %T')
			while IFS='|' read -r nom t; do
				[ -n "$nom" ] || continue
				v=TEMP_$i; v=${!v:-$t}; i=$((i + 1))
				alerte=non
				awk -v a="$v" -v b="$SEUIL" 'BEGIN { exit !(a >= b) }' && alerte=OUI
				[ "$alerte" = OUI ] && echo "ALERTE : $nom à $v °C (seuil $SEUIL °C)"
				[ "$SEULEMENT_ALERTES" = true ] && [ "$alerte" = non ] && continue
				lignes+="$maintenant;\"${nom//\"/\"\"}\";$v;$SEUIL;$alerte"$'\n'
			done <<< "$CAPTEURS"
			[ -n "$lignes" ] || { echo "Aucun capteur au-dessus du seuil : rien à enregistrer."; exit 0; }
			[ -e "$RELEVES" ] || lignes=$ENTETE$'\n'$lignes
			printf '%s' "$lignes" | sed 's/^/Ligne : /'
			agir mkdir -p -- "$ATELIER_DONNEES"
			printf '%s' "$lignes" | agir tee -a -- "$RELEVES"
			;;
		esac
		;;
	esac
}

main() {
	if [ "${1:-}" = --suite ]; then suite "$2"; exit; fi
	[ -n "${1:-}" ] && SERMO=$1
	for c in awk tee cat; do
		command -v "$c" >/dev/null || { echo "$c est absent : installez coreutils et gawk ou mawk." >&2; exit 1; }
	done
	[ -d /sys/class/thermal ] || [ -d /sys/class/hwmon ] ||
		{ echo "Ni /sys/class/thermal ni /sys/class/hwmon : /sys n'est pas monté ?" >&2; exit 1; }
	afficher 80
}

[ "${BASH_SOURCE[0]}" = "$0" ] && main "$@"
