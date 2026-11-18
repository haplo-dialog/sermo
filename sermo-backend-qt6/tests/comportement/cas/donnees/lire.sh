#!/bin/sh
# SPDX-License-Identifier: GPL-2.0-or-later
# Affiche sur UNE ligne le fichier donné (cas 85 et 86) : les sauts de ligne
# deviennent des virgules, pour qu'une <entry> — qui ne garde que la première
# ligne de son <input> — rende tout ce que « save: » a écrit. Fichier absent :
# « (absent) », ce que rend un port qui n'écrit rien.
[ -f "$1" ] || { echo "(absent)"; exit 0; }
tr '\n' ',' < "$1"
echo
