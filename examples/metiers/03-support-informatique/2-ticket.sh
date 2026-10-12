#!/bin/bash
# SPDX-License-Identifier: CC0-1.0
# Métier   : Support informatique
# Besoin   : saisir un ticket d'incident (catégorie, urgence, description) et l'enregistrer ou l'envoyer
# Enrobe   : install (fichier texte), xdg-email
# Ports    : gtk3sermo conseillé ; ncursessermo par SSH
# Terminal : xdg-email ouvre un logiciel de courriel graphique : choisir le fichier en SSH
# Usage    : ./2-ticket.sh [binaire sermo]
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

# Arguments : nom du poste, date du jour (AAAA-MM-JJ).
dialogue_principal() {
	local poste=${1:-poste-accueil} jour=${2:-2026-10-11}
	cat <<XML
<window title="Nouveau ticket" default-width="540" default-height="520">
  <vbox>
    <notebook tab-labels="Demande|Description|Envoi" space-expand="true" space-fill="true">
      <vbox>
        <hbox><text><label>Votre nom :</label></text>
          <entry space-expand="true" space-fill="true"><variable>NOM</variable></entry></hbox>
        <hbox><text><label>Poste concerné :</label></text>
          <entry space-expand="true" space-fill="true"><variable>POSTE</variable><default>$(xml "$poste")</default></entry></hbox>
        <hbox><text><label>Catégorie :</label></text>
          <comboboxtext space-expand="true" space-fill="true">
            <variable>CATEGORIE</variable>
            <default>Logiciel</default>
            <item>Matériel</item>
            <item>Logiciel</item>
            <item>Réseau et Internet</item>
            <item>Compte et mot de passe</item>
            <item>Impression</item>
            <item>Autre</item>
          </comboboxtext></hbox>
        <hbox><text><label>Objet :</label></text>
          <entry space-expand="true" space-fill="true"><variable>OBJET</variable></entry></hbox>
        <frame label="Urgence">
          <vbox>
            <radiobutton><label>Faible : je peux attendre</label><variable>U_FAIBLE</variable></radiobutton>
            <radiobutton><label>Normale : gênant, contournable</label><default>true</default><variable>U_NORMALE</variable></radiobutton>
            <radiobutton><label>Haute : je ne peux plus travailler correctement</label><variable>U_HAUTE</variable></radiobutton>
            <radiobutton><label>Bloquante : plusieurs personnes arrêtées</label><variable>U_BLOQUANTE</variable></radiobutton>
          </vbox>
        </frame>
      </vbox>
      <vbox>
        <frame label="Ce que vous faisiez, ce qui s'est passé, le message affiché">
          <edit space-expand="true" space-fill="true"><variable>DESCRIPTION</variable></edit>
        </frame>
        <text><label>Le problème a commencé le :</label></text>
        <calendar><variable>DEPUIS</variable><default>$(xml "$jour")</default></calendar>
      </vbox>
      <vbox>
        <radiobutton><label>Enregistrer le ticket dans un fichier</label><default>true</default><variable>D_FICHIER</variable></radiobutton>
        <radiobutton><label>Préparer un courriel au support (xdg-email)</label><variable>D_COURRIEL</variable></radiobutton>
        <hbox><text><label>Dossier des tickets :</label></text>
          <entry space-expand="true" space-fill="true"><variable>DOSSIER</variable><default>~/tickets</default></entry></hbox>
        <hbox><text><label>Adresse du support :</label></text>
          <entry space-expand="true" space-fill="true"><variable>ADRESSE</variable><default>support@exemple.lan</default></entry></hbox>
      </vbox>
    </notebook>
    <hbox space-expand="false" space-fill="false">
      <button cancel></button>
      <button><label>Créer le ticket</label><action>exit:creer</action></button>
    </hbox>
  </vbox>
</window>
XML
}

urgence() {
	if [ "$U_BLOQUANTE" = true ]; then echo bloquante
	elif [ "$U_HAUTE" = true ]; then echo haute
	elif [ "$U_FAIBLE" = true ]; then echo faible
	else echo normale
	fi
}

texte() {
	cat <<EOF
Objet     : $OBJET
Catégorie : $CATEGORIE
Urgence   : $(urgence)
Demandeur : $NOM
Poste     : $POSTE
Depuis le : $DEPUIS
Créé le   : $(date '+%Y-%m-%d %H:%M')

$DESCRIPTION
EOF
}

suite() {
	exec >&2
	case "$1" in
	principal)
		[ "$EXIT" = creer ] || exit 0
		[ -n "${OBJET// /}" ] || { echo "L'objet est vide." >&2; exit 1; }
		[ -n "${DESCRIPTION//[[:space:]]/}" ] || { echo "La description est vide." >&2; exit 1; }
		local t; t=$(texte)
		if [ "$D_COURRIEL" = true ]; then
			command -v xdg-email >/dev/null || { echo "xdg-email est absent : installez xdg-utils, ou choisissez le fichier." >&2; exit 1; }
			[[ "$ADRESSE" =~ ^[A-Za-z0-9._%+][A-Za-z0-9._%+-]*@[A-Za-z0-9.-]+$ ]] || { echo "Adresse refusée : $ADRESSE" >&2; exit 1; }
			agir xdg-email --utf8 --subject "[$(urgence)] $CATEGORIE : $OBJET" --body "$t" "$ADRESSE"
		else
			local dossier=${DOSSIER/#\~/$HOME}
			[ -n "$dossier" ] || { echo "Dossier vide." >&2; exit 1; }
			local f; f="$dossier/ticket-$(date +%Y%m%d-%H%M%S).txt"
			agir mkdir -p -- "$dossier" || exit 1
			agir install -m 600 /dev/stdin "$f" <<<"$t"
			[ "$SERMO_A_BLANC" = 1 ] && sed 's/^/    | /' <<<"$t"
		fi
		;;
	esac
}

main() {
	if [ "${1:-}" = --suite ]; then suite "$2"; exit; fi
	[ -n "${1:-}" ] && SERMO=$1
	command -v install >/dev/null || { echo "install est absent : installez coreutils." >&2; exit 1; }
	export DIALOGUE; DIALOGUE=$(dialogue_principal "$(hostname)" "$(date +%F)")
	"$SERMO" --program=DIALOGUE --do='"$SERMO_SCRIPT" --suite principal' >/dev/null
}

[ "${BASH_SOURCE[0]}" = "$0" ] && main "$@"
