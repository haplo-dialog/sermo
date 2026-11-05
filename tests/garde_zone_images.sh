#!/bin/bash
# SPDX-License-Identifier: GPL-2.0-or-later
# tests/garde_zone_images.sh — haplo-dialog — 2026 — GPL-2.0-or-later
#
# <drawingarea media="NOM"> (gtk3) peint les images qu'un décodeur NON FIABLE
# écrit dans $XDG_RUNTIME_DIR/sermo-media/NOM/images (sermo_images.h). Ce banc
# lui présente, sous Xvfb, des fichiers hostiles, puis valides :
#   1 — une image rouge valide est peinte ;
#   2 — signature, version, largeur, hauteur, pas, tampon hors bornes, fichier
#       tronqué, lien symbolique : gtk3sermo reste vivant, l'image d'avant
#       reste (ou rien), JAMAIS le contenu du fichier hostile ;
#   3 — un fichier valide mais en 644 (pas à nous seul) est refusé ;
#   4 — un fichier valide en 600 est peint ; effacé, la zone se vide.
# Usage : garde_zone_images.sh [binaire gtk3sermo]
set -u
RACINE=$(CDPATH= cd -- "$(dirname -- "$0")/.." && pwd)
B="${1:-$RACINE/sermo-backend-gtk3/_build/gtk3sermo}"
[[ -x "$B" ]] || { echo "gtk3sermo absent ($B)" >&2; exit 77; }
for t in xvfb-run import convert python3; do command -v $t >/dev/null || { echo "outil manquant : $t" >&2; exit 77; }; done
RUN=$(mktemp -d -p "/run/user/$(id -u)" zi.XXXX 2>/dev/null) || RUN=$(mktemp -d -p "$RACINE/sermo-source/_build" zi.XXXX)
chmod 700 "$RUN"; trap 'rm -rf -- "$RUN"' EXIT
mkdir -m 700 -p "$RUN/sermo-media/t"
F="$RUN/sermo-media/t/images"
ecrire() {   # ecrire MODE magie version largeur hauteur pas courant sequence couleur [tronquer|tampons=N]
    # Le fichier est dimensionné d'après l'en-tête (h × pas par tampon, 2 tampons
    # ou N) : un cas hostile ne bute ainsi que sur le contrôle qu'il vise.
    python3 -I - "$F" "$@" <<'PY'
import os, struct, sys
f, mode, magie, v, l, h, pas, c, s, coul = sys.argv[1:11]
opt = sys.argv[11] if len(sys.argv) > 11 else ""
tronquer = opt == "tronquer"
n = int(opt.split("=")[1]) if opt.startswith("tampons=") else 2
v, l, h, pas, c, s = map(int, (v, l, h, pas, c, s))
px = {"rouge": b"\x00\x00\xff\x00", "bleu": b"\xff\x00\x00\x00", "vert": b"\x00\xff\x00\x00"}[coul]
tampon = px * (max(h, 1) * pas // 4)
corps = struct.pack("<8s6IQ", magie.encode(), v, l, h, pas, c, 0, s).ljust(64, b"\0") + tampon * n
if tronquer:
    corps = corps[:64 + 100]
tmp = f + ".n"
with open(tmp, "wb") as o:
    o.write(corps)
os.chmod(tmp, int(mode, 8))
os.replace(tmp, f)
PY
}
export XDG_RUNTIME_DIR=$RUN F
# xvfb-run passe par sh (dash), qui perd les fonctions exportées : fichier à charger.
{ declare -f ecrire; echo "F='$F'"; } > "$RUN/fonctions.sh"
export D='<window title="zi"><vbox><drawingarea width-request="160" height-request="120" media="t"><variable>Z</variable></drawingarea></vbox></window>'
xvfb-run -a -s "-screen 0 300x200x24" bash -c '
B=$1; R=$2; shift 2
. "$R/fonctions.sh"
"$B" --program=D >"$R/log" 2>&1 & p=$!
voir() { sleep 0.6; kill -0 $p 2>/dev/null || { echo "MORT"; return; }; import -window root "$R/c.png"
  for c in red blue lime; do printf "%s=%s " $c $(convert "$R/c.png" -fuzz 8% -fill black +opaque $c -fill white -opaque $c -format "%[fx:round(mean*w*h)]" info:); done; echo; }
sleep 1.5
while read -r cas; do eval "$cas"; echo -n "$nom : "; voir; done
kill $p' _ "$B" "$RUN" > "$RUN/resultats" 2>&1 <<EOF
nom=1-rouge-valide; ecrire 600 SERMOIMG 1 160 120 640 0 1 rouge
nom=2-magie; ecrire 600 SERMOIMX 1 160 120 640 0 2 bleu
nom=2-version; ecrire 600 SERMOIMG 2 160 120 640 0 3 bleu
nom=2-largeur; ecrire 600 SERMOIMG 1 2000 120 8000 0 4 bleu
nom=2-hauteur0; ecrire 600 SERMOIMG 1 160 0 640 0 5 bleu
nom=2-pas; ecrire 600 SERMOIMG 1 160 120 320 0 6 bleu
nom=2-courant; ecrire 600 SERMOIMG 1 160 120 640 2 7 bleu tampons=3
nom=2-tronque; ecrire 600 SERMOIMG 1 160 120 640 0 8 bleu tronquer
nom=2-lien; rm -f "$F"; ln -s /etc/passwd "$F"
nom=3-mode644; rm -f "$F"; ecrire 644 SERMOIMG 1 160 120 640 0 9 bleu
nom=4-vert-valide; ecrire 600 SERMOIMG 1 160 120 640 0 10 vert
nom=4-efface; rm -f "$F"
EOF
r="$RUN/resultats"
echec() { echo "ECHEC : $*"; sed 's/^/    /' "$r"; exit 1; }
grep -q MORT "$r" && echec "gtk3sermo est mort"
v() { sed -n "s/^$1 : .*$2=\([0-9]*\).*/\1/p" "$r"; }
(( $(v 1-rouge-valide red) > 15000 )) || echec "image rouge valide non peinte"
for c in 2-magie 2-version 2-largeur 2-hauteur0 2-pas 2-courant 2-tronque 2-lien 3-mode644; do
    (( $(v $c blue) < 100 )) || echec "$c : le contenu hostile est peint"
done
(( $(v 4-vert-valide lime) > 15000 )) || echec "image verte valide non peinte"
(( $(v 4-efface lime) < 100 )) || echec "fichier effacé : la zone ne se vide pas"
echo "OK : zone d'images — 9 fichiers hostiles refusés sans plantage, valides peints, zone vidée à l'effacement"
