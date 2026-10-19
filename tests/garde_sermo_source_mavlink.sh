#!/bin/bash
# SPDX-License-Identifier: GPL-2.0-or-later
# tests/garde_sermo_source_mavlink.sh — haplo-dialog — 2026 — GPL-2.0-or-later
#
# Le VRAI programme d'aide de <input mavlink> (sermo-source/_build), joué contre
# le faux drone (examples/mavlink/faux-drone.py, pymavlink : une implémentation
# MAVLink indépendante de celle en C qu'on essaie), sur les trois liaisons :
# TCP (comme le simulateur SITL), UDP (comme un relais local), série (un
# pseudo-terminal, comme une radio USB).
#
# Vérifie : chaque champ lu exactement ; les émetteurs qui ne sont pas
# l'autopilote (station sol, caméra, avec de FAUSSES valeurs) ignorés ; armé,
# mode ; valeur « non mesurée » refusée et dite ; système absent ; champ
# inconnu, vitesse hors série, adresse fausse refusés tout de suite ; lien muet
# : main rendue dans le délai ; lien fermé ; jamais bloquant (valeur périmée
# rendue sous 200 ms, drone arrêté) ; LECTURE SEULE : le drone ne reçoit AUCUN
# octet (TCP et série).
#
# Usage : garde_sermo_source_mavlink.sh [racine du dépôt]
# Codes : 0 = tout tient · 1 = un défaut · 2 = outil ou programme manquant

set -u
export LC_ALL=C
RACINE=$(CDPATH= cd -- "${1:-$(dirname -- "$0")/..}" 2>/dev/null && pwd) || { echo "racine illisible" >&2; exit 2; }
AIDE="$RACINE/sermo-source/_build"
ML="$AIDE/sermo-source-mavlink"
DRONE="$RACINE/examples/mavlink/faux-drone.py"
[ -x "$ML" ] || { echo "programme d'aide absent : $ML (ci/construire.sh)" >&2; exit 2; }
[ -f "$DRONE" ] || { echo "faux drone absent : $DRONE" >&2; exit 2; }
python3 -I -c 'import pymavlink' 2>/dev/null || { echo "outil manquant : python3-pymavlink" >&2; exit 2; }

T=$(mktemp -d -p "$AIDE" banc-sermo-source-mavlink.XXXXXX) || exit 2
PIDS=()
trap 'for p in "${PIDS[@]}"; do kill "$p" 2>/dev/null; done; wait 2>/dev/null; rm -rf -- "$T"' EXIT
mkdir -m 700 "$T/run"; export XDG_RUNTIME_DIR="$T/run"

B=$((20000 + $$ % 20000)); PT=$B; PU=$((B + 1)); PM=$((B + 2)); PA=$((B + 3))
python3 -I "$DRONE" --tcp "$PT" --intrus --espion "$T/espion-tcp" >"$T/tcp.log" 2>&1 & PIDS+=($!); TCPD=$!
python3 -I "$DRONE" --udp "127.0.0.1:$PU" --intrus >"$T/udp.log" 2>&1 & PIDS+=($!)
python3 -I "$DRONE" --serie "$T/pts" --intrus --espion "$T/espion-serie" >"$T/serie.log" 2>&1 & PIDS+=($!)
python3 -I "$DRONE" --tcp "$PA" --arme --mode 5 --sans-courant >"$T/arme.log" 2>&1 & PIDS+=($!)
# Lien muet : accepte la connexion TCP, n'envoie rien.
python3 -I -c '
import socket,sys
s=socket.socket(); s.setsockopt(socket.SOL_SOCKET,socket.SO_REUSEADDR,1)
s.bind(("127.0.0.1",int(sys.argv[1]))); s.listen(16); c=[]
while True: c.append(s.accept()[0])' "$PM" >/dev/null 2>&1 & PIDS+=($!)
for i in $(seq 1 100); do
    n=0; for p in $PT $PM $PA; do (exec 3<>/dev/tcp/127.0.0.1/$p) 2>/dev/null && n=$((n + 1)); done
    [ -s "$T/pts" ] && n=$((n + 1))
    [ $n = 4 ] && break; sleep 0.1
