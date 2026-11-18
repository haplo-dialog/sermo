#!/bin/bash
# SPDX-License-Identifier: GPL-2.0-or-later
# tests/garde_sermo_cible_mavlink.sh — haplo-dialog — 2026 — GPL-2.0-or-later
#
# Le VRAI programme d'écriture de <action mavlink> (sermo-source/_build/sermo-cible-mavlink),
# joué contre le FAUX drone (examples/mavlink/faux-drone.py, pymavlink), qui
# note le nom de chaque message reçu (--journal) : chaque REFUS doit n'envoyer
# AUCUNE commande (ni PARAM_SET, ni COMMAND_LONG). Ce banc tourne partout, CI
# comprise ; les écritures RÉUSSIES (le drone règle, accuse) se jouent contre le
# simulateur ArduPilot : garde_sermo_cible_mavlink_sitl.sh.
#
# Liste blanche : comme garde_sermo_cible_mqtt.sh, chaque cas tourne dans un
# espace isolé (unshare -rm, /etc recouvert, /root en tmpfs) ; en CI, en root
# dans un conteneur jetable, les listes sont écrites en direct.
#
# Vérifie : aucune liste = refus ; liste personnelle lisible par d'autres,
# liste système modifiable par d'autres, ligne mal formée, famille interdite
# DANS la liste : tout refusé ; famille interdite dans l'adresse (ARMING_,
# FLTMODE, SERVOn_FUNCTION, BRD_SAFETY, FENCE_) ; hors intervalle ; $VARIABLE
# hors intervalle ; valeur non numérique ; ordre hors liste ; ordre inconnu ;
# paramètre ET ordre ; $HOME ne déplace pas la liste ; drone ARMÉ : réglage
# refusé ; une STATION SOL parle déjà : refusé ; paramètre inconnu du drone ;
# lien muet ; port fermé ; coupure des moteurs : un appui n'envoie RIEN et le
# dit (fichier message), un second appui hors délai n'envoie rien non plus.
# MISSIONS : fichier hors format, élément interdit, repère faux, point trop
# loin, trop haut, sous le sol, plus de 100 points, fichier modifiable par
# d'autres, lien symbolique, chemin relatif, pas de ligne « mission », ligne
# mission hors bornes, drone armé, autre station : rien n'est envoyé ; une
# mission permise passe toutes les gardes (le faux drone ne répond pas : « s'est tu »).
# ALLER À : clés manquantes ou en trop, nombres faux, point ou hauteur hors
# barrière, premier appui, autre point, drone au sol, hors GUIDED, barrière de
# l'autopilote coupée : rien n'est envoyé ; sinon la cible part une fois, en
# cadre 6 avec le masque 0x0DF8.
#
# Usage : garde_sermo_cible_mavlink.sh [racine du dépôt]
# Codes : 0 = tout tient · 1 = un défaut · 2 = outil ou programme manquant

set -u
export LC_ALL=C
RACINE=$(CDPATH= cd -- "${1:-$(dirname -- "$0")/..}" 2>/dev/null && pwd) || { echo "racine illisible" >&2; exit 2; }
AIDE="$RACINE/sermo-source/_build"
PROG="$AIDE/sermo-cible-mavlink"
DRONE="$RACINE/examples/mavlink/faux-drone.py"
[ -x "$PROG" ] || { echo "programme d'écriture absent : $PROG (ci/construire.sh)" >&2; exit 2; }
[ -f "$DRONE" ] || { echo "faux drone absent : $DRONE" >&2; exit 2; }
python3 -I -c 'import pymavlink' 2>/dev/null || { echo "outil manquant : python3-pymavlink" >&2; exit 2; }
command -v unshare >/dev/null || { echo "outil manquant : unshare" >&2; exit 2; }
# L'espace isolé doit pouvoir MONTER (tmpfs sur /root, overlay sur /etc) :
# dans un conteneur de CI, unshare réussit souvent alors que l'overlay est
# refusé — chaque cas échouerait alors avant de commencer (vu sur la CI
# GitLab le 2026-10-20). On essaie donc le vrai montage.
essai_isole() {
    local d; d=$(mktemp -d -p "$AIDE" essai-isole.XXXXXX) || return 1
    unshare -rm sh -c 'mkdir "$1/u" "$1/w" && mount -t tmpfs tmpfs /root \
        && mount -t overlay overlay -o "lowerdir=/etc,upperdir=$1/u,workdir=$1/w" /etc' _ "$d" 2>/dev/null
    local rc=$?; rm -rf -- "$d"; return $rc
}
if essai_isole; then
    MODE=isole
elif [ "${CI:-}" = true ] && [ "$(id -u)" = 0 ] && [ ! -e /etc/sermo/cibles-mavlink ] \
     && [ ! -e "$(getent passwd 0 | cut -d: -f6)/.config/sermo/cibles-mavlink" ]; then
    MODE=direct
    echo "note   : espace isolé impossible ; conteneur de CI en root : listes écrites en direct (conteneur jetable)"
else
    echo "espace isolé impossible (unshare -rm avec montages) et pas dans un conteneur de CI en root : rien n'est essayé" >&2
    exit 2
fi

T=$(mktemp -d -p "$AIDE" banc-sermo-cible-mavlink.XXXXXX) || exit 2
PIDS=()
trap 'for p in "${PIDS[@]}"; do kill "$p" 2>/dev/null; done; wait 2>/dev/null; rm -rf -- "$T"' EXIT
mkdir -m 700 "$T/run"; export XDG_RUNTIME_DIR="$T/run"

