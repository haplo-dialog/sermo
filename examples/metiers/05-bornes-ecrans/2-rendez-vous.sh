#!/bin/bash
# SPDX-License-Identifier: CC0-1.0
# Métier   : Bornes et écrans d'accueil
# Besoin   : prendre un rendez-vous à la borne : service, jour, créneau libre, coordonnées
# Enrobe   : tee -a (agenda CSV, séparateur « ; »), date
# Ports    : lvglsermo conseillé (écran tactile) ; gtk3sermo sur un poste
# Terminal : rien de perdu
# Usage    : ./2-rendez-vous.sh [binaire sermo]
#            SERMO_A_BLANC=0 pour agir pour de vrai (défaut : la commande est affichée)
#            BORNE_DONNEES : dossier de l'agenda (défaut ~/borne)

export SERMO="${SERMO:-sermo}"
export SERMO_A_BLANC="${SERMO_A_BLANC:-1}"
# Le script lui-même, pour --do : le shell de sermo lit la variable, sans
# citation à refaire (printf %q écrit $'…' en locale C, que dash lit de travers).
export SERMO_SCRIPT="$0"
export BORNE_DONNEES="${BORNE_DONNEES:-$HOME/borne}"
AGENDA="$BORNE_DONNEES/rendez-vous.csv"
ENTETE='date;creneau;service;nom;telephone;rappel;pris_le'

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

# Un champ CSV : entre guillemets, guillemets doublés, sans saut de ligne ; une
# valeur qui commence par = + - @ est neutralisée (elle serait une formule pour un tableur).
csv() {
	local s=${1//$'\n'/ }; s=${s//$'\r'/}
	case $s in [=+@-]*) s="'$s" ;; esac
	printf '"%s"' "${s//\"/\"\"}"
}

