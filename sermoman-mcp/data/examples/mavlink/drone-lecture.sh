#!/bin/bash
# SPDX-License-Identifier: CC0-1.0
# Métier   : Télépilote, modélisme, inspection par drone
# Besoin   : voir la télémétrie d'un drone (batterie, position, armé) sans rien lui envoyer
# Enrobe   : <input mavlink> (paquet sermo-source-mavlink) : lecture SEULE, rien n'est émis
# Ports    : gtk3sermo conseillé ; lvglsermo sur un écran de terrain ; ncursessermo par SSH
# Terminal : rien de perdu (la jauge devient une barre de caractères)
# Usage    : ./drone-lecture.sh [binaire sermo]
#            MAVLINK_ADRESSE (défaut tcp://127.0.0.1:5760, le faux drone ou le simulateur ;
#            udp://:14550 derrière un relais comme MAVProxy ; serie:///dev/ttyUSB0 pour une radio)
#            MAVLINK_INTERVALLE en ms (défaut 1000)
#            Essai sans drone : ./faux-drone.py --tcp 5760 & puis ./drone-lecture.sh
#
# sermo ne demande pas de flux au drone (il n'émet rien) : sur une radio seule,
# sans station sol ni relais, seul le battement (armé, mode) arrive. Le remède
# est côté drone : sur ArduPilot 4.7, MAV1_EXT_STAT, MAV1_POSITION et
# MAV1_EXTRA2 (par exemple à 2) ; avant la 4.7, SR0_… (les anciens noms sont
# ignorés en silence).

SERMO="${1:-${SERMO:-sermo}}"
A="${MAVLINK_ADRESSE:-tcp://127.0.0.1:5760}"
INTERVALLE="${MAVLINK_INTERVALLE:-1000}"

# L'adresse va dans le XML : « & » s'y écrit &amp;.
lire() { printf '<input mavlink>%s?champ=%s&amp;delai=1200</input>' "$A" "$1"; }

export DIALOGUE="<window title=\"Drone\" width-request=\"340\">
  <vbox>
    <frame Batterie>
      <levelbar range-min=\"0\" range-max=\"100\"><variable>BATT</variable>$(lire batterie)</levelbar>
      <hbox>
        <text><label>Tension (V)</label></text>
        <text><variable>VOLT</variable>$(lire tension)</text>
      </hbox>
    </frame>
    <frame Position>
      <hbox><text><label>Latitude</label></text><text><variable>LAT</variable>$(lire latitude)</text></hbox>
      <hbox><text><label>Longitude</label></text><text><variable>LON</variable>$(lire longitude)</text></hbox>
      <hbox><text><label>Hauteur (m)</label></text><text><variable>HAUT</variable>$(lire hauteur)</text></hbox>
    </frame>
    <frame État>
      <hbox><text><label>Armé (1 = oui)</label></text><text><variable>ARME</variable>$(lire arme)</text></hbox>
      <hbox><text><label>Satellites</label></text><text><variable>SAT</variable>$(lire satellites)</text></hbox>
    </frame>
    <timer milliseconds=\"true\" interval=\"$INTERVALLE\" visible=\"false\">
      <action>refresh:BATT</action><action>refresh:VOLT</action>
      <action>refresh:LAT</action><action>refresh:LON</action><action>refresh:HAUT</action>
      <action>refresh:ARME</action><action>refresh:SAT</action>
    </timer>
    <hbox><button ok></button></hbox>
  </vbox>
</window>"

exec "$SERMO" --program=DIALOGUE
