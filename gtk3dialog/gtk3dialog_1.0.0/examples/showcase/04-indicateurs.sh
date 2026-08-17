#!/bin/sh
# ───────────────────────────────────────────────────────────────────────────
# Repère visuel 4 : INDICATEURS
#
# Teste : progressbar, hscale, vscale, levelbar, spinbutton, spinner.
#         Chaque indicateur reçoit une valeur via <input>echo N</input> :
#         c'est LE test pour vérifier que les barres reflètent bien la valeur
#         (et non 0 %) et qu'elles sont correctement rendues sur tous les ports.
#
# Usage :  ./04-indicateurs.sh            (utilise gtk3dialog)
#          ./04-indicateurs.sh gtkdialog (le nom d'époque, même binaire)
#
# Domaine public (CC0-1.0), comme le reste de examples/ : copiez, modifiez,
# republiez sans condition.
# ───────────────────────────────────────────────────────────────────────────
DIALOG="${1:-gtk3dialog}"

export MAIN_DIALOG='
<window title="Repere 4 - Indicateurs" default-width="520" default-height="420">
  <vbox space-expand="true" space-fill="true">

    <frame label="Progression (doit afficher 65 %)">
      <progressbar space-expand="true" space-fill="true">
        <input>echo 65</input><variable>PROG</variable>
      </progressbar>
    </frame>

    <frame label="Glissieres (valeur 40)">
      <hbox space-expand="true" space-fill="true">
        <hscale range-min="0" range-max="100" space-expand="true" space-fill="true">
          <input>echo 40</input><variable>HVAL</variable>
        </hscale>
        <vscale range-min="0" range-max="100">
          <input>echo 40</input><variable>VVAL</variable>
        </vscale>
      </hbox>
    </frame>

    <frame label="Jauge de niveau (75)">
      <levelbar range-min="0" range-max="100" mode="continuous" space-expand="true" space-fill="true">
        <input>echo 75</input><variable>NIVEAU</variable>
      </levelbar>
    </frame>

    <hbox>
      <frame label="Compteur"><spinbutton range-min="0" range-max="100" range-step="1"><input>echo 12</input><variable>CPT</variable></spinbutton></frame>
      <frame label="Activite"><spinner></spinner></frame>
    </hbox>

    <hbox><button ok></button></hbox>

  </vbox>
</window>'

exec "$DIALOG" --program=MAIN_DIALOG --center
