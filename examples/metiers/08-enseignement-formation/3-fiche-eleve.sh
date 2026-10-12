#!/bin/bash
# SPDX-License-Identifier: CC0-1.0
# Métier   : Enseignement (vie scolaire, professeur principal)
# Besoin   : remplir la fiche de renseignements d'un élève (identité, responsables, scolarité) et la classer par classe
# Enrobe   : tee (fiche texte, une par élève, rangée dans le dossier de la classe)
# Ports    : gtk3sermo conseillé ; qt6sermo, fltk1sermo
# Terminal : rien de perdu (les onglets deviennent des pages, l'interrupteur une case)
# Usage    : ./3-fiche-eleve.sh [binaire sermo]
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

dialogue_principal() {
	local annee=${1:-2026-2027} dossier=${2:-$HOME/Documents/Eleves}
	cat <<XML
<window title="Fiche élève $(xml "$annee")" default-width="560">
  <vbox>
    <notebook tab-labels="Élève|Responsables|Scolarité">
      <vbox>
        <hbox><text><label>Nom :</label></text>
          <entry space-expand="true" space-fill="true"><variable>NOM</variable><default>Petit</default></entry></hbox>
        <hbox><text><label>Prénom :</label></text>
          <entry space-expand="true" space-fill="true"><variable>PRENOM</variable><default>Lucas</default></entry></hbox>
        <hbox><text><label>Né(e) le (JJ/MM/AAAA) :</label></text>
          <entry space-expand="true" space-fill="true"><variable>NAISSANCE</variable><default>14/03/2013</default></entry></hbox>
        <hbox><text><label>Classe :</label></text>
          <comboboxtext space-expand="true" space-fill="true">
            <variable>CLASSE</variable>
            <default>5e B</default>
            <item>6e A</item>
            <item>6e B</item>
            <item>5e A</item>
            <item>5e B</item>
            <item>4e A</item>
            <item>3e A</item>
          </comboboxtext></hbox>
        <hbox><text><label>Droit à l'image accordé :</label></text>
          <switch><variable>DROIT_IMAGE</variable><default>false</default></switch></hbox>
      </vbox>
      <vbox>
        <frame label="Responsable 1">
          <vbox>
            <hbox><text><label>Nom :</label></text>
              <entry space-expand="true" space-fill="true"><variable>RESP1</variable><default>Camille Petit (mère)</default></entry></hbox>
            <hbox><text><label>Téléphone :</label></text>
              <entry space-expand="true" space-fill="true"><variable>RESP1_TEL</variable><default>06 11 22 33 44</default></entry></hbox>
            <hbox><text><label>Courriel :</label></text>
              <entry space-expand="true" space-fill="true"><variable>RESP1_COURRIEL</variable><default>camille.petit@example.fr</default></entry></hbox>
          </vbox>
        </frame>
        <frame label="Responsable 2">
          <vbox>
            <hbox><text><label>Nom :</label></text>
              <entry space-expand="true" space-fill="true"><variable>RESP2</variable></entry></hbox>
            <hbox><text><label>Téléphone :</label></text>
              <entry space-expand="true" space-fill="true"><variable>RESP2_TEL</variable></entry></hbox>
          </vbox>
        </frame>
      </vbox>
      <vbox>
        <hbox><text><label>Régime :</label></text>
          <comboboxtext space-expand="true" space-fill="true">
            <variable>REGIME</variable>
            <default>Demi-pensionnaire</default>
            <item>Externe</item>
            <item>Demi-pensionnaire</item>
            <item>Interne</item>
          </comboboxtext></hbox>
        <checkbox><label>Projet d'accueil individualisé (PAI : allergie, traitement)</label><variable>PAI</variable></checkbox>
        <checkbox><label>Plan d'accompagnement personnalisé (PAP)</label><variable>PAP</variable></checkbox>
        <checkbox><label>Option latin</label><variable>LATIN</variable></checkbox>
        <text xalign="0"><label>Remarques (santé, transport, contacts d'urgence) :</label></text>
        <edit height-request="80" wrap-mode="word"><variable>REMARQUES</variable><default>Prend le car de 7 h 35.</default></edit>
      </vbox>
    </notebook>
    <hbox>
      <text><label>Dossier :</label></text>
      <entry space-expand="true" space-fill="true"><variable>DOSSIER</variable><default>$(xml "$dossier")</default></entry>
    </hbox>
    <hbox>
      <button cancel></button>
      <button><label>Enregistrer la fiche</label><action>exit:enregistrer</action></button>
    </hbox>
  </vbox>
</window>
XML
}

dialogue_ecraser() {
	local fichier=${1:-$HOME/Documents/Eleves/5e_B/PETIT_Lucas.txt}
	cat <<XML
<window title="Fiche déjà présente">
  <vbox>
    <text><label>La fiche $(xml "$fichier") existe déjà.</label></text>
    <text><label>La remplacer ? L'ancienne version sera perdue.</label></text>
    <hbox>
      <button cancel></button>
      <button><label>Remplacer la fiche</label><action>exit:remplacer</action></button>
    </hbox>
  </vbox>
</window>
XML
}

oui() { [ "$1" = true ] && echo oui || echo non; }

chemin_fiche() {
	local c=${CLASSE// /_} n="${NOM^^}_${PRENOM}"
	printf '%s/%s/%s.txt' "$DOSSIER" "${c//[^[:alnum:]_-]/_}" "${n//[^[:alnum:]_-]/_}"
}

fiche() {
	printf 'FICHE ÉLÈVE · %s %s · %s\n\n' "${NOM^^}" "$PRENOM" "$CLASSE"
	printf 'Né(e) le      : %s\n' "$NAISSANCE"
	printf "Droit à l'image : %s\n\n" "$(oui "$DROIT_IMAGE")"
	printf 'Responsable 1 : %s · %s · %s\n' "$RESP1" "$RESP1_TEL" "$RESP1_COURRIEL"
	[ -n "$RESP2" ] && printf 'Responsable 2 : %s · %s\n' "$RESP2" "$RESP2_TEL"
	printf '\nRégime        : %s\n' "$REGIME"
	printf 'PAI : %s · PAP : %s · Latin : %s\n' "$(oui "$PAI")" "$(oui "$PAP")" "$(oui "$LATIN")"
	printf '\nRemarques :\n%s\n\nFiche mise à jour le %s\n' "$REMARQUES" "$(date '+%d/%m/%Y')"
}

enregistrer() {
	local f
	f=$(chemin_fiche)
	[ -d "${f%/*}" ] || agir mkdir -p -- "${f%/*}"
	printf 'Fiche pour %s :\n' "$f"
	# À blanc, tee ne lit rien : la fiche est montrée ici. Pour de vrai, tee la recopie.
	[ "$SERMO_A_BLANC" = 1 ] && fiche
	fiche | agir tee -- "$f"
}

suite() {
	# La sortie standard de sermo part vers /dev/null (les lignes NOM=…) et
	# l'étape --do en hérite : écrire sur la sortie d'erreur, qui reste visible.
	exec >&2
	case "$1" in
	principal)
		[ "$EXIT" = enregistrer ] || exit 0
		[ -n "$NOM" ] && [ -n "$PRENOM" ] || { echo "Nom et prénom sont obligatoires." >&2; exit 1; }
		[[ "$NAISSANCE" =~ ^[0-9]{2}/[0-9]{2}/[0-9]{4}$ ]] || { echo "Date de naissance attendue au format JJ/MM/AAAA." >&2; exit 1; }
		f=$(chemin_fiche)
		if [ ! -e "$f" ]; then enregistrer; exit; fi
		export NOM PRENOM NAISSANCE CLASSE DROIT_IMAGE RESP1 RESP1_TEL RESP1_COURRIEL RESP2 RESP2_TEL REGIME PAI PAP LATIN REMARQUES DOSSIER
		export DIALOGUE; DIALOGUE=$(dialogue_ecraser "$f")
		"$SERMO" --program=DIALOGUE --do='"$SERMO_SCRIPT" --suite ecraser' >/dev/null
		;;
	ecraser)
		[ "$EXIT" = remplacer ] || exit 0
		enregistrer
		;;
	esac
}

main() {
	if [ "${1:-}" = --suite ]; then suite "$2"; exit; fi
	[ -n "${1:-}" ] && SERMO=$1
	command -v tee >/dev/null || { echo "tee est absent : installez le paquet coreutils." >&2; exit 1; }
	local a m annee
	a=$(date +%Y); m=$(date +%-m)
	if [ "$m" -ge 8 ]; then annee="$a-$((a + 1))"; else annee="$((a - 1))-$a"; fi
	export DIALOGUE; DIALOGUE=$(dialogue_principal "$annee" "$HOME/Documents/Eleves/$annee")
	"$SERMO" --program=DIALOGUE --do='"$SERMO_SCRIPT" --suite principal' >/dev/null
}

[ "${BASH_SOURCE[0]}" = "$0" ] && main "$@"
