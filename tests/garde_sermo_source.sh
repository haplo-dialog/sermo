#!/bin/bash
# SPDX-License-Identifier: GPL-2.0-or-later
# tests/garde_sermo_source.sh — haplo-dialog — 2026 — GPL-2.0-or-later
#
# Les VRAIS programmes d'aide de <input modbus> et <input opcua>
# (sermo-source/, construits par ci/construire.sh), joués contre le faux
# automate (examples/modbus/faux-automate.py) et le faux serveur OPC UA
# (examples/opcua/faux-serveur-opcua.c, compilé ici), sur 127.0.0.1.
#
# Vérifie : une valeur lue (et divisée) ; un paramètre mal orthographié, un
# registre absent, un nœud absent refusés (code 1, rien sur la sortie) ;
# OPC UA chiffré par certificats lu, et refusé quand le certificat du serveur
# n'est pas dans la confiance ; jamais bloquant (valeur périmée rendue sous
# 200 ms et relue en fond, équipement muet qui ne fige qu'à la toute première
# lecture) ; cache commun : une réponse gardée est rendue
# même équipement arrêté ; TÉMOIN (cache=0) : équipement arrêté = échec, puis
# pause (réponse sans réseau) pendant 5 s.
#
# Usage : garde_sermo_source.sh [racine du dépôt]
# Codes : 0 = tout tient · 1 = un défaut · 2 = outil ou programme manquant

set -u
export LC_ALL=C
RACINE=$(CDPATH= cd -- "${1:-$(dirname -- "$0")/..}" 2>/dev/null && pwd) || { echo "racine illisible" >&2; exit 2; }
AIDE="$RACINE/sermo-source/_build"
MB="$AIDE/sermo-source-modbus"; OU="$AIDE/sermo-source-opcua"
for f in "$MB" "$OU"; do [ -x "$f" ] || { echo "programme d'aide absent : $f (ci/construire.sh)" >&2; exit 2; }; done
for t in python3 cc pkg-config openssl timeout; do command -v "$t" >/dev/null || { echo "outil manquant : $t" >&2; exit 2; }; done
python3 -I -c 'import pymodbus' 2>/dev/null || { echo "outil manquant : python3-pymodbus" >&2; exit 2; }

# Bac dans le dossier de construction, pas sous /tmp. La pause de 5 s vit dans
# XDG_RUNTIME_DIR : on lui en donne un à nous, pour ne pas toucher à celui de
# la session ni dépendre d'un essai précédent.
T=$(mktemp -d -p "$AIDE" banc-sermo-source.XXXXXX) || exit 2
PIDS=()
trap 'for p in "${PIDS[@]}"; do kill "$p" 2>/dev/null; done; wait 2>/dev/null; rm -rf -- "$T"' EXIT
mkdir -m 700 "$T/run"; export XDG_RUNTIME_DIR="$T/run"

cc -O1 -o "$T/faux-serveur-opcua" "$RACINE/examples/opcua/faux-serveur-opcua.c" $(pkg-config --cflags --libs open62541) -lm \
    2>"$T/cc.log" || { echo "faux serveur OPC UA : compilation impossible"; cat "$T/cc.log"; exit 2; }
bash "$RACINE/examples/opcua/certificats-essai.sh" "$T/certs" >/dev/null || exit 2
openssl req -x509 -newkey rsa:2048 -nodes -days 1 -subj /CN=autre -keyout "$T/certs/autre.cle.pem" \
    -out "$T/certs/autre.cert.pem" 2>/dev/null || exit 2

# Ports au hasard dans une plage haute : deux bancs en même temps ne se gênent pas.
B=$((20000 + $$ % 20000)); PM=$B; PO=$((B + 1)); PS=$((B + 2))
python3 -I "$RACINE/examples/modbus/faux-automate.py" "$PM" >"$T/modbus.log" 2>&1 & PIDS+=($!); AUTOMATE=$!
"$T/faux-serveur-opcua" "$PO" >"$T/opcua.log" 2>&1 & PIDS+=($!); SERVEUR=$!
C="$T/certs"
"$T/faux-serveur-opcua" "$PS" --securise "$C/serveur.cert.der" "$C/serveur.cle.der" "$C/client.cert.der" >"$T/sec.log" 2>&1 & PIDS+=($!)
# Attendre que les trois écoutent (10 s au plus).
for i in $(seq 1 100); do
    n=0; for p in $PM $PO $PS; do (exec 3<>/dev/tcp/127.0.0.1/$p) 2>/dev/null && n=$((n + 1)); done
    [ $n = 3 ] && break; sleep 0.1
