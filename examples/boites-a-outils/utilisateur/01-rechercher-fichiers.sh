#!/bin/bash
# SPDX-License-Identifier: CC0-1.0
# Boîte    : Utilisateur
# Besoin   : retrouver un fichier perdu par son nom, son genre, sa taille ou sa date, puis ouvrir son dossier
# Enrobe   : find (lecture seule) ; xdg-open pour ouvrir le dossier trouvé
# Ports    : gtk3sermo conseillé ; qt6sermo ; htmlsermo
# Terminal : en ncurses, « Ouvrir le dossier » n'a pas d'effet sans bureau graphique
# Usage    : ./01-rechercher-fichiers.sh [binaire sermo]
#            Rien n'est modifié : la recherche ne fait que lire.

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
	local dossier=${1:-$HOME} depuis=${2:-2026-01-01}
	cat <<XML
<window title="Retrouver un fichier" default-width="560">
  <vbox>
    <frame label="Où chercher">
      <hbox><text><label>Dans le dossier :</label></text>
        <entry space-expand="true" space-fill="true"><variable>DOSSIER</variable><default>$(xml "$dossier")</default></entry></hbox>
    </frame>
    <frame label="Ce que je cherche">
      <vbox>
        <hbox><text><label>Le nom contient :</label></text>
          <entry space-expand="true" space-fill="true"><variable>NOM</variable><default>facture</default></entry></hbox>
        <hbox><text><label>Genre de fichier :</label></text>
          <comboboxtext><variable>GENRE</variable><default>Tous</default>
            <item>Tous</item><item>Documents</item><item>Images</item><item>Musique</item><item>Vidéos</item><item>Archives</item>
          </comboboxtext></hbox>
        <text><label>Taille :</label></text>
        <hbox>
          <radiobutton><label>Peu importe</label><variable>T_TOUTES</variable><default>true</default></radiobutton>
          <radiobutton><label>Plus de 1 Mo</label><variable>T_MOYEN</variable></radiobutton>
          <radiobutton><label>Plus de 100 Mo</label><variable>T_GROS</variable></radiobutton>
        </hbox>
      </vbox>
    </frame>
    <expander label="Plus d'options">
      <vbox>
        <checkbox><label>Seulement les fichiers changés depuis le jour choisi</label><variable>AVEC_DATE</variable></checkbox>
        <calendar><variable>DEPUIS</variable><default>$(xml "$depuis")</default></calendar>
        <checkbox><label>Chercher aussi dans les dossiers cachés</label><variable>CACHES</variable></checkbox>
      </vbox>
      <variable>PLUS</variable>
    </expander>
    <hbox>
      <button cancel></button>
      <button><label>Chercher</label><action>exit:chercher</action></button>
    </hbox>
  </vbox>
</window>
XML
}

