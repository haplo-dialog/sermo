#!/bin/bash
# SPDX-License-Identifier: GPL-2.0-or-later
# tests/garde_bluetooth_scan.sh — haplo-dialog — 2026 — GPL-2.0-or-later
#
# examples/bluetooth-scan/ lance bluetoothctl et met en forme des noms d'appareils
# arbitraires. Ce test utilise un FAUX /sys/class/bluetooth (BT_SYSFS), un faux
# systemctl et un FAUX bluetoothctl (aucun vrai adaptateur, aucun voisin montré) :
#   1 — pas d'adaptateur / service arrêté : code 3 tout de suite, bluetoothctl JAMAIS
#       lancé (le vrai attendrait sans fin) ; durée invalide : 64
#   2 — balayage : arguments bornés ; signal en « -60 » et en « 0x… (-60) » ; codes
#       de couleur retirés ; tri par signal ; appairé / connecté ; sans nom
#   3 — noms piégés (|, balises, $(…), contrôles, très long) et lignes fausses
#       (adresse invalide, RSSI hors bornes) : 4 colonnes, rien de piégé
#   4 — XML bien formé ; bluetoothctl absent : 2 ; aucun dossier ne reste
# Avec un binaire réel en argument (+ xvfb-run) : 5 — la fenêtre s'ouvre et reste ouverte.
# Usage : garde_bluetooth_scan.sh [/chemin/vers/un-port-a-fenetre]
RACINE=$(CDPATH= cd -- "$(dirname -- "$0")/.." && pwd)
APP="$RACINE/examples/bluetooth-scan/bluetooth-scan.sh"
BIN_REEL="${1:-}"
for t in python3 timeout mktemp; do
    command -v "$t" >/dev/null || { echo "outil manquant : $t" >&2; exit 77; }
done
[[ -x "$APP" ]] || { echo "script introuvable ou non exécutable : $APP" >&2; exit 1; }
T=$(mktemp -d); trap 'rm -rf "$T"' EXIT
export XDG_RUNTIME_DIR="$T/run"; mkdir -m 700 "$XDG_RUNTIME_DIR"
export APPELS="$T/appels"; : > "$APPELS"
mkdir "$T/faux" "$T/sys"
export BT_SYSFS="$T/sys"
cat > "$T/faux/systemctl" <<'FAUX'
#!/bin/bash
[ "${FAKE_SERVICE:-actif}" = actif ]
FAUX
python3 -I - "$T/scan" <<'PY'
import sys
E = "\x1b[0;92m"; Y = "\x1b[0;93m"; Z = "\x1b[0m"
L = [
 "Discovery started",
 "[" + Y + "CHG" + Z + "] Controller 00:1A:7D:DA:71:13 Discovering: yes",
 "[" + E + "NEW" + Z + "] Device AA:BB:CC:00:00:01 Casque-Audio",
 "[" + Y + "CHG" + Z + "] Device AA:BB:CC:00:00:01 RSSI: -48",
 "[" + E + "NEW" + Z + "] Device AA:BB:CC:00:00:02 Montre",
 "[" + Y + "CHG" + Z + "] Device AA:BB:CC:00:00:02 RSSI: 0xffffffb5 (-75)",
 "[" + E + "NEW" + Z + "] Device AA:BB:CC:00:00:03 AA-BB-CC-00-00-03",
 "[" + Y + "CHG" + Z + "] Device AA:BB:CC:00:00:03 RSSI: -90",
 "[" + E + "NEW" + Z + "] Device AA:BB:CC:00:00:04 a|b<b>&amp;$(id)\x07\x1b[31m" + "x" * 60,
 "[" + Y + "CHG" + Z + "] Device AA:BB:CC:00:00:04 RSSI: -60",
 "[" + E + "NEW" + Z + "] Device ZZ:BB:CC:00:00:05 adresse-fausse",
 "[" + Y + "CHG" + Z + "] Device AA:BB:CC:00:00:01 RSSI: -500",
 "[" + Y + "CHG" + Z + "] Device AA:BB:CC:00:00:06 RSSI: 99",
 "pas une ligne",
]
open(sys.argv[1], "w").write("\r\n".join(L) + "\n")
PY
printf 'Device AA:BB:CC:00:00:01 Casque-Audio\nDevice AA:BB:CC:00:00:07 Clavier\n' > "$T/appaires"
printf 'Device AA:BB:CC:00:00:01 Casque-Audio\n' > "$T/connectes"
cat > "$T/faux/bluetoothctl" <<'FAUX'
#!/bin/bash
printf 'bluetoothctl %s\n' "$*" >> "$APPELS"
case "$*" in
  "--timeout "*" scan on") cat "$FAUX_DIR/scan" ;;
  "devices Paired")        cat "$FAUX_DIR/appaires" ;;
  "devices Connected")     cat "$FAUX_DIR/connectes" ;;
esac
FAUX
chmod +x "$T/faux/"*
export PATH="$T/faux:$PATH" FAUX_DIR="$T"
export BS_DIR="$T/bs"; mkdir -m 700 "$BS_DIR"
echec() { echo "ECHEC : $*"; exit 1; }

