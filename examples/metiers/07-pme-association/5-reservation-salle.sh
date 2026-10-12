#!/bin/bash
# SPDX-License-Identifier: CC0-1.0
# Métier   : PME, association (gestion des locaux)
# Besoin   : réserver une salle (salle, date, créneau, effectif, matériel) en voyant les conflits au planning
# Enrobe   : grep (conflits, lecture seule) ; tee -a (planning CSV)
# Ports    : gtk3sermo conseillé ; lvglsermo sur un écran d'accueil ; htmlsermo sur l'intranet
# Terminal : rien de perdu (le calendrier devient une saisie de date)
# Usage    : ./5-reservation-salle.sh [binaire sermo]
#            SERMO_A_BLANC=0 pour agir pour de vrai (défaut : la commande est affichée)

export SERMO="${SERMO:-sermo}"
export SERMO_A_BLANC="${SERMO_A_BLANC:-1}"
# Le script lui-même, pour --do : le shell de sermo lit la variable, sans
# citation à refaire (printf %q écrit $'…' en locale C, que dash lit de travers).
export SERMO_SCRIPT="$0"

# Échappe une valeur avant de l'insérer dans le XML.
# (« \& » : depuis bash 5.2, un « & » nu dans le remplacement recopie le motif.)
xml() { local s=${1//&/"&amp;"}; s=${s//</"&lt;"}; s=${s//>/"&gt;"}; printf '%s' "${s//\"/"&quot;"}"; }

# Lance une commande, ou l'affiche seulement en mode à blanc.
agir() {
	if [ "$SERMO_A_BLANC" = 1 ]; then
		printf 'à blanc :'; printf ' %q' "$@"; printf '\n'
	else
		"$@"
	fi
}

# Une ligne CSV : champs entre guillemets (guillemets doublés), séparés par « ; ».
ligne_csv() {
	local sep="" c
	for c in "$@"; do c=${c//$'\n'/ }; printf '%s"%s"' "$sep" "${c//\"/\"\"}"; sep=";"; done
	printf '\n'
}

# Salles et capacités (nom|places).
SALLES=("Salle polyvalente|80" "Salle de réunion|16" "Bureau partagé|6" "Atelier|24")

dialogue_principal() {
	local date=${1:-2026-10-12} planning=${2:-$HOME/Documents/reservations-salles.csv}
	cat <<XML
<window title="Réserver une salle" default-width="640">
  <vbox>
    <hbox>
      <vbox>
        <tree scrollable="true" height-request="200">
          <label>Salle|Places</label>
          <variable>SALLE</variable>
XML
	local s
	for s in "${SALLES[@]}"; do printf '          <item>%s</item>\n' "$(xml "$s")"; done
	cat <<XML
        </tree>
      </vbox>
      <vbox>
        <frame label="Date">
          <vbox>
            <calendar><variable>DATE</variable><default>$(xml "$date")</default></calendar>
          </vbox>
        </frame>
        <frame label="Créneau">
          <vbox>
            <radiobutton><label>Matin (8 h 30 - 12 h 30)</label><variable>C_MATIN</variable><default>true</default></radiobutton>
            <radiobutton><label>Après-midi (13 h 30 - 17 h 30)</label><variable>C_APRES_MIDI</variable></radiobutton>
            <radiobutton><label>Soirée (18 h - 22 h)</label><variable>C_SOIREE</variable></radiobutton>
            <radiobutton><label>Journée entière</label><variable>C_JOURNEE</variable></radiobutton>
          </vbox>
        </frame>
      </vbox>
    </hbox>
    <frame label="Demande">
      <vbox>
        <hbox><text><label>Demandeur :</label></text>
          <entry space-expand="true" space-fill="true"><variable>DEMANDEUR</variable><default>Section théâtre</default></entry>
          <text><label>Personnes :</label></text>
          <spinbutton range-min="1" range-max="200" range-step="1"><variable>PERSONNES</variable><default>12</default></spinbutton></hbox>
        <hbox><text><label>Objet :</label></text>
          <entry space-expand="true" space-fill="true"><variable>OBJET</variable><default>Répétition du spectacle de fin d'année</default></entry></hbox>
        <hbox>
          <checkbox><label>Vidéoprojecteur</label><variable>VIDEO</variable></checkbox>
          <checkbox><label>Sonorisation</label><variable>SONO</variable></checkbox>
          <checkbox><label>Tables en U</label><variable>TABLES</variable></checkbox>
        </hbox>
      </vbox>
    </frame>
    <hbox>
      <text><label>Planning :</label></text>
      <entry space-expand="true" space-fill="true"><variable>PLANNING</variable><default>$(xml "$planning")</default></entry>
    </hbox>
    <hbox>
      <button cancel></button>
      <button><label>Réserver</label><action>exit:reserver</action></button>
    </hbox>
  </vbox>
</window>
XML
}

# Conflit au planning : on montre les réservations en place, la demande peut aller en liste d'attente.
dialogue_conflit() {
	local resume=${1:-Salle de réunion, 2026-10-12, matin} deja=${2:-'Bureau du conseil (matin)'}
	cat <<XML
<window title="Salle déjà prise" default-width="460">
  <vbox>
    <text><label>Demande : $(xml "$resume").</label></text>
    <text><label>Déjà réservé sur ce créneau :</label></text>
    <edit editable="false" height-request="70"><variable>DEJA</variable><default>"$(xml "$deja")"</default></edit>
    <text><label>Inscrire la demande en liste d'attente ?</label></text>
    <hbox>
      <button cancel></button>
      <button><label>Mettre en attente</label><action>exit:attente</action></button>
    </hbox>
  </vbox>
</window>
XML
}

creneau() {
	if [ "$C_APRES_MIDI" = true ]; then echo après-midi
	elif [ "$C_SOIREE" = true ]; then echo soirée
	elif [ "$C_JOURNEE" = true ]; then echo journée
	else echo matin; fi
}

# Réservations confirmées du planning qui chevauchent la demande (même salle, même date).
conflits() {
	local c
	c=$(creneau)
	[ -r "$PLANNING" ] || return 0
	grep -F -- "\"$DATE\";\"${SALLE%%|*}\";" "$PLANNING" | grep -F '"confirmée"' |
		awk -F'";"' -v c="$c" '$3 == c || $3 == "journée" || c == "journée" { print $4 " (" $3 ")" }'
}

reserver() {
	local statut=$1 materiel="" contenu dossier
	[ "$VIDEO" = true ] && materiel+="vidéoprojecteur "
	[ "$SONO" = true ] && materiel+="sono "
	[ "$TABLES" = true ] && materiel+="tables en U"
	contenu=""
	[ -s "$PLANNING" ] || contenu=$(ligne_csv Date Salle Créneau Demandeur Objet Personnes Matériel Statut "Saisie le")$'\n'
	contenu+=$(ligne_csv "$DATE" "${SALLE%%|*}" "$(creneau)" "$DEMANDEUR" "$OBJET" "$PERSONNES" "$materiel" "$statut" "$(date '+%F %H:%M')")
	dossier=$(dirname -- "$PLANNING")
	[ -d "$dossier" ] || agir mkdir -p -- "$dossier"
	printf 'Ajout au planning %s :\n' "$PLANNING"
	# À blanc, tee ne lit rien : les lignes sont montrées ici. Pour de vrai, tee les recopie.
	[ "$SERMO_A_BLANC" = 1 ] && printf '%s\n' "$contenu"
	printf '%s\n' "$contenu" | agir tee -a -- "$PLANNING"
}

suite() {
	# La sortie standard de sermo part vers /dev/null (les lignes NOM=…) et
	# l'étape --do en hérite : écrire sur la sortie d'erreur, qui reste visible.
	exec >&2
	case "$1" in
	principal)
		[ "$EXIT" = reserver ] || exit 0
		[ -n "$SALLE" ] || { echo "Choisissez une salle dans la liste." >&2; exit 1; }
		[ -n "$DEMANDEUR" ] || { echo "Le demandeur est obligatoire." >&2; exit 1; }
		# La liste rend la première colonne ; les places viennent du tableau des salles.
		for s in "${SALLES[@]}"; do
			[ "${s%%|*}" = "${SALLE%%|*}" ] && places=${s##*|}
		done
		if [ -n "$places" ] && [ "${PERSONNES%.*}" -gt "$places" ]; then
			echo "Attention : $PERSONNES personnes pour $places places dans « ${SALLE%%|*} »."
		fi
		deja=$(conflits)
		if [ -z "$deja" ]; then reserver confirmée; exit; fi
		export DATE SALLE C_APRES_MIDI C_SOIREE C_JOURNEE DEMANDEUR OBJET PERSONNES VIDEO SONO TABLES PLANNING
		export DIALOGUE; DIALOGUE=$(dialogue_conflit "${SALLE%%|*}, $DATE, $(creneau)" "$deja")
		"$SERMO" --program=DIALOGUE --do='"$SERMO_SCRIPT" --suite conflit' >/dev/null
		;;
	conflit)
		[ "$EXIT" = attente ] || exit 0
		reserver "liste d'attente"
		;;
	esac
}

main() {
	if [ "${1:-}" = --suite ]; then suite "$2"; exit; fi
	[ -n "${1:-}" ] && SERMO=$1
	for c in grep awk tee; do
		command -v "$c" >/dev/null || { echo "$c est absent : installez-le d'abord." >&2; exit 1; }
	done
	export DIALOGUE; DIALOGUE=$(dialogue_principal "$(date +%F)")
	"$SERMO" --program=DIALOGUE --do='"$SERMO_SCRIPT" --suite principal' >/dev/null
}

[ "${BASH_SOURCE[0]}" = "$0" ] && main "$@"
