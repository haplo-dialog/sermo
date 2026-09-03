#!/bin/sh
# SPDX-License-Identifier: CC0-1.0
# Développé avec l'assistance de Claude (Anthropic) — mai 2026
# Exemple : <menuitem> — Élément de menu Qt6
# Port : qt6sermo (Qt6 >= 6.2.0)
export MAIN_DIALOG='
<window title="MenuItem qt6sermo" width-request="400" height-request="300">
  <vbox>
    <menubar>
      <menu>
        <menuitem><label>Action 1</label><action>echo "Action 1"</action></menuitem>
        <menuitem><label>Action 2</label><action>echo "Action 2"</action></menuitem>
      
        <label>Actions</label>
      </menu>
    </menubar>
  </vbox>
</window>'
qt6sermo --program MAIN_DIALOG
