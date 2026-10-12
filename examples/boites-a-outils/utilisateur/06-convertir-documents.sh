#!/bin/bash
# SPDX-License-Identifier: CC0-1.0
# Boîte    : Utilisateur
# Besoin   : transformer un document texte (ODT, DOCX, DOC, RTF) en PDF pour l'envoyer ou l'imprimer
# Enrobe   : libreoffice --headless --convert-to pdf
# Ports    : gtk3sermo conseillé ; qt6sermo ; htmlsermo
# Terminal : rien de perdu (LibreOffice travaille sans fenêtre)
# Usage    : ./06-convertir-documents.sh [binaire sermo] [dossier]
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

# Arguments : dossier, puis une ligne par document « nom|taille|changé le ».
dialogue_principal() {
	[ $# -gt 0 ] || set -- /home/utilisateur/Documents \
		"lettre-proprietaire.odt|24 Ko|2026-09-28" "cv-2026.docx|61 Ko|2026-09-02" \
		"compte-rendu-reunion.odt|18 Ko|2026-08-30" "recette-tarte.rtf|5 Ko|2026-07-14"
	local dossier=$1 l
	shift
	cat <<XML
<window title="Transformer en PDF" default-width="600" default-height="440">
  <vbox>
    <text><label>Documents trouvés dans $(xml "$dossier") :</label></text>
    <table scrollable="true" space-expand="true" space-fill="true">
      <label>Nom|Taille|Changé le</label>
      <variable>DOCUMENT</variable>
XML
	for l in "$@"; do printf '      <item>%s</item>\n' "$(xml "$l")"; done
	cat <<XML
    </table>
    <frame label="Que transformer ?">
      <vbox>
        <radiobutton><label>Seulement le document choisi dans la liste</label><variable>UN</variable><default>true</default></radiobutton>
        <radiobutton><label>Tous les documents de la liste</label><variable>TOUS</variable></radiobutton>
      </vbox>
    </frame>
    <frame label="Où poser le PDF">
      <vbox>
        <comboboxtext><variable>OU</variable><default>À côté du document</default>
          <item>À côté du document</item><item>Dans un sous-dossier « PDF »</item>
        </comboboxtext>
        <checkbox><label>Ouvrir le PDF à la fin (un seul document)</label><variable>OUVRIR</variable></checkbox>
      </vbox>
    </frame>
    <hbox>
      <button cancel></button>
      <button><label>Transformer</label><action>exit:convertir</action></button>
    </hbox>
  </vbox>
</window>
XML
}

dialogue_remplacer() {
	local liste=${1:-cv-2026.pdf, lettre-proprietaire.pdf}
	cat <<XML
<window title="Remplacer les PDF ?">
  <vbox>
    <text><label>Ces PDF existent déjà et seront remplacés :</label></text>
    <text><label>$(xml "$liste")</label></text>
    <text><label>Les anciens seront d'abord mis à la corbeille.</label></text>
    <hbox>
      <button cancel></button>
      <button><label>Remplacer</label><action>exit:remplacer</action></button>
    </hbox>
  </vbox>
</window>
XML
}

# Les documents à transformer (chemins complets), un par ligne.
choisis() {
	local l
	if [ "$TOUS" = true ]; then
		for l in "$DOSSIER"/*; do
			[ -f "$l" ] || continue
			case "${l,,}" in *.odt|*.docx|*.doc|*.rtf) printf '%s\n' "$l" ;; esac
		done
	elif [ -n "$DOCUMENT" ]; then
		case "$DOCUMENT" in */*) return ;; esac
		[ -f "$DOSSIER/$DOCUMENT" ] && printf '%s\n' "$DOSSIER/$DOCUMENT"
	fi
}

