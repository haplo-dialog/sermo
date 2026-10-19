#!/bin/bash
# SPDX-License-Identifier: GPL-2.0-or-later
# tests/garde_wifi_scan.sh — haplo-dialog — 2026 — GPL-2.0-or-later
#
# examples/wifi-scan/ lit nmcli et met en forme des SSID arbitraires. Ce test
# utilise un FAUX nmcli (les réseaux du voisinage ne sont ni lus ni montrés) :
#   1 — liste : tri par signal, bandes 2,4/5/6 GHz, réseau utilisé, SSID masqué,
#       réseaux ouverts et WEP/WPA1 signalés, « \: » d'nmcli bien découpé
#   2 — SSID piégés (|, balises, $(…), contrôles, UTF-8 invalide, trop long) et lignes
#       mal formées : 7 colonnes, rien de piégé, lignes fausses ignorées
#   3 — canaux : comptes et signal max ; « relancer » demande --rescan yes, sinon no
#   4 — XML bien formé ; nmcli absent : 2 ; option inconnue : 64
# Avec un binaire réel en argument (+ xvfb-run) : 5 — la fenêtre s'ouvre et reste ouverte.
# Usage : garde_wifi_scan.sh [/chemin/vers/un-port-a-fenetre]
RACINE=$(CDPATH= cd -- "$(dirname -- "$0")/.." && pwd)
APP="$RACINE/examples/wifi-scan/wifi-scan.sh"
BIN_REEL="${1:-}"
for t in python3 timeout mktemp; do
    command -v "$t" >/dev/null || { echo "outil manquant : $t" >&2; exit 77; }
done
[[ -x "$APP" ]] || { echo "script introuvable ou non exécutable : $APP" >&2; exit 1; }
T=$(mktemp -d); trap 'rm -rf "$T"' EXIT
export APPELS="$T/appels"; : > "$APPELS"
mkdir "$T/faux"
python3 -I - "$T/sortie" <<'PY'
import sys
L = [
 "*:AA\\:BB\\:CC\\:00\\:00\\:01:maison\\:5G:36:5180 MHz:62:WPA2 WPA3",
 " :AA\\:BB\\:CC\\:00\\:00\\:02:voisin:6:2437 MHz:90:WPA2",
 " :AA\\:BB\\:CC\\:00\\:00\\:03:cafe-ouvert:6:2437 MHz:40:",
 " :AA\\:BB\\:CC\\:00\\:00\\:04:vieux:1:2412 MHz:30:WEP",
 " :AA\\:BB\\:CC\\:00\\:00\\:05::11:2462 MHz:20:WPA1",
 " :AA\\:BB\\:CC\\:00\\:00\\:06:six:37:6135 MHz:55:WPA3",
 " :AA\\:BB\\:CC\\:00\\:00\\:07:a|b<b>&amp;$(id)\x1b[31m\x07" + "x" * 40 + ":6:2437 MHz:50:WPA2",
 " :ZZ\\:BB\\:CC\\:00\\:00\\:08:bssid-faux:6:2437 MHz:50:WPA2",
 " :AA\\:BB\\:CC\\:00\\:00\\:09:signal-faux:6:2437 MHz:150:WPA2",
 " :AA\\:BB\\:CC\\:00\\:00\\:0A:canal-faux:six:2437 MHz:50:WPA2",
 "ligne:sans:assez:de:champs",
]
with open(sys.argv[1], "wb") as f:
    for l in L:
        f.write(l.encode("utf-8") + b"\n")
    f.write(b" :AA\\:BB\\:CC\\:00\\:00\\:0B:\xff\xfeinvalide:11:2462 MHz:10:WPA2\n")
PY
cat > "$T/faux/nmcli" <<'FAUX'
#!/bin/bash
printf 'nmcli %s\n' "$*" >> "$APPELS"
cat "$FAUX_SORTIE"
FAUX
chmod +x "$T/faux/nmcli"
export PATH="$T/faux:$PATH" FAUX_SORTIE="$T/sortie"
echec() { echo "ECHEC : $*"; exit 1; }