B=$((20000 + $$ % 20000)); PD=$B; PA=$((B + 1)); PI=$((B + 2)); PM=$((B + 3)); PX=$((B + 4))
# Pilotage : des faux drones qui RÉPONDENT aux lectures (barrière, état au sol,
# position), en GUIDED, chacun avec un défaut.
PG=$((B + 5)); PGA=$((B + 6)); PGC=$((B + 7)); PGL=$((B + 8)); PGX=$((B + 9)); PGS=$((B + 10)); PGM=$((B + 11))
PGV=$((B + 12))   # armé, en vol, en LOITER (5)
PGP=$((B + 13))   # un AVION (MAV_TYPE 1) sous ArduPilot
PG2=$((B + 14))   # deux autopilotes (systèmes 1 et 2) sur le même lien
PGN=$((B + 15))   # FENCE_RADIUS vaut NaN
PU=$((B + 16))    # UDP : deux émetteurs pour le système 1
PGT=$((B + 17))   # FENCE_TYPE vaut 1e30 : fini, hors de tout entier
PGD=$((B + 18))   # FENCE_TYPE vaut 7,5 : pas un masque de bits
python3 -I "$DRONE" --tcp "$PD" --journal "$T/j-desarme" >"$T/d.log" 2>&1 & PIDS+=($!)
python3 -I "$DRONE" --tcp "$PA" --arme --journal "$T/j-arme" >"$T/a.log" 2>&1 & PIDS+=($!)
python3 -I "$DRONE" --tcp "$PI" --intrus --journal "$T/j-intrus" >"$T/i.log" 2>&1 & PIDS+=($!)
python3 -I "$DRONE" --tcp "$PG" --repond --mode 4 --journal "$T/j-guided" >"$T/g.log" 2>&1 & PIDS+=($!)
python3 -I "$DRONE" --tcp "$PGA" --repond --mode 4 --arme --journal "$T/j-guided-arme" >"$T/ga.log" 2>&1 & PIDS+=($!)
python3 -I "$DRONE" --tcp "$PGC" --repond --mode 4 --barriere-coupee --journal "$T/j-coupee" >"$T/gc.log" 2>&1 & PIDS+=($!)
python3 -I "$DRONE" --tcp "$PGL" --repond --mode 4 --lent --journal "$T/j-lent" >"$T/gl.log" 2>&1 & PIDS+=($!)
python3 -I "$DRONE" --tcp "$PGX" --repond --mode 4 --px4 --journal "$T/j-px4" >"$T/gx.log" 2>&1 & PIDS+=($!)
python3 -I "$DRONE" --tcp "$PGS" --repond --mode 4 --signe --journal "$T/j-signe" >"$T/gs.log" 2>&1 & PIDS+=($!)
python3 -I "$DRONE" --tcp "$PGM" --repond --mode 4 --arme --sol --journal "$T/j-menteur" >"$T/gm.log" 2>&1 & PIDS+=($!)
python3 -I "$DRONE" --tcp "$PGV" --repond --mode 5 --arme --journal "$T/j-loiter" >"$T/gv.log" 2>&1 & PIDS+=($!)
python3 -I "$DRONE" --tcp "$PGP" --repond --mode 4 --type 1 --journal "$T/j-avion" >"$T/gp.log" 2>&1 & PIDS+=($!)
python3 -I "$DRONE" --tcp "$PG2" --repond --mode 4 --second --journal "$T/j-deux" >"$T/g2.log" 2>&1 & PIDS+=($!)
python3 -I "$DRONE" --tcp "$PGN" --repond --mode 4 --barriere-nan --journal "$T/j-nan" >"$T/gn.log" 2>&1 & PIDS+=($!)
python3 -I "$DRONE" --tcp "$PGT" --repond --mode 4 --barriere-type 1e30 --journal "$T/j-type" >"$T/gt.log" 2>&1 & PIDS+=($!)
python3 -I "$DRONE" --tcp "$PGD" --repond --mode 4 --barriere-type 7.5 --journal "$T/j-demi" >"$T/gd.log" 2>&1 & PIDS+=($!)
# Lien muet : accepte la connexion TCP, n'envoie rien.
python3 -I -c '
import socket,sys
s=socket.socket(); s.setsockopt(socket.SOL_SOCKET,socket.SO_REUSEADDR,1)
s.bind(("127.0.0.1",int(sys.argv[1]))); s.listen(16); c=[]
while True: c.append(s.accept()[0])' "$PM" >/dev/null 2>&1 & PIDS+=($!)
for i in $(seq 1 100); do
    n=0; for p in $PD $PA $PI $PM $PG $PGA $PGC $PGL $PGX $PGS $PGM $PGV $PGP $PG2 $PGN; do (exec 3<>/dev/tcp/127.0.0.1/$p) 2>/dev/null && n=$((n + 1)); done
    [ $n = 15 ] && break; sleep 0.1
