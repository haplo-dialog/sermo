#!/bin/bash
# SPDX-License-Identifier: GPL-2.0-or-later
# garde_identite_port.sh — un binaire dit QUI il est, et où se lit sa version.
#
# POURQUOI CE BANC EXISTE
#
# « --version » est imprimé par le cœur, compilé une fois en bibliothèque
# statique. Il n'y connaissait que ses propres PACKAGE_NAME et BUILD_DETAILS :
# les sept ports s'annonçaient tous « sermocore version 2.7.1
# sermo/libsermocore ». En 1.x chaque port compilait son gtkdialog.c et se
# nommait juste ; le cœur partagé a emporté cette justesse sans bruit.
# ci/construire.sh lisait déjà « --version » sur chaque binaire — mais n'en
# retenait que le numéro, donc rien ne pouvait le signaler.
#
# Deux choses sont vérifiées ici, et la seconde n'est pas cosmétique :
#   1. le PREMIER mot est le nom du binaire ;
#   2. le TROISIÈME mot est le numéro de version.
#
# La position est figée : des exemples livrés lisent la version par rang
# (examples/pfeme/main, examples/pfontview/main, « funcAppVersionGet
# "$GTKDIALOG -v" 2 », index base 0). La forme de cette ligne fait partie de la
# surface publique au sens de VERSIONING.md : la déplacer est une rupture.
#
# Usage : garde_identite_port.sh <binaire> [version attendue]
#         (sans second argument : le fichier VERSION du dépôt)

set -u
BIN="${1:-}"
[ -n "$BIN" ] && [ -x "$BIN" ] || { echo "usage: $0 <binaire> [version]" >&2; exit 2; }
RACINE=$(cd "$(dirname "$0")/.." && pwd)
ATTENDUE="${2:-$(tr -d '[:space:]' < "$RACINE/VERSION" 2>/dev/null)}"
[ -n "$ATTENDUE" ] || { echo "ECHEC : version attendue inconnue (VERSION absent ?)" >&2; exit 2; }

NOM=$(basename "$BIN")

# stderr écarté : GTK et EFL y écrivent des avertissements de modules qui
# n'ont rien à voir, et qui masqueraient la ligne cherchée.
LIGNE=$(timeout 20 "$BIN" --version 2>/dev/null | head -1)
[ -n "$LIGNE" ] || { echo "ECHEC : $NOM --version n'a rien écrit sur sa sortie" >&2; exit 1; }

MOT1=$(printf '%s\n' "$LIGNE" | awk '{print $1}')
MOT3=$(printf '%s\n' "$LIGNE" | awk '{print $3}')

ecarts=0
if [ "$MOT1" != "$NOM" ]; then
    echo "  1er mot : « $MOT1 », attendu « $NOM » — le binaire s'annonce sous un autre nom" >&2
    ecarts=$((ecarts + 1))
fi
if [ "$MOT3" != "$ATTENDUE" ]; then
    echo "  3e mot : « $MOT3 », attendu « $ATTENDUE » — des scripts lisent la version à ce rang" >&2
    ecarts=$((ecarts + 1))
fi

if [ "$ecarts" -gt 0 ]; then
    echo "ECHEC : $NOM --version dit « $LIGNE »" >&2
    exit 1
fi
echo "OK : $NOM s'annonce « $MOT1 », version « $MOT3 » au 3e mot"
