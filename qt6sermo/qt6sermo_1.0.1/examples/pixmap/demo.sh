#!/bin/sh
# SPDX-License-Identifier: CC0-1.0
# Développé avec l'assistance de Claude (Anthropic) — mai 2026
# Exemple : <pixmap> — Image/pixmap
# Port : qt6sermo (Qt6 >= 6.2.0)
export MAIN_DIALOG='
<window title="Image qt6sermo" width-request="350" height-request="200">
  <vbox>
    <pixmap><variable>IMG</variable><input file>/usr/share/pixmaps/debian-logo.png</input></pixmap>
    <button ok></button>
  </vbox>
</window>'
eval $(qt6sermo --program MAIN_DIALOG)
echo "IMG=${IMG}"
