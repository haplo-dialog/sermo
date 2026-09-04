#!/bin/sh
# SPDX-License-Identifier: CC0-1.0
# Développé avec l'assistance de Claude (Anthropic) — mai 2026
# Exemple : <checkbox> — Case à cocher
# Port : qt6sermo (Qt6 >= 6.2.0)
export MAIN_DIALOG='
<window title="Checkbox qt6sermo" width-request="300" height-request="150">
  <vbox>
    <checkbox><label>Activer option A</label><variable>OPT_A</variable></checkbox>
    <checkbox><label>Activer option B</label><variable>OPT_B</variable></checkbox>
    <button ok></button>
  </vbox>
</window>'
eval $(qt6sermo --program MAIN_DIALOG)
echo "OPT_A=${OPT_A}"
echo "OPT_B=${OPT_B}"
