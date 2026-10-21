#!/bin/bash
# SPDX-License-Identifier: GPL-2.0-or-later
# tests/garde_camera_apercu.sh — haplo-dialog — 2026 — GPL-2.0-or-later
#
# examples/camera-apercu/ ouvre une caméra sur un clic. Ce banc N'OUVRE JAMAIS LA
# VRAIE CAMÉRA (consigne de la fiche 06) : faux /sys/class/video4linux, faux nœud
# /dev/video (un /dev/null renommé n'est pas « video » : on simule par V4L_SYSFS et un
# faux ffplay), et un VRAI ffplay sur une mire HTTP servie par ffmpeg en local :
#   1 — liste sans ouverture, nœud de métadonnées écarté, nom piégé neutralisé
#   2 — entrées piégées (nœud, résolution, format, adresse, identifiants) : 64, lecteur
#       jamais lancé ; pas d'affichage : 2
#   3 — caméra occupée (faux ffplay « Device or resource busy ») : code 4, rien de tué
#   4 — indicateur pendant l'aperçu, absent après ; pas d'orphelin après --arreter ni
#       après SIGKILL du maître
#   5 — vrai ffplay + vraie mire HTTP : sans identifiants, puis AVEC (curl -K -) : le mot
#       de passe n'est dans les arguments d'AUCUN processus, la config curl est effacée
# Avec un binaire réel en argument (+ xvfb-run) : 6 — la fenêtre s'ouvre et reste ouverte.
# Usage : garde_camera_apercu.sh [/chemin/vers/un-port-a-fenetre]
RACINE=$(CDPATH= cd -- "$(dirname -- "$0")/.." && pwd)
APP="$RACINE/examples/camera-apercu/camera-apercu.sh"
BIN_REEL="${1:-}"
for t in python3 timeout mktemp ffmpeg ffplay curl xvfb-run; do
    command -v "$t" >/dev/null || { echo "outil manquant : $t" >&2; exit 77; }
done
[[ -x "$APP" ]] || { echo "script introuvable ou non exécutable : $APP" >&2; exit 1; }
T=$(mktemp -d); MIRE=""
nettoyer() { "$APP" --arreter >/dev/null 2>&1; [[ -n "$MIRE" ]] && kill "$MIRE" 2>/dev/null; rm -rf "$T"; }
trap nettoyer EXIT
export XDG_RUNTIME_DIR="$T/run"; mkdir -m 700 "$XDG_RUNTIME_DIR"
export APPELS="$T/appels"; : > "$APPELS"
echec() { echo "ECHEC : $*"; exit 1; }
attendre() { local i; for i in $(seq 1 40); do eval "$1" && return 0; timeout 0.1 tail -f /dev/null; done; return 1; }

# ── Faux /sys : video0 = caméra, video1 = métadonnées, video2 = nom piégé ─────
S="$T/v4l"; mkdir -p "$S/video0" "$S/video1" "$S/video2" "$S/autre"
printf 'Webcam Essai\n' > "$S/video0/name"; printf '0\n' > "$S/video0/index"
printf 'Webcam Essai\n' > "$S/video1/name"; printf '1\n' > "$S/video1/index"
printf 'Cam|<b>&amp;$(id)\033[31m\n' > "$S/video2/name"; printf '0\n' > "$S/video2/index"
export V4L_SYSFS="$S"

# ── Faux ffplay (et faux bwrap absent : CA_BAC=off pour la logique) ───────────
mkdir "$T/faux"
cat > "$T/faux/ffplay" <<'FAUX'
#!/bin/bash
printf 'ffplay %s\n' "$*" >> "$APPELS"
case "${FAKE_FFPLAY:-reste}" in
  occupee) echo "[in#0 @ 0x1] Error opening input: Device or resource busy" >&2; exit 0 ;;
  echoue)  echo "Error opening input file: No such device" >&2; exit 0 ;;
esac
exec sleep 300
FAUX
chmod +x "$T/faux/ffplay"
VRAI_PATH=$PATH
export PATH="$T/faux:$PATH" CA_BAC=off DISPLAY=:99
export CA_DIR="$T/ca"; mkdir -m 700 "$CA_DIR"
export CA_MAITRE=$$
# Le script vérifie que le nœud est un périphérique caractère /dev/videoN : sans vraie
# caméra à ouvrir, on prend un nœud qui EXISTE mais que le faux ffplay n'ouvre pas.
NOEUD=""; for n in /dev/video[0-9]*; do [ -c "$n" ] && { NOEUD=$n; break; }; done

# ── 1 — liste ───────────────────────────────────────────────────────────────
"$APP" --cameras > "$T/liste"
[[ $(cut -d'|' -f1 "$T/liste" | tr '\n' ' ') == "/dev/video0 /dev/video2 " ]] || echec "liste : $(tr '\n' ' ' < "$T/liste")"
grep -qx '/dev/video2|Cam¦<b>&amp;$(id)\[31m' "$T/liste" || echec "nom piégé : $(grep video2 "$T/liste")"
[[ ! -s "$APPELS" ]] || echec "la liste a lancé le lecteur"