# ── 1 + 2 — liste ───────────────────────────────────────────────────────────
"$APP" --liste > "$T/l" || echec "--liste : code non nul"
grep -qx 'nmcli -t -f IN-USE,BSSID,SSID,CHAN,FREQ,SIGNAL,SECURITY device wifi list --rescan no' "$APPELS" || echec "arguments : $(cat "$APPELS")"
[[ $(wc -l < "$T/l") == 8 ]] || echec "8 réseaux valides attendus : $(cut -d'|' -f2 "$T/l" | tr '\n' ' ')"
[[ $(awk -F'|' '{print NF}' "$T/l" | sort -u) == 7 ]] || echec "une ligne n'a pas 7 colonnes : $(cat "$T/l")"
[[ $(cut -d'|' -f6 "$T/l" | tr -d ' %' | tr '\n' ' ') == "90 62 55 50 40 30 20 10 " ]] || echec "tri par signal : $(cut -d'|' -f6 "$T/l" | tr '\n' ' ')"
grep -qx 'oui|maison:5G|AA:BB:CC:00:00:01|36|5 GHz|62 %|WPA2 WPA3' "$T/l" || echec "réseau utilisé / « \\: » mal découpé : $(grep maison "$T/l")"
grep -qx '|cafe-ouvert|AA:BB:CC:00:00:03|6|2,4 GHz|40 %|aucune ⚠ ouvert' "$T/l" || echec "réseau ouvert non signalé : $(grep cafe "$T/l")"
grep -qx '|vieux|AA:BB:CC:00:00:04|1|2,4 GHz|30 %|WEP ⚠ faible' "$T/l" || echec "WEP non signalé"
grep -qx '|(masqué)|AA:BB:CC:00:00:05|11|2,4 GHz|20 %|WPA1 ⚠ faible' "$T/l" || echec "SSID masqué ou WPA1 : $(grep ':05|' "$T/l")"
grep -q '|six|AA:BB:CC:00:00:06|37|6 GHz|' "$T/l" || echec "bande 6 GHz"
P=$(grep ':07|' "$T/l" | cut -d'|' -f2)
[[ "$P" == 'a¦b<b>&amp;$(id)?[31m?'* && ${#P} -le 32 ]] || echec "SSID piégé : « $P » (${#P} car.)"
! LC_ALL=C grep -q $'[\x01-\x1f\x7f]' "$T/l" || echec "caractères de contrôle restés"
iconv -f UTF-8 -t UTF-8 "$T/l" >/dev/null 2>&1 || echec "UTF-8 invalide en sortie"
grep -q '|AA:BB:CC:00:00:0B|' "$T/l" || echec "SSID en UTF-8 invalide : réseau perdu au lieu d'être réparé"
! grep -qE 'bssid-faux|signal-faux|canal-faux' "$T/l" || echec "une ligne mal formée est passée"

# ── 3 — canaux, relance ─────────────────────────────────────────────────────
"$APP" --canaux > "$T/c"
[[ $(sed -n 1p "$T/c") == '2,4 GHz|6|3|90 %' ]] || echec "canal le plus chargé : $(sed -n 1p "$T/c")"
grep -qx '2,4 GHz|11|2|20 %' "$T/c" && grep -qx '5 GHz|36|1|62 %' "$T/c" && grep -qx '6 GHz|37|1|55 %' "$T/c" || echec "canaux : $(tr '\n' ' ' < "$T/c")"
: > "$APPELS"; "$APP" --liste relancer >/dev/null
grep -q -- '--rescan yes$' "$APPELS" || echec "« relancer » ne demande pas de balayage : $(cat "$APPELS")"
"$APP" --liste autre >/dev/null 2>&1; [[ $? == 64 ]] || echec "argument inconnu de --liste accepté"

# ── 4 — XML, codes ──────────────────────────────────────────────────────────
"$APP" --xml | python3 -I -c 'import sys,xml.dom.minidom as m; d=m.parseString(sys.stdin.read()); assert len(d.getElementsByTagName("tree"))==2' || echec "XML mal formé"
mkdir "$T/maigre"; for c in bash python3 timeout head sed cat; do ln -s "$(PATH=${PATH#*:} command -v $c)" "$T/maigre/$c"; done
env PATH="$T/maigre" "$APP" --liste >/dev/null 2>&1; [[ $? == 2 ]] || echec "nmcli absent : code ≠ 2"
"$APP" --inconnu >/dev/null 2>&1; [[ $? == 64 ]] || echec "option inconnue : code ≠ 64"

# ── 5 — binaire réel (facultatif) ──────────────────────────────────────────
if [[ -n "$BIN_REEL" ]]; then
    [[ -x "$BIN_REEL" ]] && command -v xvfb-run >/dev/null || { echo "binaire ou xvfb-run absent : étape 5 non faite" >&2; exit 77; }
    timeout -k 3 10 xvfb-run -a "$APP" "$BIN_REEL" > "$T/gui" 2>&1
    rc=$?
    [[ $rc == 124 ]] || echec "binaire réel : la fenêtre ne reste pas ouverte (rc=$rc) : $(tail -n 3 "$T/gui")"
    ! grep -qiE 'syntax error|parse error|unknown tag|erreur de syntaxe' "$T/gui" || echec "binaire réel : XML refusé"
    suite=" ; fenêtre réelle ouverte"
fi
echo "OK : liste triée, bandes, ouverts/faibles signalés, « \\: » découpé, SSID piégés neutralisés, lignes fausses ignorées, canaux, relance${suite:-}"
