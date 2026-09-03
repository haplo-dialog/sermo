#!/bin/sh
# SPDX-License-Identifier: CC0-1.0
# Développé avec l'assistance de Claude (Anthropic) — mai 2026
# Exemple : <progressbar> — Barre de progression
# Port : qt6sermo (Qt6 >= 6.2.0)
export MAIN_DIALOG='
<window title="Progress qt6sermo" width-request="350" height-request="120">
  <vbox>
    <progressbar><variable>PROGRESS</variable><default>65</default></progressbar>
    <button ok></button>
  </vbox>
</window>'
eval $(qt6sermo --program MAIN_DIALOG)
echo "PROGRESS=${PROGRESS}"
