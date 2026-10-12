#!/bin/bash
# SPDX-License-Identifier: CC0-1.0
# Boîte    : Utilisateur
# Besoin   : mettre de l'ordre dans un dossier en vrac (Téléchargements…) : trier les fichiers en sous-dossiers par genre ou par mois, et mettre les très vieux à la corbeille
# Enrobe   : mkdir, mv -n ; gio trash pour la corbeille (rien n'est effacé pour de bon)
# Ports    : gtk3sermo conseillé ; qt6sermo ; htmlsermo
# Terminal : rien de perdu
# Usage    : ./07-ranger-dossier.sh [binaire sermo]
#            SERMO_A_BLANC=0 pour agir pour de vrai (défaut : la commande est affichée)
#            Seuls les fichiers posés directement dans le dossier bougent ; les
#            sous-dossiers et les fichiers cachés ne sont pas touchés. Rien n'est écrasé.

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
	local dossier=${1:-$HOME/Téléchargements}
	cat <<XML
<window title="Ranger un dossier" default-width="540">
  <vbox>
    <frame label="Dossier à ranger">
      <entry><variable>DOSSIER</variable><default>$(xml "$dossier")</default></entry>
    </frame>
    <frame label="Comment trier">
      <vbox>
        <radiobutton><label>Par genre : Images, Documents, Musique, Vidéos, Archives, Autres</label><variable>PAR_GENRE</variable><default>true</default></radiobutton>
        <radiobutton><label>Par mois : 2026-09, 2026-10…</label><variable>PAR_MOIS</variable></radiobutton>
      </vbox>
    </frame>
    <frame label="Vieux fichiers">
      <vbox>
        <checkbox><label>Mettre à la corbeille les fichiers plus vieux que :</label><variable>VIEUX</variable></checkbox>
        <hbox><hscale range-min="1" range-max="36" range-step="1" draw-value="true" space-expand="true" space-fill="true"><variable>MOIS</variable><default>12</default></hscale>
          <text><label>mois</label></text></hbox>
      </vbox>
    </frame>
    <hbox>
      <button cancel></button>
      <button><label>Voir ce qui va bouger</label><action>exit:preparer</action></button>
    </hbox>
  </vbox>
</window>
XML
}

# Arguments : le plan (une ligne par geste), le nombre de fichiers déplacés, le nombre mis à la corbeille.
dialogue_plan() {
	local plan=${1:-$'facture-garage.pdf  →  Documents/\nIMG_2041.jpg  →  Images/\nIMG_2042.jpg  →  Images/\nchanson.mp3  →  Musique/\nvieux-formulaire.pdf  →  corbeille'} \
		bouges=${2:-4} corbeille=${3:-1}
	cat <<XML
<window title="Ce qui va bouger" default-width="560" default-height="420">
  <vbox>
    <text><label>$(xml "$bouges") fichier(s) rangé(s), $(xml "$corbeille") mis à la corbeille.</label></text>
    <expander label="Voir le détail" expanded="true">
      <edit editable="false" scrollable="true" height-request="220"><variable>PLAN</variable><default>$(xml "$plan")</default></edit>
      <variable>DETAIL</variable>
    </expander>
    <text><label>Rien n'est effacé pour de bon : la corbeille se vide seulement quand vous le décidez.</label></text>
    <hbox>
      <button cancel></button>
      <button><label>Ranger</label><action>exit:ranger</action></button>
    </hbox>
  </vbox>
</window>
XML
}

# Le sous-dossier d'un fichier, selon le tri choisi.
destination() {
	if [ "$PAR_MOIS" = true ]; then
		date -r "$1" +%Y-%m
		return
	fi
	case "${1,,}" in
	*.jpg|*.jpeg|*.png|*.gif|*.webp|*.heic|*.svg|*.bmp|*.tif|*.tiff) echo Images ;;
	*.pdf|*.odt|*.ods|*.odp|*.doc|*.docx|*.xls|*.xlsx|*.ppt|*.pptx|*.txt|*.rtf|*.csv) echo Documents ;;
	*.mp3|*.ogg|*.flac|*.wav|*.m4a|*.opus) echo Musique ;;
	*.mp4|*.mkv|*.avi|*.webm|*.mov) echo Vidéos ;;
	*.zip|*.gz|*.tgz|*.xz|*.bz2|*.7z|*.rar|*.tar|*.deb|*.iso) echo Archives ;;
	*) echo Autres ;;
	esac
}

# Écrit le plan, une ligne par fichier : « corbeille|chemin » ou « ranger|chemin|sous-dossier ».
plan() {
	local f
	while IFS= read -r -d '' f; do
		# Un nom avec « | » ou un saut de ligne casserait le plan : laissé en place.
		case "$f" in *'|'*|*$'\n'*) continue ;; esac
		if [ "$VIEUX" = true ] && [ -n "$(find "$f" -maxdepth 0 -mtime +"$JOURS" -print -quit)" ]; then
			printf 'corbeille|%s\n' "$f"
		else
			printf 'ranger|%s|%s\n' "$f" "$(destination "$f")"
		fi
	done < <(find "$DOSSIER" -mindepth 1 -maxdepth 1 -type f -not -name '.*' -print0 | sort -z | head -z -n 500)
}