done
[ $n = 3 ] || { echo "faux équipements pas prêts"; tail -3 "$T"/*.log; exit 2; }

enc() { python3 -I -c 'import sys,urllib.parse as u; print(u.quote(sys.argv[1], safe="ABCDEFGHIJKLMNOPQRSTUVWXYZabcdefghijklmnopqrstuvwxyz0123456789._:/=,+@-"))' "$1"; }
echecs=0
ko() { echo "ECHEC : $*"; echecs=$((echecs + 1)); }
# lire <programme> <adresse> : pose OUT et RC.
lire() { OUT=$(timeout 15 "$1" "$(enc "$2")" 2>"$T/err"); RC=$?; }
attendu() { # attendu <nom> <motif de la valeur>
    [ "$RC" = 0 ] && [[ "$OUT" =~ ^$2$ ]] || ko "$1 : code $RC, sortie « $OUT », erreur « $(tail -1 "$T/err") »"
}
refuse() { # refuse <nom> <motif du message>
    [ "$RC" = 1 ] && [ -z "$OUT" ] && grep -q -- "$2" "$T/err" || ko "$1 : code $RC, sortie « $OUT », erreur « $(tail -1 "$T/err") »"
}

M="tcp://127.0.0.1:$PM"
lire "$MB" "$M?registre=1";              attendu "modbus registre 1" '[0-9]+'
lire "$MB" "$M?registre=1&diviseur=10";  attendu "modbus diviseur" '[0-9]+(\.[0-9])?'
lire "$MB" "$M?registre=1&nombre=4";     attendu "modbus 4 valeurs" '[0-9]+'$'\n''[0-9]+'$'\n''[0-9]+'$'\n''[0-9]+'
lire "$MB" "$M?regsitre=1";              refuse "modbus faute de frappe" 'paramètre inconnu'
lire "$MB" "$M?registre=500";            refuse "modbus registre absent" 'lecture refusée'

O="opc.tcp://127.0.0.1:$PO"
lire "$OU" "$O?noeud=ns=1;s=pieces";     attendu "opcua pièces" '[0-9]+'
lire "$OU" "$O?noeud=ns=1;s=temperature"; attendu "opcua température" '[0-9]+(\.[0-9]+)?'
lire "$OU" "$O?noeud=ns=1;s=absent";     refuse "opcua nœud absent" 'BadNodeIdUnknown'
S="opc.tcp://127.0.0.1:$PS?noeud=ns=1;s=pression&mode=chiffre&certificat=$C/client.cert.der&cle=$C/client.cle.der"
lire "$OU" "$S&confiance=$C/serveur.cert.der";  attendu "opcua chiffré" '[0-9]+'
lire "$OU" "$S&confiance=$C/autre.cert.pem";    refuse "opcua serveur hors confiance" 'BadCertificateUntrusted'

lire "$MB" "$M?registre=1&cache=-1";     refuse "modbus cache négatif" 'cache doit'

# Jamais bloquant (R3), équipement en marche : une valeur périmée est rendue
# tout de suite et relue en fond ; la relecture apporte bien une valeur neuve.
R="$M?registre=1&cache=200&perime=60000"
lire "$MB" "$R"; attendu "R3 première lecture" '[0-9]+'; V1=$OUT
sleep 1.2
t0=$(date +%s%N); lire "$MB" "$R"; ms=$(( ($(date +%s%N) - t0) / 1000000 ))
attendu "R3 valeur périmée rendue" "$V1"
[ "$ms" -lt 200 ] || ko "R3 valeur périmée : $ms ms (attendu < 200)"
neuve=""
for i in $(seq 1 20); do lire "$MB" "$R"; [ "$RC" = 0 ] && [ "$OUT" != "$V1" ] && { neuve=$OUT; break; }; sleep 0.05; done
[ -n "$neuve" ] || ko "R3 relecture en fond : aucune valeur neuve après 1 s (toujours « $V1 »)"
lire "$MB" "$M?registre=3&cache=300&perime=60000"; attendu "R3 avant arrêt" '[0-9]+'; V3=$OUT

# Cache commun : une réponse gardée 60 s est rendue même équipement arrêté ;
# avec cache=0, l'équipement est interrogé (témoin) et l'arrêt se voit.
lire "$MB" "$M?registre=2&cache=60000";  attendu "modbus mis en cache" '[0-9]+'; MV=$OUT
lire "$OU" "$O?noeud=ns=1;s=pression&cache=60000"; attendu "opcua mis en cache" '[0-9]+'; OV=$OUT
kill "$AUTOMATE" "$SERVEUR"; wait "$AUTOMATE" "$SERVEUR" 2>/dev/null
lire "$MB" "$M?registre=2&cache=60000";  attendu "modbus servi par le cache" "$MV"
lire "$OU" "$O?noeud=ns=1;s=pression&cache=60000"; attendu "opcua servi par le cache" "$OV"

# Jamais bloquant (R3), équipement arrêté : la valeur périmée reste rendue
# tout de suite ; au-delà de « perime », plus rien n'est rendu, sans attendre.
sleep 0.4
t0=$(date +%s%N); lire "$MB" "$M?registre=3&cache=300&perime=60000"; ms=$(( ($(date +%s%N) - t0) / 1000000 ))
attendu "R3 arrêté, valeur périmée rendue" "$V3"
[ "$ms" -lt 200 ] || ko "R3 arrêté, valeur périmée : $ms ms (attendu < 200)"

# Jamais bloquant (R3), équipement MUET (accepte, ne répond jamais) : la toute
# première lecture attend le délai (un dialogue sans minuteur a sa valeur) ;
# ensuite, plus jamais : pause, puis relecture en fond à la reprise.
python3 -I -c 'import socket,sys
s=socket.socket(); s.setsockopt(socket.SOL_SOCKET, socket.SO_REUSEADDR, 1)
s.bind(("127.0.0.1", int(sys.argv[1]))); s.listen(64); g=[]
while True: c,_=s.accept(); g.append(c)' $((B + 3)) & PIDS+=($!)
for i in $(seq 1 50); do (exec 3<>/dev/tcp/127.0.0.1/$((B + 3))) 2>/dev/null && break; sleep 0.1; done
MU="tcp://127.0.0.1:$((B + 3))?registre=1&delai=800"
t0=$(date +%s%N); lire "$MB" "$MU"; ms=$(( ($(date +%s%N) - t0) / 1000000 ))
refuse "R3 muet, première lecture" 'Connection timed out'
[ "$ms" -ge 700 ] || ko "R3 muet, première lecture : $ms ms (la première lecture doit attendre le délai, ~800)"
t0=$(date +%s%N); lire "$MB" "$MU"; ms=$(( ($(date +%s%N) - t0) / 1000000 ))
refuse "R3 muet, en pause" 'pas de nouvel essai\|pas de valeur récente'
[ "$ms" -lt 200 ] || ko "R3 muet, en pause : $ms ms (attendu < 200)"
sleep 5.2
t0=$(date +%s%N); lire "$MB" "$MU"; ms=$(( ($(date +%s%N) - t0) / 1000000 ))
refuse "R3 muet, après la pause" 'pas de valeur récente'
[ "$ms" -lt 200 ] || ko "R3 muet, après la pause : $ms ms (attendu < 200 : la relecture part en fond)"

# Témoin et pause : équipement arrêté, sans cache.
lire "$MB" "$M?registre=1&cache=0";      refuse "modbus arrêté" 'connexion'
lire "$MB" "$M?registre=1&cache=0";      refuse "modbus arrêté, en pause" 'pas de nouvel essai'
lire "$OU" "$O?noeud=ns=1;s=pieces&cache=0"; refuse "opcua arrêté" 'connexion'
lire "$OU" "$O?noeud=ns=1;s=pieces&cache=0"; refuse "opcua arrêté, en pause" 'pas de nouvel essai'

[ "$echecs" = 0 ] && echo "OK : programmes d'aide modbus et opcua (19 cas + 11 jamais bloquant)"
[ "$echecs" = 0 ]
