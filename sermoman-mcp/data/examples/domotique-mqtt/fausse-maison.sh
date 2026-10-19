#!/bin/bash
# SPDX-License-Identifier: CC0-1.0
# fausse-maison.sh — un serveur MQTT d'essai pour domotique.sh, sur cette machine seule.
#
# Lance mosquitto sur 127.0.0.1 (aucun accès depuis le réseau), publie une
# température et une humidité qui bougent (messages retenus), et joue la lampe :
# ce qu'on publie sur PRÉFIXE/lampe (on|off) est recopié sur PRÉFIXE/lampe/etat,
# comme le ferait une vraie prise connectée. Les commandes reçues s'affichent ici.
# Ctrl+C arrête tout ; rien ne reste (PID exacts, dossier privé effacé).
#
# Usage : ./fausse-maison.sh            FAUSSE_MAISON_PORT (défaut 18830)
#                                       MAISON_PREFIXE (défaut maison/salon)
set -u
PORT="${FAUSSE_MAISON_PORT:-18830}"
PREFIXE="${MAISON_PREFIXE:-maison/salon}"
[[ "$PORT" =~ ^[1-9][0-9]{3,4}$ ]] && [ "$PORT" -le 65535 ] || { echo "port invalide : $PORT" >&2; exit 64; }
[[ "$PREFIXE" =~ ^[A-Za-z0-9_-]{1,64}(/[A-Za-z0-9_-]{1,64}){0,7}$ ]] || { echo "préfixe invalide : $PREFIXE" >&2; exit 64; }
MOSQ=$(command -v mosquitto || echo /usr/sbin/mosquitto)
[ -x "$MOSQ" ] || { echo "mosquitto absent : sudo apt install mosquitto mosquitto-clients" >&2; exit 2; }
for c in mosquitto_pub mosquitto_sub; do
	command -v "$c" >/dev/null || { echo "$c absent : sudo apt install mosquitto-clients" >&2; exit 2; }
done

umask 077
D=$(mktemp -d "${XDG_RUNTIME_DIR:-${TMPDIR:-/tmp}}/fausse-maison.XXXXXX") || exit 2
PIDS=()
arreter() { local p; for p in "${PIDS[@]}"; do kill "$p" 2>/dev/null; done; wait 2>/dev/null; rm -rf -- "$D"; }
trap arreter EXIT
trap 'exit 130' INT; trap 'exit 143' TERM HUP

printf 'listener %s 127.0.0.1\nallow_anonymous true\npersistence false\n' "$PORT" > "$D/mosquitto.conf"
"$MOSQ" -c "$D/mosquitto.conf" > "$D/mosquitto.log" 2>&1 & PIDS+=($!)
for _ in $(seq 1 50); do mosquitto_pub -p "$PORT" -t "$PREFIXE/essai" -n 2>/dev/null && break; timeout 0.1 tail -f /dev/null; done
mosquitto_pub -p "$PORT" -t "$PREFIXE/essai" -n 2>/dev/null || { echo "mosquitto ne répond pas : $(tail -n 2 "$D/mosquitto.log")" >&2; exit 2; }
mosquitto_pub -p "$PORT" -r -t "$PREFIXE/lampe/etat" -m off

# La lampe : chaque commande reçue devient son état.
# L'abonné est lancé à part (son PID est gardé) : dans un tube « a | b & », seul b
# serait connu, et a survivrait à l'arrêt.
exec 3< <(exec mosquitto_sub -p "$PORT" -t "$PREFIXE/lampe" -t "$PREFIXE/consigne" -v); PIDS+=($!)
while read -r sujet valeur <&3; do
	printf '%s  reçu %s = %s\n' "$(date +%T)" "$sujet" "$valeur"
	[ "$sujet" = "$PREFIXE/lampe" ] && mosquitto_pub -p "$PORT" -r -t "$PREFIXE/lampe/etat" -m "$valeur"
done & PIDS+=($!)
exec 3<&-

echo "fausse maison : mqtt://127.0.0.1:$PORT, sujets $PREFIXE/… (Ctrl+C pour arrêter)"
t=205 h=48
while :; do
	t=$(( t + RANDOM % 5 - 2 )); (( t < 180 )) && t=180; (( t > 240 )) && t=240
	h=$(( h + RANDOM % 3 - 1 )); (( h < 30 )) && h=30; (( h > 70 )) && h=70
	mosquitto_pub -p "$PORT" -r -t "$PREFIXE/temperature" -m "$((t / 10)).$((t % 10))"
	mosquitto_pub -p "$PORT" -r -t "$PREFIXE/humidite" -m "$h"
	timeout 2 tail -f /dev/null & wait $!     # interruptible : Ctrl+C ou TERM arrêtent tout de suite
done
