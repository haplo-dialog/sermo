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
python3 -I "$DRONE" --tcp "$PD" --journal "$T/j-desarme" >"$T/d.log" 2>&1 & PIDS+=($!)
python3 -I "$DRONE" --tcp "$PA" --arme --journal "$T/j-arme" >"$T/a.log" 2>&1 & PIDS+=($!)
python3 -I "$DRONE" --tcp "$PI" --intrus --journal "$T/j-intrus" >"$T/i.log" 2>&1 & PIDS+=($!)
# Lien muet : accepte la connexion TCP, n'envoie rien.
python3 -I -c '
import socket,sys
s=socket.socket(); s.setsockopt(socket.SOL_SOCKET,socket.SO_REUSEADDR,1)
s.bind(("127.0.0.1",int(sys.argv[1]))); s.listen(16); c=[]
while True: c.append(s.accept()[0])' "$PM" >/dev/null 2>&1 & PIDS+=($!)
for i in $(seq 1 100); do
    n=0; for p in $PD $PA $PI $PM; do (exec 3<>/dev/tcp/127.0.0.1/$p) 2>/dev/null && n=$((n + 1)); done
    [ $n = 4 ] && break; sleep 0.1
done
[ $n = 4 ] || { echo "faux drones pas prêts"; tail -3 "$T"/*.log; exit 2; }

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
commandes() { grep -c -x -e PARAM_SET -e COMMAND_LONG -e COMMAND_INT -e MISSION_COUNT -e MISSION_ITEM_INT -e MISSION_ITEM -e MISSION_CLEAR_ALL "$1" 2>/dev/null; }
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
jouer "$L" - 600 - "$D?ordre=rtl&parametre=WP_SPD&valeur=5";     refuse "paramètre ET ordre refusés" "OU mission"
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
jouer "$LM" - 600 - "$D?mission=$MI/bon&ordre=rtl";            refuse "mission ET ordre refusés" "OU mission"
jouer "$LM" - 600 - "tcp://127.0.0.1:$PA?mission=$MI/bon";     refuse "mission : drone ARMÉ, refus" "ARMÉ"
jouer "$LM" - 600 - "tcp://127.0.0.1:$PI?mission=$MI/bon";     refuse "mission : une station sol parle, refus" "station sol parle déjà"
# Toutes les gardes passées : l'envoi commence (une annonce MISSION_COUNT), le
# faux drone ne demande rien, sermo le dit et ne recommence pas.
jouer "$LM" - 600 - "$D?mission=$MI/bon&delai=3000"
apres=$(grep -c -x MISSION_COUNT "$T/j-desarme")
[ "$RC" = 1 ] && [ "$apres" = 1 ] && [ "$(commandes "$T/j-desarme")" = $((avant + 1)) ] && grep -q "s'est tu" "$T/err" \
    && ok "mission permise : toutes les gardes passées, une seule annonce, pas de nouvel essai" \
    || ko "mission permise : code $RC, $apres annonce(s), $(cat "$T/err")"

echo
[ $echecs = 0 ] && { echo "sermo-cible-mavlink (faux drone) : tout tient"; exit 0; }
echo "sermo-cible-mavlink (faux drone) : $echecs défaut(s)"; exit 1
