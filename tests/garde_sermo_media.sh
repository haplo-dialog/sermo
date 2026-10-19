#!/bin/bash
# SPDX-License-Identifier: GPL-2.0-or-later
# tests/garde_sermo_media.sh — haplo-dialog — 2026 — GPL-2.0-or-later
#
# <input media> / <action media> : sermo-cible-media lance mpv et le
# commande, sermo-source-media lit son état. mpv n'a AUCUNE liste blanche de
# protocoles et sa commande IPC « run » exécute du shell (mesuré, J0) : tout le
# filtrage est dans ces programmes. Ce banc le prouve avec un FAUX mpv (Python,
# vrai socket, journal des arguments et de chaque requête reçue) :
#   1 — sources, noms, commandes, clés piégés : refus, AUCUN mpv lancé
#   2 — arguments exacts de mpv (durcissement, « -- », chemin réel) ; bwrap : local
#       sans réseau, source en lecture seule ; réseau REFUSÉ sans bwrap
#   3 — commandes : requêtes JSON fixes, nombres bornés ($VAR compris), jamais « run »
#   4 — états stopped/playing/paused/error, position, durée
#   5 — 4 lecteurs au plus ; le programme rend la main tout de suite (le gardien
#       n'hérite pas de la sortie) ; maître disparu → mpv arrêté, état stopped
#   6 — identifiants HTTP : jamais en argument, fichier d'en-tête 600 effacé après lecture
#   7 — socket qui n'est plus en 600 : plus aucune commande
# Avec mpv installé : 8 — vrai mpv sur un son SILENCIEUX fabriqué (sous xvfb).
# Avec un binaire réel en argument : 9 — vraie fenêtre sermo, clic « Lire » (xdotool),
#   état exporté « playing », puis sermo fermé → mpv arrêté (maître trouvé tout seul).
# Usage : garde_sermo_media.sh [/chemin/vers/gtk3sermo]
RACINE=$(CDPATH= cd -- "$(dirname -- "$0")/.." && pwd)
AIDE="$RACINE/sermo-source/_build"
C="$AIDE/sermo-cible-media"; Q="$AIDE/sermo-source-media"
BIN_REEL="${1:-}"
[[ -x "$C" && -x "$Q" ]] || { echo "programmes d'aide non construits ($AIDE) : ci/construire.sh" >&2; exit 77; }
for t in python3 timeout mktemp ffmpeg; do command -v "$t" >/dev/null || { echo "outil manquant : $t" >&2; exit 77; }; done

