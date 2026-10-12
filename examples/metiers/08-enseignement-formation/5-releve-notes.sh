#!/bin/bash
# SPDX-License-Identifier: CC0-1.0
# Métier   : Enseignement, formation
# Besoin   : saisir les notes d'un devoir, les ramener sur 20, voir moyenne, plus basse et plus haute, exporter le relevé
# Enrobe   : awk (calculs) ; tee (relevé CSV pour le tableur ou le logiciel de vie scolaire)
# Ports    : gtk3sermo conseillé ; qt6sermo, fltk1sermo ; ncursessermo par SSH
# Terminal : rien de perdu
# Usage    : ./5-releve-notes.sh [binaire sermo]
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

# Un texte sur plusieurs lignes s'écrit entre guillemets dans <default>,
# sinon sermo ne garde que la dernière ligne.
dialogue_principal() {
	cat <<XML
<window title="Relevé de notes" default-width="560">
  <vbox>
    <hbox>
      <text><label>Classe :</label></text>
      <comboboxtext>
        <variable>CLASSE</variable>
        <default>4e A</default>
        <item>6e A</item>
        <item>5e B</item>
        <item>4e A</item>
        <item>3e A</item>
        <item>BTS SIO 1</item>
      </comboboxtext>
      <text><label>Devoir :</label></text>
      <entry space-expand="true" space-fill="true"><variable>DEVOIR</variable><default>Contrôle n° 3, fractions</default></entry>
    </hbox>
    <hbox>
      <text><label>Noté sur :</label></text>
      <spinbutton range-min="5" range-max="100" range-step="1"><variable>BAREME</variable><default>40</default></spinbutton>
      <text><label>Coefficient :</label></text>
      <spinbutton range-min="0.5" range-max="5" range-step="0.5" digits="1"><variable>COEF</variable><default>2.0</default></spinbutton>
    </hbox>
    <frame label="Notes : « Nom Prénom ; note » (ABS pour un absent)">
      <vbox>
        <edit height-request="150"><variable>NOTES</variable><default>"ARNAUD Inès ; 31
BERNARD Tom ; 24,5
CHEVALIER Yanis ; ABS
DIALLO Awa ; 36
FONTAINE Hugo ; 17"</default></edit>
      </vbox>
    </frame>
    <frame label="Arrondi de la note sur 20">
      <hbox>
        <radiobutton><label>Au dixième</label><variable>A_DIXIEME</variable><default>true</default></radiobutton>
        <radiobutton><label>Au demi-point</label><variable>A_DEMI</variable></radiobutton>
        <radiobutton><label>Au point</label><variable>A_POINT</variable></radiobutton>
      </hbox>
    </frame>
    <hbox>
      <button cancel></button>
      <button><label>Calculer</label><action>exit:calculer</action></button>
    </hbox>
  </vbox>
</window>
XML
}

