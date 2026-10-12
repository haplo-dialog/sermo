#!/bin/bash
# SPDX-License-Identifier: CC0-1.0
# Métier   : Administrateur système
# Besoin   : sauvegarder un dossier vers un disque, en voyant d'abord ce qui partira
# Enrobe   : rsync
# Ports    : gtk3sermo conseillé ; ncursessermo par SSH
# Terminal : rien de perdu
# Usage    : ./1-sauvegarde.sh [binaire sermo]
#            SERMO_A_BLANC=0 pour agir pour de vrai (défaut : la commande est affichée)

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

dialogue_principal() {
	local source=${1:-$HOME/Documents} destination=${2:-/media/sauvegarde}
	cat <<XML
<window title="Sauvegarde" default-width="520">
  <vbox>
    <frame label="Quoi et où">
      <vbox>
        <hbox><text><label>Dossier à sauvegarder :</label></text>
          <entry space-expand="true" space-fill="true"><variable>SOURCE</variable><default>$(xml "$source")</default></entry></hbox>
        <hbox><text><label>Destination :</label></text>
          <entry space-expand="true" space-fill="true"><variable>DESTINATION</variable><default>$(xml "$destination")</default></entry></hbox>
      </vbox>
    </frame>
    <frame label="Options">
      <vbox>
        <checkbox><label>Garder les droits et les dates (-a)</label><default>true</default><variable>ARCHIVE</variable></checkbox>
        <checkbox><label>Compresser pendant le transfert (-z)</label><variable>COMPRESSER</variable></checkbox>
        <checkbox><label>Effacer à destination ce qui n'existe plus à la source (--delete)</label><variable>EFFACER</variable></checkbox>
      </vbox>
    </frame>
    <hbox>
      <button cancel></button>
      <button><label>Voir ce qui partira</label><action>exit:simuler</action></button>
    </hbox>
  </vbox>
</window>
XML
}

dialogue_confirmation() {
	local resume=${1:-12 fichiers, 3,4 Mo}
	cat <<XML
<window title="Confirmer la sauvegarde">
  <vbox>
    <text><label>Simulation faite : $(xml "$resume").</label></text>
    <text><label>Lancer la sauvegarde pour de vrai ?</label></text>
    <hbox>
      <button cancel></button>
      <button><label>Sauvegarder</label><action>exit:sauvegarder</action></button>
    </hbox>
  </vbox>
</window>
XML
}

# Options rsync choisies, une par ligne.
options() {
	[ "$ARCHIVE" = true ] && echo -a
	[ "$COMPRESSER" = true ] && echo -z
	[ "$EFFACER" = true ] && echo --delete
	echo --human-readable
}

suite() {
	# La sortie standard de sermo part vers /dev/null (les lignes NOM=…) et
	# l'étape --do en hérite : écrire sur la sortie d'erreur, qui reste visible.
	exec >&2
	case "$1" in
	principal)
		[ "$EXIT" = simuler ] || exit 0
		[ -d "$SOURCE" ] || { echo "Dossier introuvable : $SOURCE" >&2; exit 1; }
		mapfile -t opts < <(options)
		# La simulation ne modifie rien : elle est toujours lancée.
		resume=$(rsync "${opts[@]}" --dry-run --stats "$SOURCE/" "$DESTINATION/" 2>&1 |
			grep -E '^(Number of regular files transferred|Total transferred file size)' | tr '\n' ' ')
		export SOURCE DESTINATION ARCHIVE COMPRESSER EFFACER
		export DIALOGUE; DIALOGUE=$(dialogue_confirmation "${resume:-rien à transférer}")
		"$SERMO" --program=DIALOGUE --do='"$SERMO_SCRIPT" --suite confirmation' >/dev/null
		;;
	confirmation)
		[ "$EXIT" = sauvegarder ] || exit 0
		mapfile -t opts < <(options)
		agir rsync "${opts[@]}" "$SOURCE/" "$DESTINATION/"
		;;
	esac
}

main() {
	if [ "${1:-}" = --suite ]; then suite "$2"; exit; fi
	[ -n "${1:-}" ] && SERMO=$1
	command -v rsync >/dev/null || { echo "rsync est absent : installez le paquet rsync." >&2; exit 1; }
	export DIALOGUE; DIALOGUE=$(dialogue_principal)
	"$SERMO" --program=DIALOGUE --do='"$SERMO_SCRIPT" --suite principal' >/dev/null
}

[ "${BASH_SOURCE[0]}" = "$0" ] && main "$@"
