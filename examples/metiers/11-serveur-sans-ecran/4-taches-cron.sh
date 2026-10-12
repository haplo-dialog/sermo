#!/bin/bash
# SPDX-License-Identifier: CC0-1.0
# Métier   : Serveur sans écran
# Besoin   : relire ses tâches planifiées et en ajouter une sans écrire la ligne cron à la main
# Enrobe   : crontab -l (lecture) ; crontab - pour installer la table complétée
# Ports    : ncursessermo conseillé (par SSH) ; gtk3sermo sur un poste
# Terminal : rien de perdu
# Usage    : ./4-taches-cron.sh [binaire sermo]
#            SERMO_A_BLANC=0 pour agir pour de vrai (défaut : la commande est affichée)
#            Agit sur la table de l'utilisateur qui lance le script.

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
	# La table (plusieurs lignes) entre par <default>, échappée par xml : jamais
	# collée dans une commande <input>, où une tâche deviendrait du shell.
	cat <<XML
<window title="Tâches planifiées" default-width="580">
  <vbox>
    <frame label="Table actuelle (crontab -l)">
      <edit editable="false" height-request="120"><variable>TABLE</variable><default>$(xml "${TEXTE_TABLE:-}")</default></edit>
    </frame>
    <frame label="Nouvelle tâche">
      <vbox>
        <hbox><text><label>Commande :</label></text>
          <entry space-expand="true" space-fill="true"><variable>COMMANDE</variable><default>/usr/local/bin/rotation-certificats</default></entry></hbox>
        <hbox><text><label>Description :</label></text>
          <entry space-expand="true" space-fill="true"><variable>DESCRIPTION</variable><default>Renouvellement des certificats</default></entry></hbox>
        <hbox><text><label>Fréquence :</label></text>
          <comboboxtext><variable>FREQUENCE</variable><default>chaque jour</default>
            <item>chaque heure</item>
            <item>chaque jour</item>
            <item>chaque lundi</item>
            <item>le 1er du mois</item>
          </comboboxtext></hbox>
        <hbox><text><label>Heure :</label></text>
          <spinbutton range-min="0" range-max="23" range-step="1"><variable>HEURE</variable><default>4</default></spinbutton>
          <text><label>Minute :</label></text>
          <spinbutton range-min="0" range-max="59" range-step="5"><variable>MINUTE</variable><default>30</default></spinbutton></hbox>
        <checkbox><label>Jeter la sortie (sinon cron l'envoie par courriel)</label><variable>SILENCE</variable><default>true</default></checkbox>
      </vbox>
    </frame>
    <hbox>
      <button cancel></button>
      <button><label>Ajouter</label><action>exit:ajouter</action></button>
    </hbox>
  </vbox>
</window>
XML
}

dialogue_confirmation() {
	local ligne=${1:-30 4 * * * /usr/local/bin/rotation-certificats} silence=${2:-true}
	cat <<XML
<window title="Confirmer la tâche">
  <vbox>
    <text><label>Cette ligne sera ajoutée à votre table cron :</label></text>
    <entry editable="false" width-request="520"><variable>LIGNE</variable><default>$(xml "$ligne")</default></entry>
    <checkbox sensitive="false"><label>Sortie et erreurs jetées (vers /dev/null)</label><variable>SORTIE_JETEE</variable><default>$(xml "$silence")</default></checkbox>
    <text><label>La table entière est réinstallée : relisez-la avant.</label></text>
    <hbox>
      <button cancel></button>
      <button><label>Installer</label><action>exit:installer</action></button>
    </hbox>
  </vbox>
</window>
XML
}

# La ligne cron, construite à partir des choix (sans la redirection si $1 = court).
ligne() {
	local h=$HEURE m=$MINUTE quand
	case "$FREQUENCE" in
	"chaque heure")   quand="$m * * * *" ;;
	"chaque lundi")   quand="$m $h * * 1" ;;
	"le 1er du mois") quand="$m $h 1 * *" ;;
	*)                quand="$m $h * * *" ;;
	esac
	if [ "$SILENCE" = true ] && [ "${1:-}" != court ]; then echo "$quand $COMMANDE >/dev/null 2>&1"
	else echo "$quand $COMMANDE"; fi
}

suite() {
	# La sortie standard de sermo part vers /dev/null : écrire sur la sortie d'erreur.
	exec >&2
	case "$1" in
	principal)
		[ "$EXIT" = ajouter ] || exit 0
		[[ $HEURE =~ ^[0-9]+$ && $HEURE -le 23 ]] || { echo "Heure refusée : $HEURE" >&2; exit 1; }
		[[ $MINUTE =~ ^[0-9]+$ && $MINUTE -le 59 ]] || { echo "Minute refusée : $MINUTE" >&2; exit 1; }
		[ -n "$COMMANDE" ] || { echo "La commande est vide." >&2; exit 1; }
		# Une ligne cron tient sur une ligne ; « % » y signifie « fin de ligne ».
		[[ $COMMANDE != *$'\n'* && $COMMANDE != *%* ]] || { echo "La commande ne doit contenir ni saut de ligne ni « % »." >&2; exit 1; }
		DESCRIPTION=${DESCRIPTION//$'\n'/ }
		export COMMANDE DESCRIPTION FREQUENCE HEURE MINUTE SILENCE
		export DIALOGUE; DIALOGUE=$(dialogue_confirmation "$(ligne court)" "$SILENCE")
		"$SERMO" --program=DIALOGUE --do='"$SERMO_SCRIPT" --suite confirmation' >/dev/null
		;;
	confirmation)
		[ "$EXIT" = installer ] || exit 0
		# Ancienne table + commentaire + nouvelle ligne, installée d'un bloc.
		agir sh -c '{ crontab -l 2>/dev/null; printf "# %s\n%s\n" "$1" "$2"; } | crontab -' \
			cron "$DESCRIPTION" "$(ligne)"
		;;
	esac
}

main() {
	if [ "${1:-}" = --suite ]; then suite "$2"; exit; fi
	[ -n "${1:-}" ] && SERMO=$1
	command -v crontab >/dev/null || { echo "crontab est absent : installez le paquet cron." >&2; exit 1; }
	table=$(crontab -l 2>/dev/null)
	export TEXTE_TABLE=${table:-(aucune tâche planifiée)}
	export DIALOGUE; DIALOGUE=$(dialogue_principal)
	"$SERMO" --program=DIALOGUE --do='"$SERMO_SCRIPT" --suite principal' >/dev/null
}

[ "${BASH_SOURCE[0]}" = "$0" ] && main "$@"
