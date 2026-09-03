#!/bin/sh
# SPDX-License-Identifier: CC0-1.0
# Développé avec l'assistance de Claude (Anthropic) — mai 2026
# Exemple : <frame> — Cadre avec titre (groupbox)
# Port : qt6sermo (Qt6 >= 6.2.0)
export MAIN_DIALOG='
<window title="Frame qt6sermo" width-request="350" height-request="200">
  <vbox>
    <frame label="Options">
      <vbox>
        <checkbox><label>Option 1</label></checkbox>
        <checkbox><label>Option 2</label></checkbox>
      </vbox>
    </frame>
    <button ok></button>
  </vbox>
</window>'
qt6sermo --program MAIN_DIALOG
