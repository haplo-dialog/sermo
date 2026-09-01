#!/bin/sh
# SPDX-License-Identifier: CC0-1.0
# Développé avec l'assistance de Claude (Anthropic) — mai 2026
# Exemple : <colorbutton> — Sélecteur de couleur
# Port : qt6sermo (Qt6 >= 6.2.0)
export MAIN_DIALOG='
<window title="Couleur qt6sermo" width-request="320" height-request="130">
  <vbox>
    <colorbutton><variable>COULEUR</variable><default>#3498db</default></colorbutton>
    <button ok></button>
  </vbox>
</window>'
eval $(qt6sermo --program MAIN_DIALOG)
echo "COULEUR=${COULEUR}"
