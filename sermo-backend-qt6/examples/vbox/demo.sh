#!/bin/sh
# SPDX-License-Identifier: CC0-1.0
# Exemple : <vbox> — Conteneur vertical
# Port : qt6sermo (Qt6 >= 6.2.0)
export MAIN_DIALOG='
<window title="VBox qt6sermo" width-request="250" height-request="200">
  <vbox>
    <button><label>Haut</label></button>
    <button><label>Milieu</label></button>
    <button><label>Bas</label></button>
  </vbox>
</window>'
qt6sermo --program MAIN_DIALOG