# $1 titre, $2 bilan, puis une ligne « nom|note brute|sur 20 » par argument.
dialogue_resultats() {
	local titre=${1:-4e A · Contrôle n° 3, fractions} bilan=${2:-Moyenne 13,6 / 20 · plus basse 8,5 · plus haute 18 · 4 notés, 1 absent}
	shift 2 2>/dev/null || set --
	[ $# -gt 0 ] || set -- "ARNAUD Inès|31|15,5" "BERNARD Tom|24,5|12,3" "CHEVALIER Yanis|ABS|-" "DIALLO Awa|36|18" "FONTAINE Hugo|17|8,5"
	cat <<XML
<window title="Résultats" default-width="520" default-height="360">
  <vbox>
    <text><label>$(xml "$titre")</label></text>
    <tree scrollable="true" height-request="170">
      <label>Élève|Note brute|Sur 20</label>
      <variable>ELEVE</variable>
XML
	local l
	for l in "$@"; do printf '      <item>%s</item>\n' "$(xml "$l")"; done
	cat <<XML
    </tree>
    <text><label>$(xml "$bilan")</label></text>
    <hbox>
      <text><label>Exporter vers :</label></text>
      <entry space-expand="true" space-fill="true"><variable>EXPORT</variable><default>$(xml "$HOME/Documents/Notes/releve.csv")</default></entry>
    </hbox>
    <hbox>
      <button cancel></button>
      <button><label>Exporter le relevé</label><action>exit:exporter</action></button>
    </hbox>
  </vbox>
</window>
XML
}

pas() { if [ "$A_POINT" = true ]; then echo 1; elif [ "$A_DEMI" = true ]; then echo 0.5; else echo 0.1; fi; }

# Notes → « nom|brute|sur 20 » (virgule décimale), absents à part.
calcul() {
	printf '%s\n' "$NOTES" | LC_ALL=C awk -F';' -v b="$BAREME" -v p="$(pas)" '
		function fr(x) { s = sprintf("%.1f", x); sub(/\.0$/, "", s); sub(/\./, ",", s); return s }
		NF >= 2 { n = $1; gsub(/^ +| +$/, "", n); gsub(/\|/, "/", n); v = $2; gsub(/ /, "", v)
			if (toupper(v) == "ABS") { print n "|ABS|-"; next }
			sub(/,/, ".", v); x = v * 20 / b; x = int(x / p + 0.5) * p
			print n "|" fr(v) "|" fr(x) }'
}

bilan() {
	calcul | LC_ALL=C awk -F'|' '
		function fr(x) { s = sprintf("%.1f", x); sub(/\.0$/, "", s); sub(/\./, ",", s); return s }
		$2 == "ABS" { abs++; next }
		{ v = $3; sub(/,/, ".", v); v += 0; s += v; n++; if (n == 1 || v < mn) mn = v; if (n == 1 || v > mx) mx = v }
		END { if (n == 0) { print "Aucune note"; exit }
			printf "Moyenne %s / 20 · plus basse %s · plus haute %s · %d noté(s), %d absent(s)\n", fr(s / n), fr(mn), fr(mx), n, abs }'
}

suite() {
	# La sortie standard de sermo part vers /dev/null (les lignes NOM=…) et
	# l'étape --do en hérite : écrire sur la sortie d'erreur, qui reste visible.
	exec >&2
	case "$1" in
	principal)
		[ "$EXIT" = calculer ] || exit 0
		mapfile -t lignes < <(calcul)
		[ ${#lignes[@]} -gt 0 ] || { echo "Aucune ligne au format « Nom Prénom ; note »." >&2; exit 1; }
		hors=$(calcul | LC_ALL=C awk -F'|' '$2 != "ABS" { v = $2; sub(/,/, ".", v); if (v + 0 > '"${BAREME%.*}"' || v + 0 < 0) print $1 }')
		[ -z "$hors" ] || { echo "Note hors barème pour : $hors" >&2; exit 1; }
		export CLASSE DEVOIR BAREME COEF NOTES A_DEMI A_POINT
		export DIALOGUE; DIALOGUE=$(dialogue_resultats "$CLASSE · $DEVOIR (sur $BAREME, coef. ${COEF/./,})" "$(bilan)" "${lignes[@]}")
		"$SERMO" --program=DIALOGUE --do='"$SERMO_SCRIPT" --suite resultats' >/dev/null
		;;
	resultats)
		[ "$EXIT" = exporter ] || exit 0
		# Un relevé déjà exporté n'est pas écrasé : on refuse plutôt que perdre des notes.
		[ -e "$EXPORT" ] && { echo "$EXPORT existe déjà : choisissez un autre nom." >&2; exit 1; }
		contenu=$(ligne_csv Classe Devoir Coefficient Élève "Note brute" Sur "Note sur 20")
		while IFS='|' read -r nom brute vingt; do
			contenu+=$'\n'$(ligne_csv "$CLASSE" "$DEVOIR" "${COEF/./,}" "$nom" "$brute" "$BAREME" "$vingt")
		done < <(calcul)
		[ -d "${EXPORT%/*}" ] || agir mkdir -p -- "${EXPORT%/*}"
		printf 'Relevé pour %s :\n' "$EXPORT"
		# À blanc, tee ne lit rien : le relevé est montré ici. Pour de vrai, tee le recopie.
		[ "$SERMO_A_BLANC" = 1 ] && printf '%s\n' "$contenu"
		printf '%s\n' "$contenu" | agir tee -- "$EXPORT"
		;;
	esac
}

main() {
	if [ "${1:-}" = --suite ]; then suite "$2"; exit; fi
	[ -n "${1:-}" ] && SERMO=$1
	for c in awk tee; do
		command -v "$c" >/dev/null || { echo "$c est absent : installez-le d'abord." >&2; exit 1; }
	done
	export DIALOGUE; DIALOGUE=$(dialogue_principal)
	"$SERMO" --program=DIALOGUE --do='"$SERMO_SCRIPT" --suite principal' >/dev/null
}

[ "${BASH_SOURCE[0]}" = "$0" ] && main "$@"
