#!/bin/bash
# SPDX-License-Identifier: CC0-1.0
# Boîte    : Utilisateur
# Besoin   : copier un dossier sur une clé USB branchée, en voyant d'abord combien partira et la place libre ; rien n'est effacé sur la clé par défaut
# Enrobe   : rsync (sans --delete par défaut) ; df pour la place libre
# Ports    : gtk3sermo conseillé ; qt6sermo ; htmlsermo
# Terminal : rien de perdu
# Usage    : ./08-copie-cle-usb.sh [binaire sermo]
#            SERMO_A_BLANC=0 pour agir pour de vrai (défaut : la commande est affichée)
#            La clé doit être branchée et ouverte une fois (montée) par le bureau.

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

# Une <item> par ligne non vide.
items() {
	local l
	while IFS= read -r l; do [ -n "$l" ] && printf '<item>%s</item>' "$(xml "$l")"; done <<< "$1"
}

# Arguments : clés montées (une par ligne), dossier proposé.
dialogue_principal() {
	local cles=${1:-$'/media/utilisateur/CLE-USB\n/media/utilisateur/KINGSTON'} source=${2:-$HOME/Documents}
	cat <<XML
<window title="Copier sur une clé USB" default-width="560">
  <vbox>
    <frame label="Quoi">
      <hbox><text><label>Dossier à copier :</label></text>
        <entry space-expand="true" space-fill="true"><variable>SOURCE</variable><default>$(xml "$source")</default></entry></hbox>
    </frame>
    <frame label="Vers quelle clé">
      <vbox>
        <comboboxtext space-expand="true" space-fill="true"><variable>CLE</variable><default>$(xml "${cles%%$'\n'*}")</default>$(items "$cles")</comboboxtext>
        <hbox><text><label>Dans le dossier de la clé :</label></text>
          <entry space-expand="true" space-fill="true"><variable>SOUS_DOSSIER</variable><default>Copie</default></entry></hbox>
      </vbox>
    </frame>
    <frame label="Comment">
      <vbox>
        <hbox><switch><variable>RECENTS</variable><default>true</default></switch>
          <text><label>Ne pas recopier ce qui est déjà à jour sur la clé</label></text></hbox>
        <checkbox><label>Effacer sur la clé ce qui n'existe plus dans le dossier (déconseillé)</label><variable>EFFACER</variable></checkbox>
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

# Arguments : nombre de fichiers, taille à copier, place libre, destination, à effacer (nombre).
dialogue_confirmation() {
	local n=${1:-214} taille=${2:-1,3 Go} libre=${3:-12 Go} dest=${4:-/media/utilisateur/CLE-USB/Copie} effaces=${5:-0}
	cat <<XML
<window title="Avant de copier">
  <vbox>
    <table height-request="120">
      <label>Quoi|Combien</label>
      <item>Fichiers à copier|$(xml "$n")</item>
      <item>Taille à copier|$(xml "$taille")</item>
      <item>Place libre sur la clé|$(xml "$libre")</item>
      <item>Fichiers effacés sur la clé|$(xml "$effaces")</item>
      <variable>RESUME</variable>
    </table>
    <text><label>Destination : $(xml "$dest")</label></text>
    <statusbar><variable>CONSEIL</variable><default>À la fin, éjectez la clé avant de la débrancher.</default></statusbar>
    <hbox>
      <button cancel></button>
      <button><label>Copier</label><action>exit:copier</action></button>
    </hbox>
  </vbox>
</window>
XML
}

# Options rsync, une par ligne. -rt plutôt que -a : une clé FAT ne garde ni droits ni propriétaire.
options() {
	echo -rt; echo --modify-window=2; echo --human-readable
	[ "$RECENTS" = true ] && echo --update
	[ "$EFFACER" = true ] && echo --delete
}

# Les clés montées : un dossier par clé, au nom de la personne, sous /media ou
# sous /run/media (selon la distribution).
cles() {
	local d moi base
	moi=$(id -un)
	for base in /media /run/media; do for d in "$base/$moi"/*; do
		[ -d "$d" ] && mountpoint -q -- "$d" && printf '%s\n' "$d"
	done; done
}

suite() {
	# La sortie standard de sermo part vers /dev/null : écrire sur la sortie d'erreur.
	exec >&2
	SOURCE=${SOURCE%/}
	DEST=$CLE/$SOUS_DOSSIER
	case "$1" in
	principal)
		[ "$EXIT" = simuler ] || exit 0
		[ -d "$SOURCE" ] || { echo "Dossier introuvable : $SOURCE" >&2; exit 1; }
		cles | grep -qxF -- "$CLE" || { echo "Cette clé n'est pas branchée ou pas ouverte : $CLE" >&2; exit 1; }
		[[ $SOUS_DOSSIER =~ ^[A-Za-z0-9][A-Za-z0-9\ ._-]{0,60}$ ]] || { echo "Nom de dossier refusé : « $SOUS_DOSSIER » (lettres, chiffres, espace, . _ -)." >&2; exit 1; }
		local opts stats n taille libre effaces
		mapfile -t opts < <(options)
		# La simulation ne modifie rien : elle est toujours lancée.
		stats=$(LC_ALL=C rsync "${opts[@]}" --dry-run --stats --itemize-changes -- "$SOURCE/" "$DEST/" 2>/dev/null)
		n=$(sed -n 's/^Number of regular files transferred: //p' <<< "$stats")
		taille=$(sed -n 's/^Total transferred file size: \([^ ]*\).*/\1/p' <<< "$stats")
		effaces=$(grep -c '^\*deleting' <<< "$stats")
		libre=$(df -h --output=avail -- "$CLE" 2>/dev/null | tail -n1 | tr -d ' ')
		export SOURCE CLE SOUS_DOSSIER RECENTS EFFACER
		export DIALOGUE; DIALOGUE=$(dialogue_confirmation "${n:-?}" "${taille:-?}" "${libre:-?}" "$DEST" "$effaces")
		"$SERMO" --program=DIALOGUE --do='"$SERMO_SCRIPT" --suite confirmation' >/dev/null
		;;
	confirmation)
		[ "$EXIT" = copier ] || exit 0
		local opts
		mapfile -t opts < <(options)
		agir rsync "${opts[@]}" -- "$SOURCE/" "$DEST/"
		;;
	esac
}

main() {
	if [ "${1:-}" = --suite ]; then suite "$2"; exit; fi
	[ -n "${1:-}" ] && SERMO=$1
	local c
	for c in rsync df mountpoint; do
		command -v "$c" >/dev/null || { echo "$c est absent : il faut les paquets rsync, coreutils et util-linux." >&2; exit 1; }
	done
	local liste
	liste=$(cles)
	[ -n "$liste" ] || { echo "Aucune clé USB ouverte : branchez-la et ouvrez-la une fois dans le gestionnaire de fichiers." >&2; exit 1; }
	export DIALOGUE; DIALOGUE=$(dialogue_principal "$liste" "$HOME/Documents")
	"$SERMO" --program=DIALOGUE --do='"$SERMO_SCRIPT" --suite principal' >/dev/null
}

[ "${BASH_SOURCE[0]}" = "$0" ] && main "$@"
