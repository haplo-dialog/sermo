#!/bin/bash
# SPDX-License-Identifier: CC0-1.0
# Métier   : Enseignement, formation (salle de TP)
# Besoin   : choisir un TP dans la liste, copier son dossier dans l'espace de l'élève et l'ouvrir
# Enrobe   : cp -r --no-clobber ; xdg-open
# Ports    : gtk3sermo conseillé ; qt6sermo, efl1sermo ; lvglsermo sur poste élève tactile
# Terminal : xdg-open ouvre un gestionnaire de fichiers graphique : sans écran, seule la copie a un sens
# Usage    : ./2-lanceur-tp.sh [binaire sermo]
#            TP_RACINE=dossier des sujets (défaut : ~/TP) ; un sous-dossier par TP
#            SERMO_A_BLANC=0 pour agir pour de vrai (défaut : la commande est affichée)

export SERMO="${SERMO:-sermo}"
export SERMO_A_BLANC="${SERMO_A_BLANC:-1}"
# Le script lui-même, pour --do : le shell de sermo lit la variable, sans
# citation à refaire (printf %q écrit $'…' en locale C, que dash lit de travers).
export SERMO_SCRIPT="$0"
export TP_RACINE="${TP_RACINE:-$HOME/TP}"

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

# $1 espace de travail, puis un nom de TP (sous-dossier de TP_RACINE) par argument.
dialogue_principal() {
	local espace=${1:-$HOME/Travaux}
	shift 2>/dev/null || set --
	[ $# -gt 0 ] || set -- "TP01-tableur-formules" "TP02-traitement-de-texte-styles" "TP03-reseau-adresses-ip" "TP04-python-boucles"
	cat <<XML
<window title="Lanceur de TP" default-width="520">
  <vbox>
    <frame label="Travaux pratiques disponibles">
      <vbox>
        <list scrollable="true" height-request="150">
          <variable>TP</variable>
XML
	local t
	for t in "$@"; do printf '          <item>%s</item>\n' "$(xml "$t")"; done
	cat <<XML
        </list>
      </vbox>
    </frame>
    <frame label="Que faire ?">
      <vbox>
        <radiobutton><label>Copier le TP dans mon espace, puis ouvrir ma copie</label><variable>COPIER</variable><default>true</default></radiobutton>
        <radiobutton><label>Ouvrir le sujet seulement (lecture)</label><variable>LIRE</variable></radiobutton>
        <checkbox><label>Ouvrir aussi l'énoncé (enonce.pdf)</label><variable>ENONCE</variable><default>true</default></checkbox>
      </vbox>
    </frame>
    <hbox>
      <text><label>Mon espace :</label></text>
      <entry space-expand="true" space-fill="true"><variable>ESPACE</variable><default>$(xml "$espace")</default></entry>
    </hbox>
    <hbox>
      <button cancel></button>
      <button><label>Commencer le TP</label><action>exit:lancer</action></button>
    </hbox>
  </vbox>
</window>
XML
}

suite() {
	# La sortie standard de sermo part vers /dev/null (les lignes NOM=…) et
	# l'étape --do en hérite : écrire sur la sortie d'erreur, qui reste visible.
	exec >&2
	case "$1" in
	principal)
		[ "$EXIT" = lancer ] || exit 0
		[ -n "$TP" ] || { echo "Choisissez un TP dans la liste." >&2; exit 1; }
		# Le nom vient de la liste : on refuse tout ce qui sortirait de TP_RACINE.
		case "$TP" in */*|.|..) echo "Nom de TP refusé : $TP" >&2; exit 1 ;; esac
		sujet="$TP_RACINE/$TP"
		[ -d "$sujet" ] || echo "Attention : $sujet n'existe pas sur ce poste."
		if [ "$COPIER" = true ]; then
			copie="$ESPACE/$TP"
			[ -d "$ESPACE" ] || agir mkdir -p -- "$ESPACE"
			# --no-clobber : un travail déjà commencé n'est jamais écrasé.
			[ -e "$copie" ] && echo "Une copie existe déjà : elle est gardée telle quelle."
			agir cp -r --no-clobber -- "$sujet" "$ESPACE/"
			agir xdg-open "$copie"
		else
			agir xdg-open "$sujet"
		fi
		if [ "$ENONCE" = true ]; then agir xdg-open "$sujet/enonce.pdf"; fi
		;;
	esac
}

main() {
	if [ "${1:-}" = --suite ]; then suite "$2"; exit; fi
	[ -n "${1:-}" ] && SERMO=$1
	command -v xdg-open >/dev/null || { echo "xdg-open est absent : installez le paquet xdg-utils." >&2; exit 1; }
	[ -d "$TP_RACINE" ] || { echo "Dossier des TP introuvable : $TP_RACINE (réglez TP_RACINE)." >&2; exit 1; }
	local tps=()
	mapfile -t tps < <(find "$TP_RACINE" -mindepth 1 -maxdepth 1 -type d -printf '%f\n' | sort)
	[ ${#tps[@]} -gt 0 ] || { echo "Aucun TP dans $TP_RACINE." >&2; exit 1; }
	export DIALOGUE; DIALOGUE=$(dialogue_principal "$HOME/Travaux" "${tps[@]}")
	"$SERMO" --program=DIALOGUE --do='"$SERMO_SCRIPT" --suite principal' >/dev/null
}

[ "${BASH_SOURCE[0]}" = "$0" ] && main "$@"
