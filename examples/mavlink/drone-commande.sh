#!/bin/bash
# SPDX-License-Identifier: CC0-1.0
# Métier   : Télépilote, modélisme, inspection par drone
# Besoin   : au sol, régler la vitesse de mission ; en vol, ramener ou poser le
#            drone, et en dernier recours couper les moteurs
# Enrobe   : <action mavlink> (paquet sermo-cible-mavlink) : un clic, une liste
#            blanche, la preuve du drone ; <input mavlink> pour l'état
# Ports    : gtk3sermo conseillé ; lvglsermo sur un écran de terrain ; ncursessermo par SSH
# Terminal : rien de perdu (boutons au clavier)
# Usage    : ./drone-commande.sh [binaire sermo]
#            MAVLINK_ADRESSE (défaut tcp://127.0.0.1:5760, le simulateur SITL)
#            Rien ne part sans la liste blanche ~/.config/sermo/cibles-mavlink
#            (chmod 600), par exemple :
#                parametre WP_SPD 1..15
#                ordre rtl
#                ordre land
#                ordre terminaison
#
# « Couper les moteurs » demande DEUX appuis en moins de 5 s : le premier
# n'envoie rien et le dit dans la ligne « Dernier ordre ».

SERMO="${1:-${SERMO:-sermo}}"
A="${MAVLINK_ADRESSE:-tcp://127.0.0.1:5760}"
MESSAGE="${XDG_RUNTIME_DIR:?XDG_RUNTIME_DIR absent}/sermo-cible-mavlink/message"

# L'adresse va dans le XML : « & » s'y écrit &amp;.
lire() { printf '<input mavlink>%s?champ=%s&amp;delai=1200</input>' "$A" "$1"; }
ordre() { printf '<action mavlink>%s?ordre=%s</action><action>refresh:MSG</action>' "$A" "$1"; }

export DIALOGUE="<window title=\"Drone — commandes\" width-request=\"380\">
  <vbox>
    <frame État>
      <hbox><text><label>Armé (1 = oui)</label></text><text><variable>ARME</variable>$(lire arme)</text></hbox>
      <hbox><text><label>Mode</label></text><text><variable>MODE</variable>$(lire mode)</text></hbox>
      <hbox><text><label>Hauteur (m)</label></text><text><variable>HAUT</variable>$(lire hauteur)</text></hbox>
    </frame>
    <frame Au sol (drone désarmé)>
      <hscale range-min=\"1\" range-max=\"15\" range-step=\"0.5\"><variable>VITESSE</variable><default>5</default></hscale>
      <button><label>Régler la vitesse de mission (m/s)</label>
        <action mavlink>$A?parametre=WP_SPD&amp;valeur=\$VITESSE</action><action>refresh:MSG</action>
      </button>
    </frame>
    <frame Sécurité>
      <hbox>
        <button><label>Retour au départ</label>$(ordre rtl)</button>
        <button><label>Atterrir ici</label>$(ordre land)</button>
      </hbox>
      <button><label>COUPER LES MOTEURS (2 appuis)</label>$(ordre terminaison)</button>
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
