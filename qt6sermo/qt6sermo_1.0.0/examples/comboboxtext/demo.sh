#!/bin/sh
# SPDX-License-Identifier: CC0-1.0
# Développé avec l'assistance de Claude (Anthropic) — mai 2026
# Exemple : <comboboxtext> — Menu déroulant éditable
# Port : qt6sermo (Qt6 >= 6.2.0)
export MAIN_DIALOG='
<window title="ComboBoxText qt6sermo" width-request="300" height-request="130">
  <vbox>
    <comboboxtext><variable>VILLE</variable>
      <item>Paris</item>
      <item>Lyon</item>
      <item>Marseille</item>
    </comboboxtext>
    <button ok></button>
  </vbox>
</window>'
eval $(qt6sermo --program MAIN_DIALOG)
echo "VILLE=${VILLE}"
