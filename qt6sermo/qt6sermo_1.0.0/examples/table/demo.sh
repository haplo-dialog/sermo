#!/bin/sh
# SPDX-License-Identifier: CC0-1.0
# Développé avec l'assistance de Claude (Anthropic) — mai 2026
# Exemple : <table> — Tableau de données
# Port : qt6sermo (Qt6 >= 6.2.0)
export MAIN_DIALOG='
<window title="Tableau qt6sermo" width-request="500" height-request="300">
  <vbox>
    <table><variable>TABLE</variable>
      <column>Nom</column><column>Valeur</column>
      <item>Alpha|100</item>
      <item>Beta|200</item>
    </table>
    <button ok></button>
  </vbox>
</window>'
eval $(qt6sermo --program MAIN_DIALOG)
echo "TABLE=${TABLE}"
