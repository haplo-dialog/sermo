#!/bin/sh
# SPDX-License-Identifier: CC0-1.0
# Développé avec l'assistance de Claude (Anthropic) — mai 2026
# Exemple : <entry> — Champ texte ligne unique
# Port : qt6sermo (Qt6 >= 6.2.0)
export MAIN_DIALOG='
<window title="Saisie qt6sermo" width-request="350" height-request="130">
  <vbox>
    <text><label>Votre nom :</label></text>
    <entry><variable>NOM</variable><default>Haplo</default></entry>
    <button ok></button>
  </vbox>
</window>'
eval $(qt6sermo --program MAIN_DIALOG)
echo "NOM=${NOM}"
