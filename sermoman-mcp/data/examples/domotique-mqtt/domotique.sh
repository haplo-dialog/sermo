#!/bin/bash
# SPDX-License-Identifier: CC0-1.0
# Métier   : Domotique, maison, petit bâtiment, atelier
# Besoin   : voir des capteurs MQTT (température, humidité, état d'une lampe) et
#            commander la lampe et une consigne de chauffage, d'un clic
# Enrobe   : rien : balises natives <input mqtt> (lecture, sermo-source-mqtt) et
#            <action mqtt> (écriture, sermo-cible-mqtt, liste blanche ~/.config/sermo/cibles-mqtt)
# Ports    : tout port à fenêtre ; ncursessermo par SSH (lecture et boutons)
# Terminal : rien de perdu
# Usage    : ./fausse-maison.sh &        (serveur MQTT d'essai, local)
#            ./domotique.sh [binaire sermo]
#            MAISON_SERVEUR (défaut mqtt://127.0.0.1:18830, la fausse maison)
#            MAISON_PREFIXE (défaut maison/salon) · MAISON_INTERVALLE en s (défaut 2)
#
# Codes de sortie : 0 fenêtre fermée · 2 sermo ou sermo-source-mqtt absent · 64 réglage invalide.
# Écrire n'est possible QUE pour les sujets et valeurs de ~/.config/sermo/cibles-mqtt
# (voir cibles-mqtt.exemple) : sans ce fichier, les boutons ne publient rien, et
# sermo-cible-mqtt le dit. Ce script n'écrit jamais ce fichier.

export SERMO="${SERMO:-sermo}"
MAISON_SERVEUR="${MAISON_SERVEUR:-mqtt://127.0.0.1:18830}"
MAISON_PREFIXE="${MAISON_PREFIXE:-maison/salon}"
MAISON_INTERVALLE="${MAISON_INTERVALLE:-2}"

xml() { local s=${1//&/"&amp;"}; s=${s//</"&lt;"}; s=${s//>/"&gt;"}; printf '%s' "${s//\"/"&quot;"}"; }

dialogue_principal() {
	local base="$MAISON_SERVEUR?sujet=$MAISON_PREFIXE"
	cat <<XML
<window title="Maison — $(xml "$MAISON_PREFIXE")" default-width="460">
  <vbox spacing="8">
    <frame label="Capteurs">
      <grid columns="2" row-spacing="6" column-spacing="12">
        <text xalign="0"><label>Température (°C)</label></text>
        <text xalign="0"><variable>TEMPERATURE</variable><input mqtt>$(xml "$base/temperature")</input></text>
        <text xalign="0"><label>Humidité (%)</label></text>
        <text xalign="0"><variable>HUMIDITE</variable><input mqtt>$(xml "$base/humidite")</input></text>
        <text xalign="0"><label>Lampe</label></text>
        <text xalign="0"><variable>LAMPE</variable><input mqtt>$(xml "$base/lampe/etat")</input></text>
      </grid>
    </frame>
    <frame label="Commandes (seulement les sujets de ~/.config/sermo/cibles-mqtt)">
      <vbox>
        <hbox>
          <button space-expand="true" space-fill="true"><label>Allumer la lampe</label>
            <action mqtt>$(xml "$base/lampe&valeur=on")</action>
          </button>
          <button space-expand="true" space-fill="true"><label>Éteindre la lampe</label>
            <action mqtt>$(xml "$base/lampe&valeur=off")</action>
          </button>
        </hbox>
        <hbox>
          <text><label>Consigne (°C)</label></text>
          <hscale range-min="18" range-max="24" range-step="0.5" range-value="20" digits="1" space-expand="true" space-fill="true">
            <variable>CONSIGNE</variable>
          </hscale>
          <button><label>Appliquer</label>
            <action mqtt>$(xml "$base/consigne&valeur=\$CONSIGNE")</action>
          </button>
        </hbox>
      </vbox>
    </frame>
    <timer interval="$MAISON_INTERVALLE" visible="false">
      <variable>HORLOGE</variable>
      <action>refresh:TEMPERATURE</action>
      <action>refresh:HUMIDITE</action>
      <action>refresh:LAMPE</action>
    </timer>
    <hbox><button><label>Fermer</label><action>exit:fermer</action></button></hbox>
  </vbox>
</window>
XML
}

usage() { sed -n '/^# Usage/,/^# Codes/p' "$0" | sed '$d;s/^# \{0,1\}//' >&2; exit 64; }

main() {
	# Ces réglages entrent dans le XML et dans l'adresse MQTT : contrôlés d'abord.
	[[ "$MAISON_SERVEUR" =~ ^mqtts?://([A-Za-z0-9][A-Za-z0-9.-]{0,252}|\[[0-9A-Fa-f:]{2,39}\])(:[1-9][0-9]{0,4})?$ ]] ||
		{ echo "MAISON_SERVEUR invalide (mqtt://hôte[:port], sans chemin ni paramètre) : $MAISON_SERVEUR" >&2; exit 64; }
	[[ "$MAISON_PREFIXE" =~ ^[A-Za-z0-9_-]{1,64}(/[A-Za-z0-9_-]{1,64}){0,7}$ ]] ||
		{ echo "MAISON_PREFIXE invalide (mots séparés par /, sans joker + ni #) : $MAISON_PREFIXE" >&2; exit 64; }
	[[ "$MAISON_INTERVALLE" =~ ^([1-9]|[1-5][0-9]|60)$ ]] ||
		{ echo "MAISON_INTERVALLE invalide (1 à 60 s) : $MAISON_INTERVALLE" >&2; exit 64; }
	case "${1:-}" in
	--xml) dialogue_principal; exit 0 ;;
	-*)    usage ;;
	esac
	[ -n "${1:-}" ] && SERMO=$1
	command -v "$SERMO" >/dev/null || { echo "sermo introuvable : $SERMO" >&2; exit 2; }
	local aide=${SERMO_SOURCE_DIR:-/usr/libexec/sermo}
	[ -x "$aide/sermo-source-mqtt" ] ||
		{ echo "sermo-source-mqtt absent de $aide : sudo apt install sermo-source-mqtt" >&2; exit 2; }
	[ -x "$aide/sermo-cible-mqtt" ] ||
		echo "note : sermo-cible-mqtt absent ($aide) : les boutons ne pourront rien publier (sudo apt install sermo-cible-mqtt)" >&2
	export DIALOGUE; DIALOGUE=$(dialogue_principal)
	"$SERMO" --program=DIALOGUE >/dev/null
	exit 0
}

[ "${BASH_SOURCE[0]}" = "$0" ] && main "$@"