# ── 1 — pas prêt ────────────────────────────────────────────────────────────
: > "$APPELS"
[[ $("$APP" --etat) == "aucun adaptateur Bluetooth sur cette machine" ]] || echec "pas d'adaptateur mal dit"
"$APP" --balayer 10 >/dev/null 2>&1; [[ $? == 3 ]] || echec "pas d'adaptateur : code ≠ 3"
mkdir "$T/sys/hci0"
FAKE_SERVICE=arrete "$APP" --balayer 10 >/dev/null 2>&1; [[ $? == 3 ]] || echec "service arrêté : code ≠ 3"
[[ $(FAKE_SERVICE=arrete "$APP" --etat) == "service bluetooth arrêté"* ]] || echec "service arrêté mal dit"
[[ ! -s "$APPELS" ]] || echec "bluetoothctl lancé alors que rien n'est prêt : $(cat "$APPELS")"
for s in 4 31 x '10;id' -5; do "$APP" --balayer "$s" >/dev/null 2>&1; [[ $? == 64 ]] || echec "durée « $s » acceptée"; done
[[ $("$APP" --etat) == "adaptateur prêt"* ]] || echec "adaptateur prêt mal dit"

# ── 2 + 3 — balayage, mise en forme ─────────────────────────────────────────
"$APP" --balayer 12 || echec "balayage : code non nul"
grep -qx 'bluetoothctl --timeout 12 scan on' "$APPELS" || echec "arguments : $(cat "$APPELS")"
"$APP" --liste > "$T/l"
[[ $(awk -F'|' '{print NF}' "$T/l" | sort -u) == 4 ]] || echec "une ligne n'a pas 4 colonnes : $(cat "$T/l")"
[[ $(cut -d'|' -f1 "$T/l" | tr '\n' ' ') == "AA:BB:CC:00:00:01 AA:BB:CC:00:00:04 AA:BB:CC:00:00:02 AA:BB:CC:00:00:03 AA:BB:CC:00:00:07 " ]] \
    || echec "tri ou appareils : $(cut -d'|' -f1-3 "$T/l" | tr '\n' ' ')"
grep -qx 'AA:BB:CC:00:00:01|Casque-Audio|-48 dBm|connecté' "$T/l" || echec "casque : $(grep ':01|' "$T/l")"
grep -qx 'AA:BB:CC:00:00:02|Montre|-75 dBm|' "$T/l" || echec "RSSI en hexadécimal : $(grep ':02|' "$T/l")"
grep -qx 'AA:BB:CC:00:00:03|(sans nom)|-90 dBm|' "$T/l" || echec "nom = adresse : $(grep ':03|' "$T/l")"
! grep -q ':06|' "$T/l" || echec "appareil vu seulement avec un signal hors bornes : affiché quand même"
grep -q 'AA:BB:CC:00:00:01|[^|]*|-48 dBm' "$T/l" || echec "un signal hors bornes (-500) a écrasé le bon"
grep -qx 'AA:BB:CC:00:00:07|Clavier|—|appairé' "$T/l" || echec "appairé hors de portée : $(grep ':07|' "$T/l")"
N=$(grep ':04|' "$T/l" | cut -d'|' -f2)
# \x07 → « ? », séquence de couleur retirée
[[ "$N" == 'a¦b<b>&amp;$(id)?xxx'* && ${#N} -le 40 ]] || echec "nom piégé : « $N »"
! grep -q 'adresse-fausse' "$T/l" || echec "adresse invalide acceptée"
! LC_ALL=C grep -q $'[\x01-\x1f\x7f]' "$T/l" || echec "caractères de contrôle restés"

# ── 4 — XML, codes, ménage ──────────────────────────────────────────────────
"$APP" --xml | python3 -I -c 'import sys,xml.dom.minidom as m; m.parseString(sys.stdin.read())' || echec "XML mal formé"
mkdir "$T/maigre"; for c in bash python3 timeout find wc cat mktemp rm sed head date mv; do ln -s "$(PATH=${PATH#*:} command -v $c)" "$T/maigre/$c"; done
ln -s "$T/faux/systemctl" "$T/maigre/systemctl"
env PATH="$T/maigre" "$APP" --balayer 10 >/dev/null 2>&1; [[ $? == 2 ]] || echec "bluetoothctl absent : code ≠ 2"
( unset BS_DIR; "$APP" --liste >/dev/null )
[[ -z "$(find "$XDG_RUNTIME_DIR" -name 'sermo-bt.*')" ]] || echec "un dossier temporaire survit"

# ── 5 — binaire réel (facultatif) ──────────────────────────────────────────
if [[ -n "$BIN_REEL" ]]; then
    [[ -x "$BIN_REEL" ]] && command -v xvfb-run >/dev/null || { echo "binaire ou xvfb-run absent : étape 5 non faite" >&2; exit 77; }
    ( unset BS_DIR; timeout -k 3 10 xvfb-run -a "$APP" "$BIN_REEL" ) > "$T/gui" 2>&1
    rc=$?
    [[ $rc == 124 ]] || echec "binaire réel : la fenêtre ne reste pas ouverte (rc=$rc) : $(tail -n 3 "$T/gui")"
    ! grep -qiE 'syntax error|parse error|unknown tag|erreur de syntaxe' "$T/gui" || echec "binaire réel : XML refusé"
    suite=" ; fenêtre réelle ouverte"
fi
echo "OK : rien de lancé sans adaptateur ni service, balayage borné, RSSI décimal et hexadécimal, tri, appairé/connecté, noms piégés neutralisés${suite:-}"
