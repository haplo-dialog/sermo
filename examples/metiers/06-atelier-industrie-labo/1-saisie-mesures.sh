#!/bin/bash
# SPDX-License-Identifier: CC0-1.0
# Métier   : Atelier, industrie, laboratoire
# Besoin   : saisir les mesures d'une pièce (opérateur, lot, poste), les comparer aux tolérances, les garder en CSV
# Enrobe   : awk (contrôle des tolérances), tee -a (fichier de mesures CSV)
# Ports    : gtk3sermo conseillé ; lvglsermo sur un écran d'atelier ; ncursessermo sur un terminal de poste
# Terminal : rien de perdu
# Usage    : ./1-saisie-mesures.sh [binaire sermo]
#            SERMO_A_BLANC=0 pour agir pour de vrai (défaut : la commande est affichée)
#            ATELIER_DONNEES : dossier des mesures (défaut ~/atelier)
#            Cotes nominales et tolérances : variables NOMINAL_* et TOL_* ci-dessous.

export SERMO="${SERMO:-sermo}"
export SERMO_A_BLANC="${SERMO_A_BLANC:-1}"
# Le script lui-même, pour --do : le shell de sermo lit la variable, sans
# citation à refaire (printf %q écrit $'…' en locale C, que dash lit de travers).
export SERMO_SCRIPT="$0"
export ATELIER_DONNEES="${ATELIER_DONNEES:-$HOME/atelier}"
MESURES="$ATELIER_DONNEES/mesures.csv"
ENTETE='horodatage;operateur;poste;lot;piece;diametre_mm;longueur_mm;masse_g;verdict'
# Pièce de référence (exemple) : axe Ø 25 ± 0,02 mm, longueur 120 ± 0,1 mm, masse 462 ± 5 g.
NOMINAL_DIAMETRE=25.000; TOL_DIAMETRE=0.020
NOMINAL_LONGUEUR=120.00; TOL_LONGUEUR=0.10
NOMINAL_MASSE=462.0;     TOL_MASSE=5.0

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

# Ajoute une ligne au fichier de mesures (et l'en-tête si le fichier n'existe pas encore).
ajouter() {
	local f=$1 l=$2
	echo "Ligne : $l"
	[ -e "$f" ] || l=$ENTETE$'\n'$l
	agir mkdir -p -- "$(dirname -- "$f")"
	printf '%s\n' "$l" | agir tee -a -- "$f"
}

# Arguments : opérateur, lot, numéro de pièce, poste, compte rendu de la mesure précédente.
dialogue_principal() {
	local operateur=${1:-mdupuis} lot=${2-L2026-1042} piece=${3:-1} poste=${4:-Tour CN 2} precedent=${5:-}
	cat <<XML
<window title="Saisie des mesures" default-width="560">
  <vbox>
$([ -n "$precedent" ] && printf '    <text><label>%s</label></text>' "$(xml "$precedent")")
    <frame label="Traçabilité">
      <grid columns="4" row-spacing="6" column-spacing="8">
        <text><label>Opérateur :</label></text>
        <entry><variable>OPERATEUR</variable><default>$(xml "$operateur")</default></entry>
        <text><label>Lot :</label></text>
        <entry><variable>LOT</variable><default>$(xml "$lot")</default></entry>
        <text><label>Poste :</label></text>
        <comboboxtext><variable>POSTE</variable><default>$(xml "$poste")</default>
          <item>Tour CN 1</item><item>Tour CN 2</item><item>Fraiseuse 3 axes</item><item>Rectifieuse</item><item>Contrôle final</item>
        </comboboxtext>
        <text><label>Pièce n° :</label></text>
        <spinbutton range-min="1" range-max="9999" range-step="1"><variable>PIECE</variable><default>$(xml "$piece")</default></spinbutton>
      </grid>
    </frame>
    <frame label="Mesures">
      <grid columns="3" row-spacing="6" column-spacing="8">
        <text><label>Diamètre (mm) :</label></text>
        <spinbutton range-min="0" range-max="100" range-step="0.001" digits="3"><variable>DIAMETRE</variable><default>$NOMINAL_DIAMETRE</default></spinbutton>
        <text><label>$NOMINAL_DIAMETRE ± $TOL_DIAMETRE</label></text>
        <text><label>Longueur (mm) :</label></text>
        <spinbutton range-min="0" range-max="500" range-step="0.01" digits="2"><variable>LONGUEUR</variable><default>$NOMINAL_LONGUEUR</default></spinbutton>
        <text><label>$NOMINAL_LONGUEUR ± $TOL_LONGUEUR</label></text>
        <text><label>Masse (g) :</label></text>
        <spinbutton range-min="0" range-max="5000" range-step="0.1" digits="1"><variable>MASSE</variable><default>$NOMINAL_MASSE</default></spinbutton>
        <text><label>$NOMINAL_MASSE ± $TOL_MASSE</label></text>
      </grid>
    </frame>
    <hbox>
      <button cancel></button>
      <button><label>Enregistrer et terminer</label><action>exit:terminer</action></button>
      <button><label>Enregistrer, pièce suivante</label><action>exit:suivante</action></button>
    </hbox>
  </vbox>
</window>
XML
}

