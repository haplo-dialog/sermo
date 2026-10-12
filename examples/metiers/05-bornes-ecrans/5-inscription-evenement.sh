#!/bin/bash
# SPDX-License-Identifier: CC0-1.0
# Métier   : Bornes et écrans d'accueil
# Besoin   : inscrire le public à un atelier ou un événement, avec consentement explicite (RGPD)
# Enrobe   : tee -a (inscriptions en CSV, séparateur « ; »)
# Ports    : lvglsermo conseillé (écran tactile) ; gtk3sermo sur un poste
# Terminal : rien de perdu
# Usage    : ./5-inscription-evenement.sh [binaire sermo]
#            SERMO_A_BLANC=0 pour agir pour de vrai (défaut : la commande est affichée)
#            BORNE_DONNEES : dossier des inscriptions (défaut ~/borne) ;
#            evenements.txt : « nom de l'événement|places », une ligne par événement

export SERMO="${SERMO:-sermo}"
export SERMO_A_BLANC="${SERMO_A_BLANC:-1}"
# Le script lui-même, pour --do : le shell de sermo lit la variable, sans
# citation à refaire (printf %q écrit $'…' en locale C, que dash lit de travers).
export SERMO_SCRIPT="$0"
export BORNE_DONNEES="${BORNE_DONNEES:-$HOME/borne}"
INSCRIPTIONS="$BORNE_DONNEES/inscriptions.csv"
ENTETE='evenement;nom;prenom;courriel;telephone;places;tarif;acces_pmr;lettre_info;consentement;inscrit_le'
EVENEMENTS_DEMO=$'Atelier numérique — samedi 17 octobre, 10 h|12\nConférence « Économiser l\'énergie » — jeudi 22 octobre, 18 h 30|80\nVisite guidée des ateliers — samedi 31 octobre, 14 h|15'

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

# Ajoute une ligne aux inscriptions (et l'en-tête si le fichier n'existe pas encore).
ajouter() {
	local f=$1 l=$2
	echo "Ligne : $l"
	[ -e "$f" ] || l=$ENTETE$'\n'$l
	agir mkdir -p -- "$(dirname -- "$f")"
	printf '%s\n' "$l" | agir tee -a -- "$f"
}

# Arguments : événements (« nom|places » par ligne), message, puis les valeurs à remettre.
dialogue_principal() {
	local evenements=${1:-$EVENEMENTS_DEMO} message=${2:-} nom=${3:-} prenom=${4:-} courriel=${5:-} tel=${6:-} l
	cat <<XML
<window title="Inscription" default-width="640">
  <vbox>
$([ -n "$message" ] && printf '    <text><label>%s</label></text>' "$(xml "$message")")
    <hbox>
      <text><label>Événement :</label></text>
      <comboboxtext space-expand="true" space-fill="true"><variable>EVENEMENT</variable>
$(while IFS= read -r l; do [ -n "$l" ] && printf '        <item>%s</item>\n' "$(xml "${l%|*}")"; done <<< "$evenements")
      </comboboxtext>
    </hbox>
    <frame label="Vos coordonnées">
      <grid columns="2" row-spacing="6" column-spacing="8">
        <text><label>Nom :</label></text>
        <entry space-expand="true" space-fill="true"><variable>NOM</variable><default>$(xml "$nom")</default></entry>
        <text><label>Prénom :</label></text>
        <entry space-expand="true" space-fill="true"><variable>PRENOM</variable><default>$(xml "$prenom")</default></entry>
        <text><label>Courriel :</label></text>
        <entry space-expand="true" space-fill="true"><variable>COURRIEL</variable><default>$(xml "$courriel")</default></entry>
        <text><label>Téléphone (facultatif) :</label></text>
        <entry space-expand="true" space-fill="true"><variable>TELEPHONE</variable><default>$(xml "$tel")</default></entry>
      </grid>
    </frame>
    <frame label="Votre venue">
      <vbox>
        <hbox>
          <text><label>Nombre de places :</label></text>
          <spinbutton range-min="1" range-max="4" range-step="1"><variable>PLACES</variable><default>1</default></spinbutton>
        </hbox>
        <hbox>
          <radiobutton><label>Plein tarif</label><variable>TARIF_PLEIN</variable><default>true</default></radiobutton>
          <radiobutton><label>Tarif réduit</label><variable>TARIF_REDUIT</variable></radiobutton>
          <radiobutton><label>Gratuit (adhérent)</label><variable>TARIF_GRATUIT</variable></radiobutton>
        </hbox>
        <checkbox><label>J'ai besoin d'un accès pour personne à mobilité réduite</label><variable>PMR</variable></checkbox>
      </vbox>
    </frame>
    <frame label="Vos données">
      <vbox>
        <text><label>Vos coordonnées servent seulement à gérer cette inscription ; elles sont effacées un mois après l'événement.</label></text>
        <checkbox><label>J'accepte que mes coordonnées soient utilisées pour cette inscription (obligatoire)</label><variable>CONSENTEMENT</variable></checkbox>
        <checkbox><label>Je souhaite recevoir la lettre d'information (facultatif)</label><variable>LETTRE</variable></checkbox>
      </vbox>
    </frame>
    <hbox>
      <button cancel></button>
      <button><label>M'inscrire</label><action>exit:inscrire</action></button>
    </hbox>
  </vbox>
</window>
XML
}

