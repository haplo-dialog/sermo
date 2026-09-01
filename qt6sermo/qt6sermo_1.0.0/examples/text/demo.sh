#!/bin/sh
# SPDX-License-Identifier: CC0-1.0
# Développé avec l'assistance de Claude (Anthropic) — mai 2026
# Exemple : <text> — Étiquette texte statique / HTML
# Port : qt6sermo (Qt6 >= 6.2.0)
export MAIN_DIALOG='
<window title="Texte qt6sermo" width-request="380" height-request="160">
  <vbox>
    <text><label>Ligne 1 : information</label></text>
    <text><label>Ligne 2 : détail</label></text>
    <button ok></button>
  </vbox>
</window>'
qt6sermo --program MAIN_DIALOG
