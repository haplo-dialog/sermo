#!/bin/bash
# SPDX-License-Identifier: CC0-1.0
# Métier   : Bornes et écrans d'accueil
# Besoin   : tenir le registre des visiteurs d'un site (arrivée, départ) sur une borne
# Enrobe   : tee -a (registre CSV, séparateur « ; »)
# Ports    : lvglsermo conseillé (écran tactile) ; gtk3sermo sur un poste
# Terminal : rien de perdu
# Usage    : ./1-accueil-visiteurs.sh [binaire sermo]
#            SERMO_A_BLANC=0 pour agir pour de vrai (défaut : la commande est affichée)
#            BORNE_DONNEES : dossier du registre (défaut ~/borne) ; hotes.txt, un nom par ligne
#            BORNE_BOUCLE=1 : la borne revient au formulaire après chaque visiteur

export SERMO="${SERMO:-sermo}"
export SERMO_A_BLANC="${SERMO_A_BLANC:-1}"
# Le script lui-même, pour --do : le shell de sermo lit la variable, sans
# citation à refaire (printf %q écrit $'…' en locale C, que dash lit de travers).
export SERMO_SCRIPT="$0"
export BORNE_DONNEES="${BORNE_DONNEES:-$HOME/borne}"
REGISTRE="$BORNE_DONNEES/visiteurs.csv"
ENTETE='date;heure;sens;nom;prenom;societe;personne_visitee;motif'

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

# Ajoute une ligne au registre (et l'en-tête si le fichier n'existe pas encore).
ajouter() {
	local f=$1 l=$2
	echo "Ligne : $l"
	[ -e "$f" ] || l=$ENTETE$'\n'$l
	agir mkdir -p -- "$(dirname -- "$f")"
	printf '%s\n' "$l" | agir tee -a -- "$f"
}

# Argument : les personnes ou services visités, un par ligne.
dialogue_principal() {
	local hotes=${1:-$'Accueil\nDirection — Mme Martin\nComptabilité\nAtelier — M. Leroy\nService informatique'}
	local message=${2:-} l
	cat <<XML
<window title="Accueil des visiteurs" default-width="640">
  <vbox>
    <text><label>Bienvenue. Merci de vous enregistrer à l'arrivée et au départ.</label></text>
$([ -n "$message" ] && printf '    <text><label>%s</label></text>' "$(xml "$message")")
    <frame label="Vous">
      <grid columns="2" row-spacing="6" column-spacing="8">
        <text><label>Nom :</label></text>
        <entry space-expand="true" space-fill="true"><variable>NOM</variable></entry>
        <text><label>Prénom :</label></text>
        <entry space-expand="true" space-fill="true"><variable>PRENOM</variable></entry>
        <text><label>Société :</label></text>
        <entry space-expand="true" space-fill="true"><variable>SOCIETE</variable></entry>
        <text><label>Vous venez voir :</label></text>
        <comboboxtext space-expand="true" space-fill="true"><variable>HOTE</variable>
$(while IFS= read -r l; do [ -n "$l" ] && printf '          <item>%s</item>\n' "$(xml "$l")"; done <<< "$hotes")
        </comboboxtext>
      </grid>
    </frame>
    <frame label="Motif de la visite">
      <hbox>
        <radiobutton><label>Rendez-vous</label><variable>MOTIF_RDV</variable><default>true</default></radiobutton>
        <radiobutton><label>Livraison</label><variable>MOTIF_LIVRAISON</variable></radiobutton>
        <radiobutton><label>Intervention</label><variable>MOTIF_INTERVENTION</variable></radiobutton>
        <radiobutton><label>Autre</label><variable>MOTIF_AUTRE</variable></radiobutton>
      </hbox>
    </frame>
    <checkbox><label>J'ai lu les consignes de sécurité du site (affichées à l'accueil)</label><variable>CONSIGNES</variable></checkbox>
    <hbox>
      <button cancel></button>
      <button><label>Je pars</label><action>exit:depart</action></button>
      <button><label>J'arrive</label><action>exit:arrivee</action></button>
    </hbox>
  </vbox>
</window>
XML
}

dialogue_merci() {
	local texte=${1:-Merci, votre arrivée est enregistrée. Votre contact est prévenu.}
	cat <<XML
<window title="Merci">
  <vbox>
    <text><label>$(xml "$texte")</label></text>
    <timer visible="false" interval="8"><action>exit:fin</action></timer>
    <hbox>
      <button ok></button>
    </hbox>
  </vbox>
</window>
XML
}

motif() {
	if   [ "$MOTIF_LIVRAISON" = true ];    then echo Livraison
	elif [ "$MOTIF_INTERVENTION" = true ]; then echo Intervention
	elif [ "$MOTIF_AUTRE" = true ];        then echo Autre
	else                                        echo Rendez-vous
	fi
}

redemander() {
	export DIALOGUE; DIALOGUE=$(dialogue_principal "$HOTES" "$1")
	"$SERMO" --program=DIALOGUE --do='"$SERMO_SCRIPT" --suite principal' >/dev/null
	exit
}

suite() {
	# La sortie standard de sermo part vers /dev/null : écrire sur la sortie d'erreur.
	exec >&2
	case "$1" in
	principal)
		local sens
		case "$EXIT" in
		arrivee) sens=arrivée ;;
		depart)  sens=départ ;;
		*)       exit 0 ;;
		esac
		NOM=$(nettoyer "$NOM"); PRENOM=$(nettoyer "$PRENOM"); SOCIETE=$(nettoyer "$SOCIETE")
		[ -n "${NOM// /}" ] || redemander "Indiquez au moins votre nom."
		[ "$sens" = départ ] || [ "$CONSIGNES" = true ] || redemander "Merci de lire et de cocher les consignes de sécurité."
		local ligne
		ligne="$(date +%F);$(date +%H:%M);$(csv "$sens");$(csv "$NOM");$(csv "$PRENOM");$(csv "$SOCIETE");$(csv "$HOTE");$(csv "$(motif)")"
		ajouter "$REGISTRE" "$ligne"
		local texte="Merci, votre arrivée est enregistrée. Votre contact est prévenu."
		[ "$sens" = départ ] && texte="Merci de votre visite, votre départ est enregistré. Bonne route."
		export DIALOGUE; DIALOGUE=$(dialogue_merci "$texte")
		"$SERMO" --program=DIALOGUE >/dev/null
		;;
	esac
}

main() {
	if [ "${1:-}" = --suite ]; then suite "$2"; exit; fi
	[ -n "${1:-}" ] && SERMO=$1
	for c in tee date; do
		command -v "$c" >/dev/null || { echo "$c est absent : installez coreutils." >&2; exit 1; }
	done
	export HOTES=""
	[ -r "$BORNE_DONNEES/hotes.txt" ] && HOTES=$(grep -v '^ *#' "$BORNE_DONNEES/hotes.txt")
	while :; do
		export DIALOGUE; DIALOGUE=$(dialogue_principal "$HOTES")
		"$SERMO" --program=DIALOGUE --do='"$SERMO_SCRIPT" --suite principal' >/dev/null
		[ "${BORNE_BOUCLE:-0}" = 1 ] || break
	done
}

[ "${BASH_SOURCE[0]}" = "$0" ] && main "$@"
