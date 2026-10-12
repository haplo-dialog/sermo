#!/bin/bash
# SPDX-License-Identifier: CC0-1.0
# Boîte    : Système
# Besoin   : choisir son fuseau horaire, la mise à l'heure automatique (NTP), ou régler l'heure à la main
# Enrobe   : timedatectl (systemd)
# Ports    : gtk3sermo ou qt6sermo conseillés ; ncursessermo par SSH
# Terminal : rien de perdu
# Usage    : ./04-date-heure.sh [binaire sermo]
#            Ces réglages touchent tout le poste et demandent les droits d'administration :
#            le script ne lance rien, il affiche les commandes à lancer en administrateur.

export SERMO="${SERMO:-sermo}"
# Le script lui-même, pour --do : le shell de sermo lit la variable, sans
# citation à refaire (printf %q écrit $'…' en locale C, que dash lit de travers).
export SERMO_SCRIPT="$0"

# Échappe une valeur avant de l'insérer dans le XML.
xml() { local s=${1//&/"&amp;"}; s=${s//</"&lt;"}; s=${s//>/"&gt;"}; printf '%s' "${s//\"/"&quot;"}"; }
# Pas de fonction agir : tout réglage ici demande les droits, le script affiche la commande.

# Arguments : fuseau actuel, NTP actif (true/false), horloge matérielle en heure
# locale (true/false), date du jour (AAAA-MM-JJ), heure, minute, puis la liste des fuseaux.
dialogue_principal() {
	local zone=${1:-Europe/Paris} ntp=${2:-true} rtc=${3:-false} jour=${4:-2026-10-11} \
		heure=${5:-14} minute=${6:-30} z
	shift 6 2>/dev/null || set --
	local zones=("$@")
	[ ${#zones[@]} -gt 0 ] || zones=(Africa/Casablanca America/Montreal Europe/Brussels Europe/London
		Europe/Paris Europe/Zurich Indian/Reunion Pacific/Noumea UTC)
	cat <<XML
<window title="Date et heure" default-width="520">
  <vbox>
    <frame label="Fuseau horaire">
      <hbox><text><label>Fuseau :</label></text>
        <comboboxtext space-expand="true" space-fill="true"><variable>ZONE</variable><default>$(xml "$zone")</default>
XML
	for z in "${zones[@]}"; do printf '          <item>%s</item>\n' "$(xml "$z")"; done
	cat <<XML
        </comboboxtext></hbox>
    </frame>
    <frame label="Mise à l'heure">
      <vbox>
        <hbox><text><label>Mise à l'heure automatique par le réseau (NTP) :</label></text>
          <switch><variable>NTP</variable><default>$(xml "$ntp")</default></switch></hbox>
        <checkbox><label>Horloge matérielle en heure locale (utile seulement avec Windows en double démarrage)</label><variable>RTC_LOCALE</variable><default>$(xml "$rtc")</default></checkbox>
      </vbox>
    </frame>
    <expander label="Régler l'heure à la main (NTP éteint)">
      <vbox>
        <checkbox><label>Régler la date et l'heure ci-dessous</label><variable>MANUEL</variable></checkbox>
        <calendar><variable>JOUR</variable><default>$(xml "$jour")</default></calendar>
        <hbox><text><label>Heure :</label></text>
          <spinbutton range-min="0" range-max="23" range-step="1"><variable>HEURE</variable><default>$(xml "$heure")</default></spinbutton>
          <text><label>Minute :</label></text>
          <spinbutton range-min="0" range-max="59" range-step="1"><variable>MINUTE</variable><default>$(xml "$minute")</default></spinbutton></hbox>
      </vbox>
      <variable>DETAIL_OUVERT</variable>
    </expander>
    <hbox>
      <button cancel></button>
      <button><label>Préparer les commandes</label><action>exit:preparer</action></button>
    </hbox>
  </vbox>
</window>
XML
}

# Les commandes arrivent par <input>, depuis la variable TEXTE_COMMANDES : c'est
# le shell de sermo qui la lit, au moment d'afficher ; rien n'est collé dans le XML.
dialogue_commandes() {
	cat <<XML
<window title="Commandes à lancer en administrateur" default-width="560">
  <vbox>
    <text><label>Ces réglages touchent tout le poste. Lancez ces commandes en administrateur (root) :</label></text>
    <edit editable="false" height-request="120"><variable>COMMANDES</variable><input>printf "%s\n" "\$TEXTE_COMMANDES"</input></edit>
    <text><label>Elles s'affichent aussi dans le terminal qui a lancé l'outil.</label></text>
    <hbox>
      <button><label>Fermer</label><action>exit:fermer</action></button>
    </hbox>
  </vbox>
</window>
XML
}

# Valeur d'une propriété de timedatectl (Timezone, NTP, LocalRTC).
propriete() { timedatectl show -p "$1" --value 2>/dev/null; }

# Les commandes à proposer, une par ligne (seulement ce qui change).
commandes() {
	local q
	[ "$ZONE" != "$(propriete Timezone)" ] && echo "timedatectl set-timezone $(printf '%q' "$ZONE")"
	local ntp=false; [ "$(propriete NTP)" = yes ] && ntp=true
	[ "$NTP" != "$ntp" ] && echo "timedatectl set-ntp $NTP"
	local rtc=false; [ "$(propriete LocalRTC)" = yes ] && rtc=true
	[ "$RTC_LOCALE" != "$rtc" ] && { [ "$RTC_LOCALE" = true ] && echo "timedatectl set-local-rtc 1" || echo "timedatectl set-local-rtc 0"; }
	if [ "$MANUEL" = true ]; then
		printf -v q '%s %02d:%02d:00' "$JOUR" "$((10#$HEURE))" "$((10#$MINUTE))"
		echo "timedatectl set-time $(printf '%q' "$q")"
	fi
	return 0
}

suite() {
	# La sortie standard de sermo part vers /dev/null : écrire sur la sortie d'erreur.
	exec >&2
	case "$1" in
	principal)
		[ "$EXIT" = preparer ] || exit 0
		grep -qxF -- "$ZONE" < <(timedatectl list-timezones 2>/dev/null) || { echo "Fuseau inconnu : $ZONE" >&2; exit 1; }
		[[ $NTP =~ ^(true|false)$ && $RTC_LOCALE =~ ^(true|false)$ ]] || { echo "Réponse inattendue." >&2; exit 1; }
		if [ "$MANUEL" = true ]; then
			[ "$NTP" = false ] || { echo "Régler l'heure à la main demande d'éteindre NTP." >&2; exit 1; }
			[[ $JOUR =~ ^[0-9]{4}-[0-9]{2}-[0-9]{2}$ ]] || { echo "Date refusée : $JOUR" >&2; exit 1; }
			[[ $HEURE =~ ^[0-9]{1,2}$ ]] && [ "$((10#$HEURE))" -le 23 ] || { echo "Heure hors bornes : $HEURE" >&2; exit 1; }
			[[ $MINUTE =~ ^[0-9]{1,2}$ ]] && [ "$((10#$MINUTE))" -le 59 ] || { echo "Minute hors bornes : $MINUTE" >&2; exit 1; }
		fi
		local texte; texte=$(commandes)
		[ -n "$texte" ] || { echo "Rien ne change."; exit 0; }
		echo "À lancer en administrateur (root) :"; printf '%s\n' "$texte"
		export TEXTE_COMMANDES=$texte
		export DIALOGUE; DIALOGUE=$(dialogue_commandes)
		"$SERMO" --program=DIALOGUE >/dev/null
		;;
	esac
}

main() {
	if [ "${1:-}" = --suite ]; then suite "$2"; exit; fi
	[ -n "${1:-}" ] && SERMO=$1
	command -v timedatectl >/dev/null || { echo "timedatectl est absent : ce poste n'utilise pas systemd." >&2; exit 1; }
	local ntp=false rtc=false zones=()
	[ "$(propriete NTP)" = yes ] && ntp=true
	[ "$(propriete LocalRTC)" = yes ] && rtc=true
	mapfile -t zones < <(timedatectl list-timezones 2>/dev/null)
	export DIALOGUE; DIALOGUE=$(dialogue_principal "$(propriete Timezone)" "$ntp" "$rtc" \
		"$(date +%F)" "$(date +%-H)" "$(date +%-M)" "${zones[@]}")
	"$SERMO" --program=DIALOGUE --do='"$SERMO_SCRIPT" --suite principal' >/dev/null
}

[ "${BASH_SOURCE[0]}" = "$0" ] && main "$@"
