#!/bin/bash
# SPDX-License-Identifier: GPL-2.0-or-later
# tests/garde_camera_capture.sh — haplo-dialog — 2026 — GPL-2.0-or-later
#
# examples/camera-capture/ prend une photo ou enregistre une caméra APRÈS une
# confirmation. Ce banc N'OUVRE JAMAIS LA CAMÉRA : le programme d'aide et la
# porte d'approbation sont remplacés par des faux qui notent ce qu'ils reçoivent.
#   1 — porte refusée, délai écoulé, indisponible : le programme d'aide n'est
#       JAMAIS appelé (rien n'est écrit)
#   2 — porte acceptée : le programme d'aide reçoit le chemin EXACT montré par
#       la porte (dossier à espace et accent compris), et la durée montrée
#   3 — entrées invalides (CC_CAMERA, CC_DUREE) : code 64, rien d'appelé
#   4 — le XML : caméra dans la zone (dans=$ECRAN), photo et vidéo seulement par
#       le script (jamais une adresse d'écriture dans le XML)
# Usage : garde_camera_capture.sh [racine du dépôt]
set -u
RACINE=$(CDPATH= cd -- "${1:-$(dirname -- "$0")/..}" && pwd) || exit 2
APP="$RACINE/examples/camera-capture/camera-capture.sh"
command -v python3 >/dev/null || { echo "outil manquant : python3" >&2; exit 77; }
T=$(mktemp -d -p "$RACINE/sermo-source/_build" 2>/dev/null || mktemp -d -p "$RACINE") || exit 2
trap 'rm -rf -- "$T"' EXIT
echec() { echo "ECHEC : $*"; exit 1; }

mkdir -p "$T/aide" "$T/Mes Vidéos" "$T/Images"
cat > "$T/aide/sermo-cible-media" <<'F'
#!/bin/bash
printf '%s\n' "$1" >> "$(dirname "$0")/appels"
F
cat > "$T/porte" <<'F'
#!/bin/bash
printf '%s\n' "$@" > "$(dirname "$0")/porte-vue"
exit "${FAUX_PORTE:-1}"
F
chmod +x "$T/aide/sermo-cible-media" "$T/porte"
export SERMO_SOURCE_DIR="$T/aide" CC_APPROBATION="$T/porte" CC_PHOTOS="$T/Images" CC_VIDEOS="$T/Mes Vidéos"
decode() { python3 -I -c 'import sys, urllib.parse as u; print(u.unquote(sys.argv[1]))' "$1"; }

# 1 — refus sous toutes ses formes
for r in 1 2 3 9; do
    FAUX_PORTE=$r bash "$APP" --photo >/dev/null 2>&1; rc=$?
    [[ $rc == 1 && ! -e "$T/aide/appels" ]] || echec "porte $r : rc=$rc, appels : $(cat "$T/aide/appels" 2>/dev/null)"
    FAUX_PORTE=$r bash "$APP" --video >/dev/null 2>&1
    [[ ! -e "$T/aide/appels" ]] || echec "porte $r (vidéo) : le programme d'aide a été appelé"
done

# 2 — acceptée : le chemin montré = le chemin écrit
FAUX_PORTE=0 bash "$APP" --photo >/dev/null 2>&1 || echec "photo acceptée : code $?"
montre=$(sed -n '/^Fichier :$/{n;p;}' "$T/porte-vue")
[[ "$montre" == "$T/Images/sermo-photo-"*.jpg ]] || echec "photo : chemin montré « $montre »"
a=$(decode "$(cat "$T/aide/appels")")
[[ "$a" == "media://camera?commande=photo&fichier=$montre" ]] || echec "photo : adresse « $a » (montré : $montre)"
rm -f "$T/aide/appels"
CC_DUREE=45 FAUX_PORTE=0 bash "$APP" --video >/dev/null 2>&1 || echec "vidéo acceptée : code $?"
montre=$(sed -n '/^Fichier :$/{n;p;}' "$T/porte-vue")
grep -q '^Durée maximale : 45 s' "$T/porte-vue" || echec "vidéo : durée non montrée : $(cat "$T/porte-vue")"
[[ "$montre" == "$T/Mes Vidéos/sermo-video-"*.mkv ]] || echec "vidéo : chemin montré « $montre »"
a=$(decode "$(cat "$T/aide/appels")")
[[ "$a" == "media://camera?commande=enregistrer&fichier=$montre&duree=45" ]] || echec "vidéo : adresse « $a »"
grep -q ' \|é' "$T/aide/appels" && echec "adresse non encodée : $(cat "$T/aide/appels")"
rm -f "$T/aide/appels"

# 3 — entrées invalides
for e in "CC_CAMERA=/dev/null" "CC_CAMERA=/dev/video0;x" "CC_DUREE=0" "CC_DUREE=601" "CC_DUREE=abc"; do
    env "$e" FAUX_PORTE=0 bash "$APP" --photo >/dev/null 2>&1; rc=$?
    [[ $rc == 64 && ! -e "$T/aide/appels" ]] || echec "« $e » : rc=$rc"
done

# 4 — le XML
x=$(bash "$APP" --xml) || echec "--xml"
grep -q 'commande=lire&amp;source=v4l2:///dev/video0&amp;dans=\$ECRAN' <<<"$x" || echec "XML : pas d'aperçu dans la zone"
grep -q 'commande=photo\|commande=enregistrer\|fichier=' <<<"$x" && echec "XML : une adresse d'écriture dans le XML"
grep -q -- '--photo</action>' <<<"$x" && grep -q -- '--video</action>' <<<"$x" || echec "XML : boutons photo/vidéo"

echo "OK : porte refusée = rien d'écrit ; chemin montré = chemin écrit (espace, accent) ; durée montrée ; entrées invalides 64 ; XML sans adresse d'écriture"