dialogue_confirmation() {
	local texte=${1:-"Inscription enregistrée : 2 place(s) pour l'Atelier numérique."}
	cat <<XML
<window title="Inscription enregistrée">
  <vbox>
    <text><label>$(xml "$texte")</label></text>
    <text><label>Merci. Une confirmation vous sera remise à l'accueil.</label></text>
    <hbox>
      <button ok></button>
    </hbox>
  </vbox>
</window>
XML
}

redemander() {
	export DIALOGUE; DIALOGUE=$(dialogue_principal "$EVENEMENTS" "$1" "$NOM" "$PRENOM" "$COURRIEL" "$TELEPHONE")
	"$SERMO" --program=DIALOGUE --do='"$SERMO_SCRIPT" --suite principal' >/dev/null
	exit
}

tarif() {
	if   [ "$TARIF_REDUIT" = true ];  then echo réduit
	elif [ "$TARIF_GRATUIT" = true ]; then echo gratuit
	else                                   echo plein
	fi
}

# Places déjà prises pour un événement (lecture seule du fichier d'inscriptions).
prises() {
	[ -r "$INSCRIPTIONS" ] || { echo 0; return; }
	awk -F'";"' -v e="$1" 'NR > 1 { sub(/^"/, "", $1); if ($1 == e) n += $6 } END { print n + 0 }' "$INSCRIPTIONS"
}

suite() {
	# La sortie standard de sermo part vers /dev/null : écrire sur la sortie d'erreur.
	exec >&2
	case "$1" in
	principal)
		[ "$EXIT" = inscrire ] || exit 0
		NOM=$(nettoyer "$NOM"); PRENOM=$(nettoyer "$PRENOM"); COURRIEL=$(nettoyer "$COURRIEL"); TELEPHONE=$(nettoyer "$TELEPHONE")
		local pmr=non lettre=non
		[ "$PMR" = true ] && pmr=oui; [ "$LETTRE" = true ] && lettre=oui
		[ "$CONSENTEMENT" = true ] || redemander "Sans votre accord, nous ne pouvons pas enregistrer l'inscription."
		[ -n "$EVENEMENT" ] || redemander "Choisissez un événement."
		[ -n "${NOM// /}" ] && [ -n "${PRENOM// /}" ] || redemander "Indiquez votre nom et votre prénom."
		[[ "$COURRIEL" =~ ^[^@\ ]+@[^@\ ]+\.[^@\ ]+$ ]] || redemander "Adresse de courriel illisible."
		[[ "$PLACES" =~ ^[1-4]$ ]] || redemander "Nombre de places : de 1 à 4."
		local capacite="" l
		while IFS= read -r l; do [ "${l%|*}" = "$EVENEMENT" ] && capacite=${l##*|}; done <<< "$EVENEMENTS"
		if [[ "$capacite" =~ ^[0-9]+$ ]]; then
			local reste=$((capacite - $(prises "$EVENEMENT")))
			[ "$PLACES" -le "$reste" ] || redemander "Il ne reste que $reste place(s) pour cet événement."
		fi
		local ligne
		ligne="$(csv "$EVENEMENT");$(csv "$NOM");$(csv "$PRENOM");$(csv "$COURRIEL");$(csv "$TELEPHONE");\"$PLACES\";$(csv "$(tarif)");$(csv "$pmr");$(csv "$lettre");\"oui\";$(csv "$(date '+%F %H:%M')")"
		ajouter "$INSCRIPTIONS" "$ligne"
		export DIALOGUE; DIALOGUE=$(dialogue_confirmation "Inscription enregistrée : $PLACES place(s) pour « $EVENEMENT ».")
		"$SERMO" --program=DIALOGUE >/dev/null
		;;
	esac
}

main() {
	if [ "${1:-}" = --suite ]; then suite "$2"; exit; fi
	[ -n "${1:-}" ] && SERMO=$1
	for c in tee date awk; do
		command -v "$c" >/dev/null || { echo "$c est absent : installez coreutils et gawk ou mawk." >&2; exit 1; }
	done
	export EVENEMENTS=$EVENEMENTS_DEMO
	[ -r "$BORNE_DONNEES/evenements.txt" ] && EVENEMENTS=$(grep -v '^ *#' "$BORNE_DONNEES/evenements.txt")
	export DIALOGUE; DIALOGUE=$(dialogue_principal "$EVENEMENTS")
	"$SERMO" --program=DIALOGUE --do='"$SERMO_SCRIPT" --suite principal' >/dev/null
}

[ "${BASH_SOURCE[0]}" = "$0" ] && main "$@"
