#!/bin/sh
# ───────────────────────────────────────────────────────────────────────────
# Repère visuel 2 : CONTENEURS & MISE EN PAGE
#
# Teste : imbrication hbox/vbox/frame, alignements, expand/fill, séparateurs.
#         Permet de vérifier que l'ordre et la répartition de l'espace sont
#         identiques sur tous les ports.
#
# Usage :  ./02-conteneurs.sh            (utilise gtk3sermo)
#          ./02-conteneurs.sh gtkdialog (le nom d'époque, même binaire)
#
# Domaine public (CC0-1.0), comme le reste de examples/ : copiez, modifiez,
# republiez sans condition.
# ───────────────────────────────────────────────────────────────────────────
DIALOG="${1:-gtk3sermo}"

export MAIN_DIALOG='
<window title="Repere 2 - Conteneurs" default-width="560" default-height="420">
  <vbox space-expand="true" space-fill="true">

    <hbox space-expand="true" space-fill="true">
      <frame label="Colonne gauche" space-expand="true" space-fill="true">
        <vbox>
          <text><label>Haut gauche</label></text>
          <button><label>Bouton A</label><action>echo A</action></button>
          <button><label>Bouton B</label><action>echo B</action></button>
        </vbox>
      </frame>
      <frame label="Colonne droite" space-expand="true" space-fill="true">
        <vbox>
          <text><label>Trois boutons en ligne :</label></text>
          <hbox homogeneous="true">
            <button><label>Un</label></button>
            <button><label>Deux</label></button>
            <button><label>Trois</label></button>
          </hbox>
          <hseparator></hseparator>
          <text><label>Zone extensible :</label></text>
          <edit space-expand="true" space-fill="true"><default>Cette zone doit remplir l espace restant.</default></edit>
        </vbox>
      </frame>
    </hbox>

    <frame label="Bandeau bas (pleine largeur)" space-expand="false" space-fill="false">
      <hbox>
        <text space-expand="true" space-fill="true"><label>Etiquette a gauche</label></text>
        <button><label>Action a droite</label></button>
      </hbox>
    </frame>

    <hbox><button ok></button></hbox>

  </vbox>
</window>'

exec "$DIALOG" --program=MAIN_DIALOG --center