# Retire les blancs en tête et en fin.
nettoyer() { local s=$1; s=${s#"${s%%[![:space:]]*}"}; printf '%s' "${s%"${s##*[![:space:]]}"}"; }

# Ajoute une ligne à l'agenda (et l'en-tête si le fichier n'existe pas encore).
ajouter() {
	local f=$1 l=$2
	echo "Ligne : $l"
	[ -e "$f" ] || l=$ENTETE$'\n'$l
	agir mkdir -p -- "$(dirname -- "$f")"
	printf '%s\n' "$l" | agir tee -a -- "$f"
}

# Arguments : jour proposé (AAAA-MM-JJ), message d'erreur éventuel.
dialogue_principal() {
	local jour=${1:-2026-10-13} message=${2:-}
	cat <<XML
<window title="Prendre rendez-vous" default-width="680">
  <vbox>
$([ -n "$message" ] && printf '    <text><label>%s</label></text>' "$(xml "$message")")
    <hbox>
      <text><label>Service :</label></text>
      <comboboxtext space-expand="true" space-fill="true"><variable>SERVICE</variable><default>Accueil général</default>
        <item>Accueil général</item><item>Dépôt de dossier</item><item>Retrait de documents</item><item>Conseiller emploi</item><item>Permanence juridique</item>
      </comboboxtext>
    </hbox>
    <hbox>
      <frame label="Jour">
        <calendar><variable>JOUR</variable><default>$(xml "$jour")</default></calendar>
      </frame>
      <frame label="Créneau" space-expand="true" space-fill="true">
        <list scrollable="true" height-request="200"><variable>CRENEAU</variable>
          <item>09:00</item><item>09:30</item><item>10:00</item><item>10:30</item><item>11:00</item><item>11:30</item>
          <item>14:00</item><item>14:30</item><item>15:00</item><item>15:30</item><item>16:00</item><item>16:30</item>
        </list>
      </frame>
    </hbox>
    <frame label="Vos coordonnées">
      <grid columns="2" row-spacing="6" column-spacing="8">
        <text><label>Nom :</label></text>
        <entry space-expand="true" space-fill="true"><variable>NOM</variable></entry>
        <text><label>Téléphone :</label></text>
        <entry space-expand="true" space-fill="true"><variable>TELEPHONE</variable></entry>
      </grid>
    </frame>
    <checkbox><label>Je souhaite être rappelé la veille</label><variable>RAPPEL</variable></checkbox>
    <hbox>
      <button cancel></button>
      <button><label>Réserver</label><action>exit:reserver</action></button>
    </hbox>
  </vbox>
</window>
XML
}

dialogue_recapitulatif() {
	local texte=${1:-Rendez-vous le mardi 13 octobre 2026 à 09:30, service Accueil général.}
	cat <<XML
<window title="Rendez-vous enregistré">
  <vbox>
    <text><label>$(xml "$texte")</label></text>
    <text><label>Présentez-vous à l'accueil cinq minutes avant l'heure.</label></text>
    <hbox>
      <button ok></button>
    </hbox>
  </vbox>
</window>
XML
}

redemander() {
	export DIALOGUE; DIALOGUE=$(dialogue_principal "${JOUR:-$(date +%F)}" "$1")
	"$SERMO" --program=DIALOGUE --do='"$SERMO_SCRIPT" --suite principal' >/dev/null
	exit
}

suite() {
	# La sortie standard de sermo part vers /dev/null : écrire sur la sortie d'erreur.
	exec >&2
	case "$1" in
	principal)
		[ "$EXIT" = reserver ] || exit 0
		NOM=$(nettoyer "$NOM"); TELEPHONE=$(nettoyer "$TELEPHONE")
		local rappel=non; [ "$RAPPEL" = true ] && rappel=oui
		[[ "$JOUR" =~ ^[0-9]{4}-[0-9]{2}-[0-9]{2}$ ]] || redemander "Choisissez un jour dans le calendrier."
		[[ "$JOUR" > "$(date +%F)" ]] || redemander "Choisissez un jour à partir de demain."
		[ "$(date -d "$JOUR" +%u)" -le 5 ] || redemander "Le service est fermé le samedi et le dimanche."
		[ -n "$CRENEAU" ] || redemander "Choisissez un créneau horaire."
		[ -n "${NOM// /}" ] || redemander "Indiquez votre nom."
		[[ "$TELEPHONE" =~ ^[+0-9][0-9\ .]{8,16}$ ]] || redemander "Numéro de téléphone illisible (chiffres, espaces ou points)."
		# Créneau déjà pris pour ce service ? (lecture seule)
		if [ -r "$AGENDA" ] && grep -qF "\"$JOUR\";\"$CRENEAU\";$(csv "$SERVICE");" "$AGENDA"; then
			redemander "Ce créneau est déjà pris : choisissez-en un autre."
		fi
		local ligne
		ligne="$(csv "$JOUR");$(csv "$CRENEAU");$(csv "$SERVICE");$(csv "$NOM");$(csv "$TELEPHONE");$(csv "$rappel");$(csv "$(date '+%F %H:%M')")"
		ajouter "$AGENDA" "$ligne"
		export DIALOGUE; DIALOGUE=$(dialogue_recapitulatif "Rendez-vous le $(date -d "$JOUR" '+%A %-d %B %Y') à $CRENEAU, service $SERVICE.")
		"$SERMO" --program=DIALOGUE >/dev/null
		;;
	esac
}

main() {
	if [ "${1:-}" = --suite ]; then suite "$2"; exit; fi
	[ -n "${1:-}" ] && SERMO=$1
	for c in tee date grep; do
		command -v "$c" >/dev/null || { echo "$c est absent : installez coreutils et grep." >&2; exit 1; }
	done
	export DIALOGUE; DIALOGUE=$(dialogue_principal "$(date -d tomorrow +%F)")
	"$SERMO" --program=DIALOGUE --do='"$SERMO_SCRIPT" --suite principal' >/dev/null
}

[ "${BASH_SOURCE[0]}" = "$0" ] && main "$@"
