#!/bin/sh
# SPDX-License-Identifier: CC0-1.0
# ───────────────────────────────────────────────────────────────────────────
# Repère visuel 5 : BOUTONS & ICONES
#
# Teste : <image icon-name="…"> (icones de theme), boutons avec icone
#         (<input file icon="…">), disposition en grille, image fichier.
#         Vérifie que les icones s'affichent (5 ports sur 6) et restent
#         coherentes.
#
# Usage :  ./05-boutons-icones.sh            (utilise gtk3sermo)
#          ./05-boutons-icones.sh gtkdialog (le nom d'époque, même binaire)
#
# Domaine public (CC0-1.0), comme le reste de examples/ : copiez, modifiez,
# republiez sans condition.
# ───────────────────────────────────────────────────────────────────────────
DIALOG="${1:-gtk3sermo}"

export MAIN_DIALOG='
<window title="Repere 5 - Boutons et icones" default-width="520" default-height="400">
  <vbox space-expand="true" space-fill="true">

    <frame label="Icones de theme (image icon-name)">
      <hbox homogeneous="true">
        <vbox><image icon-name="folder" icon-size="dialog"></image><text><label>Dossier</label></text></vbox>
        <vbox><image icon-name="document-save" icon-size="dialog"></image><text><label>Enregistrer</label></text></vbox>
        <vbox><image icon-name="edit-find" icon-size="dialog"></image><text><label>Chercher</label></text></vbox>
        <vbox><image icon-name="dialog-warning" icon-size="dialog"></image><text><label>Attention</label></text></vbox>
        <vbox><image icon-name="system-run" icon-size="dialog"></image><text><label>Executer</label></text></vbox>
      </hbox>
    </frame>

    <frame label="Boutons avec icone (input file icon)">
      <vbox>
        <button image-position="left">
          <input file icon="document-open"></input><label>Ouvrir un fichier</label>
          <action>echo ouvrir</action>
        </button>
        <button image-position="left">
          <input file icon="document-save"></input><label>Enregistrer</label>
          <action>echo enregistrer</action>
        </button>
        <button image-position="left">
          <input file icon="edit-delete"></input><label>Supprimer</label>
          <action>echo supprimer</action>
        </button>
      </vbox>
    </frame>

    <hbox><button ok></button><button cancel></button></hbox>

  </vbox>
</window>'

exec "$DIALOG" --program=MAIN_DIALOG --center
