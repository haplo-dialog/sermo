#!/bin/bash
# SPDX-License-Identifier: CC0-1.0
# Métier   : Web local / intranet
# Besoin   : réserver une ressource partagée (salle, vidéoprojecteur, véhicule) en voyant les réservations déjà prises, sans double réservation
# Enrobe   : htmlsermo en mode serve (page sur 127.0.0.1) ; lecture et ajout au registre CSV des réservations
# Ports    : htmlsermo conseillé (SERMO_HTML_MODE=serve, posé par défaut) ; tout autre port en fenêtre
# Terminal : rien de perdu
# Usage    : ./4-reservation-ressource.sh [binaire sermo]   (par exemple : ./4-reservation-ressource.sh htmlsermo)
#            SERMO_A_BLANC=0 pour agir pour de vrai (défaut : la commande est affichée)
#            RESERVATIONS=fichier.csv (défaut : ~/intranet/reservations.csv ; colonnes ressource;date;debut;fin;nom)

export SERMO="${SERMO:-sermo}"
export SERMO_A_BLANC="${SERMO_A_BLANC:-1}"
# Le script lui-même, pour --do : le shell de sermo lit la variable, sans
# citation à refaire (printf %q écrit $'…' en locale C, que dash lit de travers).
export SERMO_SCRIPT="$0"
export SERMO_HTML_MODE="${SERMO_HTML_MODE:-serve}"
export RESERVATIONS="${RESERVATIONS:-$HOME/intranet/reservations.csv}"

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

RESSOURCES=("Salle Rhône (12 places)" "Salle Isère (6 places)" "Vidéoprojecteur portable" "Véhicule de service")

# Arguments : date proposée, puis les réservations à venir « ressource|date|début|fin|nom ».
dialogue_principal() {
	local date=${1:-2026-10-14}
	shift 2>/dev/null
	local prises=("$@")
	[ ${#prises[@]} -gt 0 ] || prises=("Salle Rhône (12 places)|2026-10-14|09:00|11:00|C. Martin" "Véhicule de service|2026-10-15|08:00|17:00|L. Bernard")
	cat <<XML
<window title="Réserver une ressource" default-width="620">
  <vbox>
    <frame label="Déjà réservé">
      <tree scrollable="true" height-request="110">
        <label>Ressource|Date|Début|Fin|Par</label>
        <variable>DEJA</variable>
$(for l in "${prises[@]}"; do printf '        <item>%s</item>\n' "$(xml "$l")"; done)
      </tree>
    </frame>
    <frame label="Nouvelle réservation">
      <vbox>
        <text><label>Ressource :</label></text>
        <list height-request="90"><variable>RESSOURCE</variable>
$(for r in "${RESSOURCES[@]}"; do printf '          <item>%s</item>\n' "$(xml "$r")"; done)
        </list>
        <hbox>
          <calendar><variable>JOUR</variable><default>$(xml "$date")</default></calendar>
          <vbox>
            <hbox><text><label>Début (heure) :</label></text>
              <spinbutton range-min="7" range-max="19" range-step="1"><variable>DEBUT</variable><default>14</default></spinbutton></hbox>
            <hbox><text><label>Durée (heures) :</label></text>
              <spinbutton range-min="1" range-max="10" range-step="1"><variable>DUREE</variable><default>2</default></spinbutton></hbox>
            <hbox><text><label>Votre nom :</label></text>
              <entry><variable>NOM</variable><default>C. Martin</default></entry></hbox>
          </vbox>
        </hbox>
      </vbox>
    </frame>
    <hbox>
      <button cancel></button>
      <button><label>Réserver</label><action>exit:reserver</action></button>
    </hbox>
  </vbox>
</window>
XML
}

# Réservations du jour choisi pour la ressource qui chevauchent [début, fin[ (heures entières).
conflits() {
	[ -r "$RESERVATIONS" ] || return 0
	awk -F';' -v r="$RESSOURCE" -v j="$JOUR" -v d="$DEBUT" -v f="$FIN" '
		$1 == r && $2 == j { split($3, a, ":"); split($4, b, ":");
			if (a[1] + 0 < f + 0 && b[1] + 0 > d + 0) print $3 "-" $4 " (" $5 ")" }' "$RESERVATIONS"
}

suite() {
	# La sortie standard de sermo part vers /dev/null : écrire sur la sortie d'erreur.
	exec >&2
	case "$1" in
	principal)
		[ "$EXIT" = reserver ] || exit 0
		ok=0; for r in "${RESSOURCES[@]}"; do [ "$r" = "$RESSOURCE" ] && ok=1; done
		[ "$ok" = 1 ] || { echo "Choisissez une ressource dans la liste." >&2; exit 1; }
		[[ $JOUR =~ ^[0-9]{4}-[0-9]{2}-[0-9]{2}$ ]] || { echo "Date refusée : $JOUR" >&2; exit 1; }
		[[ $DEBUT =~ ^[0-9]+$ && $DUREE =~ ^[0-9]+$ ]] || { echo "Heure ou durée refusée." >&2; exit 1; }
		[ "$JOUR" \< "$(date +%F)" ] && { echo "Cette date est passée : $JOUR" >&2; exit 1; }
		NOM=${NOM//;/,}; NOM=${NOM//$'\n'/ }
		[ -n "$NOM" ] || { echo "Le nom est obligatoire." >&2; exit 1; }
		FIN=$((DEBUT + DUREE)); [ "$FIN" -le 20 ] || { echo "Fin après 20 h : réduisez la durée." >&2; exit 1; }
		c=$(conflits)
		[ -z "$c" ] || { echo "Déjà réservé ce jour-là : $c" >&2; exit 1; }
		ligne=$(printf '%s;%s;%02d:00;%02d:00;%s' "$RESSOURCE" "$JOUR" "$DEBUT" "$FIN" "$NOM")
		agir sh -c 'mkdir -p -- "$(dirname -- "$1")" && printf "%s\n" "$2" >> "$1"' reservation "$RESERVATIONS" "$ligne"
		if [ "$SERMO_A_BLANC" != 1 ]; then echo "Réservé : $RESSOURCE le $JOUR de ${DEBUT} h à ${FIN} h."; fi
		;;
	esac
}

main() {
	if [ "${1:-}" = --suite ]; then suite "$2"; exit; fi
	[ -n "${1:-}" ] && SERMO=$1
	command -v "$SERMO" >/dev/null || { echo "$SERMO est absent : installez sermo (htmlsermo pour le mode navigateur)." >&2; exit 1; }
	# Lecture seule : les réservations d'aujourd'hui et après, triées par date.
	prises=()
	[ -r "$RESERVATIONS" ] && mapfile -t prises < <(awk -F';' -v t="$(date +%F)" \
		'NF >= 5 && $2 >= t { print $1 "|" $2 "|" $3 "|" $4 "|" $5 }' "$RESERVATIONS" | sort -t'|' -k2,2 -k3,3)
	[ ${#prises[@]} -gt 0 ] || prises=("(aucune réservation à venir)||||")
	export DIALOGUE; DIALOGUE=$(dialogue_principal "$(date -d tomorrow +%F)" "${prises[@]}")
	"$SERMO" --program=DIALOGUE --do='"$SERMO_SCRIPT" --suite principal' >/dev/null
}

[ "${BASH_SOURCE[0]}" = "$0" ] && main "$@"
