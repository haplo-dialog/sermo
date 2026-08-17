#!/bin/sh
# ───────────────────────────────────────────────────────────────────────────
# Repère visuel 1 : FORMULAIRE
#
# Teste : entry, password, spinbutton, checkbox, comboboxtext, boutons,
#         imbrication frame > vbox > hbox.
#
# Usage :  ./01-formulaire.sh            (utilise gtk3dialog)
#          ./01-formulaire.sh gtkdialog (le nom d'époque, même binaire)
#
# Domaine public (CC0-1.0), comme le reste de examples/ : copiez, modifiez,
# republiez sans condition.
# ───────────────────────────────────────────────────────────────────────────
DIALOG="${1:-gtk3dialog}"

export MAIN_DIALOG='
<window title="Repere 1 - Formulaire" default-width="480" default-height="420">
  <vbox space-expand="true" space-fill="true">

    <frame label="Identite" space-expand="false" space-fill="false">
      <vbox>
        <hbox><text><label>Nom :</label></text>
          <entry space-expand="true" space-fill="true"><default>Dupont</default><variable>NOM</variable></entry>
        </hbox>
        <hbox><text><label>Mot de passe :</label></text>
          <password space-expand="true" space-fill="true"><variable>MDP</variable></password>
        </hbox>
        <hbox><text><label>Age :</label></text>
          <spinbutton range-min="0" range-max="120" range-step="1"><input>echo 30</input><variable>AGE</variable></spinbutton>
        </hbox>
      </vbox>
    </frame>

    <frame label="Preferences" space-expand="false" space-fill="false">
      <vbox>
        <checkbox><label>Recevoir les notifications</label><variable>NOTIF</variable></checkbox>
        <checkbox><label>Mode sombre</label><default>true</default><variable>DARK</variable></checkbox>
        <hbox><text><label>Niveau :</label></text>
          <comboboxtext space-expand="true" space-fill="true">
            <variable>NIVEAU</variable>
            <item>Faible</item><item>Moyen</item><item>Eleve</item>
          </comboboxtext>
        </hbox>
      </vbox>
    </frame>

    <hseparator></hseparator>

    <hbox homogeneous="true">
      <button cancel></button>
      <button ok></button>
    </hbox>

  </vbox>
</window>'

exec "$DIALOG" --program=MAIN_DIALOG --center