# ── 2 — entrées piégées ─────────────────────────────────────────────────────
for args in "/dev/null 640x480 mjpeg" "/dev/video99 640x480 mjpeg" "'/dev/video0;id' 640x480 mjpeg" "-x 640x480 mjpeg" \
            "${NOEUD:-/dev/video0} 641x480 mjpeg" "${NOEUD:-/dev/video0} 640x480 h264" "'' 640x480 mjpeg"; do
    eval "set -- $args"; : > "$APPELS"
    "$APP" --voir-locale "$@" >/dev/null 2>&1; rc=$?
    [[ $rc == 64 && ! -s "$APPELS" ]] || echec "caméra « $args » : code $rc ou lecteur lancé"
done
for u in 'rtsp://u:p@h/x' 'http://a@h/' 'file:///etc/passwd' 'ftp://h/' 'rtsp://-x/y' 'rtsp://h/a b' $'rtsp://h/\n' 'rtsp://h/$(id)' 'concat:a|b' ''; do
    : > "$APPELS"; "$APP" --voir-ip "$u" >/dev/null 2>&1; rc=$?
    [[ $rc == 64 && ! -s "$APPELS" ]] || echec "adresse « $u » : code $rc ou lecteur lancé"
done
printf 'camera:s3cret\n' > "$T/id"; chmod 600 "$T/id"; printf 'camera:s3cret\n' > "$T/id644"; chmod 644 "$T/id644"
ln -s "$T/id" "$T/idlien"; printf 'sans-deux-points\n' > "$T/idmal"; chmod 600 "$T/idmal"
printf 'a:b"c\n' > "$T/idguill"; chmod 600 "$T/idguill"
for args in "http://h/v $T/id644" "http://h/v relatif" "http://h/v $T/idlien" "http://h/v $T/idmal" "http://h/v $T/idguill" "rtsp://h/v $T/id"; do
    eval "set -- $args"; : > "$APPELS"
    "$APP" --voir-ip "$@" >/dev/null 2>&1; rc=$?
    [[ $rc == 64 && ! -s "$APPELS" ]] || echec "identifiants « $args » : code $rc ou lecteur lancé"
done
env -u DISPLAY -u WAYLAND_DISPLAY "$APP" --voir-ip rtsp://h/v >/dev/null 2>&1; [[ $? == 2 ]] || echec "sans affichage : code ≠ 2"
CA_BAC=oui "$APP" --voir-ip rtsp://h/v >/dev/null 2>&1; [[ $? == 64 ]] || echec "CA_BAC invalide accepté"

# ── 3 — occupée ─────────────────────────────────────────────────────────────
if [[ -n "$NOEUD" ]]; then
    : > "$APPELS"; FAKE_FFPLAY=occupee "$APP" --voir-locale "$NOEUD" 640x480 mjpeg > "$T/s" 2>&1; rc=$?
    [[ $rc == 4 ]] || echec "occupée : code $rc : $(cat "$T/s")"
    grep -q 'caméra occupée' "$T/s" && grep -q 'rien n.a été forcé' "$T/s" || echec "occupée : message : $(cat "$T/s")"
    grep -q -- "-f v4l2 -input_format mjpeg -video_size 640x480 -i $NOEUD" "$APPELS" || echec "arguments du lecteur : $(cat "$APPELS")"
    ! grep -q 'sed:' "$T/s" || echec "erreur sed dans la recherche du nom : $(cat "$T/s")"
    FAKE_FFPLAY=echoue "$APP" --voir-locale "$NOEUD" 640x480 mjpeg >/dev/null 2>&1; [[ $? == 2 ]] || echec "échec du lecteur : code ≠ 2"
else
    echo "  note : aucun /dev/videoN sur ce poste : cas « occupée » local non joué (le cas IP le couvre)"
fi
FAKE_FFPLAY=occupee "$APP" --voir-ip rtsp://h/v >/dev/null 2>&1; [[ $? == 4 ]] || echec "occupée (IP) : code ≠ 4"

# ── 4 — indicateur, orphelins ───────────────────────────────────────────────
"$APP" --voir-ip rtsp://cam.test/flux >/dev/null || echec "aperçu (faux) : code non nul"
L=$(cat "$CA_DIR/lecteur"); kill -0 "$L" 2>/dev/null || echec "lecteur non démarré"
[[ $("$APP" --etat) == "● CAMÉRA ACTIVE — rtsp://cam.test/flux — bac à sable inactif (CA_BAC=off)" ]] || echec "indicateur : $("$APP" --etat)"
grep -q -- "-window_title sermo — CAMÉRA ACTIVE — rtsp://cam.test/flux" "$APPELS" || echec "titre de la fenêtre du lecteur"
"$APP" --arreter
attendre '! kill -0 "$L" 2>/dev/null' || echec "--arreter : lecteur toujours vivant"
[[ $("$APP" --etat) != *"CAMÉRA ACTIVE"* ]] || echec "indicateur resté après l'arrêt"
bash -c 'export CA_MAITRE=$$; "$1" --voir-ip rtsp://cam.test/b >/dev/null; exec sleep 60' _ "$APP" & M=$!; disown
attendre '[[ -s "$CA_DIR/lecteur" && $(cat "$CA_DIR/lecteur") != "$L" ]]' || echec "second aperçu non démarré"
L2=$(cat "$CA_DIR/lecteur"); kill -9 "$M"
attendre '! kill -0 "$L2" 2>/dev/null' || echec "maître tué par SIGKILL : lecteur orphelin ($L2)"
[[ $("$APP" --etat) != *"CAMÉRA ACTIVE"* ]] || echec "indicateur resté après la mort du lecteur"

