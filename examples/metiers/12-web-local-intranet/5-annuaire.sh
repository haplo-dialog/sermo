#!/bin/bash
# SPDX-License-Identifier: CC0-1.0
# Métier   : Web local / intranet
# Besoin   : chercher un collègue dans l'annuaire interne (nom, service, poste, courriel) et lui écrire
# Enrobe   : grep sur l'annuaire CSV (lecture) ; xdg-email pour préparer le courriel
# Ports    : htmlsermo conseillé (SERMO_HTML_MODE=serve, posé par défaut) ; tout autre port en fenêtre
# Terminal : rien de perdu
# Usage    : ./5-annuaire.sh [binaire sermo]   (par exemple : ./5-annuaire.sh htmlsermo)
#            SERMO_A_BLANC=0 pour agir pour de vrai (défaut : la commande est affichée)
#            ANNUAIRE=fichier.csv (défaut : ~/intranet/annuaire.csv ; colonnes nom;service;poste;courriel)

export SERMO="${SERMO:-sermo}"
export SERMO_A_BLANC="${SERMO_A_BLANC:-1}"
# Le script lui-même, pour --do : le shell de sermo lit la variable, sans
# citation à refaire (printf %q écrit $'…' en locale C, que dash lit de travers).
export SERMO_SCRIPT="$0"
export SERMO_HTML_MODE="${SERMO_HTML_MODE:-serve}"
export ANNUAIRE="${ANNUAIRE:-$HOME/intranet/annuaire.csv}"

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

# Arguments : le terme cherché, puis les fiches trouvées « nom — service — poste ».
dialogue_principal() {
	local terme=${1-mar}
	shift 2>/dev/null
	local fiches=("$@")
	[ ${#fiches[@]} -gt 0 ] || fiches=("Camille Martin — Comptabilité — 214" "Marc Dubois — Informatique — 301" "Maryse Lefèvre — Direction — 100")
	cat <<XML
<window title="Annuaire interne" default-width="520" default-height="380">
  <vbox>
    <hbox><text><label>Nom ou service :</label></text>
      <searchentry space-expand="true" space-fill="true"><variable>TERME</variable><default>$(xml "$terme")</default></searchentry>
      <button><label>Chercher</label><action>exit:chercher</action></button></hbox>
    <text><label>$(xml "${#fiches[@]}") fiche(s) :</label></text>
    <list scrollable="true" height-request="180"><variable>FICHE</variable>
$(for f in "${fiches[@]}"; do printf '      <item>%s</item>\n' "$(xml "$f")"; done)
    </list>
    <hbox>
      <button><label>Fermer</label><action>exit:fermer</action></button>
      <button><label>Voir la fiche</label><action>exit:voir</action></button>
    </hbox>
  </vbox>
</window>
XML
}

# Arguments : nom, service, poste, courriel.
dialogue_fiche() {
	local nom=${1:-Camille Martin} service=${2:-Comptabilité} poste=${3:-214} courriel=${4:-c.martin@exemple.fr}
	cat <<XML
<window title="$(xml "$nom")" default-width="420">
  <vbox>
    <frame label="Fiche">
      <vbox>
        <text><label>Service : $(xml "$service")</label></text>
        <text><label>Poste téléphonique : $(xml "$poste")</label></text>
        <text><label>Courriel : $(xml "$courriel")</label></text>
      </vbox>
    </frame>
    <hbox><text><label>Objet du courriel :</label></text>
      <entry space-expand="true" space-fill="true"><variable>OBJET</variable><default>Question</default></entry></hbox>
    <hbox>
      <button><label>Fermer</label><action>exit:fermer</action></button>
      <button><label>Écrire un courriel</label><action>exit:ecrire</action></button>
    </hbox>
  </vbox>
</window>
XML
}

# Fiches dont le nom ou le service contient le terme (sans tenir compte de la casse).
chercher() {
	awk -F';' -v t="$1" 'BEGIN { t = tolower(t) }
		NR > 1 && (t == "" || index(tolower($1), t) || index(tolower($2), t)) { print $1 " — " $2 " — " $3 }' "$ANNUAIRE" | head -50
}

ouvrir() {
	local terme=$1 fiches
	mapfile -t fiches < <(chercher "$terme")
	[ ${#fiches[@]} -gt 0 ] || fiches=("(aucune fiche pour « $terme »)")
	export DIALOGUE; DIALOGUE=$(dialogue_principal "$terme" "${fiches[@]}")
	"$SERMO" --program=DIALOGUE --do='"$SERMO_SCRIPT" --suite principal' >/dev/null
}

suite() {
	# La sortie standard de sermo part vers /dev/null : écrire sur la sortie d'erreur.
	exec >&2
	case "$1" in
	principal)
		case "$EXIT" in
		chercher) ouvrir "$TERME" ;;
		voir)
			nom=${FICHE%% — *}
			IFS=';' read -r n s p c < <(awk -F';' -v n="$nom" 'NR > 1 && $1 == n { print; exit }' "$ANNUAIRE")
			[ -n "$n" ] || { echo "Choisissez une fiche dans la liste." >&2; exit 1; }
			export COURRIEL=$c
			export DIALOGUE; DIALOGUE=$(dialogue_fiche "$n" "$s" "$p" "$c")
			"$SERMO" --program=DIALOGUE --do='"$SERMO_SCRIPT" --suite fiche' >/dev/null
			;;
		esac
		;;
	fiche)
		[ "$EXIT" = ecrire ] || exit 0
		[[ $COURRIEL =~ ^[A-Za-z0-9][A-Za-z0-9._%+-]*@[A-Za-z0-9.-]+$ ]] || { echo "Courriel refusé : $COURRIEL" >&2; exit 1; }
		agir xdg-email --subject "$OBJET" "$COURRIEL"
		;;
	esac
}

main() {
	if [ "${1:-}" = --suite ]; then suite "$2"; exit; fi
	[ -n "${1:-}" ] && SERMO=$1
	command -v "$SERMO" >/dev/null || { echo "$SERMO est absent : installez sermo (htmlsermo pour le mode navigateur)." >&2; exit 1; }
	command -v xdg-email >/dev/null || { echo "xdg-email est absent : installez le paquet xdg-utils." >&2; exit 1; }
	[ -r "$ANNUAIRE" ] || { echo "Annuaire illisible : $ANNUAIRE (colonnes nom;service;poste;courriel)." >&2; exit 1; }
	ouvrir ""
}

[ "${BASH_SOURCE[0]}" = "$0" ] && main "$@"
