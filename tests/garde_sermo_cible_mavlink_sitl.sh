#!/bin/bash
# SPDX-License-Identifier: GPL-2.0-or-later
# tests/garde_sermo_cible_mavlink_sitl.sh — haplo-dialog — 2026 — GPL-2.0-or-later
#
# Le VRAI programme d'écriture de <action mavlink> contre un VRAI autopilote
# simulé : ArduPilot Copter en SITL, construit HORS du dépôt (jamais livré).
# Le banc lance son propre simulateur ; un pilote d'essai (tests/pilote-sitl.py,
# second port, sans battement) le fait décoller et lit son état.
#
#   SERMO_SITL=/chemin/vers/ardupilot   (contient build/sitl/bin/arducopter)
# Sans SERMO_SITL : rien d'essayé (code 2). La CI ne le joue pas.
#
# Vérifie, au sol : paramètre réglé ET relu par le pilote ; entier refusé avec
# une valeur décimale (le drone garde l'ancienne) ; paramètre inconnu du
# drone ; coupure en un appui : rien. En vol : réglage refusé (drone armé, la
# valeur ne bouge pas) ; retour au départ accepté (mode 6) ; atterrissage
# accepté (mode 9) ; une station sol présente : refus, le mode ne bouge pas ;
# coupure en deux appuis : acceptée, le drone est désarmé. Missions : au sol,
# trois points envoyés puis relus par le pilote (identiques, attente
# comprise) ; une mission plus courte remplace la précédente ; en vol, refus
# (drone armé), la mission gardée ne bouge pas. PILOTAGE (deux appuis à chaque
# ordre) : GUIDED, armer puis désarmer au sol ; armer, décoller à la hauteur de
# la liste, désarmer refusé en montée, LOITER ; drone hors de la barrière de
# sermo : GUIDED refusé, LAND passe ; barrière de l'autopilote coupée : armer
# refusé. ALLER À : le drone part vers le point et y arrive à la hauteur
# demandée ; point hors barrière refusé ; en LOITER, refusé (jamais de
# changement de mode). La barrière de l'autopilote est activée pour le banc
# (FENCE_ENABLE 1).
#
# Usage : garde_sermo_cible_mavlink_sitl.sh [racine du dépôt]
# Codes : 0 = tout tient · 1 = un défaut · 2 = rien d'essayé (outil, SITL absents)

set -u
export LC_ALL=C
RACINE=$(CDPATH= cd -- "${1:-$(dirname -- "$0")/..}" 2>/dev/null && pwd) || { echo "racine illisible" >&2; exit 2; }
AIDE="$RACINE/sermo-source/_build"
PROG="$AIDE/sermo-cible-mavlink"
PILOTE="$RACINE/tests/pilote-sitl.py"
[ -n "${SERMO_SITL:-}" ] || { echo "SERMO_SITL non posé (dossier ardupilot construit) : rien d'essayé" >&2; exit 2; }
SITL="$SERMO_SITL/build/sitl/bin/arducopter"
PARM="$SERMO_SITL/Tools/autotest/default_params/copter.parm"
[ -x "$SITL" ] && [ -f "$PARM" ] || { echo "simulateur absent : $SITL ou $PARM" >&2; exit 2; }
[ -x "$PROG" ] || { echo "programme d'écriture absent : $PROG (ci/construire.sh)" >&2; exit 2; }
python3 -I -c 'import pymavlink' 2>/dev/null || { echo "outil manquant : python3-pymavlink" >&2; exit 2; }
unshare -rm true 2>/dev/null || { echo "espace isolé impossible (unshare -rm) : rien d'essayé" >&2; exit 2; }

T=$(mktemp -d -p "$AIDE" banc-sermo-cible-mavlink-sitl.XXXXXX) || exit 2
PIDS=()
trap 'for p in "${PIDS[@]}"; do kill "$p" 2>/dev/null; done; wait 2>/dev/null; rm -rf -- "$T"' EXIT
mkdir -m 700 "$T/run"; export XDG_RUNTIME_DIR="$T/run"