T=$(mktemp -d -p "$AIDE" banc-media.XXXXXX) || exit 2
MAITRE=""
nettoyer() {
    for d in "$XDG_RUNTIME_DIR"/sermo-media/*/; do
        g=$(cat "$d/gardien" 2>/dev/null); [[ "$g" =~ ^[0-9]+$ ]] && kill "$g" 2>/dev/null
    done
    [[ -n "$MAITRE" ]] && kill "$MAITRE" 2>/dev/null
    for n in "vrai-$$" "fen-$$"; do
        [[ -n "$VRAI_RUN" && -d "$VRAI_RUN/sermo-media/$n" ]] && { XDG_RUNTIME_DIR=$VRAI_RUN "$C" "media://$n?commande=arreter" >/dev/null 2>&1; rm -rf -- "$VRAI_RUN/sermo-media/$n"; }
    done
    rm -rf -- "$T" "$RUN"
}
trap nettoyer EXIT
# Dossier de session COURT : un socket Unix tient en 108 octets ; le vrai est
# /run/user/<uid>/sermo-media/NOM/ipc. Rien d'exécutable n'y est posé.
VRAI_RUN=${XDG_RUNTIME_DIR:-}
RUN=$(mktemp -d -p "/run/user/$(id -u)" bm.XXXX 2>/dev/null) || RUN=$(mktemp -d -p "$T" r.XXXX)
export XDG_RUNTIME_DIR="$RUN"; chmod 700 "$RUN"
echec() { echo "ECHEC : $*"; exit 1; }
attendre() { local i; for i in $(seq 1 50); do eval "$1" && return 0; timeout 0.1 tail -f /dev/null; done; return 1; }
# Un « sermo » de remplacement à surveiller : un sleep qu'on tue à la main.
sleep 600 & MAITRE=$!; export SERMO_MEDIA_MAITRE=$MAITRE

mkdir "$T/faux" "$T/medias"
ffmpeg -hide_banner -loglevel error -f lavfi -i anullsrc=r=22050:cl=mono -t 5 "$T/medias/silence.ogg" || exit 2
cat > "$T/faux/mpv" <<'FAUX'
#!/usr/bin/python3 -I
# Faux mpv : un vrai socket ; journal dans le dossier du lecteur (seul endroit
# inscriptible dans bwrap) ; FAKE_MODE=erreur → échec immédiat (rc 2).
import json, os, socket, sys, time
ipc = [a.split("=", 1)[1] for a in sys.argv if a.startswith("--input-ipc-server=")][0]
d = os.path.dirname(ipc)
open(d + "/appels", "a").write(json.dumps(sys.argv[1:]) + "\n")
inc = [a.split("=", 1)[1] for a in sys.argv if a.startswith("--include=")]
if inc:
    open(d + "/include-lu", "w").write(open(inc[0]).read())
if os.environ.get("FAKE_MODE") == "erreur":
    sys.exit(2)
# Comme le vrai mpv avec --no-terminal : rien sur stderr, tout dans --log-file.
log = [a.split("=", 1)[1] for a in sys.argv if a.startswith("--log-file=")]
def journal(t):
    if log:
        open(log[0], "a").write(t)
if os.environ.get("FAKE_MODE") == "occupee":
    journal("[e][ffmpeg/demuxer] video4linux2,v4l2: Cannot open video device /dev/video0: Device or resource busy\n")
    sys.exit(2)
if os.environ.get("FAKE_MODE") == "premiere":
    n = d + "/tentatives"
    k = int(open(n).read()) if os.path.exists(n) else 0
    open(n, "w").write(str(k + 1))
    if k == 0:
        journal("[e][lavf] avformat_open_input() failed\n")
        sys.exit(2)
if os.environ.get("FAKE_SONDE"):                # quels sockets de session la cage laisse-t-elle joindre ?
    r = []
    for nom in os.environ["FAKE_SONDE"].split(":"):
        t = socket.socket(socket.AF_UNIX)
        try:
            t.connect(nom); r.append("joint " + nom)
        except OSError:
            r.append("refus " + nom)
        t.close()
    r.append("DBUS " + ("présent" if "DBUS_SESSION_BUS_ADDRESS" in os.environ else "absent"))
    open(d + "/sonde", "w").write("\n".join(r) + "\n")
os.umask(0o077)
s = socket.socket(socket.AF_UNIX); s.bind(ipc); s.listen(4); s.settimeout(0.2)
if os.environ.get("FAKE_MODE") == "tardif":      # socket d'abord, puis échec sans image (mesuré sur la vraie caméra)
    time.sleep(0.6)
    journal("[e][lavf] avformat_open_input() failed\n")
    sys.exit(2)
etat = {"pause": False, "volume": 100, "mute": False, "time-pos": 0.0, "duration": 5.0, "video-params": {"w": 640, "h": 480}}
fin = time.time() + float(os.environ.get("FAKE_DUREE", "60"))
while time.time() < fin:
    try:
        c, _ = s.accept()
    except socket.timeout:
        continue
    c.settimeout(1)
    try:
        ligne = c.makefile().readline()
    except Exception:
        c.close(); continue
    open(d + "/requetes", "a").write(ligne)
    try:
        cmd = json.loads(ligne)["command"]
    except Exception:
        c.sendall(b'{"error":"invalid parameter"}\n'); c.close(); continue
    c.sendall(b'{"event":"bavard"}\n')            # un évènement avant la réponse
    if cmd[0] == "get_property":
        c.sendall(json.dumps({"data": etat.get(cmd[1]), "request_id": 0, "error": "success"}).encode() + b"\n")
    elif cmd[0] == "set_property":
        etat[cmd[1]] = cmd[2]; c.sendall(b'{"request_id":0,"error":"success"}\n')
    elif cmd[0] == "seek":
        etat["time-pos"] = float(cmd[1]); c.sendall(b'{"request_id":0,"error":"success"}\n')
    elif cmd[0] == "quit":
        c.sendall(b'{"request_id":0,"error":"success"}\n'); c.close(); sys.exit(0)
    else:
        c.sendall(b'{"request_id":0,"error":"error running command"}\n')
    c.close()
sys.exit(0)
FAUX
chmod +x "$T/faux/mpv"
VRAI_PATH=$PATH
export PATH="$T/faux:$PATH"
L() { echo "$XDG_RUNTIME_DIR/sermo-media/$1"; }
cible() { "$C" "media://$1?$(sed 's/&/%26/g; s/\$/%24/g' <<<"$2")"; }
etat() { "$Q" "media://$1?donnee=${2:-etat}"; }
aucun_mpv() { ! ls "$XDG_RUNTIME_DIR"/sermo-media/*/appels >/dev/null 2>&1; }

