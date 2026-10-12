#!/bin/bash
# SPDX-License-Identifier: CC0-1.0
# Métier   : Association (secrétariat)
# Besoin   : inscrire un adhérent (identité, formule, règlement, consentements) dans le registre de la saison
# Enrobe   : grep (doublon, lecture seule) ; tee -a (registre CSV)
# Ports    : gtk3sermo conseillé ; qt6sermo ; htmlsermo pour une permanence sur tablette
# Terminal : rien de perdu (les onglets deviennent des pages)
# Usage    : ./2-adherent.sh [binaire sermo]
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

# Un texte sur plusieurs lignes s'écrit entre guillemets dans <default>,
# sinon sermo ne garde que la dernière ligne.
dialogue_principal() {
	local saison=${1:-2026-2027} registre=${2:-$HOME/Documents/adherents-2026-2027.csv}
	cat <<XML
<window title="Inscription d'un adhérent" default-width="560">
  <vbox>
    <notebook tab-labels="Identité|Adhésion|Consentements">
      <vbox>
        <hbox><text><label>Nom :</label></text>
          <entry space-expand="true" space-fill="true"><variable>NOM</variable><default>Durand</default></entry></hbox>
        <hbox><text><label>Prénom :</label></text>
          <entry space-expand="true" space-fill="true"><variable>PRENOM</variable><default>Sophie</default></entry></hbox>
        <text xalign="0"><label>Adresse :</label></text>
        <edit height-request="60"><variable>ADRESSE</variable><default>"12 rue des Lilas
26000 Valence"</default></edit>
        <hbox><text><label>Courriel :</label></text>
          <entry space-expand="true" space-fill="true"><variable>COURRIEL</variable><default>sophie.durand@example.org</default></entry></hbox>
        <hbox><text><label>Téléphone :</label></text>
          <entry space-expand="true" space-fill="true"><variable>TELEPHONE</variable><default>06 12 34 56 78</default></entry></hbox>
      </vbox>
      <vbox>
        <frame label="Formule">
          <vbox>
            <radiobutton><label>Individuelle (20 €)</label><variable>F_INDIVIDUELLE</variable><default>true</default></radiobutton>
            <radiobutton><label>Famille (35 €)</label><variable>F_FAMILLE</variable></radiobutton>
            <radiobutton><label>Tarif réduit, étudiant ou sans emploi (10 €)</label><variable>F_REDUITE</variable></radiobutton>
          </vbox>
        </frame>
        <hbox><text><label>Don en plus de la cotisation (€) :</label></text>
          <spinbutton range-min="0" range-max="1000" range-step="5"><variable>DON</variable><default>0</default></spinbutton></hbox>
        <hbox><text><label>Règlement :</label></text>
          <comboboxtext space-expand="true" space-fill="true">
            <variable>REGLEMENT</variable>
            <default>Chèque</default>
            <item>Chèque</item>
            <item>Espèces</item>
            <item>Virement</item>
            <item>Paiement en ligne</item>
          </comboboxtext></hbox>
        <hbox><text><label>Saison :</label></text>
          <entry space-expand="true" space-fill="true"><variable>SAISON</variable><default>$(xml "$saison")</default></entry></hbox>
      </vbox>
      <vbox>
        <checkbox><label>J'accepte que mes données servent à la gestion des adhésions (obligatoire)</label><variable>RGPD</variable></checkbox>
        <checkbox><label>Je souhaite recevoir la lettre d'information</label><variable>LETTRE</variable><default>true</default></checkbox>
        <checkbox><label>J'autorise la publication de photos où j'apparais</label><variable>DROIT_IMAGE</variable></checkbox>
        <checkbox><label>Je souhaite un reçu fiscal pour mon don</label><variable>RECU</variable></checkbox>
      </vbox>
    </notebook>
    <hbox>
      <text><label>Registre :</label></text>
      <entry space-expand="true" space-fill="true"><variable>REGISTRE</variable><default>$(xml "$registre")</default></entry>
    </hbox>
    <hbox>
      <button cancel></button>
      <button><label>Inscrire</label><action>exit:inscrire</action></button>
    </hbox>
  </vbox>
</window>
XML
}