suite() {
	# La sortie standard de sermo part vers /dev/null : écrire sur la sortie d'erreur.
	exec >&2
	[[ $MOIS =~ ^[0-9]+$ ]] && [ "$MOIS" -ge 1 ] && [ "$MOIS" -le 36 ] || MOIS=12
	JOURS=$((MOIS * 30))
	case "$1" in
	principal)
		[ "$EXIT" = preparer ] || exit 0
		case "$DOSSIER" in /*) ;; *) echo "Donnez le chemin complet du dossier (il commence par /)." >&2; exit 1 ;; esac
		[ -d "$DOSSIER" ] || { echo "Dossier introuvable : $DOSSIER" >&2; exit 1; }
		# Forme canonique AVANT toute comparaison : « $HOME/. », « a/../ » ou un lien ne contournent rien.
		DOSSIER=$(realpath -e -- "$DOSSIER") || { echo "Dossier illisible : $DOSSIER" >&2; exit 1; }
		local moi
		moi=$(realpath -e -- "$HOME") || moi=$HOME
		[ "$DOSSIER" != / ] || { echo "La racine du système n'est pas rangée." >&2; exit 1; }
		[ "$DOSSIER" != "$moi" ] || { echo "Par prudence, le dossier personnel lui-même n'est pas rangé : choisissez un de ses sous-dossiers." >&2; exit 1; }
		case "$DOSSIER/" in */.*/*) echo "Les dossiers cachés (nom commençant par un point) ne sont pas rangés." >&2; exit 1 ;; esac
		local quoi f sous texte="" bouges=0 corbeille=0 montre
		# Le plan MONTRÉ est gardé tel quel : c'est lui, et lui seul, qui sera exécuté.
		montre=$(plan)
		[ ${#montre} -le 100000 ] || { echo "Trop de fichiers d'un coup (500 au plus) : rangez en plusieurs fois." >&2; exit 1; }
		while IFS='|' read -r quoi f sous; do
			[ -n "$quoi" ] || continue
			if [ "$quoi" = corbeille ]; then
				texte+="$(basename -- "$f")  →  corbeille"$'\n'; corbeille=$((corbeille + 1))
			else
				texte+="$(basename -- "$f")  →  $sous/"$'\n'; bouges=$((bouges + 1))
			fi
		done <<< "$montre"
		[ $((bouges + corbeille)) -gt 0 ] || { echo "Rien à ranger dans $DOSSIER." >&2; exit 0; }
		export DOSSIER PAR_GENRE PAR_MOIS VIEUX MOIS
		export PLAN_MONTRE=$montre
		export DIALOGUE; DIALOGUE=$(dialogue_plan "${texte%$'\n'}" "$bouges" "$corbeille")
		"$SERMO" --program=DIALOGUE --do='"$SERMO_SCRIPT" --suite plan' >/dev/null
		;;
	plan)
		[ "$EXIT" = ranger ] || exit 0
		[ -d "$DOSSIER" ] || exit 1
		local quoi f sous
		local -A faits=()
		# Seul le plan montré est exécuté, ligne par ligne, et chaque ligne est
		# revérifiée : fichier ordinaire, posé directement dans DOSSIER, sous-dossier connu.
		while IFS='|' read -r quoi f sous; do
			[ -n "$quoi" ] || continue
			[ "$(dirname -- "$f")" = "$DOSSIER" ] && [ -f "$f" ] && [ ! -L "$f" ] || { echo "Laissé en place (changé depuis l'aperçu) : $f" >&2; continue; }
			case "$quoi|$sous" in
			"corbeille|") ;;
			ranger\|Images|ranger\|Documents|ranger\|Musique|ranger\|Vidéos|ranger\|Archives|ranger\|Autres) ;;
			ranger\|[0-9][0-9][0-9][0-9]-[0-9][0-9]) ;;
			*) echo "Ligne de plan refusée : $quoi|$f|$sous" >&2; continue ;;
			esac
			if [ "$quoi" = corbeille ]; then
				agir gio trash -- "$f"
			else
				[ -d "$DOSSIER/$sous" ] || [ -n "${faits[$sous]:-}" ] || agir mkdir -- "$DOSSIER/$sous"
				faits[$sous]=1
				agir mv -n -- "$f" "$DOSSIER/$sous/"
			fi
		done <<< "$PLAN_MONTRE"
		;;
	esac
}

main() {
	if [ "${1:-}" = --suite ]; then suite "$2"; exit; fi
	[ -n "${1:-}" ] && SERMO=$1
	local c
	for c in find mv gio; do
		command -v "$c" >/dev/null || { echo "$c est absent : il faut les paquets findutils, coreutils et libglib2.0-bin." >&2; exit 1; }
	done
	local dossier=$HOME/Téléchargements
	[ -d "$dossier" ] || dossier=$HOME/Downloads
	export DIALOGUE; DIALOGUE=$(dialogue_principal "$dossier")
	"$SERMO" --program=DIALOGUE --do='"$SERMO_SCRIPT" --suite principal' >/dev/null
}

[ "${BASH_SOURCE[0]}" = "$0" ] && main "$@"
