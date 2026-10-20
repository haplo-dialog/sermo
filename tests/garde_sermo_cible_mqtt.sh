#!/bin/bash
# SPDX-License-Identifier: GPL-2.0-or-later
# tests/garde_sermo_cible_mqtt.sh — haplo-dialog — 2026 — GPL-2.0-or-later
#
# Le VRAI programme d'écriture de <action mqtt> (sermo-source/_build/sermo-cible-mqtt),
# joué contre deux serveurs mosquitto à nous (clair et chiffré) sur 127.0.0.1.
# Un TÉMOIN abonné à tout (« # ») note ce qui est réellement publié : chaque
# cas permis doit arriver UNE fois, chaque cas refusé ne doit RIEN publier.
#
# La liste blanche vit dans ~/.config/sermo/cibles-mqtt et /etc/sermo/cibles-mqtt :
# pour ne toucher à aucun des deux, chaque cas tourne dans un espace isolé
# (unshare -rm) où /etc est recouvert (overlay, rien n'est écrit dans le vrai)
# et le dossier personnel (/root, celui de l'uid 0 de l'espace) est un tmpfs.
# CI : les conteneurs interdisent souvent cet espace isolé ; on y tourne en root
# dans un conteneur JETABLE, et le banc écrit alors directement /root/.config et
# /etc/sermo (mode « direct »), seulement si CI=true, uid 0, et aucune liste
# déjà présente. Partout ailleurs, sans espace isolé, le banc s'arrête (code 2).
#
# Vérifie : publication permise (liste, intervalle, $VARIABLE) ; refus d'une
# valeur hors liste, hors intervalle, non numérique, d'un sujet absent, d'un
# joker, d'un autre port ; aucune liste = refus ; liste personnelle lisible par
# d'autres, liste système modifiable par d'autres, ligne mal formée : TOUT
# refusé ; $HOME ne déplace pas la liste ; liste système bien protégée
# acceptée ; chiffré (mqtts://) ; serveur arrêté = refus dit ; une seule
# publication par appel (jamais répétée).
#
# Usage : garde_sermo_cible_mqtt.sh [racine du dépôt]
# Codes : 0 = tout tient · 1 = un défaut · 2 = outil ou programme manquant

set -u
export LC_ALL=C
RACINE=$(CDPATH= cd -- "${1:-$(dirname -- "$0")/..}" 2>/dev/null && pwd) || { echo "racine illisible" >&2; exit 2; }
AIDE="$RACINE/sermo-source/_build"
PROG="$AIDE/sermo-cible-mqtt"
[ -x "$PROG" ] || { echo "programme d'écriture absent : $PROG (ci/construire.sh)" >&2; exit 2; }
MOSQ=$(command -v mosquitto || echo /usr/sbin/mosquitto)
[ -x "$MOSQ" ] || { echo "outil manquant : mosquitto" >&2; exit 2; }
for t in mosquitto_sub unshare openssl timeout; do
    command -v "$t" >/dev/null || { echo "outil manquant : $t" >&2; exit 2; }
done
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
elif [ "${CI:-}" = true ] && [ "$(id -u)" = 0 ] && [ ! -e /etc/sermo/cibles-mqtt ] \
     && [ ! -e "$(getent passwd 0 | cut -d: -f6)/.config/sermo/cibles-mqtt" ]; then
    MODE=direct
    echo "note   : espace isolé impossible ; conteneur de CI en root : listes écrites en direct (conteneur jetable)"
else
    echo "espace isolé impossible (unshare -rm avec montages) et pas dans un conteneur de CI en root : rien n'est essayé" >&2
    exit 2
fi

T=$(mktemp -d -p "$AIDE" banc-sermo-cible-mqtt.XXXXXX) || exit 2
PIDS=()
trap 'for p in "${PIDS[@]}"; do kill "$p" 2>/dev/null; done; wait 2>/dev/null; rm -rf -- "$T"' EXIT

