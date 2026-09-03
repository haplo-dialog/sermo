#!/bin/sh
# SPDX-License-Identifier: CC0-1.0
# Développé avec l'assistance de Claude (Anthropic) — mai 2026
# Exemple : <menubar> — Barre de menus native Qt6
# Port : qt6sermo (Qt6 >= 6.2.0)
export MAIN_DIALOG='
<window title="Menu qt6sermo" width-request="400" height-request="300">
  <vbox>
    <menubar>
      <menu>
        <menuitem><label>Nouveau</label><action>echo Nouveau</action></menuitem>
        <menuitem><label>Quitter</label><action>EXIT:0</action></menuitem>
      
        <label>Fichier</label>
      </menu>
    </menubar>
    <text><label>Zone principale</label></text>
  </vbox>
</window>'
qt6sermo --program MAIN_DIALOG
