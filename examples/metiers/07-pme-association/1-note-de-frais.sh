#!/bin/bash
# SPDX-License-Identifier: CC0-1.0
# Métier   : PME, association (trésorerie)
# Besoin   : saisir une dépense avancée (date, montant, catégorie) dans le registre CSV des notes de frais
# Enrobe   : tee -a (registre CSV, séparateur « ; », ouvrable dans un tableur)
# Ports    : gtk3sermo conseillé ; qt6sermo, fltk1sermo ; ncursessermo par SSH
# Terminal : rien de perdu (le calendrier devient une saisie de date)
# Usage    : ./1-note-de-frais.sh [binaire sermo]
#            SERMO_A_BLANC=0 pour agir pour de vrai (défaut : la commande est affichée)

export SERMO="${SERMO:-sermo}"
export SERMO_A_BLANC="${SERMO_A_BLANC:-1}"
# Le script lui-même, pour --do : le shell de sermo lit la variable, sans
# citation à refaire (printf %q écrit $'…' en locale C, que dash lit de travers).
export SERMO_SCRIPT="$0"

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

# Une ligne CSV : champs entre guillemets (guillemets doublés), séparés par « ; ».
ligne_csv() {
	local sep="" c
	for c in "$@"; do c=${c//$'\n'/ }; printf '%s"%s"' "$sep" "${c//\"/\"\"}"; sep=";"; done
	printf '\n'
}

dialogue_principal() {
	local date=${1:-2026-10-12} nom=${2-Claire Martin} fichier=${3:-$HOME/Documents/notes-de-frais.csv}
	cat <<XML
<window title="Note de frais" default-width="600">
  <vbox>
    <hbox>
      <frame label="Date de la dépense">
        <vbox>
          <calendar><variable>DATE</variable><default>$(xml "$date")</default></calendar>
        </vbox>
      </frame>
      <frame label="Dépense">
        <vbox>
          <text xalign="0"><label>Qui a payé :</label></text>
          <entry><variable>NOM</variable><default>$(xml "$nom")</default></entry>
          <text xalign="0"><label>Objet :</label></text>
          <entry><variable>OBJET</variable><default>Train Lyon-Paris, réunion fédérale</default></entry>
          <text xalign="0"><label>Catégorie :</label></text>
          <comboboxtext>
            <variable>CATEGORIE</variable>
            <default>Déplacement</default>
            <item>Déplacement</item>
            <item>Repas</item>
            <item>Hébergement</item>
            <item>Fournitures</item>
            <item>Affranchissement</item>
            <item>Autre</item>
          </comboboxtext>
          <text xalign="0"><label>Montant TTC (euros) :</label></text>
          <spinbutton range-min="0" range-max="10000" range-step="0.5" digits="2">
            <variable>MONTANT</variable><default>48.50</default>
          </spinbutton>
        </vbox>
      </frame>
    </hbox>
    <frame label="Paiement">
      <vbox>
        <radiobutton><label>Avance personnelle, à rembourser</label><variable>PAYE_PERSO</variable><default>true</default></radiobutton>
        <radiobutton><label>Carte de la structure</label><variable>PAYE_CARTE</variable></radiobutton>
        <radiobutton><label>Espèces de la caisse</label><variable>PAYE_CAISSE</variable></radiobutton>
        <checkbox><label>Justificatif (ticket, facture) remis au trésorier</label><variable>JUSTIFICATIF</variable><default>true</default></checkbox>
      </vbox>
    </frame>
    <hbox>
      <text><label>Registre :</label></text>
      <entry space-expand="true" space-fill="true"><variable>FICHIER</variable><default>$(xml "$fichier")</default></entry>
    </hbox>
    <hbox>
      <button cancel></button>
      <button><label>Enregistrer la dépense</label><action>exit:enregistrer</action></button>
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
		[ "$EXIT" = enregistrer ] || exit 0
		[ -n "$OBJET" ] || { echo "Objet vide : rien n'est enregistré." >&2; exit 1; }
		case "$MONTANT" in ''|0|0.0|0.00) echo "Montant nul : rien n'est enregistré." >&2; exit 1 ;; esac
		paiement="avance personnelle"
		[ "$PAYE_CARTE" = true ] && paiement="carte de la structure"
		[ "$PAYE_CAISSE" = true ] && paiement="caisse"
		justif=non; [ "$JUSTIFICATIF" = true ] && justif=oui
		# Le tableur français attend une virgule décimale.
		contenu=""
		[ -s "$FICHIER" ] || contenu=$(ligne_csv Date Nom Objet Catégorie "Montant TTC" Paiement Justificatif)$'\n'
		contenu+=$(ligne_csv "$DATE" "$NOM" "$OBJET" "$CATEGORIE" "${MONTANT/./,}" "$paiement" "$justif")
		dossier=$(dirname -- "$FICHIER")
		[ -d "$dossier" ] || agir mkdir -p -- "$dossier"
		printf 'Ajout au registre %s :\n' "$FICHIER"
		# À blanc, tee ne lit rien : les lignes sont montrées ici. Pour de vrai, tee les recopie.
		[ "$SERMO_A_BLANC" = 1 ] && printf '%s\n' "$contenu"
		printf '%s\n' "$contenu" | agir tee -a -- "$FICHIER"
		;;
	esac
}

main() {
	if [ "${1:-}" = --suite ]; then suite "$2"; exit; fi
	[ -n "${1:-}" ] && SERMO=$1
	command -v tee >/dev/null || { echo "tee est absent : installez le paquet coreutils." >&2; exit 1; }
	export DIALOGUE; DIALOGUE=$(dialogue_principal "$(date +%F)" "$(getent passwd "$USER" | cut -d: -f5 | cut -d, -f1)")
	"$SERMO" --program=DIALOGUE --do='"$SERMO_SCRIPT" --suite principal' >/dev/null
}

[ "${BASH_SOURCE[0]}" = "$0" ] && main "$@"
