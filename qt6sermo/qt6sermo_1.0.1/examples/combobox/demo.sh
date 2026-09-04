#!/bin/sh
# SPDX-License-Identifier: CC0-1.0
# Développé avec l'assistance de Claude (Anthropic) — mai 2026
# Exemple : <combobox> — Menu déroulant
# Port : qt6sermo (Qt6 >= 6.2.0)
export MAIN_DIALOG='
<window title="ComboBox qt6sermo" width-request="300" height-request="130">
  <vbox>
    <combobox><variable>LANGUE</variable>
      <item>Français</item>
      <item>English</item>
      <item>Español</item>
    </combobox>
    <button ok></button>
  </vbox>
</window>'
eval $(qt6sermo --program MAIN_DIALOG)
echo "LANGUE=${LANGUE}"
