#!/bin/sh
# SPDX-License-Identifier: CC0-1.0
# Développé avec l'assistance de Claude (Anthropic) — mai 2026
# Exemple : <edit> — Zone d'édition texte riche
# Port : qt6sermo (Qt6 >= 6.2.0)
export MAIN_DIALOG='
<window title="Éditeur qt6sermo" width-request="450" height-request="300">
  <vbox>
    <edit><variable>CONTENU</variable><default>Saisissez ici...</default></edit>
    <button ok></button>
  </vbox>
</window>'
eval $(qt6sermo --program MAIN_DIALOG)
echo "CONTENU=${CONTENU}"