done
[ $n = 4 ] || { echo "faux drones pas prêts"; tail -3 "$T"/*.log; exit 2; }
PTS=$(cat "$T/pts")

echecs=0
ko() { echo "ECHEC : $*"; echecs=$((echecs + 1)); }
ok() { echo "ok     : $*"; }
lire() { "$ML" "$1" 2>"$T/err"; }
ms() { date +%s%3N; }
attendu() {  # attendu LIBELLÉ ADRESSE VALEUR
    v=$(lire "$2"); rc=$?
    [ $rc = 0 ] && [ "$v" = "$3" ] && ok "$1 = $3" || ko "$1 : rc=$rc v=[$v] attendu [$3] $(cat "$T/err")"
}
refus() {  # refus LIBELLÉ ADRESSE RAISON [ms max]
    t0=$(ms); v=$(lire "$2"); rc=$?; d=$(( $(ms) - t0 ))
    [ $rc = 1 ] && [ -z "$v" ] && grep -q -- "$3" "$T/err" && [ $d -lt "${4:-300}" ] && ok "$1 ($d ms)" \
        || ko "$1 : rc=$rc v=[$v] en $d ms, raison : $(cat "$T/err")"
}

# 1. Chaque champ, en TCP, malgré les intrus (station sol, caméra : 9,90 V, -10…).
for cv in tension:12.60 courant:12.50 batterie:76 latitude:45.1234567 longitude:5.7654321 \
          altitude:245.50 hauteur:30.25 cap:90.00 vitesse:4.20 montee:0.50 satellites:11 fix:3 arme:0 mode:0; do
    attendu "tcp ${cv%%:*}" "tcp://127.0.0.1:$PT?champ=${cv%%:*}&cache=0" "${cv#*:}"
done
# systeme=1 écrit : la caméra du MÊME système (composant 100) parle avant le
# drone avec 9,90 V ; seul l'autopilote (composant 1) compte.
attendu "systeme=1 : la caméra du même système ignorée" "tcp://127.0.0.1:$PT?champ=tension&systeme=1&cache=0" 12.60
attendu "systeme=1 : position de l'autopilote" "tcp://127.0.0.1:$PT?champ=latitude&systeme=1&cache=0" 45.1234567
# 2. UDP et série.
attendu "udp tension" "udp://:$PU?champ=tension&cache=0" 12.60
attendu "udp latitude" "udp://127.0.0.1:$PU?champ=latitude&cache=0" 45.1234567
attendu "série tension" "serie://$PTS?champ=tension&cache=0&vitesse=57600" 12.60
attendu "série hauteur" "serie://$PTS?champ=hauteur&cache=0" 30.25
# 3. Armé, mode, valeur non mesurée.
attendu "drone armé" "tcp://127.0.0.1:$PA?champ=arme&cache=0" 1
attendu "mode annoncé" "tcp://127.0.0.1:$PA?champ=mode&cache=0" 5
refus "courant non mesuré refusé" "tcp://127.0.0.1:$PA?champ=courant&cache=0&delai=1000" "non mesuré" 1500
# 4. Système absent, refus d'adresse.
refus "système 2 absent" "tcp://127.0.0.1:$PT?champ=tension&systeme=2&cache=0&delai=600" "aucun autopilote" 1200
# Le lien parlait : pas de pause pour les autres lectures du même drone.
attendu "après un système absent, pas de pause" "tcp://127.0.0.1:$PT?champ=batterie&cache=0" 76
refus "champ inconnu" "tcp://127.0.0.1:$PT?champ=pression" "champ inconnu"
refus "champ manquant" "tcp://127.0.0.1:$PT?delai=300" "champ manquant"
refus "vitesse hors série" "udp://:$PU?champ=tension&vitesse=57600" "serie"
refus "paramètre inconnu" "tcp://127.0.0.1:$PT?champ=tension&chmap=x" "inconnu"
refus "transport inconnu" "http://127.0.0.1:$PT?champ=tension" "udp://"
# 5. Lien muet : main rendue dans le délai.
t0=$(ms); v=$(lire "tcp://127.0.0.1:$PM?champ=tension&cache=0&delai=800"); rc=$?; d=$(( $(ms) - t0 ))
[ $rc = 1 ] && [ $d -ge 700 ] && [ $d -lt 2000 ] && ok "lien muet : main rendue en $d ms (délai 800)" \
    || ko "lien muet : rc=$rc en $d ms $(cat "$T/err")"
# Rien reçu : c'est une panne, la pause est posée (pas de nouvel essai réseau).
refus "lien muet : pause après la panne" "tcp://127.0.0.1:$PM?champ=tension&cache=0&delai=800" "moins de 5 s"
# 6. Jamais bloquant : valeur gardée, puis drone arrêté.
A="tcp://127.0.0.1:$PT?champ=tension&cache=100&perime=10000"
lire "$A" >/dev/null; sleep 0.3
kill "$TCPD"; wait "$TCPD" 2>/dev/null
t0=$(ms); v=$(lire "$A"); rc=$?; d=$(( $(ms) - t0 ))
[ $rc = 0 ] && [ "$v" = 12.60 ] && [ $d -lt 200 ] && ok "valeur périmée rendue en $d ms, drone arrêté" \
    || ko "périmée : rc=$rc v=[$v] $d ms"
v=$(lire "tcp://127.0.0.1:$PT?champ=tension&cache=0"); rc=$?
[ $rc = 1 ] && [ -z "$v" ] && ok "drone arrêté, sans cache : échec dit" || ko "drone arrêté : rc=$rc v=[$v]"
# 7. LECTURE SEULE : rien n'a été envoyé au drone.
for e in tcp serie; do
    r=$(cat "$T/espion-$e" 2>/dev/null)
    [ "$r" = 0 ] && ok "lecture seule ($e) : le drone n'a reçu aucun octet" || ko "lecture seule ($e) : le drone a reçu [$r] octets"
done

echo
[ $echecs = 0 ] && { echo "sermo-source-mavlink : tout tient"; exit 0; }
echo "sermo-source-mavlink : $echecs défaut(s)"; exit 1
