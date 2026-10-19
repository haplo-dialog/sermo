#!/bin/bash
# SPDX-License-Identifier: GPL-2.0-or-later
# tests/garde_sermo_source_mqtt.sh — haplo-dialog — 2026 — GPL-2.0-or-later
#
# Le VRAI programme d'aide de <input mqtt> (sermo-source/_build, construit par
# ci/construire.sh), joué contre deux serveurs mosquitto à nous, lancés ici sur
# 127.0.0.1 (un sans mot de passe, un avec), jamais celui du système.
#
# Vérifie : le message retenu lu ; le message suivant lu quand rien n'est retenu ;
# sujet sans message = refus dans le délai ; jokers « + » « # », paramètre
# inconnu refusés (code 1, rien sur la sortie) ; « localhost » (::1 puis
# 127.0.0.1) lu ; serveur muet (accepte
# la connexion, ne répond jamais) : rend la main dans le délai ; identifiants :
# fichier en 600 accepté, mot de passe faux refusé, fichier lisible par d'autres
# refusé AVANT toute connexion ; le mot de passe n'est jamais dans les
# arguments ; jamais bloquant (valeur périmée rendue sous 200 ms) ; TÉMOIN
# (cache=0) : serveur arrêté = échec, puis pause de 5 s sans réseau ; aucun
# message publié par le programme d'aide (lecture seule).
# CHIFFRÉ (mqtts://, trois serveurs à nous) : lu avec la bonne autorité ;
# refusé avec une autre autorité, avec un certificat au nom d'un autre serveur,
# sans confiance= (aucun mode « tout accepter ») ; confiance= sur mqtt:// refusé ;
# serveur qui exige un certificat du poste : refusé sans lui, lu avec lui, clé
# lisible par d'autres refusée avant toute connexion.
#
# Usage : garde_sermo_source_mqtt.sh [racine du dépôt]
# Codes : 0 = tout tient · 1 = un défaut · 2 = outil ou programme manquant

set -u
export LC_ALL=C
RACINE=$(CDPATH= cd -- "${1:-$(dirname -- "$0")/..}" 2>/dev/null && pwd) || { echo "racine illisible" >&2; exit 2; }
AIDE="$RACINE/sermo-source/_build"
MQ="$AIDE/sermo-source-mqtt"
[ -x "$MQ" ] || { echo "programme d'aide absent : $MQ (ci/construire.sh)" >&2; exit 2; }
MOSQ=$(command -v mosquitto || echo /usr/sbin/mosquitto)
[ -x "$MOSQ" ] || { echo "outil manquant : mosquitto" >&2; exit 2; }
for t in mosquitto_pub mosquitto_sub mosquitto_passwd python3 timeout openssl; do
    command -v "$t" >/dev/null || { echo "outil manquant : $t" >&2; exit 2; }
done

# Bac dans le dossier de construction, pas sous /tmp ; XDG_RUNTIME_DIR à nous
# (cache et pause y vivent).
T=$(mktemp -d -p "$AIDE" banc-sermo-source-mqtt.XXXXXX) || exit 2
PIDS=()
trap 'for p in "${PIDS[@]}"; do kill "$p" 2>/dev/null; done; wait 2>/dev/null; rm -rf -- "$T"' EXIT
mkdir -m 700 "$T/run"; export XDG_RUNTIME_DIR="$T/run"

B=$((20000 + $$ % 20000)); P1=$B; P2=$((B + 1)); PM=$((B + 2)); PA=$((B + 3))
PT=$((B + 4)); PN=$((B + 5)); PC=$((B + 6))

