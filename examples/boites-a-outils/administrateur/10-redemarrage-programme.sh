#!/bin/bash
# SPDX-License-Identifier: CC0-1.0
# Boîte    : Administrateur
# Besoin   : programmer un redémarrage ou un arrêt à une heure donnée (ou dans N minutes), prévenir les personnes connectées, ou annuler
# Enrobe   : shutdown -r / shutdown -h HH:MM ou +N, avec message ; shutdown -c pour annuler
# Ports    : gtk3sermo conseillé ; ncursessermo par SSH
# Terminal : rien de perdu
# Usage    : ./10-redemarrage-programme.sh [binaire sermo]
#            SERMO_A_BLANC=0 pour agir pour de vrai (défaut : la commande est affichée)
#            Programmer demande les droits root : sans eux, le script le dit et affiche la
#            commande. shutdown n'accepte qu'une heure dans les 24 h qui viennent.

export SERMO="${SERMO:-sermo}"
export SERMO_A_BLANC="${SERMO_A_BLANC:-1}"
# Le script lui-même, pour --do : le shell de sermo lit la variable, sans
# citation à refaire (printf %q écrit $'…' en locale C, que dash lit de travers).
export SERMO_SCRIPT="$0"

# Échappe une valeur avant de l'insérer dans le XML.
xml() { local s=${1//&/"&amp;"}; s=${s//</"&lt;"}; s=${s//>/"&gt;"}; printf '%s' "${s//\"/"&quot;"}"; }

# Lance une commande, ou l'affiche seulement en mode à blanc.
# Sans les droits root, ne lance rien : affiche la commande à lancer en root.
agir() {
	if [ "$SERMO_A_BLANC" = 1 ]; then
		printf 'à blanc :'; printf ' %q' "$@"; printf '\n'
	elif [ "$EUID" -ne 0 ]; then
		printf 'Droits root nécessaires. Commande à lancer en root :'; printf ' %q' "$@"; printf '\n'
		return 1
	else
		"$@"
	fi
}

# Argument : ce qui est déjà programmé (texte), vide s'il n'y a rien.
dialogue_principal() {
	local prevu=${1-"aucun arrêt programmé"}
	cat <<XML
<window title="Redémarrage programmé" default-width="540" default-height="480">
  <vbox>
    <text><label>Déjà programmé : $(xml "$prevu")</label></text>
    <frame label="Quoi">
      <hbox>
        <radiobutton><label>Redémarrer</label><default>true</default><variable>Q_REDEMARRER</variable></radiobutton>
        <radiobutton><label>Éteindre</label><variable>Q_ETEINDRE</variable></radiobutton>
        <radiobutton><label>Annuler ce qui est programmé</label><variable>Q_ANNULER</variable></radiobutton>
      </hbox>
    </frame>
    <frame label="Quand">
      <vbox>
        <radiobutton><label>À une heure précise (dans les 24 h)</label><default>true</default><variable>W_HEURE</variable></radiobutton>
        <hbox><text><label>Heure :</label></text>
          <spinbutton range-min="0" range-max="23" range-step="1"><variable>HEURE</variable><default>22</default></spinbutton>
          <text><label>h</label></text>
          <spinbutton range-min="0" range-max="59" range-step="5"><variable>MINUTE</variable><default>30</default></spinbutton></hbox>
        <radiobutton><label>Dans un délai (minutes)</label><variable>W_DELAI</variable></radiobutton>
        <hscale range-min="1" range-max="240" range-step="1" draw-value="true" space-expand="true" space-fill="true"><variable>DELAI</variable><default>15</default></hscale>
      </vbox>
    </frame>
    <hbox><text><label>Message aux personnes connectées :</label></text>
      <entry space-expand="true" space-fill="true"><variable>MESSAGE</variable><default>Mise à jour du serveur : enregistrez votre travail.</default></entry></hbox>
    <hbox>
      <button cancel></button>
      <button><label>Programmer</label><action>exit:programmer</action></button>
    </hbox>
  </vbox>
</window>
XML
}

# Arguments : action (redémarrage, arrêt), échéance lisible.
dialogue_confirmation() {
	local quoi=${1:-redémarrage} quand=${2:-à 22:30}
	cat <<XML
<window title="Confirmer">
  <vbox>
    <text><label>Programmer un $(xml "$quoi") $(xml "$quand") ?</label></text>
    <text><label>Les programmes ouverts seront fermés sans enregistrer. Les connexions seront refusées 5 minutes avant.</label></text>
    <hbox>
      <button cancel></button>
      <button><label>Programmer</label><action>exit:confirmer</action></button>
    </hbox>
  </vbox>
</window>
XML
}

suite() {
	# La sortie standard de sermo part vers /dev/null : écrire sur la sortie d'erreur.
	exec >&2
	case "$1" in
	principal)
		[ "$EXIT" = programmer ] || exit 0
		if [ "$Q_ANNULER" = true ]; then
			agir shutdown -c
			exit
		fi
		[[ $MESSAGE != *[[:cntrl:]]* && $MESSAGE != -* && ${#MESSAGE} -le 200 ]] || { echo "Message refusé (une ligne, 200 caractères au plus, pas de tiret au début)." >&2; exit 1; }
		# Les nombres arrivent parfois avec des décimales selon le port (22.0) : partie entière.
		HEURE=${HEURE%%[.,]*}; MINUTE=${MINUTE%%[.,]*}; DELAI=${DELAI%%[.,]*}
		if [ "$W_DELAI" = true ]; then
			[[ $DELAI =~ ^[0-9]+$ ]] && [ "$DELAI" -ge 1 ] && [ "$DELAI" -le 240 ] || { echo "Délai refusé : $DELAI" >&2; exit 1; }
			QUAND=+$DELAI; lisible="dans $DELAI minutes"
		else
			[[ $HEURE =~ ^[0-9]{1,2}$ ]] && [ "$HEURE" -le 23 ] || { echo "Heure refusée : $HEURE" >&2; exit 1; }
			[[ $MINUTE =~ ^[0-9]{1,2}$ ]] && [ "$MINUTE" -le 59 ] || { echo "Minute refusée : $MINUTE" >&2; exit 1; }
			QUAND=$(printf '%02d:%02d' "$((10#$HEURE))" "$((10#$MINUTE))"); lisible="à $QUAND"
		fi
		OPT=-r; quoi=redémarrage
		[ "$Q_ETEINDRE" = true ] && { OPT=-h; quoi=arrêt; }
		export OPT QUAND MESSAGE
		export DIALOGUE; DIALOGUE=$(dialogue_confirmation "$quoi" "$lisible")
		"$SERMO" --program=DIALOGUE --do='"$SERMO_SCRIPT" --suite confirmation' >/dev/null
		;;
	confirmation)
		[ "$EXIT" = confirmer ] || exit 0
		case "$OPT" in -r|-h) ;; *) exit 1 ;; esac
		[[ $QUAND =~ ^(\+[0-9]{1,3}|[0-2][0-9]:[0-5][0-9])$ ]] || exit 1
		if [ -n "$MESSAGE" ]; then agir shutdown "$OPT" "$QUAND" "$MESSAGE"; else agir shutdown "$OPT" "$QUAND"; fi
		echo "Pour annuler : relancez cet outil et choisissez « Annuler », ou shutdown -c en root."
		;;
	esac
}

main() {
	if [ "${1:-}" = --suite ]; then suite "$2"; exit; fi
	[ -n "${1:-}" ] && SERMO=$1
	command -v shutdown >/dev/null || PATH=$PATH:/usr/sbin:/sbin
	command -v shutdown >/dev/null || { echo "shutdown est absent : installez le paquet systemd-sysv." >&2; exit 1; }
	# systemd-logind note l'arrêt programmé ici (MODE=reboot|poweroff, USEC=échéance).
	local prevu="aucun arrêt programmé" f=/run/systemd/shutdown/scheduled
	if [ -r "$f" ]; then
		local mode usec
		mode=$(sed -n 's/^MODE=//p' "$f"); usec=$(sed -n 's/^USEC=//p' "$f")
		[[ $usec =~ ^[0-9]+$ ]] && prevu="$mode le $(date -d "@$((usec / 1000000))" '+%d/%m à %H:%M')"
	fi
	export DIALOGUE; DIALOGUE=$(dialogue_principal "$prevu")
	"$SERMO" --program=DIALOGUE --do='"$SERMO_SCRIPT" --suite principal' >/dev/null
}

[ "${BASH_SOURCE[0]}" = "$0" ] && main "$@"