# ── 1 — refus, rien de lancé ────────────────────────────────────────────────
S="$T/medias/silence.ogg"
for src in 'edl://x' 'av://lavfi:sine' 'concat:a|b' 'lavf://x' 'ftp://h/x' 'file://relatif.ogg' 'file:///etc' \
           'file:///etc/shadow' 'file:///nexiste.ogg' 'http://u:p@h/x' 'http://h@x/' 'http://-h/x' 'http://h/a b' \
           'http://h:0/x' 'http://h:99999/x' 'https://[::1/x' 'rtsp://h/x"y' "file://$T/medias" ''; do
    cible essai "commande=lire&source=$src" >/dev/null 2>&1; rc=$?
    [[ $rc == 1 ]] && aucun_mpv || echec "source « $src » : code $rc ou mpv lancé"
done
"$C" $'media://essai?commande=lire%26source=file:///a\nb' >/dev/null 2>&1; [[ $? == 1 ]] && aucun_mpv || echec "source avec fin de ligne acceptée"
for n in '../x' 'a/b' 'a b' '' "$(printf 'n%.0s' $(seq 1 33))" 'é'; do
    "$C" "media://$n?commande=lire%26source=file://$S" >/dev/null 2>&1; [[ $? == 1 ]] && aucun_mpv || echec "nom « $n » accepté"
done
"$C" "mqtt://essai?commande=lire%26source=file://$S" >/dev/null 2>&1; [[ $? == 1 ]] || echec "schéma d'adresse autre que media:// accepté"
for c in run quit set_property loadfile 'pause;id' ''; do
    cible essai "commande=$c" >/dev/null 2>&1; [[ $? == 1 ]] || echec "commande « $c » acceptée"
done
cible essai "commande=lire&source=file://$S&cmd=x" >/dev/null 2>&1; [[ $? == 1 ]] && aucun_mpv || echec "clé inconnue acceptée"
cible essai "commande=pause" >/dev/null 2>&1; [[ $? == 1 ]] || echec "pause sans lecteur acceptée"

# ── 2 — arguments de mpv, bac à sable ───────────────────────────────────────
OUT=$( { time -p cible essai "commande=lire&source=file://$S" ; } 2>&1 ); rc=$?
[[ $rc == 0 ]] || echec "lire (faux mpv) : $OUT"
duree=$(sed -n 's/^real \([0-9]*\).*/\1/p' <<<"$OUT"); (( ${duree:-9} < 5 )) || echec "lire ne rend pas la main (${duree} s) : le gardien garde-t-il la sortie ?"
attendre '[[ -s "$(L essai)/appels" ]]' || echec "faux mpv non lancé"
# Un descripteur hérité AU-DELÀ de 2 (un tube que l'appelant lit jusqu'au bout) :
# si le gardien le gardait, la lecture attendrait la fin de mpv (60 s).
debut=$(date +%s)
X=$(timeout 10 "$C" "media://fd3?commande=lire%26source=file://$S" 3>&1 >/dev/null 2>&1); rc=$?
(( $(date +%s) - debut < 6 )) && [[ $rc == 0 ]] || echec "le gardien garde un descripteur hérité (fd 3) : l'appelant attend ($rc)"
"$C" "media://fd3?commande=arreter" >/dev/null
python3 -I - "$(L essai)/appels" "$(L essai)" "$(readlink -f "$S")" <<'PY' || echec "arguments de mpv : $(cat "$(L essai)/appels")"
import json, shutil, sys
a = json.loads(open(sys.argv[1]).readline())
attendu = ["--no-config", "--no-terminal", "--idle=no", "--ytdl=no", "--load-scripts=no", "--load-auto-profiles=no",
           "--input-default-bindings=no", "--load-unsafe-playlists=no", "--access-references=no"] \
          + (["--ao=pulse,null"] if shutil.which("bwrap") else []) + [
           "--input-ipc-server=" + sys.argv[2] + "/ipc", "--title=sermo — essai", "--", sys.argv[3]]
assert a == attendu, a
PY
if command -v bwrap >/dev/null; then
    G=$(cat "$(L essai)/gardien"); B=$(cat "$(L essai)/mpv")
    tr '\0' ' ' < /proc/$B/cmdline > "$T/bwrap.cmd" 2>/dev/null
    # Sockets de session : la cage ne laisse joindre que le son (pulse) ; ni un
    # autre socket du dossier de session (bus D-Bus…), ni la variable du bus.
    mkdir -p -m 700 "$RUN/pulse"
    python3 -I -c 'import socket,sys,time
