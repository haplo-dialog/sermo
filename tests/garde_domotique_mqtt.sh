#!/bin/bash
# SPDX-License-Identifier: GPL-2.0-or-later
# tests/garde_domotique_mqtt.sh — haplo-dialog — 2026 — GPL-2.0-or-later
#
# examples/domotique-mqtt/ lit des capteurs par <input mqtt> et commande par
# <action mqtt>. Ce test joue la VRAIE chaîne : fausse-maison.sh (vrai mosquitto,
# 127.0.0.1), vrai port sermo sous xvfb, vrais programmes d'aide de sermo-source,
# clics au clavier par xdotool. Il vérifie :
#   1 — les capteurs publiés arrivent dans la fenêtre (valeurs exportées)
#   2 — avec la liste blanche : « Allumer » publie on, « Appliquer » publie la consigne
#   3 — SANS liste blanche : les mêmes clics ne publient RIEN
#   4 — un message piégé (<, &, $(…)) reste du texte, rien n'est exécuté
#   5 — réglages invalides : 64 ; programme d'aide absent : 2 ; XML bien formé
#   6 — rien ne reste : fausse-maison arrêtée, port fermé, dossier effacé
# La liste blanche de sermo-cible-mqtt se lit dans le dossier personnel de
# l'utilisateur (getpwuid, $HOME n'y fait rien) : chaque fenêtre tourne donc dans un
# espace isolé (unshare -rm) où /root est un tmpfs. Le vrai ~/.config n'est JAMAIS lu
# ni écrit (même technique que garde_sermo_cible_mqtt.sh).
# Usage : garde_domotique_mqtt.sh [/chemin/vers/un-port-a-fenetre]   (défaut gtk3sermo)
RACINE=$(CDPATH= cd -- "$(dirname -- "$0")/.." && pwd)
EX="$RACINE/examples/domotique-mqtt"
AIDE="$RACINE/sermo-source/_build"
BIN="${1:-$(command -v gtk3sermo)}"
for t in mosquitto_pub mosquitto_sub xvfb-run xdotool python3 timeout unshare; do
    command -v "$t" >/dev/null || { echo "outil manquant : $t" >&2; exit 77; }
done
[[ -x "$(command -v mosquitto || echo /usr/sbin/mosquitto)" ]] || { echo "mosquitto absent" >&2; exit 77; }
[[ -x "$AIDE/sermo-source-mqtt" && -x "$AIDE/sermo-cible-mqtt" ]] || { echo "programmes d'aide non construits ($AIDE) : ci/construire.sh" >&2; exit 77; }
[[ -n "$BIN" && -x "$BIN" ]] || { echo "aucun port à fenêtre (gtk3sermo)" >&2; exit 77; }
unshare -rm true 2>/dev/null || { echo "espace isolé impossible (unshare -rm) : rien n'est essayé" >&2; exit 77; }

T=$(mktemp -d -p "$AIDE" banc-domotique.XXXXXX) || exit 2
MAISON=""
trap '[[ -n "$MAISON" ]] && kill "$MAISON" 2>/dev/null; wait 2>/dev/null; rm -rf -- "$T"' EXIT
export XDG_RUNTIME_DIR="$T/run"; mkdir -m 700 "$XDG_RUNTIME_DIR"
export SERMO_SOURCE_DIR="$AIDE"
P=$((20000 + $$ % 20000)); PRE=maison/salon
export MAISON_SERVEUR="mqtt://127.0.0.1:$P" MAISON_PREFIXE=$PRE
echec() { echo "ECHEC : $*"; exit 1; }
lire() { mosquitto_sub -p "$P" -t "$1" -C 1 -W 3 2>/dev/null; }

# ── 5 (d'abord, sans serveur) — réglages, XML ──────────────────────────────
for s in 'mqtt://a?x=1' 'http://a' 'mqtt://a b' 'mqtt://-a' 'mqtt://a/b' 'mqtt://a:99999x' "mqtt://a\"><x"; do
    MAISON_SERVEUR=$s "$EX/domotique.sh" --xml >/dev/null 2>&1; [[ $? == 64 ]] || echec "serveur « $s » accepté"