# Verdict : « conforme » ou la liste des cotes hors tolérance.
verdict() {
	awk -v d="$DIAMETRE" -v l="$LONGUEUR" -v m="$MASSE" \
		-v nd="$NOMINAL_DIAMETRE" -v td="$TOL_DIAMETRE" -v nl="$NOMINAL_LONGUEUR" -v tl="$TOL_LONGUEUR" \
		-v nm="$NOMINAL_MASSE" -v tm="$TOL_MASSE" '
	function hors(v, n, t) { return (v < n - t - 1e-9 || v > n + t + 1e-9) }
	BEGIN {
		r = ""
		if (hors(d, nd, td)) r = r " diamètre"
		if (hors(l, nl, tl)) r = r " longueur"
		if (hors(m, nm, tm)) r = r " masse"
		print (r == "" ? "conforme" : "NON CONFORME :" r)
	}'
}

suite() {
	# La sortie standard de sermo part vers /dev/null : écrire sur la sortie d'erreur.
	exec >&2
	case "$1" in
	principal)
		case "$EXIT" in terminer|suivante) ;; *) exit 0 ;; esac
		local n
		for n in "$DIAMETRE" "$LONGUEUR" "$MASSE" "$PIECE"; do
			[[ "$n" =~ ^[0-9]+(\.[0-9]+)?$ ]] || { echo "Valeur illisible : $n"; exit 1; }
		done
		[ -n "${OPERATEUR// /}" ] && [ -n "${LOT// /}" ] || { echo "Opérateur et lot sont obligatoires."; exit 1; }
		local v ligne
		v=$(verdict)
		ligne="$(date '+%F %T');$(csv "$OPERATEUR");$(csv "$POSTE");$(csv "$LOT");$PIECE;$DIAMETRE;$LONGUEUR;$MASSE;$(csv "$v")"
		ajouter "$MESURES" "$ligne"
		echo "Pièce $PIECE : $v"
		if [ "$EXIT" = suivante ]; then
			export DIALOGUE
			DIALOGUE=$(dialogue_principal "$OPERATEUR" "$LOT" "$((10#${PIECE%%.*} + 1))" "$POSTE" "Pièce $PIECE enregistrée : $v")
			"$SERMO" --program=DIALOGUE --do='"$SERMO_SCRIPT" --suite principal' >/dev/null
		fi
		;;
	esac
}

main() {
	if [ "${1:-}" = --suite ]; then suite "$2"; exit; fi
	[ -n "${1:-}" ] && SERMO=$1
	for c in awk tee date; do
		command -v "$c" >/dev/null || { echo "$c est absent : installez coreutils et gawk ou mawk." >&2; exit 1; }
	done
	export DIALOGUE; DIALOGUE=$(dialogue_principal "${USER:-operateur}" "" 1 "Tour CN 2")
	"$SERMO" --program=DIALOGUE --do='"$SERMO_SCRIPT" --suite principal' >/dev/null
}

[ "${BASH_SOURCE[0]}" = "$0" ] && main "$@"