ss=[]
for p in sys.argv[1:]:
    s=socket.socket(socket.AF_UNIX); s.bind(p); s.listen(4); ss.append(s)
time.sleep(30)' "$RUN/bus-essai" "$RUN/pulse/native" & SONDES=$!
    attendre '[[ -S "$RUN/pulse/native" ]]' || echec "sondes non prêtes"
    FAKE_SONDE="$RUN/bus-essai:$RUN/pulse/native" DBUS_SESSION_BUS_ADDRESS="unix:path=$RUN/bus-essai" \
        cible sonde "commande=lire&source=file://$S" >/dev/null || echec "lecteur sonde"
    attendre '[[ -s "$(L sonde)/sonde" ]]' || echec "sonde : pas de résultat"
    grep -qx "refus $RUN/bus-essai" "$(L sonde)/sonde" && grep -qx "joint $RUN/pulse/native" "$(L sonde)/sonde" \
        && grep -qx "DBUS absent" "$(L sonde)/sonde" || echec "cage : sockets de session : $(cat "$(L sonde)/sonde")"
    cible sonde "commande=arreter" >/dev/null; kill $SONDES 2>/dev/null
    grep -q '^.*bwrap .*--unshare-net' "$T/bwrap.cmd" && grep -qF -- "--ro-bind $(readlink -f "$S") $(readlink -f "$S")" "$T/bwrap.cmd" \
        && grep -qF -- "--tmpfs $HOME" "$T/bwrap.cmd" || echec "bwrap (local) : $(cat "$T/bwrap.cmd")"
fi
cible essai "commande=arreter" >/dev/null
mkdir "$T/sansbac"; for c in bash python3 sleep; do ln -s "$(command -v $c)" "$T/sansbac/$c"; done; ln -s "$T/faux/mpv" "$T/sansbac/mpv"
# Adresses encodées AVANT de réduire le PATH (sed n'y est plus).
A_NET="media://net?commande=lire%26source=https://cam.test/flux"; A_LOC="media://loc?commande=lire%26source=file://$S"
PATH="$T/sansbac" "$C" "$A_NET" >/dev/null 2>"$T/refus"; rc=$?
[[ $rc == 1 && ! -e "$(L net)/appels" ]] && grep -q 'sans bac à sable (bwrap absent' "$T/refus" || echec "source réseau sans bwrap : rc=$rc, $(cat "$T/refus")"
PATH="$T/sansbac" "$C" "$A_LOC" 2>"$T/note" >/dev/null || echec "local sans bwrap refusé : $(cat "$T/note")"
grep -q 'sans bac à sable' "$T/note" || echec "absence de bwrap non signalée"
cible loc "commande=arreter" >/dev/null

# ── 3 — commandes ───────────────────────────────────────────────────────────
cible essai "commande=lire&source=file://$S" >/dev/null || echec "relance"
attendre '[[ -S "$(L essai)/ipc" ]]' || echec "socket absent"
: > "$(L essai)/requetes"
cible essai "commande=pause" && cible essai "commande=muet" && cible essai "commande=son" && cible essai "commande=reprendre" \
    && cible essai "commande=aller&valeur=42" && cible essai "commande=volume&valeur=7" || echec "une commande valide a échoué"
VOL=55.9 cible essai 'commande=volume&valeur=$VOL' || echec "volume=\$VOL refusé"
# Locale française : « 55.9 » doit encore se lire 55 (un strtod sous la locale de
# l'utilisateur couperait au point et refuserait).
if locale -a 2>/dev/null | grep -qi '^fr_FR.utf'; then
    LC_ALL=fr_FR.UTF-8 VOL=55.9 cible essai 'commande=volume&valeur=$VOL' || echec "volume=55.9 refusé sous une locale française"
else
    VOL=55.9 cible essai 'commande=volume&valeur=$VOL' >/dev/null   # même requête, pour garder le compte
    echo "  note : pas de locale fr_FR.UTF-8 : lecture du nombre sous locale étrangère non jouée"
fi
for v in -1 1x '' nan inf '1;run' '$NEXISTE' '$a;b' 86401; do
    cible essai "commande=aller&valeur=$v" >/dev/null 2>&1 && echec "aller=« $v » accepté"
done
for v in 101 -5 '7 ' 'run'; do
    cible essai "commande=volume&valeur=$v" >/dev/null 2>&1 && echec "volume=« $v » accepté"
