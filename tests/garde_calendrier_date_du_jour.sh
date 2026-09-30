#!/bin/bash
# SPDX-License-Identifier: GPL-2.0-or-later
# garde_calendrier_date_du_jour.sh — un <calendar> pose la date demandée, quel
# que soit le jour où le dialogue s'ouvre.
#
# POURQUOI CE BANC EXISTE
#
# Le 2026-09-30, gtk4sermo 2.7.3 rendait CAL_B="2000-09-29" pour un <default>
# 2000-02-29. Le calendrier part de la date du jour et reçoit l'année, puis le
# mois, puis le jour : parti du 30 septembre, il passait par le « 30 février
# 2000 », que GTK refuse sans rien dire, et le mois restait septembre. Le banc
# de comportement ne le voyait que les jours 29 à 31 : vert le 29, rouge le 30.
#
# Ce banc fixe la date du jour du SEUL binaire (faketime, l'horloge avance
# ensuite normalement) sur des jours pièges, plus un jour sans piège pour
# témoin, et joue les cas de tests/calendrier/cas avec le runner de
# comportement (valeurs de l'étalon gtk3sermo).
#
# Usage : garde_calendrier_date_du_jour.sh <binaire>
# Codes : 0 = la date demandée tient chaque jour · 1 = écart · 2 = usage/outil absent

set -u
BIN="${1:-}"
[ -n "$BIN" ] && [ -x "$BIN" ] || { echo "usage: $0 <binaire>" >&2; exit 2; }
command -v faketime >/dev/null 2>&1 || { echo "ÉCHEC : faketime absent (paquet faketime) — rien vérifié" >&2; exit 2; }
ICI=$(CDPATH= cd -- "$(dirname -- "$0")" && pwd)
BIN=$(CDPATH= cd -- "$(dirname -- "$BIN")" && pwd)/$(basename -- "$BIN")
NOM=$(basename "$BIN")
T=$(mktemp -d) || exit 2
trap 'rm -rf "$T"' EXIT

echecs=0
for jour in 2026-01-31 2026-09-30 2026-06-15; do
    # Le lanceur porte le NOM du binaire : le runner en déduit l'environnement
    # sans écran propre à chaque toolkit.
    mkdir -p "$T/$jour"
    cat > "$T/$jour/$NOM" <<LANCEUR
#!/bin/sh
FAKETIME_DONT_FAKE_MONOTONIC=1 exec faketime -f "@$jour 12:00:00" "$BIN" "\$@"
LANCEUR
    chmod +x "$T/$jour/$NOM"
    if CAS="$ICI/calendrier/cas" TIMEOUT=20 sh "$ICI/comportement/run.sh" "$T/$jour/$NOM" > "$T/$jour.log" 2>&1; then
        echo "OK     jour courant $jour : dates demandées posées"
    else
        echo "ÉCHEC  jour courant $jour :"; sed -n '/ÉCART\|étalon\|rendu\|BLOQU/p' "$T/$jour.log" | head -12
        echecs=$((echecs + 1))
    fi
done
[ "$echecs" -eq 0 ] && echo "garde_calendrier_date_du_jour : OK — $NOM pose la date demandée, quel que soit le jour."
[ "$echecs" -eq 0 ]
