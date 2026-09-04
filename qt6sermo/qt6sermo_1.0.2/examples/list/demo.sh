#!/bin/sh
# SPDX-License-Identifier: CC0-1.0
# Développé avec l'assistance de Claude (Anthropic) — mai 2026
# Exemple : <list> — Liste de sélection
# Port : qt6sermo (Qt6 >= 6.2.0)
export MAIN_DIALOG='
<window title="Liste qt6sermo" width-request="300" height-request="250">
  <vbox>
    <list><variable>ITEM</variable>
      <item>Alpha</item>
      <item>Beta</item>
      <item>Gamma</item>
    </list>
    <button ok></button>
  </vbox>
</window>'
eval $(qt6sermo --program MAIN_DIALOG)
echo "ITEM=${ITEM}"
