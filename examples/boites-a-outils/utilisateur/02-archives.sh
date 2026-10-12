#!/bin/bash
# SPDX-License-Identifier: CC0-1.0
# Boîte    : Utilisateur
# Besoin   : regrouper un dossier dans un seul fichier zip ou tar.gz, ou sortir les fichiers d'une archive reçue
# Enrobe   : zip, unzip, tar
# Ports    : gtk3sermo conseillé ; qt6sermo ; htmlsermo
# Terminal : rien de perdu
# Usage    : ./02-archives.sh [binaire sermo]
#            SERMO_A_BLANC=0 pour agir pour de vrai (défaut : la commande est affichée)
#            L'extraction se fait toujours dans un dossier NEUF : rien d'existant n'est écrasé.

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
	local dossier=${1:-$HOME/Documents/Vacances-2026} archive=${2:-$HOME/Téléchargements/photos-mariage.zip}
	cat <<XML
<window title="Archives zip et tar.gz" default-width="560">
  <vbox>
    <notebook tab-labels="Faire une archive|Ouvrir une archive">
      <vbox>
        <hbox><text><label>Dossier à regrouper :</label></text>
          <entry space-expand="true" space-fill="true"><variable>DOSSIER</variable><default>$(xml "$dossier")</default></entry></hbox>
        <hbox><text><label>Format :</label></text>
          <comboboxtext><variable>FORMAT</variable><default>zip (lu partout, Windows compris)</default>
            <item>zip (lu partout, Windows compris)</item><item>tar.gz (Linux)</item>
          </comboboxtext></hbox>
        <hbox><text><label>Compression (1 rapide, 9 plus petite) :</label></text>
          <hscale range-min="1" range-max="9" range-step="1" draw-value="true" space-expand="true" space-fill="true"><variable>NIVEAU</variable><default>6</default></hscale></hbox>
        <text><label>L'archive sera posée à côté du dossier, avec le même nom.</label></text>
        <hbox><button><label>Faire l'archive</label><action>exit:creer</action></button></hbox>
      </vbox>
      <vbox>
        <hbox><text><label>Archive à ouvrir :</label></text>
          <entry space-expand="true" space-fill="true"><variable>ARCHIVE</variable><default>$(xml "$archive")</default></entry></hbox>
        <text><label>Les fichiers sortiront dans un nouveau dossier, à côté de l'archive.</label></text>
        <hbox><button><label>Voir ce qu'elle contient</label><action>exit:regarder</action></button></hbox>
      </vbox>
    </notebook>
    <hbox>
      <button cancel></button>
    </hbox>
  </vbox>
</window>
XML
}

# Arguments : archive, dossier de sortie, puis un nom de fichier contenu par argument.
dialogue_contenu() {
	[ $# -gt 0 ] || set -- /home/utilisateur/Téléchargements/photos-mariage.zip \
		/home/utilisateur/Téléchargements/photos-mariage \
		"photos-mariage/" "photos-mariage/eglise-01.jpg" "photos-mariage/eglise-02.jpg" \
		"photos-mariage/repas-01.jpg" "photos-mariage/lisez-moi.txt"
	local archive=$1 sortie=$2 l
	shift 2
	cat <<XML
<window title="Contenu de l'archive" default-width="520" default-height="380">
  <vbox>
    <text><label>$(xml "$archive") contient :</label></text>
    <list scrollable="true" space-expand="true" space-fill="true">
      <variable>LIGNE</variable>
XML
	for l in "$@"; do printf '      <item>%s</item>\n' "$(xml "$l")"; done
	cat <<XML
    </list>
    <text><label>Les fichiers iront dans : $(xml "$sortie")</label></text>
    <checkbox><label>Ouvrir ce dossier ensuite</label><variable>OUVRIR</variable><default>true</default></checkbox>
    <hbox>
      <button cancel></button>
      <button><label>Sortir les fichiers</label><action>exit:extraire</action></button>
    </hbox>
  </vbox>
</window>
XML
}

dialogue_remplacer() {
	local fichier=${1:-/home/utilisateur/Documents/Vacances-2026.zip}
	cat <<XML
<window title="Remplacer l'archive ?">
  <vbox>
    <text><label>Ce fichier existe déjà : $(xml "$fichier")</label></text>
    <text><label>L'ancien sera mis à la corbeille, puis remplacé par le nouveau.</label></text>
    <hbox>
      <button cancel></button>
      <button><label>Remplacer</label><action>exit:remplacer</action></button>
    </hbox>
  </vbox>
</window>
XML
}

# Fait l'archive de DOSSIER à côté de lui ($SORTIE).
creer() {
	local parent nom
	parent=$(dirname -- "$DOSSIER"); nom=$(basename -- "$DOSSIER")
	cd -- "$parent" || exit 1
	if [ "$GENRE" = zip ]; then
		# zip refuse « -- » avant le nom de l'archive ; SORTIE est un chemin absolu.
		agir zip -r -q "-$NIVEAU" "$SORTIE" -- "$nom"
	else
		agir tar -I "gzip -$NIVEAU" -cf "$SORTIE" -C "$parent" -- "$nom"
	fi
}

# Liste le contenu d'une archive (lecture seule).
contenu() {
	case "$ARCHIVE" in
	*.zip) unzip -Z1 -- "$ARCHIVE" ;;
	*) tar -tzf "$ARCHIVE" ;;
	esac
}