done
VOL='12, "run"' cible essai 'commande=volume&valeur=$VOL' >/dev/null 2>&1 && echec "volume piégé par variable accepté"
diff "$(L essai)/requetes" - >/dev/null <<'J' || echec "requêtes reçues par mpv : $(cat "$(L essai)/requetes")"
{"command":["set_property","pause",true]}
{"command":["set_property","mute",true]}
{"command":["set_property","mute",false]}
{"command":["set_property","pause",false]}
{"command":["seek",42,"absolute"]}
{"command":["set_property","volume",7]}
{"command":["set_property","volume",55]}
{"command":["set_property","volume",55]}
J

# ── 4 — états ───────────────────────────────────────────────────────────────
[[ $(etat essai) == playing ]] || echec "état : $(etat essai)"
cible essai "commande=pause"; [[ $(etat essai) == paused ]] || echec "état après pause"
[[ $(etat essai position) == 42.0 && $(etat essai duree) == 5.0 ]] || echec "position/durée : $(etat essai position) / $(etat essai duree)"
cible essai "commande=arreter" >/dev/null; [[ $(etat essai) == stopped && -z $(etat essai position) ]] || echec "état après arrêt : $(etat essai)"
[[ $(etat jamais) == stopped ]] || echec "lecteur inconnu : $(etat jamais)"
FAKE_MODE=erreur cible err "commande=lire&source=file://$S" >/dev/null 2>&1; [[ $? == 1 ]] || echec "échec de mpv non signalé"
attendre '[[ $(etat err) == error ]]' || echec "état après échec : $(etat err)"
"$Q" "media://essai?donnee=run" >/dev/null 2>&1; [[ $? == 1 ]] || echec "donnée inconnue acceptée"
FAKE_DUREE=1 cible court "commande=lire&source=file://$S" >/dev/null
attendre '[[ $(etat court) == stopped ]]' || echec "fin de lecture : $(etat court)"

# ── 5 — limites, durée de vie ───────────────────────────────────────────────
for n in l1 l2 l3 l4; do cible $n "commande=lire&source=file://$S" >/dev/null || echec "lecteur $n refusé"; done
cible l5 "commande=lire&source=file://$S" >/dev/null 2>&1 && echec "5e lecteur accepté"
cible l1 "commande=lire&source=file://$S" >/dev/null || echec "relancer un lecteur existant refusé (limite mal comptée)"
P=$(cat "$(L l2)/mpv")
kill "$MAITRE"; MAITRE=""
attendre '! kill -0 "$P" 2>/dev/null' || echec "maître disparu : mpv toujours vivant"
attendre '[[ $(etat l2) == stopped ]]' || echec "maître disparu : état $(etat l2) au lieu de stopped"
for n in l1 l3 l4; do attendre '[[ $(etat '$n') == stopped ]]' || echec "maître disparu : $n encore $(etat $n)"; done
sleep 600 & MAITRE=$!; export SERMO_MEDIA_MAITRE=$MAITRE

# ── 6 — identifiants HTTP ───────────────────────────────────────────────────
printf 'camera:s3cret-tres\n' > "$T/id"; chmod 600 "$T/id"; printf 'a:b\n' > "$T/id644"; chmod 644 "$T/id644"; ln -s "$T/id" "$T/idlien"
for f in "$T/id644" "$T/idlien" relatif "$T/nexiste"; do
    cible web "commande=lire&source=https://cam.test/flux&identifiants=$f" >/dev/null 2>&1 && echec "identifiants « $f » acceptés"
done
cible web "commande=lire&source=rtsp://cam.test/flux&identifiants=$T/id" >/dev/null 2>&1 && echec "RTSP avec identifiants accepté"
if command -v bwrap >/dev/null; then
    cible web "commande=lire&source=https://cam.test/flux&identifiants=$T/id" >/dev/null || echec "HTTPS avec identifiants refusé"
    attendre '[[ -s "$(L web)/include-lu" ]]' || echec "fichier d'en-tête non lu par mpv"
    grep -qx "http-header-fields=\"Authorization: Basic $(printf 'camera:s3cret-tres' | base64)\"" "$(L web)/include-lu" || echec "en-tête : $(cat "$(L web)/include-lu")"
    ! grep -q 's3cret' "$(L web)/appels" || echec "le mot de passe est dans les arguments de mpv"
    for c in /proc/[0-9]*/cmdline; do { tr '\0' ' ' < "$c"; } 2>/dev/null; echo; done | grep -v 'grep\|garde_sermo_media' | grep -q 's3cret' && echec "mot de passe visible dans un processus"
    attendre '[[ ! -e "$(L web)/auth.conf" ]]' || echec "fichier d'en-tête (identifiants) non effacé"
    cible web "commande=arreter" >/dev/null