# Certificats d'essai : une autorité, un serveur signé par elle (localhost et
# 127.0.0.1), un serveur signé par elle mais au nom d'un autre, un poste client
# signé par elle, et une autorité étrangère.
C="$T/certs"; mkdir -m 700 "$C"
cert() {  # cert NOM SAN : clé + certificat signés par l'autorité
    openssl req -newkey rsa:2048 -nodes -subj "/CN=$1" -keyout "$C/$1.cle" -out "$C/$1.csr" 2>/dev/null &&
    printf 'subjectAltName=%s\n' "$2" >"$C/$1.ext" &&
    openssl x509 -req -in "$C/$1.csr" -CA "$C/ac.pem" -CAkey "$C/ac.cle" -CAcreateserial -days 1 \
        -extfile "$C/$1.ext" -out "$C/$1.pem" 2>/dev/null
}
openssl req -x509 -newkey rsa:2048 -nodes -days 1 -subj /CN=essai-ac -keyout "$C/ac.cle" -out "$C/ac.pem" 2>/dev/null || exit 2
openssl req -x509 -newkey rsa:2048 -nodes -days 1 -subj /CN=autre-ac -keyout "$C/autre.cle" -out "$C/autre.pem" 2>/dev/null || exit 2
cert serveur "DNS:localhost,IP:127.0.0.1" && cert ailleurs "DNS:ailleurs.example" && cert poste "DNS:poste" || exit 2
chmod 600 "$C"/*.cle
tls_conf() {  # tls_conf PORT CERT [exiger]
    printf 'listener %s 127.0.0.1\nallow_anonymous true\npersistence false\ncafile %s\ncertfile %s\nkeyfile %s\n' \
        "$1" "$C/ac.pem" "$C/$2.pem" "$C/$2.cle"
    [ "${3:-}" = exiger ] && printf 'require_certificate true\n'
    printf 'user %s\n' "$(id -un)"   # en root (CI) : garder le compte, voir plus bas
    return 0
}
printf 'listener %s 127.0.0.1\nallow_anonymous true\npersistence false\n' "$P1" >"$T/libre.conf"
mosquitto_passwd -c -b "$T/mdp" maison secret123 >/dev/null 2>&1 || exit 2
chmod 600 "$T/mdp"
printf 'listener %s 127.0.0.1\nallow_anonymous false\npassword_file %s\npersistence false\n' "$P2" "$T/mdp" >"$T/auth.conf"
# Lancé en root (conteneur de CI), mosquitto passe sous le compte « mosquitto »
# et ne lit plus nos clés en 600 : il garde le compte qui l'a lancé.
for f in "$T"/*.conf; do printf 'user %s\n' "$(id -un)" >>"$f"; done
"$MOSQ" -c "$T/libre.conf" >"$T/libre.log" 2>&1 & PIDS+=($!); LIBRE=$!
"$MOSQ" -c "$T/auth.conf" >"$T/auth.log" 2>&1 & PIDS+=($!)
tls_conf "$PT" serveur >"$T/tls.conf"; tls_conf "$PN" ailleurs >"$T/nom.conf"; tls_conf "$PC" serveur exiger >"$T/client.conf"
for c in tls nom client; do "$MOSQ" -c "$T/$c.conf" >"$T/$c.log" 2>&1 & PIDS+=($!); done
# Serveurs muets : acceptent la connexion TCP, ne répondent jamais. PM pour le
# délai (cas 5), PA pour relever les arguments (cas 6, base sans pause).
for p in "$PM" "$PA"; do
python3 -I -c '
import socket,sys,time
s=socket.socket(); s.setsockopt(socket.SOL_SOCKET,socket.SO_REUSEADDR,1)
s.bind(("127.0.0.1",int(sys.argv[1]))); s.listen(16); c=[]
while True: c.append(s.accept()[0])' "$p" >/dev/null 2>&1 & PIDS+=($!)
done
for i in $(seq 1 100); do
    n=0; for p in $P1 $P2 $PM $PA $PT $PN $PC; do (exec 3<>/dev/tcp/127.0.0.1/$p) 2>/dev/null && n=$((n + 1)); done
    [ $n = 7 ] && break; sleep 0.1
done
[ $n = 7 ] || { echo "serveurs d'essai pas prêts"; tail -3 "$T"/*.log; exit 2; }

echecs=0
ko() { echo "ECHEC : $*"; echecs=$((echecs + 1)); }
ok() { echo "ok     : $*"; }
lire() { "$MQ" "$1" 2>"$T/err"; }
ms() { date +%s%3N; }

mosquitto_pub -p "$P1" -t maison/salon/temperature -m 21.5 -r || exit 2
mosquitto_pub -p "$P2" -u maison -P secret123 -t maison/cave -m 12.0 -r || exit 2

# Témoin de lecture seule : on écoute tout ce qui passe sur le serveur libre.
mosquitto_sub -p "$P1" -t '#' -v >"$T/tout.log" 2>/dev/null & PIDS+=($!); ESPION=$!
sleep 0.3

# 1. message retenu
v=$(lire "mqtt://127.0.0.1:$P1?sujet=maison/salon/temperature&cache=0"); rc=$?
[ $rc = 0 ] && [ "$v" = 21.5 ] && ok "message retenu lu (21.5)" || ko "retenu : rc=$rc v=[$v] $(cat "$T/err")"

# 2. message suivant (rien de retenu)
( sleep 0.4; mosquitto_pub -p "$P1" -t maison/porte -m fermee ) &
v=$(lire "mqtt://127.0.0.1:$P1?sujet=maison/porte&delai=3000&cache=0"); rc=$?; wait $!
[ $rc = 0 ] && [ "$v" = fermee ] && ok "message suivant lu (fermee)" || ko "suivant : rc=$rc v=[$v] $(cat "$T/err")"

# 3. sujet sans message : refus dans le délai
t0=$(ms); v=$(lire "mqtt://127.0.0.1:$P1?sujet=maison/vide&delai=300&cache=0"); rc=$?; d=$(( $(ms) - t0 ))
[ $rc = 1 ] && [ -z "$v" ] && [ $d -lt 1500 ] && ok "sujet muet : refus en $d ms" || ko "sujet muet : rc=$rc v=[$v] $d ms"

# 4. refus d'adresse : immédiat (avant toute connexion) et dit avec sa raison.
# Sans le refus du joker, l'abonnement à « maison/# » finirait aussi en échec,
# mais au bout du délai et pour une autre raison : on exige les deux.
for cas in "sujet=maison/%23|jokers" "sujet=maison/%2B/t|jokers" "sujet=x&sujett=y|inconnu" "delai=300|sujet manquant"; do
    a=${cas%%|*}; raison=${cas#*|}
    t0=$(ms); v=$(lire "mqtt://127.0.0.1:$P1?$a"); rc=$?; d=$(( $(ms) - t0 ))
    if [ $rc = 1 ] && [ -z "$v" ] && [ $d -lt 300 ] && grep -q "$raison" "$T/err"; then
        ok "refusé : $a ($raison, $d ms)"
    else
        ko "refus de $a : rc=$rc en $d ms, raison lue : $(cat "$T/err")"
    fi
done
# « localhost » : ::1 d'abord (rien n'y écoute), puis 127.0.0.1. La connexion
# non bloquante n'essayait que la première adresse (défaut de D1, 2026-10-17).
v=$(lire "mqtt://localhost:$P1?sujet=maison/salon/temperature&cache=0"); rc=$?
[ $rc = 0 ] && [ "$v" = 21.5 ] && ok "localhost (::1 puis 127.0.0.1) lu" || ko "localhost : rc=$rc v=[$v] $(cat "$T/err")"

# 5. serveur muet : la main rendue dans le délai
t0=$(ms); v=$(lire "mqtt://127.0.0.1:$PM?sujet=x&delai=800&cache=0"); rc=$?; d=$(( $(ms) - t0 ))
[ $rc = 1 ] && [ $d -ge 700 ] && [ $d -lt 2000 ] && ok "serveur muet : main rendue en $d ms (délai 800)" \
    || ko "serveur muet : rc=$rc en $d ms"

# 6. identifiants
printf 'maison:secret123\n' >"$T/id-bon"; chmod 600 "$T/id-bon"
printf 'maison:faux\n' >"$T/id-faux"; chmod 600 "$T/id-faux"
printf 'maison:secret123\n' >"$T/id-ouvert"; chmod 644 "$T/id-ouvert"
v=$(lire "mqtt://127.0.0.1:$P2?sujet=maison/cave&cache=0&clair=oui&identifiants=$T/id-bon"); rc=$?
[ $rc = 0 ] && [ "$v" = 12.0 ] && ok "identifiants (600) acceptés" || ko "identifiants bons : rc=$rc v=[$v] $(cat "$T/err")"
v=$(lire "mqtt://127.0.0.1:$P2?sujet=maison/cave&cache=0&clair=oui&identifiants=$T/id-faux"); rc=$?
[ $rc = 1 ] && [ -z "$v" ] && ok "mot de passe faux refusé" || ko "mot de passe faux : rc=$rc"
v=$(lire "mqtt://127.0.0.1:$P2?sujet=maison/cave&cache=0&clair=oui&identifiants=$T/id-ouvert"); rc=$?
[ $rc = 1 ] && grep -q 'chmod 600' "$T/err" && ok "fichier lisible par d'autres refusé" || ko "fichier 644 : rc=$rc $(cat "$T/err")"
v=$(lire "mqtt://127.0.0.1:$P2?sujet=maison/cave&cache=0&clair=oui&identifiants=id-bon"); rc=$?
[ $rc = 1 ] && grep -q "chemin absolu" "$T/err" && ok "chemin d'identifiants relatif refusé" || ko "chemin relatif accepté"
# Sur mqtt:// (non chiffré), un mot de passe partirait en clair : refusé sans
# clair=oui, et rien n'est envoyé au serveur (le refus vient avant la connexion).
v=$(lire "mqtt://127.0.0.1:$P2?sujet=maison/cave&cache=0&identifiants=$T/id-bon"); rc=$?
[ $rc = 1 ] && [ -z "$v" ] && grep -q 'passeraient en clair' "$T/err" && ok "identifiants sur mqtt:// sans clair=oui refusés" \
    || ko "identifiants en clair acceptés sans clair=oui : rc=$rc v=[$v] $(cat "$T/err")"
v=$(lire "mqtt://127.0.0.1:$P2?sujet=maison/cave&cache=0&clair=non&identifiants=$T/id-bon"); rc=$?
[ $rc = 1 ] && grep -q "n'admet que" "$T/err" && ok "clair=non refusé (seul « oui » est admis)" || ko "clair=non : rc=$rc $(cat "$T/err")"
# Le mot de passe n'est jamais un argument : on relève la ligne de commande
# du programme d'aide pendant qu'il attend un serveur muet (PA : aucune pause
# posée sur cette base).
"$MQ" "mqtt://127.0.0.1:$PA?sujet=x&delai=1500&cache=0&clair=oui&identifiants=$T/id-bon" 2>/dev/null & q=$!
sleep 0.3; tr '\0' ' ' </proc/$q/cmdline >"$T/cmdline" 2>/dev/null; wait $q
if ! grep -q 'identifiants=' "$T/cmdline" 2>/dev/null; then
    ko "ligne de commande non relevée (le contrôle n'a rien vu)"
elif grep -q secret123 "$T/cmdline"; then
    ko "mot de passe visible dans les arguments"
else
    ok "mot de passe absent des arguments (ligne de commande relevée)"
fi

# 6 bis. CHIFFRÉ
mosquitto_pub -p "$PT" --cafile "$C/ac.pem" -h 127.0.0.1 -t maison/cave -m 12.0 -r || exit 2
mosquitto_pub -p "$PC" --cafile "$C/ac.pem" --cert "$C/poste.pem" --key "$C/poste.cle" -h 127.0.0.1 -t maison/coffre -m clos -r || exit 2
tls_ok() {  # tls_ok LIBELLÉ ADRESSE VALEUR
    v=$(lire "$2"); rc=$?
    [ $rc = 0 ] && [ "$v" = "$3" ] && ok "$1" || ko "$1 : rc=$rc v=[$v] $(cat "$T/err")"
}
tls_refus() {  # tls_refus LIBELLÉ ADRESSE RAISON
    v=$(lire "$2"); rc=$?
    [ $rc = 1 ] && [ -z "$v" ] && grep -q "$3" "$T/err" && ok "$1" || ko "$1 : rc=$rc v=[$v] $(cat "$T/err")"
}
tls_ok "chiffré : bonne autorité (127.0.0.1)" "mqtts://127.0.0.1:$PT?sujet=maison/cave&cache=0&confiance=$C/ac.pem" 12.0
tls_ok "chiffré : bonne autorité (localhost)" "mqtts://localhost:$PT?sujet=maison/cave&cache=0&confiance=$C/ac.pem" 12.0
tls_refus "chiffré : autre autorité refusée" "mqtts://127.0.0.1:$PT?sujet=maison/cave&cache=0&confiance=$C/autre.pem" "chiffré refusé"
tls_refus "chiffré : certificat au nom d'un autre serveur refusé" "mqtts://127.0.0.1:$PN?sujet=maison/cave&cache=0&confiance=$C/ac.pem" "chiffré refusé"
tls_refus "chiffré : sans confiance= refusé (aucun « tout accepter »)" "mqtts://127.0.0.1:$PT?sujet=maison/cave&cache=0" "tout accepter"
tls_refus "clair=oui sur mqtts:// refusé" "mqtts://127.0.0.1:$PT?sujet=maison/cave&cache=0&confiance=$C/ac.pem&clair=oui" "ne vaut qu'avec mqtt://"
tls_refus "confiance= sur mqtt:// refusé" "mqtt://127.0.0.1:$P1?sujet=x&confiance=$C/ac.pem" "mqtts"
tls_refus "certificat sans cle refusé" "mqtts://127.0.0.1:$PC?sujet=x&confiance=$C/ac.pem&certificat=$C/poste.pem" "ensemble"
tls_refus "serveur exigeant : sans certificat du poste refusé" "mqtts://127.0.0.1:$PC?sujet=maison/coffre&cache=0&confiance=$C/ac.pem" "chiffré refusé"
tls_ok "serveur exigeant : certificat du poste accepté" "mqtts://127.0.0.1:$PC?sujet=maison/coffre&cache=0&confiance=$C/ac.pem&certificat=$C/poste.pem&cle=$C/poste.cle" clos
cp "$C/poste.cle" "$C/poste-ouverte.cle"; chmod 644 "$C/poste-ouverte.cle"
tls_refus "clé du poste lisible par d'autres refusée" "mqtts://127.0.0.1:$PC?sujet=maison/coffre&cache=0&confiance=$C/ac.pem&certificat=$C/poste.pem&cle=$C/poste-ouverte.cle" "chmod 600"

# 7. jamais bloquant : valeur gardée puis serveur arrêté
A="mqtt://127.0.0.1:$P1?sujet=maison/salon/temperature&cache=100&perime=10000"
lire "$A" >/dev/null
sleep 0.3
kill "$LIBRE"; wait "$LIBRE" 2>/dev/null
t0=$(ms); v=$(lire "$A"); rc=$?; d=$(( $(ms) - t0 ))
[ $rc = 0 ] && [ "$v" = 21.5 ] && [ $d -lt 200 ] && ok "valeur périmée rendue en $d ms, serveur arrêté" \
    || ko "périmée : rc=$rc v=[$v] $d ms"

# 8. TÉMOIN : sans cache, serveur arrêté = échec, puis pause sans réseau
v=$(lire "mqtt://127.0.0.1:$P1?sujet=maison/salon/temperature&cache=0"); rc=$?
[ $rc = 1 ] && [ -z "$v" ] && ok "témoin : serveur arrêté, sans cache = échec" || ko "témoin : rc=$rc v=[$v]"
v=$(lire "mqtt://127.0.0.1:$P1?sujet=maison/salon/temperature&cache=0"); rc=$?
[ $rc = 1 ] && grep -q 'moins de 5 s' "$T/err" && ok "pause après échec (pas de nouvel essai)" || ko "pause : $(cat "$T/err")"

# 9. lecture seule : l'espion n'a vu que nos propres publications
sleep 0.2; kill "$ESPION" 2>/dev/null
autres=$(grep -v -e '^maison/salon/temperature 21.5$' -e '^maison/porte fermee$' "$T/tout.log" | head -3)
[ -z "$autres" ] && ok "lecture seule : aucun message publié par le programme d'aide" || ko "messages inattendus : $autres"

echo
[ $echecs = 0 ] && { echo "sermo-source-mqtt : tout tient"; exit 0; }
echo "sermo-source-mqtt : $echecs défaut(s)"; exit 1
