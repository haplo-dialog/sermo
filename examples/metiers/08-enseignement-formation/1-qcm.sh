#!/bin/bash
# SPDX-License-Identifier: CC0-1.0
# Métier   : Enseignement, formation
# Besoin   : faire passer un QCM (une question par onglet), afficher le score et la correction, garder le résultat
# Enrobe   : tee -a (résultats CSV) ; questions lues dans un fichier texte (QCM_FICHIER)
# Ports    : gtk3sermo conseillé ; qt6sermo ; lvglsermo sur tablette
# Terminal : rien de perdu (les onglets deviennent des pages, la barre un pourcentage)
# Usage    : ./1-qcm.sh [binaire sermo]
#            QCM_FICHIER=questions.txt : une question par ligne, « question|réponse 1|réponse 2|réponse 3|n° de la bonne »
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

# Questions par défaut : « question|réponse 1|réponse 2|réponse 3|n° de la bonne ».
QUESTIONS=(
	"Un courriel vous demande votre mot de passe « pour vérification ». Que faites-vous ?|Je réponds vite pour ne pas perdre mon compte|Je ne réponds pas et je le signale|Je clique sur le lien pour vérifier|2"
	"Que contient une sauvegarde « 3-2-1 » ?|3 copies, 2 supports, 1 copie hors site|3 disques, 2 ordinateurs, 1 serveur|3 mots de passe, 2 comptes, 1 clé|1"
	"Quel mot de passe est le plus solide ?|Soleil2026!|tortue-agrafe-pluie-lanterne|P@ssw0rd|2"
	"Que signifie le cadenas à côté de l'adresse d'un site ?|Le site est forcément honnête|La connexion est chiffrée|Le site est hébergé en France|2"
	"Avant de brancher une clé USB trouvée sur un parking, il faut…|la brancher pour retrouver le propriétaire|la donner au service informatique sans la brancher|la formater|2"
)

# Lit QCM_FICHIER (s'il est donné) à la place des questions par défaut.
charger_questions() {
	[ -n "${QCM_FICHIER:-}" ] || return 0
	[ -r "$QCM_FICHIER" ] || { echo "Fichier de questions illisible : $QCM_FICHIER" >&2; exit 1; }
	mapfile -t QUESTIONS < <(grep -v '^[[:space:]]*\(#\|$\)' "$QCM_FICHIER")
}

dialogue_principal() {
	local titre=${1:-Les bons réflexes numériques} i=0 q r1 r2 r3 bonne onglets=""
	for q in "${QUESTIONS[@]}"; do i=$((i + 1)); onglets+="${onglets:+|}Q$i"; done
	cat <<XML
<window title="QCM : $(xml "$titre")" default-width="560">
  <vbox>
    <hbox>
      <text><label>Nom :</label></text>
      <entry space-expand="true" space-fill="true"><variable>ELEVE</variable><default>Léa Moreau</default></entry>
      <text><label>Groupe :</label></text>
      <comboboxtext>
        <variable>GROUPE</variable>
        <default>Groupe A</default>
        <item>Groupe A</item>
        <item>Groupe B</item>
        <item>Formation continue</item>
      </comboboxtext>
    </hbox>
    <notebook tab-labels="$(xml "$onglets")">
XML
	i=0
	for q in "${QUESTIONS[@]}"; do
		i=$((i + 1))
		IFS='|' read -r q r1 r2 r3 bonne <<<"$q"
		cat <<XML
      <vbox>
        <text wrap="true" xalign="0"><label>$(xml "$q")</label></text>
        <radiobutton><label>Je ne sais pas</label><variable>Q${i}_R0</variable><default>true</default></radiobutton>
        <radiobutton><label>$(xml "$r1")</label><variable>Q${i}_R1</variable></radiobutton>
        <radiobutton><label>$(xml "$r2")</label><variable>Q${i}_R2</variable></radiobutton>
        <radiobutton><label>$(xml "$r3")</label><variable>Q${i}_R3</variable></radiobutton>
      </vbox>
XML
	done
	cat <<XML
    </notebook>
    <hbox>
      <button cancel></button>
      <button><label>Rendre la copie</label><action>exit:rendre</action></button>
    </hbox>
  </vbox>
</window>
XML
}