B=$((20000 + $$ % 20000)); P=$B; PT=$((B + 1)); PX=$((B + 2))
C="$T/certs"; mkdir -m 700 "$C"
openssl req -x509 -newkey rsa:2048 -nodes -days 1 -subj /CN=essai-ac -keyout "$C/ac.cle" -out "$C/ac.pem" 2>/dev/null || exit 2
openssl req -newkey rsa:2048 -nodes -subj /CN=localhost -keyout "$C/serveur.cle" -out "$C/serveur.csr" 2>/dev/null || exit 2
printf 'subjectAltName=DNS:localhost,IP:127.0.0.1\n' >"$C/ext"
openssl x509 -req -in "$C/serveur.csr" -CA "$C/ac.pem" -CAkey "$C/ac.cle" -CAcreateserial -days 1 \
    -extfile "$C/ext" -out "$C/serveur.pem" 2>/dev/null || exit 2
chmod 600 "$C"/*.cle
printf 'listener %s 127.0.0.1\nallow_anonymous true\npersistence false\n' "$P" >"$T/clair.conf"
printf 'listener %s 127.0.0.1\nallow_anonymous true\npersistence false\ncafile %s\ncertfile %s\nkeyfile %s\n' \
    "$PT" "$C/ac.pem" "$C/serveur.pem" "$C/serveur.cle" >"$T/tls.conf"
# Lancé en root (conteneur de CI), mosquitto passe sous le compte « mosquitto »
# et ne lit plus nos clés en 600 : il garde le compte qui l'a lancé.
for f in "$T"/*.conf; do printf 'user %s\n' "$(id -un)" >>"$f"; done
"$MOSQ" -c "$T/clair.conf" >"$T/clair.log" 2>&1 & PIDS+=($!)
"$MOSQ" -c "$T/tls.conf" >"$T/tls.log" 2>&1 & PIDS+=($!)
for i in $(seq 1 100); do
    n=0; for p in $P $PT; do (exec 3<>/dev/tcp/127.0.0.1/$p) 2>/dev/null && n=$((n + 1)); done
    [ $n = 2 ] && break; sleep 0.1
done
[ $n = 2 ] || { echo "serveurs d'essai pas prêts"; exit 2; }

# Les témoins : tout ce qui passe, sur chaque serveur.
mosquitto_sub -p "$P" -t '#' -v >"$T/vu-clair" 2>/dev/null & PIDS+=($!); E1=$!
mosquitto_sub -p "$PT" --cafile "$C/ac.pem" -h 127.0.0.1 -t '#' -v >"$T/vu-tls" 2>/dev/null & PIDS+=($!); E2=$!
sleep 0.4

echecs=0
ko() { echo "ECHEC : $*"; echecs=$((echecs + 1)); }
ok() { echo "ok     : $*"; }

# jouer PERSO SYSTEME MODE_PERSO MODE_SYSTEME ADRESSE [VAR=valeur…]
#   PERSO / SYSTEME : contenu de la liste (« - » : pas de fichier)
# Dans l'espace isolé ; remplit $T/rc et $T/err.
jouer() {
    local perso=$1 systeme=$2 mp=$3 ms=$4 adresse=$5
    shift 5
    # Ce que les témoins ont vu AVANT ce cas : seul ce qui arrive pendant compte.
    sleep 0.2; cat "$T/vu-clair" "$T/vu-tls" >"$T/vu-avant" 2>/dev/null
    printf '%s' "$perso" >"$T/l-perso"; printf '%s' "$systeme" >"$T/l-sys"
    rm -rf -- "$T/u" "$T/w"; mkdir -p "$T/u" "$T/w"
    if [ "$MODE" = direct ]; then
        local maison; maison=$(getent passwd 0 | cut -d: -f6)
        rm -f -- "$maison/.config/sermo/cibles-mqtt" /etc/sermo/cibles-mqtt
        if [ "$perso" != - ]; then mkdir -p "$maison/.config/sermo"; cp "$T/l-perso" "$maison/.config/sermo/cibles-mqtt"
            chmod "$mp" "$maison/.config/sermo/cibles-mqtt"; fi
        if [ "$systeme" != - ]; then mkdir -p /etc/sermo; cp "$T/l-sys" /etc/sermo/cibles-mqtt; chmod "$ms" /etc/sermo/cibles-mqtt; fi
        env "$@" "$PROG" "$adresse" 2>"$T/err"; echo $? >"$T/rc"
        rm -f -- "$maison/.config/sermo/cibles-mqtt" /etc/sermo/cibles-mqtt
        RC=$(cat "$T/rc"); return
    fi
    env "$@" unshare -rm bash -c '
        set -e
        mount -t overlay overlay -o "lowerdir=/etc,upperdir=$1/u,workdir=$1/w" /etc
        mount -t tmpfs tmpfs /root
        if [ "$2" != - ]; then mkdir -p /root/.config/sermo; cp "$1/l-perso" /root/.config/sermo/cibles-mqtt
            chmod "$4" /root/.config/sermo/cibles-mqtt; fi
        if [ "$3" != - ]; then mkdir -p /etc/sermo; cp "$1/l-sys" /etc/sermo/cibles-mqtt
            chmod "$5" /etc/sermo/cibles-mqtt; fi
        set +e
        "$6" "$7" 2>"$1/err"; echo $? >"$1/rc"' _ "$T" "$perso" "$systeme" "$mp" "$ms" "$PROG" "$adresse"
    RC=$(cat "$T/rc" 2>/dev/null || echo ?)
}
# Lignes vues pendant le cas (après celles de « vu-avant »).
nouveau() { sleep 0.3; cat "$T/vu-clair" "$T/vu-tls" | tail -n +"$(( $(wc -l <"$T/vu-avant") + 1 ))"; }
publie() {  # publie LIBELLÉ SUJET VALEUR : code 0 et un message, un seul
    local nb; nb=$(nouveau | grep -c -x -F "$2 $3")
    [ "$RC" = 0 ] && [ "$nb" = 1 ] && ok "$1" || ko "$1 : code $RC, vu $nb fois, $(cat "$T/err")"
}
refuse() {  # refuse LIBELLÉ SUJET RAISON : code 1, rien publié pendant le cas, raison dite
    local nb; nb=$(nouveau | wc -l)
    [ "$RC" = 1 ] && [ "$nb" = 0 ] && grep -q -- "$3" "$T/err" && ok "$1" \
        || ko "$1 : code $RC, vu $nb fois, raison : $(cat "$T/err")"
}
S="mqtt://127.0.0.1:$P"
L="$S t/lampe on|off
$S t/consigne 18..24
$S t/var 18..24
"

jouer "$L" - 600 - "$S?sujet=t/lampe&valeur=on";            publie "valeur de la liste publiée, une fois" t/lampe on
jouer "$L" - 600 - "$S?sujet=t/lampe&valeur=bleu";          refuse "valeur hors liste refusée" t/lampe "non permise"
jouer "$L" - 600 - "$S?sujet=t/consigne&valeur=21.5";       publie "valeur dans l'intervalle publiée" t/consigne 21.5
jouer "$L" - 600 - "$S?sujet=t/consigne&valeur=30";         refuse "valeur hors intervalle refusée" t/consigne "non permise"
jouer "$L" - 600 - "$S?sujet=t/consigne&valeur=1e1";        refuse "nombre hors intervalle (1e1) refusé" t/consigne "non permise"
jouer "$L" - 600 - "$S?sujet=t/consigne&valeur=abc";        refuse "valeur non numérique refusée" t/consigne "non permise"
jouer "$L" - 600 - "$S?sujet=t/var&valeur=%24CONSIGNE" CONSIGNE=20; publie "valeur=\$CONSIGNE publiée" t/var 20
jouer "$L" - 600 - "$S?sujet=t/var&valeur=%24CONSIGNE" CONSIGNE=99; refuse "valeur=\$CONSIGNE hors intervalle refusée" t/var "non permise"
jouer "$L" - 600 - "$S?sujet=t/var&valeur=%24ABSENTE";      refuse "variable absente refusée" t/var "n'existe pas"
jouer "$L" - 600 - "$S?sujet=t/four&valeur=on";             refuse "sujet hors liste refusé" t/four "pas dans la liste"
jouer "$L" - 600 - "$S?sujet=t/%23&valeur=on";              refuse "joker refusé" "t/#" "joker"
jouer "$L" - 600 - "mqtt://127.0.0.1:$PX?sujet=t/lampe&valeur=on"; refuse "autre port que la liste refusé" t/lampe "pas dans la liste"
jouer - - - - "$S?sujet=t/lampe&valeur=on";                 refuse "aucune liste blanche : refus" t/lampe "pas dans la liste"
jouer "$L" - 644 - "$S?sujet=t/lampe&valeur=on";            refuse "liste personnelle lisible par d'autres : refus" t/lampe "refusée"
jouer "$S t/lampe on|off
$S t/# on
" - 600 - "$S?sujet=t/lampe&valeur=on";                     refuse "ligne mal formée (joker) : tout refusé" t/lampe "mal formée"
jouer "$S t/lampe on||off
" - 600 - "$S?sujet=t/lampe&valeur=on";                     refuse "valeurs mal formées : tout refusé" t/lampe "mal formée"
jouer - "$L" - 664 "$S?sujet=t/lampe&valeur=on";            refuse "liste système modifiable par d'autres : refus" t/lampe "refusée"
jouer - "$L" - 644 "$S?sujet=t/lampe&valeur=off";           publie "liste système (644, root) acceptée" t/lampe off
jouer - - - - "$S?sujet=t/lampe&valeur=on" HOME="$T/faux"   # $HOME ne déplace pas la liste
mkdir -p "$T/faux/.config/sermo"; printf '%s' "$L" >"$T/faux/.config/sermo/cibles-mqtt"; chmod 600 "$T/faux/.config/sermo/cibles-mqtt"
jouer - - - - "$S?sujet=t/lampe&valeur=on" HOME="$T/faux";  refuse "\$HOME ne déplace pas la liste blanche" t/lampe "pas dans la liste"
# Chiffré : la liste vise le serveur chiffré.
LT="mqtts://127.0.0.1:$PT t/tls on|off
"
jouer "$LT" - 600 - "mqtts://127.0.0.1:$PT?sujet=t/tls&valeur=on&confiance=$C/ac.pem"; publie "chiffré (mqtts://) publié" t/tls on
jouer "$LT" - 600 - "mqtts://127.0.0.1:$PT?sujet=t/tls&valeur=off";                   refuse "chiffré sans confiance= refusé" t/tls "tout accepter"
# Serveur arrêté : refus dit, rien publié.
jouer "$L" - 600 - "mqtt://127.0.0.1:$PX?sujet=t/lampe&valeur=on"
L2="mqtt://127.0.0.1:$PX t/arret on|off
"
jouer "$L2" - 600 - "mqtt://127.0.0.1:$PX?sujet=t/arret&valeur=on&delai=800"
[ "$RC" = 1 ] && grep -q "impossible\|ne répond pas" "$T/err" && ok "serveur arrêté : refus dit" || ko "serveur arrêté : code $RC, $(cat "$T/err")"

sleep 0.3
kill "$E1" "$E2" 2>/dev/null
# Rien d'autre que les publications permises.
autres=$(grep -v -x -e 't/lampe on' -e 't/consigne 21.5' -e 't/var 20' -e 't/lampe off' -e 't/tls on' "$T/vu-clair" "$T/vu-tls" | head -3)
[ -z "$autres" ] && ok "le témoin n'a vu que les publications permises" || ko "publications inattendues : $autres"

echo
[ $echecs = 0 ] && { echo "sermo-cible-mqtt : tout tient"; exit 0; }
echo "sermo-cible-mqtt : $echecs défaut(s)"; exit 1