# Instance 1 à 8 : ports 5760+10·I (sermo) et 5762+10·I (pilote).
I=$((1 + $$ % 8)); PS=$((5760 + 10 * I)); PP=$((5762 + 10 * I))
# Copter 4.7 n'émet que son battement sans ces réglages (anciens SR0_*).
printf 'MAV1_EXT_STAT 2\nMAV1_POSITION 2\nMAV1_EXTRA2 2\nFENCE_ENABLE 1\n' >"$T/flux.parm"
(cd "$T" && exec "$SITL" --model + --speedup 1 --defaults "$PARM,$T/flux.parm" -I"$I" </dev/null >"$T/sitl.log" 2>&1) & PIDS+=($!)
for i in $(seq 1 100); do grep -q "Waiting for connection" "$T/sitl.log" 2>/dev/null && break; sleep 0.2; done
grep -q "Waiting for connection" "$T/sitl.log" || { echo "simulateur pas prêt"; tail -5 "$T/sitl.log"; exit 2; }
S="tcp://127.0.0.1:$PS"

echecs=0
ko() { echo "ECHEC : $*"; echecs=$((echecs + 1)); }
ok() { echo "ok     : $*"; }
pilote() { python3 -I "$PILOTE" "$PP" "$@" 2>>"$T/pilote.err"; }
L="parametre WP_SPD 1..15
parametre FS_THR_ENABLE 0..2
parametre WPNAV_SPEED 100..1500
ordre rtl
ordre land
ordre terminaison
mission rayon 500 hauteur 120
pilotage lat -35.363262 lon 149.165237 rayon 300 hauteur 120 decollage 10 modes 4,5,6,9
"
printf '%s' "$L" >"$T/liste"; cp "$T/liste" "$T/liste-base"
# deux ADRESSE [LISTE] : deux appuis (le pilotage se confirme) ; RC et $T/err du second.
deux() {
    [ -n "${2:-}" ] && printf '%s' "$2" >"$T/liste"
    jouer "$1"; grep -q "appuyer encore" "$T/err" || echo "  (premier appui : $(cat "$T/err"))"
    jouer "$1"
    cp "$T/liste-base" "$T/liste"
}
# Missions près du départ du simulateur (-35.363262, 149.165237, CMAC).
wpl() {  # wpl NOM « lat lon hauteur [attente] »…
    local f="$T/$1" i=1; shift
    { printf 'QGC WPL 110\n0\t1\t0\t16\t0\t0\t0\t0\t0\t0\t0\t1\n'
      for p in "$@"; do set -- $p
          printf '%d\t0\t3\t16\t%s\t0\t0\t0\t%s\t%s\t%s\t1\n' $i "${4:-0}" "$1" "$2" "$3"; i=$((i + 1))
      done; } >"$f"; chmod 644 "$f"
}
wpl vol "-35.3622000 149.1652000 20" "-35.3622000 149.1665000 30.5 5" "-35.3632000 149.1665000 25"
wpl court "-35.3627000 149.1658000 15"
jouer() {  # jouer ADRESSE : liste $L, espace isolé ; remplit RC et $T/err
    unshare -rm bash -c '
        mount -t tmpfs tmpfs /root; mkdir -p /root/.config/sermo
        cp "$1/liste" /root/.config/sermo/cibles-mavlink; chmod 600 /root/.config/sermo/cibles-mavlink
        "$2" "$3" 2>"$1/err"; echo $? >"$1/rc"' _ "$T" "$PROG" "$1"
    RC=$(cat "$T/rc" 2>/dev/null || echo ?)
}
reussi() { [ "$RC" = 0 ] && grep -q -- "$2" "$T/err" && ok "$1" || ko "$1 : code $RC, $(cat "$T/err")"; }
refuse() { [ "$RC" = 1 ] && grep -q -- "$2" "$T/err" && ok "$1" || ko "$1 : code $RC, $(cat "$T/err")"; }

