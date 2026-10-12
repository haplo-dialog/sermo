#!/bin/bash
# SPDX-License-Identifier: CC0-1.0
# Boîte    : Utilisateur
# Besoin   : savoir quels dossiers prennent le plus de place dans son dossier personnel, descendre dedans, et ouvrir celui qu'on veut trier soi-même
# Enrobe   : du (lecture seule) ; df pour la place libre ; xdg-open pour ouvrir le dossier
# Ports    : gtk3sermo conseillé ; qt6sermo ; htmlsermo
# Terminal : en ncurses, « Ouvrir ce dossier » n'a pas d'effet sans bureau graphique
# Usage    : ./10-place-disque.sh [binaire sermo] [dossier]
#            Rien n'est modifié ni effacé : l'outil ne fait que mesurer.

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

# Arguments : dossier mesuré, place libre sur le disque, puis une ligne « dossier|taille » par sous-dossier.
dialogue_principal() {
	[ $# -gt 0 ] || set -- /home/utilisateur "38 Go" \
		"/home/utilisateur/Vidéos|21 Go" "/home/utilisateur/Images|9,6 Go" \
		"/home/utilisateur/.cache|3,1 Go" "/home/utilisateur/Téléchargements|2,4 Go" \
		"/home/utilisateur/Musique|1,8 Go" "/home/utilisateur/Documents|640 Mo"
	local dossier=$1 libre=$2 l
	shift 2
	cat <<XML
<window title="Ce qui prend de la place" default-width="600" default-height="480">
  <vbox>
    <text><label>Les plus gros dossiers dans $(xml "$dossier") :</label></text>
    <tree scrollable="true" space-expand="true" space-fill="true">
      <label>Dossier|Taille</label>
      <variable>CHOIX</variable>
XML
	for l in "$@"; do printf '      <item>%s</item>\n' "$(xml "$l")"; done
	cat <<XML
    </tree>
    <hbox>
      <checkbox><label>Compter les dossiers cachés (.cache…)</label><variable>CACHES</variable><default>true</default></checkbox>
      <text><label>Lignes montrées :</label></text>
      <spinbutton range-min="5" range-max="50" range-step="5"><variable>LIGNES</variable><default>15</default></spinbutton>
    </hbox>
    <statusbar><variable>LIBRE</variable><default>Place encore libre sur le disque : $(xml "$libre")</default></statusbar>
    <hbox>
      <button><label>Fermer</label><action>exit:fermer</action></button>
      <button><label>Recompter</label><action>exit:recompter</action></button>
      <button><label>Regarder dedans</label><action>exit:descendre</action></button>
      <button><label>Ouvrir ce dossier</label><action>exit:ouvrir</action></button>
    </hbox>
  </vbox>
</window>
XML
}

# Mesure un dossier et ouvre la fenêtre.
mesurer() {
	local dossier=$1 lignes=() t c libre
	[[ $LIGNES =~ ^[0-9]+$ ]] && [ "$LIGNES" -ge 5 ] && [ "$LIGNES" -le 50 ] || LIGNES=15
	local filtre=(cat)
	[ "$CACHES" = true ] || filtre=(grep -v '/\.[^/]*$')
	while IFS=$'\t' read -r t c; do
		[ "$c" = "$dossier" ] && continue
		lignes+=("${c//|//}|$t")
	done < <(du -xh --max-depth=1 -- "$dossier" 2>/dev/null | "${filtre[@]}" | sort -rh | head -n "$((LIGNES + 1))")
	libre=$(df -h --output=avail -- "$dossier" 2>/dev/null | tail -n1 | tr -d ' ')
	export DOSSIER=$dossier LIGNES CACHES
	export DIALOGUE; DIALOGUE=$(dialogue_principal "$dossier" "${libre:-?}" "${lignes[@]}")
	"$SERMO" --program=DIALOGUE --do='"$SERMO_SCRIPT" --suite principal' >/dev/null
}

suite() {
	# La sortie standard de sermo part vers /dev/null : écrire sur la sortie d'erreur.
	exec >&2
	case "$1" in
	principal)
		case "$EXIT" in
		recompter) mesurer "$DOSSIER" ;;
		descendre|ouvrir)
			[ -n "$CHOIX" ] || { echo "Choisissez d'abord une ligne." >&2; exit 1; }
			[ -d "$CHOIX" ] || { echo "Dossier introuvable : $CHOIX" >&2; exit 1; }
			# On reste dans le dossier personnel.
			case "$CHOIX/" in "$HOME"/*) ;; *) echo "Hors du dossier personnel : $CHOIX" >&2; exit 1 ;; esac
			if [ "$EXIT" = descendre ]; then
				mesurer "$CHOIX"
			else
				# Ouvrir un dossier ne modifie rien : lancé directement, détaché.
				xdg-open "$CHOIX" >/dev/null 2>&1 &
			fi
			;;
		esac
		;;
	esac
}

main() {
	if [ "${1:-}" = --suite ]; then suite "$2"; exit; fi
	[ -n "${1:-}" ] && SERMO=$1
	local c
	for c in du df sort xdg-open; do
		command -v "$c" >/dev/null || { echo "$c est absent : il faut les paquets coreutils et xdg-utils." >&2; exit 1; }
	done
	local dossier=${2:-$HOME}
	dossier=${dossier%/}
	[ -d "$dossier" ] || { echo "Dossier introuvable : $dossier" >&2; exit 1; }
	CACHES=true LIGNES=15
	echo "Je mesure $dossier, cela peut prendre une minute…" >&2
	mesurer "$dossier"
}

[ "${BASH_SOURCE[0]}" = "$0" ] && main "$@"