# Arguments : nombre trouvé, puis une ligne par fichier « nom|taille|date|dossier ».
dialogue_resultats() {
	[ $# -gt 0 ] || set -- 4 \
		"facture-electricite-2026-03.pdf|182 Ko|2026-03-14|/home/utilisateur/Documents/Maison" \
		"facture-garage.pdf|1,2 Mo|2026-05-02|/home/utilisateur/Téléchargements" \
		"facture-internet-avril.pdf|96 Ko|2026-04-08|/home/utilisateur/Documents/Maison" \
		"facture-dentiste.jpg|2,4 Mo|2026-06-21|/home/utilisateur/Images/Papiers"
	local nombre=$1 l
	shift
	cat <<XML
<window title="Fichiers trouvés" default-width="720" default-height="420">
  <vbox>
    <text><label>Choisissez une ligne, puis « Ouvrir son dossier ».</label></text>
    <tree scrollable="true" exported-column="3" space-expand="true" space-fill="true">
      <label>Nom|Taille|Changé le|Dossier</label>
      <variable>CHOIX</variable>
XML
	for l in "$@"; do printf '      <item>%s</item>\n' "$(xml "$l")"; done
	cat <<XML
    </tree>
    <statusbar><variable>BILAN</variable><default>$(xml "$nombre") fichier(s) trouvé(s) (200 au plus)</default></statusbar>
    <hbox>
      <button><label>Fermer</label><action>exit:fermer</action></button>
      <button><label>Ouvrir son dossier</label><action>exit:ouvrir</action></button>
    </hbox>
  </vbox>
</window>
XML
}

# Les critères de find, un par ligne (lus par mapfile).
criteres() {
	[ "$CACHES" = true ] || { echo -not; echo -path; echo '*/.*'; }
	echo -type; echo f
	[ -n "$NOM" ] && { echo -iname; echo "*$NOM*"; }
	local ext=()
	case "$GENRE" in
	Documents) ext=(pdf odt ods odp doc docx xls xlsx ppt pptx txt rtf) ;;
	Images)    ext=(jpg jpeg png gif webp heic svg bmp tif tiff) ;;
	Musique)   ext=(mp3 ogg flac wav m4a opus) ;;
	Vidéos)    ext=(mp4 mkv avi webm mov) ;;
	Archives)  ext=(zip gz tgz xz bz2 7z rar tar) ;;
	esac
	if [ ${#ext[@]} -gt 0 ]; then
		local e premier=1
		echo '('
		for e in "${ext[@]}"; do
			[ $premier = 1 ] || echo -o
			premier=0; echo -iname; echo "*.$e"
		done
		echo ')'
	fi
	[ "$T_MOYEN" = true ] && { echo -size; echo +1M; }
	[ "$T_GROS" = true ] && { echo -size; echo +100M; }
	[ "$AVEC_DATE" = true ] && { echo -newermt; echo "$DEPUIS"; }
}

chercher() {
	local args lignes=() nom taille date dossier
	mapfile -t args < <(criteres)
	echo "find $DOSSIER ${args[*]}"
	while IFS=$'\t' read -r nom taille date dossier; do
		taille=$(numfmt --to=iec --suffix=o -- "$taille" 2>/dev/null || echo "$taille")
		lignes+=("${nom//|//}|$taille|$date|${dossier//|//}")
	done < <(find "$DOSSIER" -xdev "${args[@]}" -printf '%f\t%s\t%TY-%Tm-%Td\t%h\n' 2>/dev/null | head -n 200)
	export DIALOGUE; DIALOGUE=$(dialogue_resultats "${#lignes[@]}" "${lignes[@]}")
	"$SERMO" --program=DIALOGUE --do='"$SERMO_SCRIPT" --suite resultats' >/dev/null
}

suite() {
	# La sortie standard de sermo part vers /dev/null : écrire sur la sortie d'erreur.
	exec >&2
	case "$1" in
	principal)
		[ "$EXIT" = chercher ] || exit 0
		# Chemin complet : un dossier nommé « -delete » serait lu par find comme une action.
		case "$DOSSIER" in /*) ;; *) echo "Donnez le chemin complet du dossier (il commence par /)." >&2; exit 1 ;; esac
		[ -d "$DOSSIER" ] || { echo "Ce dossier n'existe pas : $DOSSIER" >&2; exit 1; }
		[ ${#NOM} -le 100 ] || { echo "Nom trop long (100 caractères au plus)." >&2; exit 1; }
		case "$NOM" in */*) echo "Le nom ne doit pas contenir de « / »." >&2; exit 1 ;; esac
		case "$GENRE" in Tous|Documents|Images|Musique|Vidéos|Archives) ;; *) GENRE=Tous ;; esac
		[[ $DEPUIS =~ ^[0-9]{4}-[0-9]{2}-[0-9]{2}$ ]] || AVEC_DATE=false
		chercher
		;;
	resultats)
		[ "$EXIT" = ouvrir ] || exit 0
		[ -n "$CHOIX" ] || { echo "Aucune ligne choisie." >&2; exit 1; }
		[ -d "$CHOIX" ] || { echo "Dossier introuvable : $CHOIX" >&2; exit 1; }
		# Ouvrir un dossier ne modifie rien : lancé directement, détaché.
		xdg-open "$CHOIX" >/dev/null 2>&1 &
		;;
	esac
}

main() {
	if [ "${1:-}" = --suite ]; then suite "$2"; exit; fi
	[ -n "${1:-}" ] && SERMO=$1
	local c
	for c in find numfmt xdg-open; do
		command -v "$c" >/dev/null || { echo "$c est absent : installez-le (paquets findutils, coreutils, xdg-utils)." >&2; exit 1; }
	done
	export DIALOGUE; DIALOGUE=$(dialogue_principal "$HOME")
	"$SERMO" --program=DIALOGUE --do='"$SERMO_SCRIPT" --suite principal' >/dev/null
}

[ "${BASH_SOURCE[0]}" = "$0" ] && main "$@"
