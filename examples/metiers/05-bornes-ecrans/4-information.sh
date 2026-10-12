#!/bin/bash
# SPDX-License-Identifier: CC0-1.0
# Métier   : Bornes et écrans d'accueil
# Besoin   : écran d'information en onglets (horaires, plan, contacts, actualités), avec impression du plan
# Enrobe   : lp (impression du plan) ; textes lus dans des fichiers simples
# Ports    : lvglsermo conseillé (écran tactile) ; gtk3sermo sur un poste
# Terminal : rien de perdu
# Usage    : ./4-information.sh [binaire sermo]
#            SERMO_A_BLANC=0 pour agir pour de vrai (défaut : la commande est affichée)
#            BORNE_DONNEES/info : horaires.txt (jour|matin|après-midi), contacts.txt
#            (service|téléphone|bureau), plan.txt, actualites.txt ; BORNE_IMPRIMANTE : file lp

export SERMO="${SERMO:-sermo}"
export SERMO_A_BLANC="${SERMO_A_BLANC:-1}"
# Le script lui-même, pour --do : le shell de sermo lit la variable, sans
# citation à refaire (printf %q écrit $'…' en locale C, que dash lit de travers).
export SERMO_SCRIPT="$0"
export BORNE_DONNEES="${BORNE_DONNEES:-$HOME/borne}"
INFO="$BORNE_DONNEES/info"

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

# Une <item> par ligne non vide (colonnes séparées par « | »).
items() {
	local l
	while IFS= read -r l; do [ -n "$l" ] && printf '          <item>%s</item>\n' "$(xml "$l")"; done <<< "$1"
}

# Un <text> par ligne : chaque ligne reste un paragraphe, aligné et espacé de la même façon
# sur tous les ports (un <label> sur plusieurs lignes les garde toutes, mais d'un seul bloc).
textes() {
	local l
	while IFS= read -r l; do [ -n "$l" ] && printf '          <text><label>%s</label></text>\n' "$(xml "$l")"; done <<< "$1"
}

PLAN_DEMO="Rez-de-chaussée : accueil, salle d'attente, guichets 1 à 4.
1er étage : salles de réunion A et B, service social.
2e étage : direction, comptabilité.
Ascenseur à droite de l'accueil ; toilettes accessibles au rez-de-chaussée."

# Arguments : horaires, plan, contacts, actualités (textes déjà lus).
dialogue_principal() {
	local horaires=${1:-$'Lundi|8 h 30 – 12 h|13 h 30 – 17 h\nMardi|8 h 30 – 12 h|13 h 30 – 17 h\nMercredi|8 h 30 – 12 h|fermé\nJeudi|8 h 30 – 12 h|13 h 30 – 19 h\nVendredi|8 h 30 – 12 h|13 h 30 – 16 h\nSamedi|9 h – 12 h|fermé'}
	local plan=${2:-$PLAN_DEMO}
	local contacts=${3:-$'Accueil|01 23 45 67 00|RDC\nService social|01 23 45 67 10|1er, bureau 104\nComptabilité|01 23 45 67 20|2e, bureau 203\nUrgences techniques|01 23 45 67 99|—'}
	local actualites=${4:-$'Fermeture exceptionnelle le 11 novembre.\nNouvelle permanence juridique le jeudi soir, sur rendez-vous.'}
	cat <<XML
<window title="Informations" default-width="640" default-height="420">
  <vbox>
    <notebook tab-labels="Horaires|Plan|Contacts|Actualités" space-expand="true" space-fill="true">
      <vbox>
        <text><label>Horaires d'ouverture au public</label></text>
        <tree scrollable="true" height-request="220">
          <label>Jour|Matin|Après-midi</label>
$(items "$horaires")
        </tree>
      </vbox>
      <vbox>
        <frame label="Où trouver quoi">
          <vbox>
$(textes "$plan")
          </vbox>
        </frame>
        <text><label>Vous pouvez imprimer ce plan avec le bouton ci-dessous.</label></text>
      </vbox>
      <vbox>
        <table scrollable="true" height-request="220">
          <label>Service|Téléphone|Où</label>
$(items "$contacts")
        </table>
      </vbox>
      <vbox>
        <frame label="Actualités du site">
          <vbox>
$(textes "$actualites")
          </vbox>
        </frame>
      </vbox>
    </notebook>
    <hbox space-expand="false" space-fill="false">
      <button><label>Imprimer le plan</label><action>exit:imprimer</action></button>
      <button><label>Fermer</label><action>exit:fermer</action></button>
    </hbox>
  </vbox>
</window>
XML
}

# Lit un fichier d'information s'il existe ; sinon rien (le dialogue montre ses valeurs d'exemple).
lire() { [ -r "$INFO/$1" ] && grep -v '^ *#' "$INFO/$1"; }

suite() {
	# La sortie standard de sermo part vers /dev/null : écrire sur la sortie d'erreur.
	exec >&2
	case "$1" in
	principal)
		[ "$EXIT" = imprimer ] || exit 0
		local plan imprimante=()
		plan=$(lire plan.txt); plan=${plan:-$PLAN_DEMO}
		[ -n "${BORNE_IMPRIMANTE:-}" ] && imprimante=(-d "$BORNE_IMPRIMANTE")
		# Le texte du plan part sur l'entrée standard de lp.
		printf 'PLAN DU BÂTIMENT\n\n%s\n' "$plan" | agir lp "${imprimante[@]}" -t "Plan du bâtiment" -o cpi=12
		;;
	esac
}

main() {
	if [ "${1:-}" = --suite ]; then suite "$2"; exit; fi
	[ -n "${1:-}" ] && SERMO=$1
	command -v lp >/dev/null || { echo "lp est absent : installez cups-client pour imprimer le plan." >&2; exit 1; }
	export DIALOGUE
	DIALOGUE=$(dialogue_principal "$(lire horaires.txt)" "$(lire plan.txt)" "$(lire contacts.txt)" "$(lire actualites.txt)")
	"$SERMO" --program=DIALOGUE --do='"$SERMO_SCRIPT" --suite principal' >/dev/null
}

[ "${BASH_SOURCE[0]}" = "$0" ] && main "$@"
