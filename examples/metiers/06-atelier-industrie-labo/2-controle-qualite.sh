#!/bin/bash
# SPDX-License-Identifier: CC0-1.0
# Métier   : Atelier, industrie, laboratoire
# Besoin   : contrôle qualité d'un ordre de fabrication : chaque point conforme ou non, remarque, rapport daté
# Enrobe   : tee (rapport texte par OF, et ligne CSV de synthèse)
# Ports    : gtk3sermo conseillé ; lvglsermo sur un écran d'atelier
# Terminal : rien de perdu
# Usage    : ./2-controle-qualite.sh [binaire sermo]
#            SERMO_A_BLANC=0 pour agir pour de vrai (défaut : la commande est affichée)
#            ATELIER_DONNEES : dossier des rapports (défaut ~/atelier)
#            ATELIER_DONNEES/points-controle.txt : cinq points, un par ligne (sinon la liste ci-dessous)

export SERMO="${SERMO:-sermo}"
export SERMO_A_BLANC="${SERMO_A_BLANC:-1}"
# Le script lui-même, pour --do : le shell de sermo lit la variable, sans
# citation à refaire (printf %q écrit $'…' en locale C, que dash lit de travers).
export SERMO_SCRIPT="$0"
export ATELIER_DONNEES="${ATELIER_DONNEES:-$HOME/atelier}"
SYNTHESE="$ATELIER_DONNEES/controles.csv"
ENTETE='horodatage;of;controleur;verdict;non_conformites;remarque'
POINTS_DEMO=$'Aspect visuel (rayures, chocs, bavures)\nCotes fonctionnelles (plan indice C)\nMarquage et traçabilité du lot\nPropreté (copeaux, huile)\nEmballage et étiquette'

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

# Arguments : points de contrôle (un par ligne, cinq au plus), message, OF, contrôleur.
dialogue_principal() {
	local points=${1:-$POINTS_DEMO} message=${2:-} of=${3-OF-26-0815} controleur=${4:-mdupuis}
	local i=0 p
	cat <<XML
<window title="Contrôle qualité" default-width="620">
  <vbox>
$([ -n "$message" ] && printf '    <text><label>%s</label></text>' "$(xml "$message")")
    <hbox>
      <text><label>Ordre de fabrication :</label></text>
      <entry><variable>OF</variable><default>$(xml "$of")</default></entry>
      <text><label>Contrôleur :</label></text>
      <entry><variable>CONTROLEUR</variable><default>$(xml "$controleur")</default></entry>
    </hbox>
    <frame label="Points de contrôle">
      <grid columns="2" row-spacing="6" column-spacing="12">
$(while IFS= read -r p && [ $i -lt 5 ]; do
	[ -n "$p" ] || continue
	i=$((i + 1))
	printf '        <text><label>%s</label></text>\n' "$(xml "$p")"
	printf '        <comboboxtext><variable>POINT_%d</variable><default>À contrôler</default><item>À contrôler</item><item>Conforme</item><item>Non conforme</item><item>Sans objet</item></comboboxtext>\n' "$i"
done <<< "$points")
      </grid>
    </frame>
    <frame label="Remarque (obligatoire en cas de non-conformité)">
      <edit wrap-mode="word" height-request="100"><variable>REMARQUE</variable></edit>
    </frame>
    <hbox>
      <button cancel></button>
      <button><label>Valider le contrôle</label><action>exit:valider</action></button>
    </hbox>
  </vbox>
</window>
XML
}

redemander() {
	export DIALOGUE; DIALOGUE=$(dialogue_principal "$POINTS" "$1" "$OF" "$CONTROLEUR")
	"$SERMO" --program=DIALOGUE --do='"$SERMO_SCRIPT" --suite principal' >/dev/null
	exit
}

suite() {
	# La sortie standard de sermo part vers /dev/null : écrire sur la sortie d'erreur.
	exec >&2
	case "$1" in
	principal)
		[ "$EXIT" = valider ] || exit 0
		[[ "$OF" =~ ^[A-Za-z0-9._-]+$ ]] || redemander "Numéro d'OF : lettres, chiffres, point, tiret."
		[ -n "${CONTROLEUR// /}" ] || redemander "Indiquez le contrôleur."
		local i=0 p v rapport nc="" restant=0
		rapport="CONTRÔLE QUALITÉ — OF $OF
Date : $(date '+%F %H:%M')    Contrôleur : $CONTROLEUR
"
		while IFS= read -r p && [ $i -lt 5 ]; do
			[ -n "$p" ] || continue
			i=$((i + 1))
			v=POINT_$i; v=${!v}
			[ "$v" = "À contrôler" ] && restant=$((restant + 1))
			[ "$v" = "Non conforme" ] && nc="$nc${nc:+, }$p"
			rapport+=$(printf '\n  %-45s %s' "$p" "$v")
		done <<< "$POINTS"
		[ "$restant" = 0 ] || redemander "$restant point(s) encore « À contrôler »."
		[ -z "$nc" ] || [ -n "${REMARQUE//[[:space:]]/}" ] || redemander "Non-conformité : décrivez-la dans la remarque."
		local verdict=ACCEPTÉ; [ -n "$nc" ] && verdict=REFUSÉ
		rapport+="

Verdict : $verdict
Remarque : ${REMARQUE:-aucune}"
		echo "$rapport"
		agir mkdir -p -- "$ATELIER_DONNEES/rapports"
		printf '%s\n' "$rapport" | agir tee -- "$ATELIER_DONNEES/rapports/controle-$OF-$(date +%Y%m%d-%H%M).txt"
		local ligne
		ligne="$(date '+%F %T');$(csv "$OF");$(csv "$CONTROLEUR");$verdict;$(csv "$nc");$(csv "$REMARQUE")"
		[ -e "$SYNTHESE" ] || ligne=$ENTETE$'\n'$ligne
		printf '%s\n' "$ligne" | agir tee -a -- "$SYNTHESE"
		;;
	esac
}

main() {
	if [ "${1:-}" = --suite ]; then suite "$2"; exit; fi
	[ -n "${1:-}" ] && SERMO=$1
	for c in tee date; do
		command -v "$c" >/dev/null || { echo "$c est absent : installez coreutils." >&2; exit 1; }
	done
	export POINTS=$POINTS_DEMO
	[ -r "$ATELIER_DONNEES/points-controle.txt" ] && POINTS=$(grep -v '^ *#' "$ATELIER_DONNEES/points-controle.txt")
	export DIALOGUE; DIALOGUE=$(dialogue_principal "$POINTS" "" "" "${USER:-}")
	"$SERMO" --program=DIALOGUE --do='"$SERMO_SCRIPT" --suite principal' >/dev/null
}

[ "${BASH_SOURCE[0]}" = "$0" ] && main "$@"
