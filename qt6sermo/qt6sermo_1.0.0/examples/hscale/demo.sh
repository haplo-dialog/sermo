#!/bin/sh
# SPDX-License-Identifier: CC0-1.0
# Développé avec l'assistance de Claude (Anthropic) — mai 2026
# Exemple : <hscale> — Curseur horizontal
# Port : qt6sermo (Qt6 >= 6.2.0)
export MAIN_DIALOG='
<window title="HScale qt6sermo" width-request="400" height-request="130">
  <vbox>
    <hscale><variable>VOL</variable><value>50</value><lower>0</lower><upper>100</upper></hscale>
    <button ok></button>
  </vbox>
</window>'
eval $(qt6sermo --program MAIN_DIALOG)
echo "VOL=${VOL}"