# 1. Au sol.
jouer "$S?parametre=WP_SPD&valeur=7.5";            reussi "WP_SPD = 7.5 réglé" "relu sur le drone"
[ "$(pilote param WP_SPD)" = 7.5 ] && ok "le pilote relit WP_SPD = 7.5" || ko "WP_SPD relu : $(pilote param WP_SPD)"
jouer "$S?parametre=FS_THR_ENABLE&valeur=1.5";     refuse "entier : 1.5 refusé" "est un entier"
v=$(pilote param FS_THR_ENABLE); [ "$v" != 1.5 ] && ok "FS_THR_ENABLE inchangé ($v)" || ko "FS_THR_ENABLE = $v"
jouer "$S?parametre=WPNAV_SPEED&valeur=500";       refuse "paramètre inconnu de Copter 4.7 (WPNAV_SPEED)" "ne connaît pas"
jouer "$S?ordre=terminaison";                      refuse "coupure, un appui : rien" "appuyer encore"
rm -f -- "$XDG_RUNTIME_DIR"/sermo-cible-mavlink/terminaison-*
jouer "$S?mission=$T/vol";                         reussi "mission de 3 points envoyée et relue" "relue et identique"
pilote mission >"$T/m1"; sed 1d "$T/m1" >"$T/m1-points"
printf '1 16 3 -35.3622000 149.1652000 20.00 0\n2 16 3 -35.3622000 149.1665000 30.50 5\n3 16 3 -35.3632000 149.1665000 25.00 0\n' >"$T/m1-attendu"
cmp -s "$T/m1-points" "$T/m1-attendu" && ok "le pilote relit les 3 points, attente comprise" \
    || ko "mission relue par le pilote : $(tr '\n' '|' <"$T/m1")"
jouer "$S?mission=$T/court";                       reussi "mission d'un point envoyée" "relue et identique"
n=$(pilote mission | wc -l); [ "$n" = 2 ] && ok "la mission courte remplace la précédente (départ + 1 point)" || ko "éléments gardés : $n"
# 2. En vol.
# 3. PILOTAGE, au sol puis en vol. D'abord : le drone accepte d'être armé
# (GPS et estimateur prêts), sinon c'est lui qui refuse, à juste titre.
pilote pret && ok "le simulateur est prêt à être armé" || { ko "simulateur jamais prêt : $(tail -1 "$T/pilote.err")"; }
deux "$S?pilote=mode&mode=guided";                 reussi "pilotage : GUIDED accepté" "accepté par le drone"
e=$(pilote etat); [ "$e" = "0 4" ] && ok "désarmé, GUIDED (4)" || ko "état : $e"
deux "$S?pilote=armer";                            reussi "pilotage : armer accepté" "accepté par le drone"
e=$(pilote etat); [ "${e%% *}" = 1 ] && ok "le drone est armé" || ko "état : $e"
deux "$S?pilote=desarmer";                         reussi "pilotage : désarmer (posé depuis 2 s) accepté" "accepté par le drone"
e=$(pilote etat); [ "${e%% *}" = 0 ] && ok "le drone est désarmé" || ko "état : $e"
jouer "$S?pilote=mode&mode=stabilize";             refuse "pilotage : STABILIZE refusé" "jamais stabilize"
deux "$S?pilote=mode&mode=loiter";                 refuse "pilotage : LOITER au sol refusé" "qu'en vol"
deux "$S?pilote=decoller";                         refuse "pilotage : décoller désarmé refusé" "armé, au sol"
deux "$S?pilote=armer";                            reussi "pilotage : armer de nouveau" "accepté par le drone"
deux "$S?pilote=decoller";                         reussi "pilotage : décoller accepté" "accepté par le drone"
deux "$S?pilote=desarmer";                         refuse "pilotage : désarmer pendant la montée refusé" "POSÉ"
for i in $(seq 1 30); do h=$(pilote hauteur); awk -v h="$h" 'BEGIN{exit !(h >= 9)}' && break; sleep 1; done
sleep 6; h=$(pilote hauteur)
awk -v h="$h" 'BEGIN{exit !(h >= 9.5 && h <= 10.5)}' && ok "décollé à la hauteur de la liste (10 m), stable : $h m" || ko "hauteur stabilisée : $h"
# Aller à un point : 70 m au nord-est du départ, à 20 m.
deux "$S?pilote=aller&lat=-35.3627000&lon=149.1658000&hauteur=20"; reussi "aller à : cible confirmée par le drone" "cible confirmée"
for i in $(seq 1 40); do
    pos=$(pilote position)
    awk -v p="$pos" 'BEGIN{split(p,v," "); exit !(v[1] > -35.36275 && v[1] < -35.36265 && v[2] > 149.16575 && v[2] < 149.16585 && v[3] > 19 && v[3] < 21)}' && break
    sleep 1
