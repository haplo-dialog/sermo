#!/bin/bash
# SPDX-License-Identifier: CC0-1.0
# Métier   : Télépilote, modélisme, inspection par drone (en vue directe)
# Besoin   : armer, décoller à une hauteur fixée, aller à un point, tenir la
#            position, poser, sans station sol complète
# Enrobe   : <action mavlink> pilote= (paquet sermo-cible-mavlink) : deux appuis
#            par ordre, liste blanche « pilotage », barrières de sermo et de
#            l'autopilote ; <input mavlink> pour l'état
# Ports    : gtk3sermo conseillé ; lvglsermo sur un écran de terrain
# Terminal : rien de perdu (boutons au clavier)
# Usage    : ./drone-pilotage.sh [binaire sermo]
#            MAVLINK_ADRESSE (défaut tcp://127.0.0.1:5760, le simulateur SITL)
#            Rien ne part sans la ligne « pilotage » de ~/.config/sermo/cibles-mavlink
#            (chmod 600), par exemple :
#                pilotage lat 45.1234 lon 5.7654 rayon 300 hauteur 60 decollage 20 modes 4,5,6,9
#
# LE TÉLÉPILOTE RESTE SEUL RESPONSABLE DU VOL et du respect des règles
# (catégorie, zone, hauteur, vue directe). sermo n'est PAS une station sol
# certifiée ; ce pilotage n'est pas fait pour un vol hors vue.

SERMO="${1:-${SERMO:-sermo}}"
A="${MAVLINK_ADRESSE:-tcp://127.0.0.1:5760}"
MESSAGE="${XDG_RUNTIME_DIR:?XDG_RUNTIME_DIR absent}/sermo-cible-mavlink/message"

# L'adresse va dans le XML : « & » s'y écrit &amp;.
lire() { printf '<input mavlink>%s?champ=%s&amp;delai=1200</input>' "$A" "$1"; }
ordre() { printf '<action mavlink>%s?pilote=%s</action><action>refresh:MSG</action>' "$A" "$1"; }
mode() { printf '<action mavlink>%s?pilote=mode&amp;mode=%s</action><action>refresh:MSG</action>' "$A" "$1"; }

export DIALOGUE="<window title=\"Drone — pilotage\" width-request=\"420\">
  <vbox>
    <text use-markup=\"true\"><label>\"<b>Vous restez seul responsable du vol</b> (règles, zone, hauteur, vue directe). sermo n'est pas une station sol certifiée ; pas de vol hors vue. Chaque ordre : <b>deux appuis</b> en moins de 5 s.\"</label></text>
    <frame État>
      <hbox><text><label>Armé (1 = oui)</label></text><text><variable>ARME</variable>$(lire arme)</text></hbox>
      <hbox><text><label>Mode (4 GUIDED, 5 LOITER, 6 RTL, 9 LAND)</label></text><text><variable>MODE</variable>$(lire mode)</text></hbox>
      <hbox><text><label>Hauteur (m)</label></text><text><variable>HAUT</variable>$(lire hauteur)</text></hbox>
    </frame>
    <frame Au sol>
      <hbox>
        <button><label>GUIDED</label>$(mode guided)</button>
        <button><label>Armer</label>$(ordre armer)</button>
        <button><label>Décoller</label>$(ordre decoller)</button>
        <button><label>Désarmer</label>$(ordre desarmer)</button>
      </hbox>
    </frame>
    <frame Aller à un point (en GUIDED)>
      <hbox><text><label>Latitude</label></text><entry><variable>LAT</variable></entry></hbox>
      <hbox><text><label>Longitude</label></text><entry><variable>LON</variable></entry></hbox>
      <hbox><text><label>Hauteur au-dessus du départ (m)</label></text><entry><variable>H</variable><default>20</default></entry></hbox>
      <button><label>Aller au point</label>
        <action mavlink>$A?pilote=aller&amp;lat=\$LAT&amp;lon=\$LON&amp;hauteur=\$H</action><action>refresh:MSG</action>
      </button>
    </frame>
    <frame En vol>
      <hbox>
        <button><label>Tenir (LOITER)</label>$(mode loiter)</button>
        <button><label>Retour (RTL)</label>$(mode rtl)</button>
        <button><label>Poser (LAND)</label>$(mode land)</button>
      </hbox>
    </frame>
    <frame Dernier ordre>
      <text><variable>MSG</variable><input file>$MESSAGE</input></text>
    </frame>
    <timer milliseconds=\"true\" interval=\"1000\" visible=\"false\">
      <action>refresh:ARME</action><action>refresh:MODE</action><action>refresh:HAUT</action>
    </timer>
    <hbox><button ok></button></hbox>
  </vbox>
</window>"

exec "$SERMO" --program=DIALOGUE
