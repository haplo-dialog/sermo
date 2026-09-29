#!/bin/sh
# SPDX-License-Identifier: CC0-1.0
# Exemple : <expander> — Section dépliable
# Port : qt6sermo (Qt6 >= 6.2.0)
export MAIN_DIALOG='
<window title="Expander qt6sermo" width-request="350" height-request="200">
  <vbox>
    <expander label="Détails">
      <vbox><text><label>Option cachée</label></text></vbox>
    </expander>
    <button ok></button>
  </vbox>
</window>'
qt6sermo --program MAIN_DIALOG