# $1 nom, $2 score « 3/5 », $3 pourcentage entier, $4 correction (plusieurs lignes).
dialogue_resultat() {
	local eleve=${1:-Léa Moreau} score=${2:-3/5} pourcent=${3:-60} correction=${4:-Q1 : Je ne réponds pas et je le signale
Q4 : La connexion est chiffrée}
	cat <<XML
<window title="Résultat du QCM" default-width="520">
  <vbox>
    <text><label>$(xml "$eleve") : $(xml "$score") bonnes réponses</label></text>
    <progressbar>
      <label>$(xml "$pourcent") %</label>
      <input>echo $(xml "$pourcent")</input>
    </progressbar>
    <frame label="Correction des réponses manquées">
      <vbox>
        <edit editable="false" height-request="110" wrap-mode="word"><variable>CORRECTION</variable><default>"$(xml "$correction")"</default></edit>
      </vbox>
    </frame>
    <checkbox><label>Garder ce résultat dans le fichier de la classe</label><variable>GARDER</variable><default>true</default></checkbox>
    <hbox>
      <button cancel></button>
      <button><label>Terminer</label><action>exit:terminer</action></button>
    </hbox>
  </vbox>
</window>
XML
}

suite() {
	# La sortie standard de sermo part vers /dev/null (les lignes NOM=…) et
	# l'étape --do en hérite : écrire sur la sortie d'erreur, qui reste visible.
	exec >&2
	charger_questions
	case "$1" in
	principal)
		[ "$EXIT" = rendre ] || exit 0
		total=${#QUESTIONS[@]} justes=0 correction="" i=0 reponses=""
		for q in "${QUESTIONS[@]}"; do
			i=$((i + 1))
			IFS='|' read -r texte r1 r2 r3 bonne <<<"$q"
			choix=0
			for n in 1 2 3; do v="Q${i}_R$n"; [ "${!v:-}" = true ] && choix=$n; done
			reponses+="$choix"
			if [ "$choix" = "$bonne" ]; then
				justes=$((justes + 1))
			else
				r="r$bonne"; correction+="Q$i : ${!r}"$'\n'
			fi
		done
		[ -n "$correction" ] || correction="Aucune erreur : bravo."
		pourcent=$((justes * 100 / total))
		export ELEVE GROUPE JUSTES=$justes TOTAL=$total REPONSES=$reponses
		export DIALOGUE; DIALOGUE=$(dialogue_resultat "$ELEVE" "$justes/$total" "$pourcent" "${correction%$'\n'}")
		"$SERMO" --program=DIALOGUE --do='"$SERMO_SCRIPT" --suite resultat' >/dev/null
		;;
	resultat)
		echo "$ELEVE ($GROUPE) : $JUSTES/$TOTAL."
		[ "$EXIT" = terminer ] && [ "$GARDER" = true ] || exit 0
		f="$HOME/Documents/QCM/resultats.csv"
		contenu=""
		[ -s "$f" ] || contenu=$(ligne_csv Date Élève Groupe Score Sur "Réponses données")$'\n'
		contenu+=$(ligne_csv "$(date '+%F %H:%M')" "$ELEVE" "$GROUPE" "$JUSTES" "$TOTAL" "$REPONSES")
		[ -d "${f%/*}" ] || agir mkdir -p -- "${f%/*}"
		printf 'Ajout à %s :\n' "$f"
		# À blanc, tee ne lit rien : les lignes sont montrées ici. Pour de vrai, tee les recopie.
		[ "$SERMO_A_BLANC" = 1 ] && printf '%s\n' "$contenu"
		printf '%s\n' "$contenu" | agir tee -a -- "$f"
		;;
	esac
}

main() {
	if [ "${1:-}" = --suite ]; then suite "$2"; exit; fi
	[ -n "${1:-}" ] && SERMO=$1
	command -v tee >/dev/null || { echo "tee est absent : installez le paquet coreutils." >&2; exit 1; }
	charger_questions
	[ ${#QUESTIONS[@]} -gt 0 ] || { echo "Aucune question à poser." >&2; exit 1; }
	export DIALOGUE; DIALOGUE=$(dialogue_principal)
	"$SERMO" --program=DIALOGUE --do='"$SERMO_SCRIPT" --suite principal' >/dev/null
}

[ "${BASH_SOURCE[0]}" = "$0" ] && main "$@"
