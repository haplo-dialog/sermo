#!/bin/sh
# SPDX-License-Identifier: CC0-1.0
# Développé avec l'assistance de Claude (Anthropic) — mai 2026
# Exemple : <togglebutton> — Bouton bascule
# Port : qt6sermo (Qt6 >= 6.2.0)
export MAIN_DIALOG='
<window title="Toggle qt6sermo" width-request="300" height-request="150">
  <vbox>
    <togglebutton><label>Mode sombre</label><variable>DARK</variable></togglebutton>
    <button ok></button>
  </vbox>
</window>'
eval $(qt6sermo --program MAIN_DIALOG)
echo "DARK=${DARK}"
