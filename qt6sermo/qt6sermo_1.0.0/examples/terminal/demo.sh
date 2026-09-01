#!/bin/sh
# SPDX-License-Identifier: CC0-1.0
# Développé avec l'assistance de Claude (Anthropic) — mai 2026
# Exemple : <terminal> — Terminal (QT6_TODO — pas de VTE Qt natif)
# Port : qt6sermo (Qt6 >= 6.2.0)
export MAIN_DIALOG='
<window title="Terminal qt6sermo" width-request="600" height-request="400">
  <vbox>
    <terminal><variable>TERM</variable><value>bash</value></terminal>
    <button ok></button>
  </vbox>
</window>'
eval $(qt6sermo --program MAIN_DIALOG)
echo "TERM=${TERM}"