convertir() {
	local docs sortie=$DOSSIER d n
	mapfile -t docs < <(choisis)
	[ "$OU" = "Dans un sous-dossier « PDF »" ] && { sortie=$DOSSIER/PDF; [ -d "$sortie" ] || agir mkdir -- "$sortie"; }
	if [ "$1" = remplacer ]; then
		for d in "${docs[@]}"; do
			n=$(basename -- "$d"); n=$sortie/${n%.*}.pdf
			[ -e "$n" ] && agir gio trash -- "$n"
		done
	fi
	agir libreoffice --headless --convert-to pdf --outdir "$sortie" "${docs[@]}"
	if [ "$OUVRIR" = true ] && [ ${#docs[@]} = 1 ] && [ "$SERMO_A_BLANC" != 1 ]; then
		n=$(basename -- "${docs[0]}"); xdg-open "$sortie/${n%.*}.pdf" >/dev/null 2>&1 &
	fi
}

suite() {
	# La sortie standard de sermo part vers /dev/null : écrire sur la sortie d'erreur.
	exec >&2
	case "$1" in
	principal)
		[ "$EXIT" = convertir ] || exit 0
		[ -d "$DOSSIER" ] || { echo "Dossier introuvable : $DOSSIER" >&2; exit 1; }
		case "$OU" in "À côté du document"|"Dans un sous-dossier « PDF »") ;; *) OU="À côté du document" ;; esac
		local docs d n sortie=$DOSSIER deja=()
		mapfile -t docs < <(choisis)
		[ ${#docs[@]} -gt 0 ] || { echo "Aucun document choisi." >&2; exit 1; }
		[ "$OU" = "Dans un sous-dossier « PDF »" ] && sortie=$DOSSIER/PDF
		for d in "${docs[@]}"; do
			n=$(basename -- "$d"); n=${n%.*}.pdf
			[ -e "$sortie/$n" ] && deja+=("$n")
		done
		export DOSSIER DOCUMENT TOUS OU OUVRIR
		if [ ${#deja[@]} -gt 0 ]; then
			local IFS=,
			export DIALOGUE; DIALOGUE=$(dialogue_remplacer "${deja[*]}")
			"$SERMO" --program=DIALOGUE --do='"$SERMO_SCRIPT" --suite remplacer' >/dev/null
		else
			convertir neuf
		fi
		;;
	remplacer)
		[ "$EXIT" = remplacer ] || exit 0
		convertir remplacer
		;;
	esac
}

main() {
	if [ "${1:-}" = --suite ]; then suite "$2"; exit; fi
	[ -n "${1:-}" ] && SERMO=$1
	command -v libreoffice >/dev/null || { echo "libreoffice est absent : il faut le paquet libreoffice-writer." >&2; exit 1; }
	command -v gio >/dev/null || { echo "gio est absent : il faut le paquet libglib2.0-bin." >&2; exit 1; }
	export DOSSIER=${2:-$HOME/Documents}
	DOSSIER=${DOSSIER%/}
	# Chemin complet : un nom qui commencerait par « - » ne passe jamais pour une option.
	case "$DOSSIER" in /*) ;; *) DOSSIER=$PWD/$DOSSIER ;; esac
	[ -d "$DOSSIER" ] || { echo "Dossier introuvable : $DOSSIER" >&2; exit 1; }
	local lignes=() nom taille date
	while IFS=$'\t' read -r nom taille date; do
		taille=$(numfmt --to=iec --suffix=o -- "$taille" 2>/dev/null || echo "$taille")
		lignes+=("${nom//|//}|$taille|$date")
	done < <(find "$DOSSIER" -maxdepth 1 -type f \( -iname '*.odt' -o -iname '*.docx' -o -iname '*.doc' -o -iname '*.rtf' \) \
		-printf '%f\t%s\t%TY-%Tm-%Td\n' 2>/dev/null | sort | head -n 200)
	[ ${#lignes[@]} -gt 0 ] || { echo "Aucun document ODT, DOCX, DOC ou RTF dans $DOSSIER." >&2; exit 1; }
	export DIALOGUE; DIALOGUE=$(dialogue_principal "$DOSSIER" "${lignes[@]}")
	"$SERMO" --program=DIALOGUE --do='"$SERMO_SCRIPT" --suite principal' >/dev/null
}

[ "${BASH_SOURCE[0]}" = "$0" ] && main "$@"