suite() {
	# La sortie standard de sermo part vers /dev/null : écrire sur la sortie d'erreur.
	exec >&2
	case "$FORMAT" in tar.gz*) GENRE=tar.gz ;; *) GENRE=zip ;; esac
	[[ $NIVEAU =~ ^[1-9]$ ]] || NIVEAU=6
	DOSSIER=${DOSSIER%/}
	SORTIE=$DOSSIER.$GENRE
	case "$1" in
	principal)
		case "$EXIT" in
		creer)
			case "$DOSSIER" in /?*) ;; *) echo "Donnez le chemin complet du dossier (il commence par /)." >&2; exit 1 ;; esac
			[ -d "$DOSSIER" ] || { echo "Ce dossier n'existe pas : $DOSSIER" >&2; exit 1; }
			if [ -e "$SORTIE" ]; then
				export DOSSIER FORMAT NIVEAU
				export DIALOGUE; DIALOGUE=$(dialogue_remplacer "$SORTIE")
				"$SERMO" --program=DIALOGUE --do='"$SERMO_SCRIPT" --suite remplacer' >/dev/null
			else
				creer
			fi
			;;
		regarder)
			case "$ARCHIVE" in /?*) ;; *) echo "Donnez le chemin complet de l'archive (il commence par /)." >&2; exit 1 ;; esac
			[ -f "$ARCHIVE" ] || { echo "Archive introuvable : $ARCHIVE" >&2; exit 1; }
			case "$ARCHIVE" in
			*.zip|*.tar.gz|*.tgz) ;;
			*) echo "Seuls les fichiers .zip, .tar.gz et .tgz sont pris en charge." >&2; exit 1 ;;
			esac
			local base sortie lignes=()
			base=${ARCHIVE%.zip}; base=${base%.tar.gz}; base=${base%.tgz}
			sortie=$base; local n=2
			while [ -e "$sortie" ]; do sortie="$base-$n"; n=$((n + 1)); done
			mapfile -t lignes < <(contenu 2>/dev/null | head -n 300)
			[ ${#lignes[@]} -gt 0 ] || { echo "Archive vide ou illisible : $ARCHIVE" >&2; exit 1; }
			export ARCHIVE SORTIE_EXTRAIT=$sortie
			export DIALOGUE; DIALOGUE=$(dialogue_contenu "$ARCHIVE" "$sortie" "${lignes[@]}")
			"$SERMO" --program=DIALOGUE --do='"$SERMO_SCRIPT" --suite contenu' >/dev/null
			;;
		esac
		;;
	remplacer)
		[ "$EXIT" = remplacer ] || exit 0
		agir gio trash -- "$SORTIE" || exit 1
		creer
		;;
	contenu)
		[ "$EXIT" = extraire ] || exit 0
		[ ! -e "$SORTIE_EXTRAIT" ] || { echo "Le dossier existe déjà : $SORTIE_EXTRAIT" >&2; exit 1; }
		agir mkdir -- "$SORTIE_EXTRAIT" || exit 1
		case "$ARCHIVE" in
		*.zip) agir unzip -q -- "$ARCHIVE" -d "$SORTIE_EXTRAIT" ;;
		*)     agir tar -xzf "$ARCHIVE" --no-same-owner -C "$SORTIE_EXTRAIT" ;;
		esac
		[ "$OUVRIR" = true ] && [ "$SERMO_A_BLANC" != 1 ] && { xdg-open "$SORTIE_EXTRAIT" >/dev/null 2>&1 & }
		;;
	esac
}

main() {
	if [ "${1:-}" = --suite ]; then suite "$2"; exit; fi
	[ -n "${1:-}" ] && SERMO=$1
	local c
	for c in zip unzip tar gzip gio; do
		command -v "$c" >/dev/null || { echo "$c est absent : installez le paquet $c (gio : libglib2.0-bin)." >&2; exit 1; }
	done
	export DIALOGUE; DIALOGUE=$(dialogue_principal)
	"$SERMO" --program=DIALOGUE --do='"$SERMO_SCRIPT" --suite principal' >/dev/null
}

[ "${BASH_SOURCE[0]}" = "$0" ] && main "$@"