done
for p in 'maison/#' 'maison/+/x' '/maison' 'a b' 'maison//x' 'a/' "a&valeur=on"; do
    MAISON_PREFIXE=$p "$EX/domotique.sh" --xml >/dev/null 2>&1; [[ $? == 64 ]] || echec "préfixe « $p » accepté"
done
MAISON_INTERVALLE=0 "$EX/domotique.sh" --xml >/dev/null 2>&1; [[ $? == 64 ]] || echec "intervalle 0 accepté"
mkdir "$T/vide"
# Sans affichage : si le contrôle manquait, aucune fenêtre ne doit s'ouvrir sur l'écran de la personne.
env -u DISPLAY -u WAYLAND_DISPLAY SERMO_SOURCE_DIR="$T/vide" timeout 10 "$EX/domotique.sh" "$BIN" >/dev/null 2>&1; [[ $? == 2 ]] || echec "sermo-source-mqtt absent : code ≠ 2"
"$EX/domotique.sh" --xml > "$T/xml" || echec "--xml : code non nul"
sed 's/<input mqtt>/<input mqtt="1">/g; s/<action mqtt>/<action mqtt="1">/g' "$T/xml" |
    python3 -I -c 'import sys,xml.dom.minidom as m; d=m.parseString(sys.stdin.read()); a=[x.firstChild.data for x in d.getElementsByTagName("action") if x.getAttribute("mqtt")]; assert len(a)==3, a; assert a[2].endswith("consigne&valeur=$CONSIGNE"), a' \
    || echec "XML mal formé, ou actions MQTT inattendues"

# ── démarrage de la fausse maison ──────────────────────────────────────────
FAUSSE_MAISON_PORT=$P "$EX/fausse-maison.sh" > "$T/maison.log" 2>&1 & MAISON=$!
for _ in $(seq 1 50); do [[ -n "$(lire $PRE/temperature)" ]] && break; timeout 0.2 tail -f /dev/null; done
[[ $(lire $PRE/temperature) =~ ^[0-9]{2}\.[0-9]$ ]] || echec "fausse maison : pas de température ($(tail -n 2 "$T/maison.log"))"
[[ $(lire $PRE/lampe/etat) == off ]] || echec "fausse maison : lampe pas à off au départ"

# Une fenêtre d'essai : l'exemple tel quel, plus un minuteur qui la ferme après 5 s
# pour que sermo rende les valeurs. Touches jouées après l'ouverture.
"$EX/domotique.sh" --xml | python3 -I -c 'import sys; s=sys.stdin.read(); i=s.rindex("</vbox>"); print(s[:i] + "<timer interval=\"5\" visible=\"false\"><action>exit:essai</action></timer>\n  " + s[i:])' > "$T/essai.xml"
fenetre() {  # fenetre <avec-liste:0|1> <touches…> → valeurs exportées dans $T/sortie
    local liste=$1; shift
    LISTE="$liste" CIBLES="mqtt://127.0.0.1:$P $PRE/lampe on|off
mqtt://127.0.0.1:$P $PRE/consigne 18..24" XML="$T/essai.xml" BINR="$BIN" TOUCHES="$*" \
    timeout 40 unshare -rm bash -c '
        mount -t tmpfs tmpfs /root || exit 3
        if [ "$LISTE" = 1 ]; then
            mkdir -p -m 700 /root/.config/sermo
            printf "%s\n" "$CIBLES" > /root/.config/sermo/cibles-mqtt; chmod 600 /root/.config/sermo/cibles-mqtt
        fi
        xvfb-run -a sh -c '"'"'
            "$BINR" --file="$XML" & p=$!
            w=$(xdotool search --sync --onlyvisible --name . | head -n 1)
            timeout 1 tail -f /dev/null; xdotool windowfocus --sync "$w"
            for k in $TOUCHES; do
                case $k in pause) timeout 0.6 tail -f /dev/null ;; *) xdotool key --window "$w" "$k" ;; esac
            done
            wait $p'"'"'' > "$T/sortie" 2> "$T/erreurs"
}
exporte() { sed -n "s/^$1=\"\\(.*\\)\"\$/\\1/p" "$T/sortie" | head -n 1; }

