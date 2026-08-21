#!/bin/sh
# ───────────────────────────────────────────────────────────────────────────
# Repère visuel 3 : ONGLETS (notebook)
#
# Teste : notebook a 3 onglets, contenu different par onglet, bascule.
#         Vérifie le placement des onglets (a gauche, comme gtk3) et le
#         changement de page sur tous les ports.
#
# Usage :  ./03-onglets.sh            (utilise gtk3sermo)
#          ./03-onglets.sh gtkdialog (le nom d'époque, même binaire)
#
# Domaine public (CC0-1.0), comme le reste de examples/ : copiez, modifiez,
# republiez sans condition.
# ───────────────────────────────────────────────────────────────────────────
DIALOG="${1:-gtk3sermo}"

export MAIN_DIALOG='
<window title="Repere 3 - Onglets" default-width="560" default-height="400">
  <vbox space-expand="true" space-fill="true">

    <notebook tab-pos="top" tab-labels="Texte|Champs|Listes"
              space-expand="true" space-fill="true">

      <vbox>
        <frame label="Onglet 1 : texte">
          <vbox>
            <text><label>Premier onglet.</label></text>
            <text><label>Plusieurs lignes de texte simple</label></text>
            <text><label>pour verifier la coherence typographique.</label></text>
          </vbox>
        </frame>
      </vbox>

      <vbox>
        <frame label="Onglet 2 : champs">
          <vbox>
            <hbox><text><label>Recherche :</label></text>
              <entry space-expand="true" space-fill="true"><default>exemple</default></entry>
            </hbox>
            <checkbox><label>Respecter la casse</label></checkbox>
            <hbox><button><label>Valider</label></button><button cancel></button></hbox>
          </vbox>
        </frame>
      </vbox>

      <vbox>
        <frame label="Onglet 3 : liste">
          <vbox>
            <hbox><text><label>Choix :</label></text>
              <comboboxtext space-expand="true" space-fill="true">
                <item>Alpha</item><item>Bravo</item><item>Charlie</item><item>Delta</item>
              </comboboxtext>
            </hbox>
            <edit space-expand="true" space-fill="true"><default>Contenu du troisieme onglet.</default></edit>
          </vbox>
        </frame>
      </vbox>

    </notebook>

    <hbox><button ok></button></hbox>

  </vbox>
</window>'

exec "$DIALOG" --program=MAIN_DIALOG --center