# Le courriel figure déjà au registre : on le montre avant d'ajouter une seconde ligne.
dialogue_doublon() {
	local courriel=${1:-sophie.durand@example.org} trouve=${2:-2026-09-14 · 2026-2027 · Durand · Sophie}
	cat <<XML
<window title="Adhérent déjà inscrit ?">
  <vbox>
    <text><label>Le courriel $(xml "$courriel") figure déjà au registre :</label></text>
    <text><label>$(xml "$trouve")</label></text>
    <text><label>Ajouter quand même une nouvelle ligne ?</label></text>
    <hbox>
      <button cancel></button>
      <button><label>Ajouter quand même</label><action>exit:ajouter</action></button>
    </hbox>
  </vbox>
</window>
XML
}

oui() { [ "$1" = true ] && echo oui || echo non; }

# Écrit la ligne au registre (avec l'en-tête si le registre est neuf).
inscrire() {
	local montant=20 formule=individuelle contenu dossier
	[ "$F_FAMILLE" = true ] && { montant=35; formule=famille; }
	[ "$F_REDUITE" = true ] && { montant=10; formule=réduite; }
	contenu=""
	[ -s "$REGISTRE" ] || contenu=$(ligne_csv "Date d'inscription" Saison Nom Prénom Adresse Courriel Téléphone Formule Cotisation Don Règlement "Lettre d'information" "Droit à l'image" "Reçu fiscal")$'\n'
	contenu+=$(ligne_csv "$(date +%F)" "$SAISON" "$NOM" "$PRENOM" "$ADRESSE" "$COURRIEL" "$TELEPHONE" \
		"$formule" "$montant" "$DON" "$REGLEMENT" "$(oui "$LETTRE")" "$(oui "$DROIT_IMAGE")" "$(oui "$RECU")")
	dossier=$(dirname -- "$REGISTRE")
	[ -d "$dossier" ] || agir mkdir -p -- "$dossier"
	printf 'Ajout au registre %s :\n' "$REGISTRE"
	# À blanc, tee ne lit rien : les lignes sont montrées ici. Pour de vrai, tee les recopie.
	[ "$SERMO_A_BLANC" = 1 ] && printf '%s\n' "$contenu"
	printf '%s\n' "$contenu" | agir tee -a -- "$REGISTRE"
	echo "À encaisser : $((montant + ${DON%.*})) € ($REGLEMENT)."
}

suite() {
	# La sortie standard de sermo part vers /dev/null (les lignes NOM=…) et
	# l'étape --do en hérite : écrire sur la sortie d'erreur, qui reste visible.
	exec >&2
	case "$1" in
	principal)
		[ "$EXIT" = inscrire ] || exit 0
		[ -n "$NOM" ] && [ -n "$PRENOM" ] || { echo "Nom et prénom sont obligatoires." >&2; exit 1; }
		[ "$RGPD" = true ] || { echo "Sans l'accord sur les données, l'inscription n'est pas enregistrée." >&2; exit 1; }
		trouve=""
		if [ -n "$COURRIEL" ] && [ -r "$REGISTRE" ]; then
			trouve=$(grep -iF -m1 -- "\"$COURRIEL\"" "$REGISTRE" | cut -d';' -f1-4)
			trouve=${trouve//\"/}; trouve=${trouve//;/ · }
		fi
		if [ -z "$trouve" ]; then inscrire; exit; fi
		export NOM PRENOM ADRESSE COURRIEL TELEPHONE F_FAMILLE F_REDUITE DON REGLEMENT SAISON LETTRE DROIT_IMAGE RECU REGISTRE
		export DIALOGUE; DIALOGUE=$(dialogue_doublon "$COURRIEL" "$trouve")
		"$SERMO" --program=DIALOGUE --do='"$SERMO_SCRIPT" --suite doublon' >/dev/null
		;;
	doublon)
		[ "$EXIT" = ajouter ] || exit 0
		inscrire
		;;
	esac
}

main() {
	if [ "${1:-}" = --suite ]; then suite "$2"; exit; fi
	[ -n "${1:-}" ] && SERMO=$1
	for c in grep tee; do
		command -v "$c" >/dev/null || { echo "$c est absent : installez-le d'abord." >&2; exit 1; }
	done
	# Saison de septembre à août.
	local a m saison
	a=$(date +%Y); m=$(date +%-m)
	if [ "$m" -ge 9 ]; then saison="$a-$((a + 1))"; else saison="$((a - 1))-$a"; fi
	export DIALOGUE; DIALOGUE=$(dialogue_principal "$saison" "$HOME/Documents/adherents-$saison.csv")
	"$SERMO" --program=DIALOGUE --do='"$SERMO_SCRIPT" --suite principal' >/dev/null
}

[ "${BASH_SOURCE[0]}" = "$0" ] && main "$@"
