#!/bin/sh
# SPDX-License-Identifier: CC0-1.0
# Développé avec l'assistance de Claude (Anthropic) — mai 2026
# Exemple : <statusbar> — Barre de statut
# Port : qt6sermo (Qt6 >= 6.2.0)
export MAIN_DIALOG='
<window title="Status qt6sermo" width-request="400" height-request="150">
  <vbox>
    <text><label>Interface principale</label></text>
    <button><label>Action</label><action>echo "OK"</action></button>
    <statusbar><variable>STATUT</variable><default>Prêt.</default></statusbar>
  </vbox>
</window>'
eval $(qt6sermo --program MAIN_DIALOG)
echo "STATUT=${STATUT}"
