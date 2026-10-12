#!/bin/bash
# SPDX-License-Identifier: CC0-1.0
# Métier   : Développeur / DevOps
# Besoin   : changer de branche ou valider (commit) les modifications d'un dépôt git
# Enrobe   : git switch, git commit
# Ports    : gtk3sermo conseillé ; ncursessermo par SSH
# Terminal : rien de perdu
# Usage    : ./2-git.sh [binaire sermo]
#            SERMO_A_BLANC=0 pour agir pour de vrai (défaut : la commande est affichée)
#            Le dépôt est le dossier courant (ou la variable DEPOT).

export SERMO="${SERMO:-sermo}"
export SERMO_A_BLANC="${SERMO_A_BLANC:-1}"
# Le script lui-même, pour --do : le shell de sermo lit la variable, sans
# citation à refaire (printf %q écrit $'…' en locale C, que dash lit de travers).
export SERMO_SCRIPT="$0"

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

# Arguments : dépôt, branche actuelle, puis les branches, « -- », puis les
# fichiers modifiés « état|chemin ».
dialogue_principal() {
	# Sans argument : valeurs de démonstration.
	[ $# -gt 0 ] || set -- "$HOME/src/mon-projet" main main correctif-dates fonction-export -- \
		" M|src/export.c" " M|README.md" "??|tests/test_export.c"
	local depot=$1 actuelle=$2
	shift 2
	local branches=() fichiers=() a
	while [ $# -gt 0 ] && [ "$1" != -- ]; do branches+=("$1"); shift; done
	[ "${1:-}" = -- ] && shift
	fichiers=("$@")
	cat <<XML
<window title="Git" default-width="560" default-height="460">
  <vbox>
    <text><label>Dépôt $(xml "$depot") · branche actuelle : $(xml "$actuelle")</label></text>
    <notebook tab-labels="Valider (commit)|Changer de branche">
      <vbox>
        <table scrollable="true" space-expand="true" space-fill="true">
          <label>État|Fichier</label>
XML
	for a in "${fichiers[@]}"; do printf '          <item>%s</item>\n' "$(xml "$a")"; done
	cat <<XML
        </table>
        <checkbox><label>Inclure toutes les modifications des fichiers suivis (-a)</label><default>true</default><variable>TOUT</variable></checkbox>
        <checkbox><label>Signer le commit : ligne Signed-off-by (-s)</label><variable>SIGNER</variable></checkbox>
        <frame label="Message du commit (première ligne : le résumé)">
          <edit space-expand="true" space-fill="true"><variable>MESSAGE</variable></edit>
        </frame>
        <hbox><button><label>Valider</label><action>exit:valider</action></button></hbox>
      </vbox>
      <vbox>
        <list scrollable="true" space-expand="true" space-fill="true">
          <variable>BRANCHE</variable>
XML
	for a in "${branches[@]}"; do printf '          <item>%s</item>\n' "$(xml "$a")"; done
	cat <<XML
        </list>
        <hbox><text><label>Ou créer une nouvelle branche :</label></text>
          <entry space-expand="true" space-fill="true"><variable>NOUVELLE</variable></entry></hbox>
        <hbox><button><label>Changer de branche</label><action>exit:changer</action></button></hbox>
      </vbox>
    </notebook>
    <hbox><button cancel></button></hbox>
  </vbox>
</window>
XML
}

suite() {
	exec >&2
	case "$1" in
	principal)
		case "$EXIT" in
		valider)
			local m=${MESSAGE//[[:space:]]/}
			[ -n "$m" ] || { echo "Message de commit vide : rien n'est fait." >&2; exit 1; }
			local opts=()
			[ "$TOUT" = true ] && opts+=(-a)
			[ "$SIGNER" = true ] && opts+=(-s)
			agir git -C "$DEPOT" commit "${opts[@]}" -m "$MESSAGE"
			;;
		changer)
			if [ -n "$NOUVELLE" ]; then
				git check-ref-format --branch "$NOUVELLE" >/dev/null 2>&1 ||
					{ echo "Nom de branche refusé par git : $NOUVELLE" >&2; exit 1; }
				agir git -C "$DEPOT" switch -c "$NOUVELLE"
			else
				[ -n "$BRANCHE" ] || { echo "Aucune branche choisie." >&2; exit 1; }
				git -C "$DEPOT" show-ref --verify --quiet "refs/heads/$BRANCHE" ||
					{ echo "Branche inconnue : $BRANCHE" >&2; exit 1; }
				agir git -C "$DEPOT" switch "$BRANCHE"
			fi
			;;
		esac
		;;
	esac
}

main() {
	if [ "${1:-}" = --suite ]; then suite "$2"; exit; fi
	[ -n "${1:-}" ] && SERMO=$1
	command -v git >/dev/null || { echo "git est absent : installez le paquet git." >&2; exit 1; }
	DEPOT=$(git -C "${DEPOT:-$PWD}" rev-parse --show-toplevel 2>/dev/null) ||
		{ echo "Pas de dépôt git ici : lancez le script depuis un dépôt (ou DEPOT=…)." >&2; exit 1; }
	export DEPOT
	local actuelle branches=() fichiers=() l
	actuelle=$(git -C "$DEPOT" branch --show-current)
	mapfile -t branches < <(git -C "$DEPOT" for-each-ref --format='%(refname:short)' refs/heads/)
	while IFS= read -r l; do
		l=${l//|//}; fichiers+=("${l:0:2}|${l:3}")
	done < <(git -C "$DEPOT" status --porcelain 2>/dev/null | head -n 200)
	export DIALOGUE; DIALOGUE=$(dialogue_principal "$DEPOT" "${actuelle:-(détachée)}" "${branches[@]}" -- "${fichiers[@]}")
	"$SERMO" --program=DIALOGUE --do='"$SERMO_SCRIPT" --suite principal' >/dev/null
}

[ "${BASH_SOURCE[0]}" = "$0" ] && main "$@"
