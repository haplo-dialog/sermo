#!/bin/sh
# SPDX-License-Identifier: CC0-1.0
# Développé avec l'assistance de Claude (Anthropic) — mai 2026
# Exemple : <vscale> — Curseur vertical
# Port : qt6sermo (Qt6 >= 6.2.0)
export MAIN_DIALOG='
<window title="VScale qt6sermo" width-request="150" height-request="300">
  <hbox>
    <vscale><variable>NIV</variable><default>30</default></vscale>
    <button ok></button>
  </hbox>
</window>'
eval $(qt6sermo --program MAIN_DIALOG)
echo "NIV=${NIV}"