done
[ $n = 15 ] || { echo "faux drones pas prêts"; tail -3 "$T"/*.log; exit 2; }

echecs=0
ko() { echo "ECHEC : $*"; echecs=$((echecs + 1)); }
ok() { echo "ok     : $*"; }

# jouer PERSO SYSTEME MODE_PERSO MODE_SYSTEME ADRESSE [VAR=valeur…]  (comme le banc MQTT)
jouer() {
    local perso=$1 systeme=$2 mp=$3 ms=$4 adresse=$5
    shift 5
    printf '%s' "$perso" >"$T/l-perso"; printf '%s' "$systeme" >"$T/l-sys"
    rm -rf -- "$T/u" "$T/w"; mkdir -p "$T/u" "$T/w"
    if [ "$MODE" = direct ]; then
        local maison; maison=$(getent passwd 0 | cut -d: -f6)
        rm -f -- "$maison/.config/sermo/cibles-mavlink" /etc/sermo/cibles-mavlink
        if [ "$perso" != - ]; then mkdir -p "$maison/.config/sermo"; cp "$T/l-perso" "$maison/.config/sermo/cibles-mavlink"
            chmod "$mp" "$maison/.config/sermo/cibles-mavlink"; fi
        if [ "$systeme" != - ]; then mkdir -p /etc/sermo; cp "$T/l-sys" /etc/sermo/cibles-mavlink; chmod "$ms" /etc/sermo/cibles-mavlink; fi
        env "$@" "$PROG" "$adresse" 2>"$T/err"; echo $? >"$T/rc"
        rm -f -- "$maison/.config/sermo/cibles-mavlink" /etc/sermo/cibles-mavlink
        RC=$(cat "$T/rc"); return
    fi
    env "$@" unshare -rm bash -c '
        set -e
        mount -t overlay overlay -o "lowerdir=/etc,upperdir=$1/u,workdir=$1/w" /etc
        mount -t tmpfs tmpfs /root
        if [ "$2" != - ]; then mkdir -p /root/.config/sermo; cp "$1/l-perso" /root/.config/sermo/cibles-mavlink
            chmod "$4" /root/.config/sermo/cibles-mavlink; fi
        if [ "$3" != - ]; then mkdir -p /etc/sermo; cp "$1/l-sys" /etc/sermo/cibles-mavlink
            chmod "$5" /etc/sermo/cibles-mavlink; fi
        set +e
        "$6" "$7" 2>"$1/err"; echo $? >"$1/rc"' _ "$T" "$perso" "$systeme" "$mp" "$ms" "$PROG" "$adresse"
    RC=$(cat "$T/rc" 2>/dev/null || echo ?)
}
# Les commandes (écritures) qu'a reçues un faux drone, tous cas confondus.
# Écritures reçues : tout sauf les LECTURES (PARAM_REQUEST_READ,
# MAV_CMD_REQUEST_MESSAGE = COMMAND_LONG:512).
commandes() { grep -E -x 'PARAM_SET|COMMAND_LONG:[0-9]+|COMMAND_INT|MISSION_COUNT|MISSION_ITEM_INT|MISSION_ITEM|MISSION_CLEAR_ALL|SET_POSITION_TARGET_GLOBAL_INT|SET_POSITION_TARGET_LOCAL_NED' "$1" 2>/dev/null \
    | grep -c -v -x 'COMMAND_LONG:512'; }
# Commandes reçues, tous drones ; BASE : celles envoyées EXPRÈS avant (la
# coupure en deux appuis), que les refus suivants ne doivent pas augmenter.
total_commandes() { local nb=0 j; for j in "$T"/j-*; do nb=$((nb + $(commandes "$j"))); done; echo $nb; }
BASE=0
refuse() {  # refuse LIBELLÉ RAISON : code 1, raison dite, aucune commande de plus reçue par aucun drone
    local nb=$(( $(total_commandes) - BASE ))
    [ "$RC" = 1 ] && [ "$nb" = 0 ] && grep -q -- "$2" "$T/err" && ok "$1" \
        || ko "$1 : code $RC, $nb commande(s) reçue(s), raison : $(cat "$T/err")"
}
D="tcp://127.0.0.1:$PD"
L="parametre WP_SPD 1..15
parametre FS_THR_ENABLE 0..2
ordre rtl
ordre terminaison
"

# 1. Liste blanche.
jouer - - - - "$D?parametre=WP_SPD&valeur=5";                    refuse "aucune liste blanche : refus" "pas dans la liste"
jouer "$L" - 600 - "$D?parametre=WP_SPD&valeur=20";              refuse "hors intervalle refusé" "hors de l'intervalle"
jouer "$L" - 600 - "$D?parametre=WP_SPD&valeur=%24V" V=99;       refuse "valeur=\$V hors intervalle refusée" "hors de l'intervalle"
jouer "$L" - 600 - "$D?parametre=WP_SPD&valeur=%24ABSENTE";      refuse "variable absente refusée" "n'existe pas"
jouer "$L" - 600 - "$D?parametre=WP_SPD&valeur=vite";            refuse "valeur non numérique refusée" "un nombre attendu"
jouer "$L" - 600 - "$D?parametre=RTL_ALT&valeur=20";             refuse "paramètre hors liste refusé" "pas dans la liste"
jouer "$L" - 600 - "$D?ordre=land";                              refuse "ordre hors liste refusé" "pas dans la liste"
jouer "$L" - 600 - "$D?ordre=decoller";                          refuse "ordre inconnu refusé" "ordre inconnu"
jouer "$L" - 600 - "$D?ordre=rtl&parametre=WP_SPD&valeur=5";     refuse "paramètre ET ordre refusés" "OU pilote"
jouer "$L" - 600 - "$D?parametre=wp_spd&valeur=5";               refuse "nom de paramètre invalide refusé" "invalide"
jouer "$L" - 644 - "$D?parametre=WP_SPD&valeur=5";               refuse "liste personnelle lisible par d'autres : refus" "refusée"
jouer - "$L" - 664 "$D?parametre=WP_SPD&valeur=5";               refuse "liste système modifiable par d'autres : refus" "refusée"
jouer "parametre WP_SPD 1..15
parametre WP_SPD
" - 600 - "$D?parametre=WP_SPD&valeur=5";                        refuse "ligne mal formée : tout refusé" "mal formée"
jouer "parametre WP_SPD 15..1
" - 600 - "$D?parametre=WP_SPD&valeur=5";                        refuse "intervalle à l'envers : tout refusé" "mal formée"
jouer "parametre WP_SPD 1..15
ordre decoller
" - 600 - "$D?parametre=WP_SPD&valeur=5";                        refuse "ordre inconnu dans la liste : tout refusé" "mal formée"
mkdir -p "$T/faux/.config/sermo"; printf '%s' "$L" >"$T/faux/.config/sermo/cibles-mavlink"; chmod 600 "$T/faux/.config/sermo/cibles-mavlink"
jouer - - - - "$D?parametre=WP_SPD&valeur=5" HOME="$T/faux";     refuse "\$HOME ne déplace pas la liste blanche" "pas dans la liste"
# 2. Familles interdites : dans l'adresse, et DANS la liste (tout refusé).
for p in ARMING_CHECK FLTMODE1 SERVO3_FUNCTION SERVO14_FUNCTION BRD_SAFETY_DEFLT FENCE_ENABLE; do
    jouer "$L" - 600 - "$D?parametre=$p&valeur=0";               refuse "famille interdite dans l'adresse : $p" "jamais réglée"
done
jouer "parametre WP_SPD 1..15
parametre ARMING_CHECK 0..1
" - 600 - "$D?parametre=WP_SPD&valeur=5";                        refuse "famille interdite DANS la liste : tout refusé" "jamais réglée"
jouer "$L" - 600 - "$D?parametre=SERVO3_MIN&valeur=0"
grep -q "jamais réglée" "$T/err" && ko "SERVO3_MIN pris à tort pour une famille interdite" || ok "SERVO3_MIN n'est pas SERVOn_FUNCTION"
# 3. État du drone, autres émetteurs, liens.
jouer "$L" - 600 - "tcp://127.0.0.1:$PA?parametre=WP_SPD&valeur=5"; refuse "drone ARMÉ : réglage refusé" "ARMÉ"
jouer "$L" - 600 - "tcp://127.0.0.1:$PI?ordre=rtl";               refuse "une station sol parle déjà : refusé" "station sol parle déjà"
jouer "$L" - 600 - "$D?parametre=FS_THR_ENABLE&valeur=1";         refuse "paramètre inconnu du drone : refusé" "ne connaît pas"
t0=$(date +%s%3N)
jouer "$L" - 600 - "tcp://127.0.0.1:$PM?ordre=rtl&delai=2000";    d=$(( $(date +%s%3N) - t0 ))
refuse "lien muet : refusé" "aucun autopilote"
[ $d -lt 3500 ] && ok "lien muet : main rendue en $d ms (délai 2000)" || ko "lien muet : $d ms"
jouer "$L" - 600 - "tcp://127.0.0.1:$PX?ordre=rtl";               refuse "port fermé : refusé" "injoignable"
# Seule demande permise avant un refus : la LECTURE (PARAM_REQUEST_READ).
autres=$(cat "$T"/j-* | grep -v -x -e PARAM_REQUEST_READ | sort | uniq -c | head -3)
[ -z "$autres" ] && ok "les drones n'ont reçu que des demandes de lecture" || ko "messages inattendus : $autres"
# 4. Coupure des moteurs : un appui n'envoie rien, et le dit.
jouer "$L" - 600 - "$D?ordre=terminaison";                       refuse "coupure, premier appui : rien n'est envoyé" "appuyer encore"
M="$XDG_RUNTIME_DIR/sermo-cible-mavlink"
grep -q "appuyer encore" "$M/message" 2>/dev/null && ok "le message est écrit pour le dialogue" || ko "message absent : $(cat "$M/message" 2>&1)"
[ "$(stat -c %a "$M")" = 700 ] && [ "$(stat -c %a "$M/message")" = 600 ] && ok "dossier 700, message 600" \
    || ko "droits : $(stat -c '%a %n' "$M" "$M"/* 2>&1)"
n=$(ls "$M" | grep -c '^terminaison-')
[ "$n" = 1 ] && ok "une marque posée" || ko "marques : $n"
# Hors délai : la marque est vieillie de 6 s (elle porte l'heure de pose), le
# second appui redevient un premier appui.
for f in "$M"/terminaison-*; do printf '%s\n' $(( $(cut -d. -f1 /proc/uptime) * 1000 - 6000 )) >"$f"; done
jouer "$L" - 600 - "$D?ordre=terminaison";                       refuse "coupure, second appui après 5 s : rien n'est envoyé" "appuyer encore"
# Une marque posée par quelqu'un d'autre (droits ouverts) ne confirme rien.
for f in "$M"/terminaison-*; do chmod 644 "$f"; done
jouer "$L" - 600 - "$D?ordre=terminaison";                       refuse "marque aux droits ouverts : ne confirme pas" "appuyer encore"
# Deux appuis dans les 5 s : l'ordre PART (une fois). Le faux drone n'accuse
# pas : « pas confirmé », code 1.
rm -f -- "$M"/terminaison-*
avant=$(commandes "$T/j-desarme")
jouer "$L" - 600 - "$D?ordre=terminaison"
# Un second appui collé au premier (rebond, double clic) ne confirme rien, et
# la marque reste : l'appui suivant, plus de 500 ms après le premier, confirme.
jouer "$L" - 600 - "$D?ordre=terminaison";                       refuse "coupure, second appui à moins de 500 ms : rien n'est envoyé" "trop rapproché"
sleep 0.6
jouer "$L" - 600 - "$D?ordre=terminaison&delai=2500"
apres=$(commandes "$T/j-desarme")
[ "$RC" = 1 ] && [ $((apres - avant)) = 1 ] && grep -q "pas confirmé" "$T/err" \
    && ok "coupure, deux appuis dans les 5 s : l'ordre part, une fois" \
    || ko "deux appuis : code $RC, $((apres - avant)) commande(s), $(cat "$T/err")"
[ -z "$(ls "$M" | grep '^terminaison-')" ] && ok "la marque est consommée" || ko "marque restée"

BASE=$(total_commandes)
# 5. Missions (QGC WPL 110). Le faux drone est en 45.1234567 N, 5.7654321 E.
MI="$T/missions"; mkdir -p "$MI"
wpl() {  # wpl NOM « lat lon hauteur [commande repère attente] »…
    local f="$MI/$1" i=1; shift
    { printf 'QGC WPL 110\n0\t1\t0\t16\t0\t0\t0\t0\t0\t0\t0\t1\n'
      for p in "$@"; do set -- $p
          printf '%d\t0\t%s\t%s\t%s\t0\t0\t0\t%s\t%s\t%s\t1\n' $i "${5:-3}" "${4:-16}" "${6:-0}" "$1" "$2" "$3"; i=$((i + 1))
      done; } >"$f"; chmod 644 "$f"
}
LM="mission rayon 500 hauteur 120
ordre rtl
"
wpl bon "45.1244 5.7654 20" "45.1244 5.7664 30 16 3 5"
wpl decollage "45.1244 5.7654 20 22"
wpl repere "45.1244 5.7654 20 16 0"
wpl loin "45.1244 5.7654 20" "45.1434 5.7654 20"
wpl haut "45.1244 5.7654 130"
wpl sous "45.1244 5.7654 -5"
wpl attente "45.1244 5.7654 20 16 3 90"
pts=(); for i in $(seq 1 101); do pts+=("45.1244 5.7654 20"); done; wpl cent-un "${pts[@]}"
printf 'QGC WPL 120\n' >"$MI/format"; chmod 644 "$MI/format"
cp "$MI/bon" "$MI/ouvert"; chmod 666 "$MI/ouvert"
ln -s "$MI/bon" "$MI/lien"
avant=$(commandes "$T/j-desarme")
jouer "$LM" - 600 - "$D?mission=$MI/format";                   refuse "mission : fichier hors format refusé" "QGC WPL 110"
jouer "$LM" - 600 - "$D?mission=$MI/decollage";                refuse "mission : décollage (22) refusé" "commande 22"
jouer "$LM" - 600 - "$D?mission=$MI/repere";                   refuse "mission : repère 0 refusé" "repère 0"
jouer "$LM" - 600 - "$D?mission=$MI/attente";                  refuse "mission : attente de 90 s refusée" "attente"
jouer "$LM" - 600 - "$D?mission=$MI/cent-un";                  refuse "mission : 101 points refusés" "plus de 100"
jouer "$LM" - 600 - "$D?mission=$MI/haut";                     refuse "mission : point à 130 m refusé (120 permis)" "hauteur"
jouer "mission rayon 500 hauteur 120
mission rayon 500 hauteur 15
" - 600 - "$D?mission=$MI/bon";                                refuse "mission : deux lignes, la plus stricte (15 m) l'emporte" "hors de 0..15"
jouer "mission rayon 500 hauteur 15
" - 600 - "$D?mission=$MI/bon";                                refuse "mission : la plus stricte des deux listes l'emporte" "hors de 0..15"
jouer "$LM" - 600 - "$D?mission=$MI/sous";                     refuse "mission : point sous le départ refusé" "hauteur"
jouer "$LM" - 600 - "$D?mission=$MI/loin";                     refuse "mission : point à 2 km refusé (500 m permis)" "au-delà des 500"
jouer "$LM" - 600 - "$D?mission=$MI/ouvert";                   refuse "mission : fichier modifiable par d'autres refusé" "personne d'autre"
jouer "$LM" - 600 - "$D?mission=$MI/lien";                     refuse "mission : lien symbolique refusé" "illisible"
jouer "$LM" - 600 - "$D?mission=missions/bon";                 refuse "mission : chemin relatif refusé" "chemin absolu"
jouer "ordre rtl
" - 600 - "$D?mission=$MI/bon";                                refuse "mission : sans ligne « mission », refus" "pas dans la liste"
jouer "mission rayon 500 hauteur 150
" - 600 - "$D?mission=$MI/bon";                                refuse "mission : ligne au-delà de 120 m, liste refusée" "mal formée"
jouer "$LM" - 600 - "$D?mission=$MI/bon&ordre=rtl";            refuse "mission ET ordre refusés" "OU pilote"
jouer "$LM" - 600 - "tcp://127.0.0.1:$PA?mission=$MI/bon";     refuse "mission : drone ARMÉ, refus" "ARMÉ"
jouer "$LM" - 600 - "tcp://127.0.0.1:$PI?mission=$MI/bon";     refuse "mission : une station sol parle, refus" "station sol parle déjà"
# Toutes les gardes passées : l'envoi commence (une annonce MISSION_COUNT), le
# faux drone ne demande rien, sermo le dit et ne recommence pas.
jouer "$LM" - 600 - "$D?mission=$MI/bon&delai=3000"
apres=$(grep -c -x MISSION_COUNT "$T/j-desarme")
[ "$RC" = 1 ] && [ "$apres" = 1 ] && [ "$(commandes "$T/j-desarme")" = $((avant + 1)) ] && grep -q "s'est tu" "$T/err" \
    && ok "mission permise : toutes les gardes passées, une seule annonce, pas de nouvel essai" \
    || ko "mission permise : code $RC, $apres annonce(s), $(cat "$T/err")"

# 6. PILOTAGE (faux drones en GUIDED à 45.1234567 N 5.7654321 E, 30 m de haut).
BASE=$(total_commandes)
LP="pilotage lat 45.1234 lon 5.7654 rayon 300 hauteur 120 decollage 20 modes 4,5,6,9
"
G="tcp://127.0.0.1:$PG"
# deux_appuis LISTE ADRESSE : premier appui (rien), puis le second, dans les 5 s.
deux_appuis() {  # deux_appuis LISTE ADRESSE [VAR=valeur…]
    local l=$1 ad=$2; shift 2
    jouer "$l" - 600 - "$ad" "$@"; grep -q "appuyer encore" "$T/err" || { echo "  (premier appui : $(cat "$T/err"))"; }
    sleep 0.6   # un second appui à moins de 500 ms est un rebond
    jouer "$l" - 600 - "$ad" "$@"
}
jouer "ordre rtl
" - 600 - "$G?pilote=armer";                                  refuse "pilotage : sans ligne « pilotage », refus" "pilotage n'est pas dans la liste"
jouer "pilotage lat 45.1234 lon 5.7654 rayon 300 hauteur 120 decollage 150 modes 4
" - 600 - "$G?pilote=armer";                                  refuse "pilotage : décollage plus haut que la hauteur, liste refusée" "mal formée"
jouer "pilotage lat 45.1234 lon 5.7654 rayon 300 hauteur 150 decollage 20 modes 4
" - 600 - "$G?pilote=armer";                                  refuse "pilotage : hauteur au-delà de 120 m, liste refusée" "mal formée"
jouer "pilotage lat 45.1234 lon 5.7654 rayon 300 hauteur 120 decollage 20 modes 4,0
" - 600 - "$G?pilote=armer";                                  refuse "pilotage : STABILIZE (0) dans la liste, liste refusée" "mal formée"
jouer "pilotage lat 45.1234 lon 5.7654 rayon 300 hauteur 120 decollage 20 modes 4,3
" - 600 - "$G?pilote=armer";                                  refuse "pilotage : AUTO (3) dans la liste, liste refusée" "mal formée"
jouer "$LP$LP" - 600 - "$G?pilote=armer";                      refuse "pilotage : deux lignes dans un fichier, refus" "une seule ligne"
jouer "$LP" "pilotage lat 46.1234 lon 5.7654 rayon 300 hauteur 120 decollage 20 modes 4
" 600 644 "$G?pilote=armer";                                  refuse "pilotage : deux références différentes, refus" "même point de référence"
jouer "$LP" - 600 - "$G?pilote=mode&mode=stabilize";          refuse "pilotage : mode stabilize refusé" "jamais stabilize"
jouer "$LP" - 600 - "$G?pilote=mode&mode=auto";               refuse "pilotage : mode auto refusé" "jamais stabilize"
jouer "$LP" - 600 - "$G?pilote=mode&mode=brake";              refuse "pilotage : mode brake hors de la liste blanche" "pas permis"
jouer "$LP" - 600 - "$G?pilote=voler";                        refuse "pilotage : ordre inconnu" "inconnu"
jouer "$LP" - 600 - "$G?pilote=mode";                         refuse "pilotage : mode= manquant" "demande mode="
jouer "$LP" - 600 - "$G?ordre=rtl&mode=land";                 refuse "pilotage : mode= sans pilote=mode" "ne va qu'avec"
jouer "$LP" - 600 - "$G?pilote=armer&ordre=rtl";              refuse "pilotage ET ordre refusés" "OU pilote"
rm -f -- "$M"/pilote-*
jouer "$LP" - 600 - "$G?pilote=armer";                        refuse "pilotage : premier appui, rien n'est envoyé" "appuyer encore"
grep -q "ARMER" "$M/message" && ok "pilotage : le message dit ce qui partirait" || ko "message : $(cat "$M/message")"
# La marque est propre à l'ordre : un autre ordre ne confirme pas l'armement.
jouer "$LP" - 600 - "$G?pilote=decoller";                     refuse "pilotage : un AUTRE ordre ne confirme pas (premier appui)" "appuyer encore"
rm -f -- "$M"/pilote-*
deux_appuis "$LP" "tcp://127.0.0.1:$PGA?pilote=armer";        refuse "pilotage : armer un drone armé, refus" "désarmé, au sol"
# Armé mais « au sol » : seul l'état armé refuse.
deux_appuis "$LP" "tcp://127.0.0.1:$PGM?pilote=armer";        refuse "pilotage : armer un drone armé posé, refus" "ici : armé, au sol"
deux_appuis "$LP" "$G?pilote=decoller";                       refuse "pilotage : décoller désarmé, refus" "armé, au sol"
deux_appuis "$LP" "$G?pilote=mode&mode=loiter";               refuse "pilotage : LOITER au sol, refus" "qu'en vol"
deux_appuis "$LP" "tcp://127.0.0.1:$PGA?pilote=desarmer";     refuse "pilotage : désarmer à 30 m, refus" "POSÉ"
# L'autopilote se dit « au sol » à 30 m : seule la hauteur observée refuse.
deux_appuis "$LP" "tcp://127.0.0.1:$PGM?pilote=desarmer";     refuse "pilotage : « au sol » à 30 m, désarmer refusé (hauteur observée)" "POSÉ"
deux_appuis "$LP" "tcp://127.0.0.1:$PGC?pilote=armer";        refuse "pilotage : barrière de l'autopilote coupée, refus" "inactive"
deux_appuis "$LP" "tcp://127.0.0.1:$PGX?pilote=armer";        refuse "pilotage : autopilote PX4, refus" "pas ArduPilot"
deux_appuis "$LP" "tcp://127.0.0.1:$PGS?pilote=armer";        refuse "pilotage : drone qui signe, refus" "signe ses messages"
deux_appuis "$LP" "tcp://127.0.0.1:$PGP?pilote=armer";        refuse "pilotage : un avion (MAV_TYPE 1), refus" "ni un multirotor"
deux_appuis "$LP" "tcp://127.0.0.1:$PGP?pilote=mode&mode=rtl"; refuse "pilotage : un avion, même le mode de secours est refusé" "ni un multirotor"
deux_appuis "$LP" "tcp://127.0.0.1:$PGN?pilote=armer";        refuse "pilotage : FENCE_RADIUS NaN, refus" "incohérente"
deux_appuis "$LP" "tcp://127.0.0.1:$PGT?pilote=armer";        refuse "pilotage : FENCE_TYPE 1e30 (hors de tout entier), refus" "incohérente (FENCE_TYPE"
deux_appuis "$LP" "tcp://127.0.0.1:$PGD?pilote=armer";        refuse "pilotage : FENCE_TYPE 7,5 (pas un masque), refus" "incohérente (FENCE_TYPE"
deux_appuis "$LP" "tcp://127.0.0.1:$PG2?pilote=armer";        refuse "pilotage : deux autopilotes sans systeme=, refus" "plusieurs autopilotes"
jouer "ordre rtl
" - 600 - "tcp://127.0.0.1:$PG2?ordre=rtl";                   refuse "ordre : deux autopilotes sans systeme=, refus" "plusieurs autopilotes"
# Un second appui collé au premier ne confirme pas ; la marque reste.
rm -f -- "$M"/pilote-*
jouer "$LP" - 600 - "$G?pilote=armer"
jouer "$LP" - 600 - "$G?pilote=armer";                        refuse "pilotage : second appui à moins de 500 ms, rien n'est envoyé" "trop rapproché"
rm -f -- "$M"/pilote-*
deux_appuis "$LP" "tcp://127.0.0.1:$PI?pilote=armer";         refuse "pilotage : une station sol parle, refus" "station sol parle déjà"
deux_appuis "$LP" "tcp://127.0.0.1:$PGL?pilote=armer";        refuse "pilotage : battement lent (3 s), refus (délai ou état trop ancien)" "lien trop lent\|trop ancien"
deux_appuis "pilotage lat 45.1534 lon 5.7654 rayon 300 hauteur 120 decollage 20 modes 4,9
" "$G?pilote=mode&mode=guided";                               refuse "pilotage : drone hors de la barrière de sermo, GUIDED refusé" "hors de la barrière"
deux_appuis "pilotage lat 45.1234 lon 5.7654 rayon 100 hauteur 120 decollage 20 modes 4
" "$G?pilote=armer";                                          refuse "pilotage : barrière de l'autopilote plus large que celle de sermo" "plus large"
# Les gardes passées : l'ordre PART (une fois) ; le faux drone n'accuse pas.
avant=$(total_commandes)
deux_appuis "$LP" "$G?pilote=armer"
[ "$RC" = 1 ] && [ $(( $(total_commandes) - avant )) = 1 ] && grep -q -x 'COMMAND_LONG:400' "$T/j-guided" && grep -q "pas confirmé" "$T/err" \
    && [ "$(cat "$T/j-guided.parametres")" = "400 1 0 0" ] \
    && ok "pilotage : armer, gardes passées, l'ordre part une fois (400, param1 1, param2 0 : jamais forcé)" \
    || ko "pilotage armer : code $RC, $(( $(total_commandes) - avant )) écriture(s), $(cat "$T/err")"
# Deux autopilotes, mais systeme= dit lequel : l'ordre part vers lui seul.
avant=$(total_commandes)
deux_appuis "$LP" "tcp://127.0.0.1:$PG2?pilote=armer&systeme=1"
[ "$RC" = 1 ] && [ $(( $(total_commandes) - avant )) = 1 ] && grep -q -x 'COMMAND_LONG:400' "$T/j-deux" && grep -q "pas confirmé" "$T/err" \
    && ok "pilotage : deux autopilotes et systeme=1, l'ordre part une fois" \
    || ko "pilotage deux autopilotes, systeme=1 : code $RC, $(( $(total_commandes) - avant )) écriture(s), $(cat "$T/err")"
# UDP : deux émetteurs (deux adresses) annoncent le système 1 : aucun ordre.
python3 -I "$DRONE" --udp "127.0.0.1:$PU" >/dev/null 2>&1 & U1=$!; PIDS+=($U1)
python3 -I "$DRONE" --udp "127.0.0.1:$PU" >/dev/null 2>&1 & U2=$!; PIDS+=($U2)
sleep 0.5
BASE=$(total_commandes)   # les ordres partis exprès juste avant ne comptent pas
jouer "ordre rtl
" - 600 - "udp://127.0.0.1:$PU?ordre=rtl";                    refuse "UDP : deux émetteurs pour un même système, refus" "deux émetteurs"
kill $U2 2>/dev/null; wait $U2 2>/dev/null
# Un seul émetteur : l'ordre part (le faux drone UDP n'écoute pas : « pas confirmé »).
jouer "ordre rtl
" - 600 - "udp://127.0.0.1:$PU?ordre=rtl&delai=2500"
[ "$RC" = 1 ] && grep -q "pas confirmé" "$T/err" && ok "UDP : un seul émetteur, l'ordre part" \
    || ko "UDP un seul émetteur : code $RC, $(cat "$T/err")"
kill $U1 2>/dev/null; wait $U1 2>/dev/null
avant=$(total_commandes)
deux_appuis "pilotage lat 45.1534 lon 5.7654 rayon 300 hauteur 120 decollage 20 modes 4,9
" "tcp://127.0.0.1:$PGC?pilote=mode&mode=land"
[ "$RC" = 1 ] && [ $(( $(total_commandes) - avant )) = 1 ] && grep -q -x 'COMMAND_LONG:176' "$T/j-coupee" \
    && ok "pilotage : LAND passe hors barrière et barrière coupée (secours), une fois" \
    || ko "pilotage land de secours : code $RC, $(( $(total_commandes) - avant )) écriture(s), $(cat "$T/err")"

# 7. ALLER À UN POINT (faux drones à 45.1234567 N 5.7654321 E ; référence 45.1234 5.7654, rayon 300 m).
BASE=$(total_commandes)
PA2="tcp://127.0.0.1:$PGA"
jouer "$LP" - 600 - "$PA2?pilote=aller&lat=45.1244&lon=5.7654";                 refuse "aller : hauteur= manquante" "demande lat="
jouer "$LP" - 600 - "$PA2?pilote=armer&lat=45.1244&lon=5.7654&hauteur=20";      refuse "aller : lat/lon/hauteur avec un autre ordre" "ne vont qu'avec lui"
jouer "$LP" - 600 - "$PA2?pilote=aller&lat=nord&lon=5.7654&hauteur=20";         refuse "aller : latitude non numérique" "un nombre attendu"
jouer "$LP" - 600 - "$PA2?pilote=aller&lat=95&lon=5.7654&hauteur=20";           refuse "aller : latitude impossible" "position impossible"
jouer "$LP" - 600 - "$PA2?pilote=aller&lat=45.1534&lon=5.7654&hauteur=20";      refuse "aller : point à 3 km, hors barrière" "hors de la barrière"
jouer "$LP" - 600 - "$PA2?pilote=aller&lat=45.1244&lon=5.7654&hauteur=130";     refuse "aller : 130 m, hors barrière" "hors de la barrière"
jouer "$LP" - 600 - "$PA2?pilote=aller&lat=45.1244&lon=5.7654&hauteur=0";       refuse "aller : hauteur 0 (le sol), refusée" "hors de la barrière"
jouer "$LP" - 600 - "$PA2?pilote=aller&lat=45.1244&lon=5.7654&hauteur=%24H" H=x; refuse "aller : \$H non numérique" "un nombre attendu"
rm -f -- "$M"/pilote-*
jouer "$LP" - 600 - "$PA2?pilote=aller&lat=45.1244&lon=5.7654&hauteur=20";      refuse "aller : premier appui, rien n'est envoyé" "appuyer encore"
grep -q "ALLER à 45.1244000, 5.7654000, 20 m" "$M/message" && grep -q "de la référence" "$M/message" \
    && ok "aller : le message montre le point, la hauteur et la distance" || ko "message : $(cat "$M/message")"
# Un AUTRE point ne confirme pas le premier.
jouer "$LP" - 600 - "$PA2?pilote=aller&lat=45.1245&lon=5.7654&hauteur=20";      refuse "aller : un autre point ne confirme pas (premier appui)" "appuyer encore"
rm -f -- "$M"/pilote-*
deux_appuis "$LP" "$G?pilote=aller&lat=45.1244&lon=5.7654&hauteur=20";          refuse "aller : drone au sol, refus" "armé, en vol"
deux_appuis "$LP" "tcp://127.0.0.1:$PGV?pilote=aller&lat=45.1244&lon=5.7654&hauteur=20"; refuse "aller : en LOITER, refus (jamais de changement de mode)" "DÉJÀ en GUIDED"
deux_appuis "$LP" "tcp://127.0.0.1:$PGC?pilote=aller&lat=45.1244&lon=5.7654&hauteur=20"; refuse "aller : barrière de l'autopilote coupée, refus" "inactive"
# Gardes passées : la cible part UNE fois, en cadre 6, position seule ; le faux
# drone ne la montre pas : « NON confirmé ».
avant=$(total_commandes)
deux_appuis "$LP" "$PA2?pilote=aller&lat=45.1244&lon=5.7654&hauteur=%24H" H=20
[ "$RC" = 1 ] && [ $(( $(total_commandes) - avant )) = 1 ] && grep -q "NON confirmé" "$T/err" \
    && [ "$(grep '^cible' "$T/j-guided-arme.parametres")" = "cible 6 0x0DF8 451244000 57654000 20" ] \
    && ok "aller : la cible part une fois (cadre 6, type_mask 0x0DF8, point exact, 20 m via \$H)" \
    || ko "aller : code $RC, $(( $(total_commandes) - avant )) écriture(s), cible [$(grep '^cible' "$T/j-guided-arme.parametres" 2>/dev/null)], $(cat "$T/err")"

# Poste GELÉ entre le clic et l'envoi (processus arrêté 2,6 s par SIGSTOP), le
# drone répondant vite : seul le DÉLAI depuis le clic refuse (l'état relu
# après le dégel est frais).
if [ "$MODE" = isole ]; then
rm -f -- "$M"/pilote-*
jouer "$LP" - 600 - "$G?pilote=armer"
sleep 0.6
printf '%s' "$LP" >"$T/l-gel"
unshare -rm bash -c '
    mount -t tmpfs tmpfs /root; mkdir -p /root/.config/sermo
    cp "$1/l-gel" /root/.config/sermo/cibles-mavlink; chmod 600 /root/.config/sermo/cibles-mavlink
    "$2" "$3" 2>"$1/err" & p=$!
    sleep 0.3; kill -STOP $p; sleep 2.6; kill -CONT $p
    wait $p; echo $? >"$1/rc"' _ "$T" "$PROG" "$G?pilote=armer"
RC=$(cat "$T/rc" 2>/dev/null || echo ?)
[ "$RC" = 1 ] && grep -q "depuis le clic" "$T/err" && ok "pilotage : poste gelé 2,6 s après le clic, refus pour le délai" \
    || ko "pilotage poste gelé : code $RC, $(cat "$T/err")"
else
    echo "note   : pas d'espace isolé : cas « poste gelé » non joué"
fi

# Radio LENTE : 400 ms de latence sur la boucle locale, dans un espace réseau
# isolé (unshare -rn, tc netem) ; armer doit être refusé pour le DÉLAI depuis
# le clic. Sans netem (conteneur de CI), le cas est sauté et c'est dit.
TC=$(command -v tc || echo /usr/sbin/tc)
if [ "$MODE" = isole ] && [ -x "$TC" ] && unshare -rn sh -c "ip link set lo up && $TC qdisc add dev lo root netem delay 1ms" 2>/dev/null; then
    mkdir -m 700 "$T/run-lent"
    printf '%s' "$LP" >"$T/l-lente"
    unshare -rn bash -c '
        ip link set lo up; "$1" qdisc add dev lo root netem delay 400ms
        python3 -I "$2" --tcp 29931 --repond --mode 4 >/dev/null 2>&1 & d=$!
        for i in $(seq 1 50); do (exec 3<>/dev/tcp/127.0.0.1/29931) 2>/dev/null && break; sleep 0.2; done
        for k in 1 2; do
            XDG_RUNTIME_DIR="$3/run-lent" unshare -m bash -c "mount -t tmpfs tmpfs /root; mkdir -p /root/.config/sermo
                cp \"$3/l-lente\" /root/.config/sermo/cibles-mavlink; chmod 600 /root/.config/sermo/cibles-mavlink
                \"$4\" \"tcp://127.0.0.1:29931?pilote=armer\" 2>\"$3/err\"; echo \$? >\"$3/rc\""
            sleep 0.6
        done
        kill $d' _ "$TC" "$DRONE" "$T" "$PROG"
    RC=$(cat "$T/rc" 2>/dev/null || echo ?)
    [ "$RC" = 1 ] && grep -q "lien trop lent" "$T/err" && ok "pilotage : radio lente (400 ms), refus pour le délai depuis le clic" \
        || ko "pilotage radio lente : code $RC, $(cat "$T/err")"
else
    echo "note   : pas de latence simulable ici (tc netem dans un espace isolé) : cas « radio lente » non joué"
fi

echo
[ $echecs = 0 ] && { echo "sermo-cible-mavlink (faux drone) : tout tient"; exit 0; }
echo "sermo-cible-mavlink (faux drone) : $echecs défaut(s)"; exit 1
