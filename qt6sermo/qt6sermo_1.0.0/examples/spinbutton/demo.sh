#!/bin/sh
# SPDX-License-Identifier: CC0-1.0
# Développé avec l'assistance de Claude (Anthropic) — mai 2026
# Exemple : <spinbutton> — Compteur numérique
# Port : qt6sermo (Qt6 >= 6.2.0)
export MAIN_DIALOG='
<window title="Spin qt6sermo" width-request="300" height-request="130">
  <vbox>
    <spinbutton><variable>QTE</variable><value>1</value><lower>0</lower><upper>99</upper></spinbutton>
    <button ok></button>
  </vbox>
</window>'
eval $(qt6sermo --program MAIN_DIALOG)
echo "QTE=${QTE}"
