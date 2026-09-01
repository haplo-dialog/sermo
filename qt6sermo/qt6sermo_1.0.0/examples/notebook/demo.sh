#!/bin/sh
# SPDX-License-Identifier: CC0-1.0
# Développé avec l'assistance de Claude (Anthropic) — mai 2026
# Exemple : <notebook> — Onglets
# Port : qt6sermo (Qt6 >= 6.2.0)
export MAIN_DIALOG='
<window title="Notebook qt6sermo" width-request="400" height-request="250">
  <notebook>
    <vbox label="Onglet 1"><text><label>Contenu 1</label></text></vbox>
    <vbox label="Onglet 2"><text><label>Contenu 2</label></text></vbox>
  </notebook>
</window>'
qt6sermo --program MAIN_DIALOG