done
awk -v p="$pos" 'BEGIN{split(p,v," "); exit !(v[1] > -35.36275 && v[1] < -35.36265 && v[2] > 149.16575 && v[2] < 149.16585 && v[3] > 19 && v[3] < 21)}' \
    && ok "aller à : le drone est arrivé au point, à 20 m ($pos)" || ko "aller à : position $pos"
deux "$S?pilote=aller&lat=-35.3532620&lon=149.1652370&hauteur=20"; refuse "aller à : point à 1,1 km refusé" "hors de la barrière"
deux "$S?pilote=mode&mode=loiter";                 reussi "pilotage : LOITER en vol accepté" "accepté par le drone"
e=$(pilote etat); [ "$e" = "1 5" ] && ok "armé, LOITER (5)" || ko "état : $e"
deux "$S?pilote=aller&lat=-35.3630000&lon=149.1655000&hauteur=20"; refuse "aller à : en LOITER, refusé (jamais de changement de mode)" "DÉJÀ en GUIDED"
LOIN="pilotage lat -35.373262 lon 149.165237 rayon 300 hauteur 120 decollage 10 modes 4,5,6,9
"
deux "$S?pilote=mode&mode=guided" "$LOIN";         refuse "pilotage : hors de la barrière de sermo, GUIDED refusé" "hors de la barrière"
deux "$S?pilote=mode&mode=land" "$LOIN";           reussi "pilotage : hors de la barrière, LAND passe" "accepté par le drone"
for i in $(seq 1 40); do e=$(pilote etat); [ "${e%% *}" = 0 ] && break; sleep 1; done
[ "${e%% *}" = 0 ] && ok "posé et désarmé par l'autopilote après LAND" || ko "état après LAND : $e"
pilote regler FENCE_ENABLE 0 || ko "le pilote n'a pas pu couper la barrière"
deux "$S?pilote=mode&mode=guided";                 refuse "pilotage : barrière de l'autopilote coupée, GUIDED refusé" "inactive"
pilote regler FENCE_ENABLE 1 || ko "le pilote n'a pas pu rétablir la barrière"
pilote decoller 10 && ok "le pilote fait décoller le drone" || { ko "décollage : $(tail -2 "$T/pilote.err")"; exit 1; }
jouer "$S?parametre=WP_SPD&valeur=3";              refuse "en vol : réglage refusé" "ARMÉ"
[ "$(pilote param WP_SPD)" = 7.5 ] && ok "WP_SPD n'a pas bougé" || ko "WP_SPD = $(pilote param WP_SPD)"
jouer "$S?mission=$T/vol";                         refuse "en vol : mission refusée" "ARMÉ"
n=$(pilote mission | wc -l); [ "$n" = 2 ] && ok "en vol : la mission gardée n'a pas bougé" || ko "éléments gardés : $n"
jouer "$S?ordre=rtl";                              reussi "retour au départ accepté" "accepté par le drone"
sleep 1; e=$(pilote etat); [ "$e" = "1 6" ] && ok "mode RTL (6), armé" || ko "état après rtl : $e"
jouer "$S?ordre=land";                             reussi "atterrissage accepté" "accepté par le drone"
sleep 1; e=$(pilote etat); [ "$e" = "1 9" ] && ok "mode LAND (9)" || ko "état après land : $e"
pilote station 6 & STA=$!
sleep 0.5
jouer "$S?ordre=rtl";                              refuse "une station sol présente : refus" "station sol parle déjà"
wait "$STA"
e=$(pilote etat); [ "$e" = "1 9" ] && ok "le mode n'a pas bougé (LAND)" || ko "état après refus : $e"
jouer "$S?ordre=terminaison"
jouer "$S?ordre=terminaison";                      reussi "coupure, deux appuis : acceptée" "accepté par le drone"
sleep 1; e=$(pilote etat); [ "${e%% *}" = 0 ] && ok "le drone est désarmé" || ko "état après coupure : $e"

echo
[ $echecs = 0 ] && { echo "sermo-cible-mavlink (SITL) : tout tient"; exit 0; }
echo "sermo-cible-mavlink (SITL) : $echecs défaut(s)"; exit 1
