#!/bin/sh
# SPDX-License-Identifier: CC0-1.0
# Développé avec l'assistance de Claude (Anthropic) — mai 2026
# Exemple : <fontbutton> — Sélecteur de police
# Port : qt6sermo (Qt6 >= 6.2.0)
export MAIN_DIALOG='
<window title="Police qt6sermo" width-request="320" height-request="130">
  <vbox>
    <fontbutton><variable>FONT</variable><default>Sans 12</default></fontbutton>
    <button ok></button>
  </vbox>
</window>'
eval $(qt6sermo --program MAIN_DIALOG)
echo "FONT=${FONT}"