else
    echo "  note : bwrap absent : identifiants HTTP non joués (réseau refusé sans bac)"
fi

# ── 7 — socket détourné ─────────────────────────────────────────────────────
cible sock "commande=lire&source=file://$S" >/dev/null; attendre '[[ -S "$(L sock)/ipc" ]]' || echec "socket (7)"
chmod 666 "$(L sock)/ipc"; : > "$(L sock)/requetes"
cible sock "commande=pause" >/dev/null 2>&1 && echec "commande acceptée vers un socket en 666"
[[ ! -s "$(L sock)/requetes" ]] || echec "une requête est partie vers un socket en 666"
chmod 600 "$(L sock)/ipc"; cible sock "commande=arreter" >/dev/null

# ── 7b — caméra (source=v4l2://) : faux mpv, la vraie caméra n'est JAMAIS ouverte ──
NOEUD=""; for nd in /dev/video[0-9]*; do [ -c "$nd" ] && { NOEUD=$nd; break; }; done
if [[ -n "$NOEUD" ]]; then
    N=${NOEUD#/dev/video}
    mkdir -p "$T/v4l/video$N" "$T/v4l/video99"; printf '0\n' > "$T/v4l/video$N/index"
    export SERMO_MEDIA_V4L_SYSFS="$T/v4l"
    for src in 'v4l2://relatif' 'v4l2:///dev/null' 'v4l2:///dev/video99' "v4l2://${NOEUD}x" "v4l2://${NOEUD};id" \
               'v4l2:///dev/../dev/video0' 'v4l2://' 'av://v4l2:/dev/video0'; do
        cible cam "commande=lire&source=$src" >/dev/null 2>&1; rc=$?
        [[ $rc == 1 && ! -e "$(L cam)/appels" ]] || echec "caméra « $src » : code $rc ou mpv lancé"
    done
    printf '1\n' > "$T/v4l/video$N/index"            # le même nœud vu comme « métadonnées »
    cible cam "commande=lire&source=v4l2://$NOEUD" >/dev/null 2>&1 && echec "nœud de métadonnées accepté"
    printf '0\n' > "$T/v4l/video$N/index"
    for o in 'taille=800x600' 'format=h264' 'taille=640x480;id' 'format='; do
        cible cam "commande=lire&source=v4l2://$NOEUD&$o" >/dev/null 2>&1 && echec "caméra : « $o » accepté"
    done
    cible fic "commande=lire&source=file://$S&taille=640x480" >/dev/null 2>&1 && echec "taille= accepté pour un fichier"
    [[ ! -e "$(L cam)/appels" && ! -e "$(L fic)/appels" ]] || echec "mpv lancé malgré un refus (caméra)"
    cible cam "commande=lire&source=v4l2://$NOEUD&taille=1280x720&format=yuyv422" >/dev/null || echec "caméra valide refusée"
    attendre '[[ -s "$(L cam)/appels" ]]' || echec "caméra : faux mpv non lancé"
    python3 -I - "$(L cam)/appels" "$NOEUD" <<'PY2' || echec "caméra : arguments de mpv : $(cat "$(L cam)/appels")"
import json, sys
a = json.loads(open(sys.argv[1]).readline())
assert a[-2:] == ["--", "av://v4l2:" + sys.argv[2]], a[-2:]
assert "--demuxer-lavf-o=video_size=1280x720,input_format=yuyv422" in a, a
assert "--title=sermo — CAMÉRA ACTIVE — cam" in a, a
PY2
    if command -v bwrap >/dev/null; then
        tr '\0' ' ' < /proc/$(cat "$(L cam)/mpv")/cmdline > "$T/cam.cmd" 2>/dev/null
        grep -qF -- "--dev-bind $NOEUD $NOEUD" "$T/cam.cmd" && grep -q -- '--unshare-net' "$T/cam.cmd" || echec "caméra : bwrap : $(cat "$T/cam.cmd")"
    fi
    cible cam "commande=arreter" >/dev/null
    rm -f "$(L cam)/appels"
    FAKE_MODE=occupee cible cam "commande=lire&source=v4l2://$NOEUD" >/dev/null 2>"$T/occ"; rc=$?
    [[ $rc == 1 ]] && grep -q 'caméra occupée ou indisponible' "$T/occ" && grep -q 'rien n.a été forcé' "$T/occ" || echec "caméra occupée : rc=$rc, $(cat "$T/occ")"
    attendre '[[ $(etat cam) == error ]]' || echec "caméra occupée : état $(etat cam)"
    [[ $(wc -l < "$(L cam)/appels") == 2 ]] || echec "caméra occupée : $(wc -l < "$(L cam)/appels") tentative(s) au lieu d'une nouvelle tentative, puis l'abandon"
    rm -f "$(L cam)/appels" "$(L cam)/tentatives"
    FAKE_MODE=premiere cible cam "commande=lire&source=v4l2://$NOEUD" >/dev/null 2>"$T/prem" || echec "première ouverture ratée : pas de nouvelle tentative ($(cat "$T/prem"))"
    [[ $(cat "$(L cam)/tentatives") == 2 && $(etat cam) == playing ]] || echec "nouvelle tentative : $(cat "$(L cam)/tentatives") essai(s), état $(etat cam)"
    cible cam "commande=arreter" >/dev/null
    rm -f "$(L cam)/appels"
    FAKE_MODE=tardif cible cam "commande=lire&source=v4l2://$NOEUD" >/dev/null 2>"$T/tard"; rc=$?
    [[ $rc == 1 ]] && grep -q 'occupée ou indisponible' "$T/tard" || echec "socket sans image puis échec : annoncé comme lecture (rc=$rc) $(cat "$T/tard")"
    FAKE_MODE=erreur cible cam2 "commande=lire&source=v4l2://$NOEUD" >/dev/null 2>&1
    [[ $(wc -l < "$(L cam2)/appels") == 1 ]] || echec "échec ordinaire de la caméra : relancé quand même"
    # Caméra incrustée : le titre n'est plus visible, l'indicateur passe dans l'image.
    rm -f "$(L cam)/appels"
    ECRAN=4242 DISPLAY=:99 cible cam "commande=lire&source=v4l2://$NOEUD&dans=\$ECRAN" >/dev/null 2>&1 || echec "caméra incrustée refusée"
    attendre '[[ -s "$(L cam)/appels" ]]' || echec "caméra incrustée : faux mpv non lancé"
    python3 -I -c 'import json,sys; a=json.loads(open(sys.argv[1]).readline()); assert "--wid=4242" in a and "--osd-msg1=CAMÉRA ACTIVE — cam" in a' "$(L cam)/appels" \
        || echec "caméra incrustée sans --wid ou sans indicateur dans l'image : $(cat "$(L cam)/appels")"
    cible cam "commande=arreter" >/dev/null
    unset SERMO_MEDIA_V4L_SYSFS
    camera=" ; caméra (v4l2) : nœud, taille, format, occupée, incrustée"
else
    echo "  note : aucun /dev/videoN : section caméra non jouée"
fi

# ── 7c — dans=$VAR : incrustation (--wid) ou repli sur une fenêtre séparée ──
rm -f "$(L inc)/appels"
ECRAN=0x2a DISPLAY=:99 cible inc "commande=lire&source=file://$S&dans=\$ECRAN" >/dev/null 2>&1 || echec "dans= valide refusé"
attendre '[[ -s "$(L inc)/appels" ]]' || echec "dans= : faux mpv non lancé"
grep -qF -e '"--wid=42"' "$(L inc)/appels" || echec "dans= : pas de --wid=42 : $(cat "$(L inc)/appels")"
# Incrusté, mpv qui lit la souris de la fenêtre sermo relançait l'action (clic laissé sur le bouton, xfwm4).
grep -qF -e '"--input-cursor=no"' "$(L inc)/appels" && grep -qF -e '"--input-vo-keyboard=no"' "$(L inc)/appels" \
    || echec "dans= : mpv incrusté lit encore souris/clavier : $(cat "$(L inc)/appels")"
grep -qF -e 'osd-msg1' "$(L inc)/appels" && echec "dans= : indicateur caméra pour un fichier"
cible inc "commande=arreter" >/dev/null; rm -f "$(L inc)/appels"
for cas in "ECRAN= DISPLAY=:99" "ECRAN=42 DISPLAY="; do       # zone vide (Wayland, autre port) ou pas de X11
    env $cas "$C" "media://inc?commande=lire%26source=file://$S%26dans=%24ECRAN" >/dev/null 2>"$T/repli" || echec "repli ($cas) : refusé"
    attendre '[[ -s "$(L inc)/appels" ]]' || echec "repli ($cas) : faux mpv non lancé"
    grep -q 'fenêtre séparée' "$T/repli" && ! grep -qF -e '--wid' "$(L inc)/appels" || echec "repli ($cas) : $(cat "$T/repli") $(cat "$(L inc)/appels")"
    cible inc "commande=arreter" >/dev/null; rm -f "$(L inc)/appels"
done
for v in abc 0 -5 +42 0x1ffffffff '12 34' '42;id'; do
    ECRAN="$v" DISPLAY=:99 cible inc "commande=lire&source=file://$S&dans=\$ECRAN" >/dev/null 2>&1 && echec "dans= : « $v » accepté"
done
for d in ECRAN '$1ECRAN' '$' '$A-B'; do
    cible inc "commande=lire&source=file://$S&dans=$d" >/dev/null 2>&1 && echec "dans=$d accepté"
done
[[ ! -e "$(L inc)/appels" ]] || echec "mpv lancé malgré un dans= refusé"

# ── 8 — vrai mpv (son silencieux, sous xvfb) ────────────────────────────────
# Le VRAI dossier de session : mpv y trouve le serveur de son (pulse/native).
export PATH="$VRAI_PATH" XDG_RUNTIME_DIR="$VRAI_RUN"
V="vrai-$$"; F="fen-$$"
if command -v mpv >/dev/null && command -v xvfb-run >/dev/null; then
    timeout 60 xvfb-run -a bash -c '
        C=$1; Q=$2; S=$3
        "$C" "media://$4?commande=lire%26source=file://$S" >/dev/null || exit 11
        for i in $(seq 1 30); do [ "$("$Q" media://$4?donnee=etat)" = playing ] && break; sleep 0.1; done
        [ "$("$Q" media://$4?donnee=etat)" = playing ] || exit 12
        "$C" "media://$4?commande=pause" || exit 13
        [ "$("$Q" media://$4?donnee=etat)" = paused ] || exit 14
        "$C" "media://$4?commande=aller%26valeur=2" || exit 15
        # Avec pulse (seul son remonté dans la cage), mpv retranche la latence du
        # tampon de la position en pause : 1.86 pour 2 (mesuré). ±0,25 s admis.
        p=$("$Q" "media://$4?donnee=position"); awk -v p="$p" "BEGIN{exit !(p >= 1.75 && p <= 2.25)}" || exit 16
        "$C" "media://$4?commande=arreter" >/dev/null || exit 17
        [ "$("$Q" media://$4?donnee=etat)" = stopped ] || exit 18' _ "$C" "$Q" "$S" "$V"
    rc=$?; [[ $rc == 0 ]] || echec "vrai mpv : étape $rc ($(cat "$(L "$V")/mpv.err" 2>/dev/null | tail -n 2))"
    vrai=" ; vrai mpv : lire/pause/aller/arrêter"
else
    echo "  note : mpv ou xvfb-run absent : étape 8 non jouée"
fi

# ── 9 — vraie fenêtre sermo (facultatif) ────────────────────────────────────
if [[ -n "$BIN_REEL" ]]; then
    command -v xdotool >/dev/null && command -v mpv >/dev/null || { echo "xdotool ou mpv absent : étape 9 non faite" >&2; exit 77; }
    unset SERMO_MEDIA_MAITRE
    export SERMO_SOURCE_DIR="$AIDE" XML="$T/d.xml"
    cat > "$XML" <<X
<window><vbox>
<button><label>Lire</label><action media>media://$F?commande=lire&amp;source=file://$S</action></button>
<text><variable>ETAT</variable><input media>media://$F?donnee=etat</input></text>
<timer interval="2" visible="false"><action>refresh:ETAT</action></timer>
<timer interval="4" visible="false"><action>exit:fin</action></timer>
</vbox></window>
X
    timeout 40 xvfb-run -a sh -c '"$0" --file="$XML" & p=$!; w=$(xdotool search --sync --onlyvisible --name . | head -n 1); sleep 0.8; xdotool windowfocus --sync "$w"; xdotool key --window "$w" space; wait $p' "$BIN_REEL" > "$T/sortie" 2>"$T/err"
    grep -q '^ETAT="playing' "$T/sortie" || echec "vraie fenêtre : état $(grep ETAT "$T/sortie") ($(tail -n 2 "$T/err"))"
    attendre '[[ $(etat "$F") == stopped ]]' || echec "vraie fenêtre fermée : mpv toujours là (maître non trouvé ?)"
    suite=" ; vraie fenêtre : clic → playing, fermeture → mpv arrêté"
fi
echo "OK : sources/noms/commandes piégés refusés sans mpv, durcissement, bac (réseau refusé sans bwrap), requêtes fixes, états, 4 lecteurs, pas d'orphelin, identifiants hors arguments, socket 600 exigé${camera:-}${vrai:-}${suite:-}"
