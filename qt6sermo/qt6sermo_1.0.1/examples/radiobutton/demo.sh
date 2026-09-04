#!/bin/sh
# SPDX-License-Identifier: CC0-1.0
# Développé avec l'assistance de Claude (Anthropic) — mai 2026
# Exemple : <radiobutton> — Bouton radio (QButtonGroup)
# Port : qt6sermo (Qt6 >= 6.2.0)
export MAIN_DIALOG='
<window title="RadioButton qt6sermo" width-request="300" height-request="180">
  <vbox>
    <radiobutton><label>Option 1</label><variable>CHOIX</variable></radiobutton>
    <radiobutton><label>Option 2</label><variable>CHOIX</variable></radiobutton>
    <button ok></button>
  </vbox>
</window>'
eval $(qt6sermo --program MAIN_DIALOG)
echo "CHOIX=${CHOIX}"
echo "CHOIX=${CHOIX}"