# ── 1 + 3 — lecture, et clics SANS liste blanche ───────────────────────────
fenetre 0 space pause Tab Tab Tab space pause
grep -q '^EXIT="essai"' "$T/sortie" || echec "fenêtre sans liste : pas fermée par le minuteur ($(tail -n 3 "$T/erreurs"))"
sed -n '/^TEMPERATURE="/,/"$/p' "$T/sortie" | grep -qE '^TEMPERATURE="[0-9]{2}\.[0-9]$' || echec "température non lue : $(grep -A1 TEMPERATURE "$T/sortie")"
sed -n '/^HUMIDITE="/,/"$/p' "$T/sortie" | grep -qE '^HUMIDITE="[0-9]{2}$' || echec "humidité non lue"
sed -n '/^LAMPE="/,/"$/p' "$T/sortie" | grep -q '^LAMPE="off$' || echec "état de la lampe non lu"
timeout 1 tail -f /dev/null
[[ $(lire $PRE/lampe/etat) == off ]] || echec "SANS liste blanche, la lampe a été allumée"
! grep -q 'reçu' "$T/maison.log" || echec "SANS liste blanche, un message est parti : $(grep reçu "$T/maison.log")"
grep -q 'liste blanche' "$T/erreurs" || echec "le refus faute de liste blanche n'est pas dit"

# ── 2 — clics AVEC liste blanche ───────────────────────────────────────────
fenetre 1 space pause Tab Tab Tab space pause
grep -q '^EXIT="essai"' "$T/sortie" || echec "fenêtre avec liste : pas fermée par le minuteur"
timeout 1 tail -f /dev/null
grep -qE "reçu $PRE/lampe = on\$" "$T/maison.log" || echec "« Allumer » n'a rien publié : $(cat "$T/erreurs" | tail -n 3)"
grep -qE "reçu $PRE/consigne = 20(\.0+)?\$" "$T/maison.log" || echec "« Appliquer » n'a pas publié la consigne : $(grep reçu "$T/maison.log")"
[[ $(lire $PRE/lampe/etat) == on ]] || echec "la lampe n'est pas passée à on"

# ── 4 — message piégé ──────────────────────────────────────────────────────
kill "$MAISON"; wait "$MAISON" 2>/dev/null; MAISON=""
mosquitto_pub -p "$P" -t x -n 2>/dev/null && echec "fausse maison arrêtée, mais le port $P répond encore"
[[ -z "$(find "$XDG_RUNTIME_DIR" -name 'fausse-maison.*')" ]] || echec "fausse maison : dossier privé non effacé"
FAUSSE_MAISON_PORT=$P "$EX/fausse-maison.sh" > "$T/maison.log" 2>&1 & MAISON=$!
for _ in $(seq 1 50); do [[ -n "$(lire $PRE/temperature)" ]] && break; timeout 0.2 tail -f /dev/null; done
PIEGE='<b>&amp;$(touch '"$T"'/pwn)`id`'
mosquitto_pub -p "$P" -r -t $PRE/lampe/etat -m "$PIEGE"
fenetre 0 pause
grep -q '^EXIT="essai"' "$T/sortie" || echec "message piégé : la fenêtre ne s'est pas ouverte normalement ($(tail -n 3 "$T/erreurs"))"
grep -qF 'LAMPE="<b>&amp;' "$T/sortie" || echec "message piégé non affiché tel quel : $(grep -A1 LAMPE "$T/sortie")"
[[ ! -e "$T/pwn" ]] || echec "le message piégé a été exécuté"

# ── 6 — ménage ─────────────────────────────────────────────────────────────
kill "$MAISON"; wait "$MAISON" 2>/dev/null; MAISON=""
mosquitto_pub -p "$P" -t x -n 2>/dev/null && echec "port $P encore ouvert après l'arrêt"
[[ -z "$(find "$XDG_RUNTIME_DIR" -name 'fausse-maison.*')" ]] || echec "dossier privé non effacé"
echo "OK : capteurs lus, commandes publiées avec la liste blanche et RIEN sans elle, message piégé affiché sans effet, réglages contrôlés, rien ne reste"