# ── 5 — vrai ffplay, vraie mire HTTP locale ─────────────────────────────────
export PATH="$VRAI_PATH"
P=$(python3 -I -c 'import socket; s=socket.socket(); s.bind(("127.0.0.1",0)); print(s.getsockname()[1])')
reel() {  # reel <identifiants ou vide> : un aperçu réel sous xvfb, 3 s, puis contrôles
    timeout 40 ffmpeg -hide_banner -loglevel error -re -f lavfi -i testsrc=size=160x120:rate=10 -t 30 -c:v mpeg2video -f mpegts -listen 1 "http://127.0.0.1:$P/mire.ts" 2>/dev/null & MIRE=$!
    timeout 0.8 tail -f /dev/null
    IDF="$1" CA_BAC="${2:-off}" timeout 30 xvfb-run -a bash -c '
        "$1" --voir-ip "http://127.0.0.1:$2/mire.ts" $IDF > "$3/reel" 2>&1 || exit 9
        sleep 3
        "$1" --etat > "$3/etat-reel"
        { for p in /proc/[0-9]*; do tr "\0" " " < $p/cmdline 2>/dev/null; echo; done; } > "$3/cmdlines"
        ls "$CA_DIR" > "$3/fichiers"
        "$1" --arreter' _ "$APP" "$P" "$T"
    rc=$?
    kill "$MIRE" 2>/dev/null; wait "$MIRE" 2>/dev/null; MIRE=""
    [[ $rc == 0 ]] || echec "aperçu réel ($1) : rc=$rc : $(cat "$T/reel")"
    grep -q '^● CAMÉRA ACTIVE — http://127.0.0.1' "$T/etat-reel" || echec "aperçu réel ($1) : indicateur : $(cat "$T/etat-reel")"
}
reel ""
# bwrap présent ne veut pas dire bwrap utilisable : un conteneur de CI peut
# refuser ses espaces de noms. Le même bac que l'exemple est essayé d'abord.
BWRAP_OK=0
command -v bwrap >/dev/null && bwrap --ro-bind / / --dev /dev --tmpfs /tmp --die-with-parent true 2>/dev/null && BWRAP_OK=1
command -v bwrap >/dev/null && [[ $BWRAP_OK == 0 ]] && echo "  note : bwrap présent mais refusé ici (espaces de noms) : contrôle du bac à sable non joué"
reel "$T/id" auto        # avec identifiants ET bwrap : le tube traverse le bac à sable
! grep -q 's3cret' "$T/cmdlines" || echec "le mot de passe est dans les arguments d'un processus : $(grep s3cret "$T/cmdlines")"
[[ $BWRAP_OK == 1 ]] && { grep -q '^bwrap .*ffplay' "$T/cmdlines" || echec "bwrap non utilisé en mode auto (bac : $(cat "$CA_DIR/bac" 2>/dev/null) ; processus vus : $(grep -aE 'bwrap|ffplay|curl' "$T/cmdlines" | cut -c1-160 | tr '\n' '|') ; sortie : $(tail -n 3 "$T/reel" | tr '\n' '|'))"; }
grep -q 'curl -sS -K -' "$T/cmdlines" || echec "curl non vu pendant l'aperçu à identifiants (le test ne prouve rien)"
! grep -q 'curl.conf' "$T/fichiers" || echec "la config de curl (identifiants) n'a pas été effacée"
"$APP" --xml | python3 -I -c 'import sys,xml.dom.minidom as m; m.parseString(sys.stdin.read())' || echec "XML mal formé"

# ── 6 — binaire réel (facultatif) ──────────────────────────────────────────
if [[ -n "$BIN_REEL" ]]; then
    ( unset CA_DIR; timeout -k 3 10 xvfb-run -a "$APP" "$BIN_REEL" ) > "$T/gui" 2>&1
    rc=$?
    [[ $rc == 124 ]] || echec "binaire réel : la fenêtre ne reste pas ouverte (rc=$rc) : $(tail -n 3 "$T/gui")"
    ! grep -qiE 'syntax error|parse error|unknown tag|erreur de syntaxe' "$T/gui" || echec "binaire réel : XML refusé"
    suite=" ; fenêtre réelle ouverte"
fi
echo "OK : liste sans ouverture, entrées piégées refusées, occupée = 4 sans rien forcer, indicateur, pas d'orphelin, aperçu réel HTTP avec et sans identifiants, mot de passe hors des arguments${suite:-}"
