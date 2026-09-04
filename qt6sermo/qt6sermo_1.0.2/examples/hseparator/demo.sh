#!/bin/sh
# SPDX-License-Identifier: CC0-1.0
# Développé avec l'assistance de Claude (Anthropic) — mai 2026
# Exemple : <hseparator> — Séparateur horizontal
# Port : qt6sermo (Qt6 >= 6.2.0)
export MAIN_DIALOG='
<window title="HSep qt6sermo" width-request="350" height-request="200">
  <vbox>
    <text><label>Section 1</label></text>
    <hseparator></hseparator>
    <text><label>Section 2</label></text>
    <button ok></button>
  </vbox>
</window>'
qt6sermo --program MAIN_DIALOG
