#!/bin/bash
# SPDX-License-Identifier: CC0-1.0
# Boîte    : Utilisateur
# Besoin   : trouver d'un coup d'œil les dix petits outils du quotidien, et en ouvrir un d'un clic
# Enrobe   : rien (ouvre les outils de ce dossier, un par un)
# Ports    : gtk3sermo conseillé ; qt6sermo ; htmlsermo
# Terminal : rien de perdu (mais la boîte vise un bureau graphique)
# Usage    : ./lanceur.sh [binaire sermo]
#            SERMO_A_BLANC=0 pour que les outils agissent pour de vrai (défaut : la commande est affichée)

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

# Une ligne d'outil : bouton à gauche, explication à droite.
outil() {
	printf '        <hbox><button width-request="210"><label>%s</label><action>exit:%s</action></button><text><label>%s</label></text></hbox>\n' \
		"$(xml "$2")" "$1" "$(xml "$3")"
}

dialogue_lanceur() {
	local mode=${1:-à blanc : rien ne sera modifié, les commandes seront seulement affichées}
	cat <<XML
<window title="Mes petits outils" default-width="640">
  <vbox>
    <text><label>Choisissez ce que vous voulez faire.</label></text>
    <notebook tab-labels="Mes fichiers|Tous les jours|Partager">
      <vbox>
$(outil 01-rechercher-fichiers "Retrouver un fichier" "par son nom, son genre, sa taille ou sa date")
$(outil 02-archives "Faire ou ouvrir un zip" "regrouper des fichiers, ou sortir ceux d'une archive")
$(outil 07-ranger-dossier "Ranger un dossier" "trier par genre ou par mois ; vieux fichiers à la corbeille")
$(outil 10-place-disque "Ce qui prend de la place" "les plus gros dossiers de votre dossier personnel")
      </vbox>
      <vbox>
$(outil 03-mot-de-passe "Inventer un mot de passe" "solide, affiché une fois, jamais enregistré")
$(outil 04-rappel "Me faire un rappel" "un message qui s'affichera à l'heure choisie")
$(outil 05-imprimer "Imprimer" "choisir l'imprimante, les copies, le recto verso")
$(outil 06-convertir-documents "Transformer en PDF" "un document texte (ODT, DOCX) en PDF")
      </vbox>
      <vbox>
$(outil 08-copie-cle-usb "Copier sur une clé USB" "un dossier vers la clé, sans rien effacer sur la clé")
$(outil 09-partager-sur-le-reseau "Prêter un fichier" "sur le réseau de la maison, pendant quelques minutes")
      </vbox>
    </notebook>
    <statusbar><variable>ETAT</variable><default>Mode $(xml "$mode")</default></statusbar>
    <hbox>
      <button cancel></button>
    </hbox>
  </vbox>
</window>
XML
}

suite() {
	# La sortie standard de sermo part vers /dev/null : écrire sur la sortie d'erreur.
	exec >&2
	case "$1" in
	lanceur)
		# Abandon (Cancel, abort, closewindow) : on sort.
		[[ $EXIT =~ ^[0-9][0-9]-[a-z0-9-]+$ ]] || exit 0
		local ICI
		ICI=$(CDPATH= cd -- "$(dirname -- "$0")" && pwd) || exit 1
		[ -f "$ICI/$EXIT.sh" ] || { echo "Outil introuvable : $EXIT" >&2; exit 1; }
		"$ICI/$EXIT.sh" "$SERMO"
		exec "$0" "$SERMO"
		;;
	esac
}

main() {
	if [ "${1:-}" = --suite ]; then suite "$2"; exit; fi
	[ -n "${1:-}" ] && SERMO=$1
	local mode="à blanc : rien ne sera modifié, les commandes seront seulement affichées"
	[ "$SERMO_A_BLANC" = 1 ] || mode="réel : les outils agissent pour de vrai"
	export DIALOGUE; DIALOGUE=$(dialogue_lanceur "$mode")
	"$SERMO" --program=DIALOGUE --do='"$SERMO_SCRIPT" --suite lanceur' >/dev/null
}

[ "${BASH_SOURCE[0]}" = "$0" ] && main "$@"
