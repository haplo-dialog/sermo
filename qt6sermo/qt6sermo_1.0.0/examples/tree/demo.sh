#!/bin/sh
# SPDX-License-Identifier: CC0-1.0
# Développé avec l'assistance de Claude (Anthropic) — mai 2026
# Exemple : <tree> — Arbre hiérarchique
# Port : qt6sermo (Qt6 >= 6.2.0)
export MAIN_DIALOG='
<window title="Arbre qt6sermo" width-request="350" height-request="300">
  <vbox>
    <tree><variable>NODE</variable>
      <item>Racine
        <item>Enfant 1</item>
        <item>Enfant 2</item>
      </item>
    </tree>
    <button ok></button>
  </vbox>
</window>'
eval $(qt6sermo --program MAIN_DIALOG)
echo "NODE=${NODE}"
