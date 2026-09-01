#!/bin/sh
# SPDX-License-Identifier: CC0-1.0
# Développé avec l'assistance de Claude (Anthropic) — mai 2026
# Exemple : <vseparator> — Séparateur vertical
# Port : qt6sermo (Qt6 >= 6.2.0)
export MAIN_DIALOG='
<window title="VSep qt6sermo" width-request="350" height-request="150">
  <hbox>
    <text><label>Panneau gauche</label></text>
    <vseparator></vseparator>
    <text><label>Panneau droit</label></text>
  </hbox>
</window>'
qt6sermo --program MAIN_DIALOG
