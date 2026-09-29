#!/bin/bash
# SPDX-License-Identifier: GPL-2.0-or-later
# garde_liens_langue.sh — un document anglais n'envoie pas son lecteur dans le mur.
#
# POURQUOI CE BANC EXISTE
#
# Le dépôt tient treize documents en deux langues, mais cinq documents anglais
# renvoyaient vers des documents qui n'existaient qu'en français : README.en.md
# envoyait son lecteur vers le manuel utilisateur — trente-six kilo-octets de
# français — sans le prévenir. Le lien marchait : rien ne pouvait le signaler.
#
# Deux règles, donc :
#   1. si la version anglaise existe, le lien anglais y va ;
#   2. sinon, le lien le DIT — « (French only) » — au lieu d'y envoyer en silence.
#
# Usage : garde_liens_langue.sh [racine]   (défaut : la racine du dépôt)

set -u
RACINE="${1:-$(cd "$(dirname "$0")/.." && pwd)}"
[ -d "$RACINE" ] || { echo "usage: $0 [racine]" >&2; exit 2; }
cd "$RACINE" || exit 2

ecarts=0
vus=0
signale() { printf '  %s\n' "$1"; ecarts=$((ecarts + 1)); }

while IFS= read -r doc; do
    [ -n "$doc" ] || continue
    [ -f "$doc" ] || continue
    vus=$((vus + 1))
    # grep -a : un document en ISO-8859-1 serait pris pour du binaire et sauté
    # sans un mot — le banc rendrait « 0 trouvé » sur un dépôt fautif.
    while IFS= read -r ligne; do
        [ -n "$ligne" ] || continue
        no=${ligne%%:*}
        lien=$(printf '%s' "${ligne#*:}" | sed -E 's/.*\]\(([^)]+)\).*/\1/')
        case "$lien" in *.en.md|http*|'') continue;; esac
        [ -f "$lien" ] || continue
        # Le lien de bascule de langue est l'exception : « [Français](X.md) » en
        # tête d'un X.en.md DOIT mener au français, c'est sa raison d'être. Sans
        # cette exception le banc condamnerait ce qu'il cherche à garantir.
        [ "$lien" = "${doc%.en.md}.md" ] && continue
        printf '%s' "$ligne" | grep -aq '\[Français\]' && continue
        base=${lien%.md}
        if [ -f "$base.en.md" ]; then
            signale "$doc:$no renvoie à $lien alors que $base.en.md existe"
        else
            # pas de version anglaise : le lien doit le dire sur la même ligne
            if ! printf '%s' "$ligne" | grep -aq 'French only'; then
                signale "$doc:$no renvoie à $lien (français) sans le dire — ajouter « (French only) »"
            fi
        fi
    done <<< "$(grep -anoE '\]\([A-Za-z0-9_./-]+\.md\)[^)]*' "$doc" 2>/dev/null)"
done <<< "$(git ls-files -- '*.en.md' 2>/dev/null)"

# ⛔ Un banc qui n'a rien vérifié est un échec : sans ce contrôle, un jour où
#    « git ls-files » ne rendrait rien, ce banc passerait au vert sans rien lire.
if [ "$vus" -eq 0 ]; then
    echo "ECHEC : aucun document anglais trouvé — ce banc n'a rien vérifié" >&2
    exit 77
fi

if [ "$ecarts" -gt 0 ]; then
    echo "ECHEC : $ecarts lien(s) de langue à corriger, sur $vus document(s) anglais :" >&2
    exit 1
fi
echo "OK : $vus documents anglais, aucun lien qui envoie au français sans le dire"
