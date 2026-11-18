#!/bin/bash
# SPDX-License-Identifier: GPL-2.0-or-later
# tests/garde_pixmap_taille.sh — haplo-dialog — 2026
#
# Un <pixmap> montre son image à SA taille. Jusqu'à la 3.2.0, gtk4sermo la
# rendait à la taille d'une icône (16 px) : un dialogue portant une image de
# 358 px sortait en 27 × 89 pixels au lieu de 368 × 448 (gtk3), et une icône de
# thème (<input file icon="…">) était toujours l'image cassée.
#
# Mesuré sur le rendu (--render-png) : la fenêtre est au moins aussi large et
# haute que l'image.
# Usage : garde_pixmap_taille.sh <gtk3sermo|gtk4sermo>
# Codes : 0 = tient · 1 = défaut · 2 = outil, binaire ou image manquant
set -u
ICI=$(CDPATH= cd -- "$(dirname -- "$0")" && pwd)
BIN="${1:?usage: garde_pixmap_taille.sh /chemin/vers/binaire}"
[ -x "$BIN" ] || { echo "binaire introuvable : $BIN" >&2; exit 2; }
for t in xvfb-run python3 timeout; do command -v "$t" >/dev/null || { echo "outil manquant : $t" >&2; exit 2; }; done
IMG="$ICI/../sermoia-mcp/icon/sermoia-mark.png"
[ -f "$IMG" ] || { echo "image d'essai absente : $IMG" >&2; exit 2; }
T=$(mktemp -d) || exit 2; trap 'rm -rf "$T"' EXIT
printf '<window><vbox><pixmap><input file>%s</input></pixmap><pixmap><input file icon="folder"></input></pixmap></vbox></window>' "$IMG" >"$T/d.xml"
GTK_A11Y=none GSK_RENDERER=cairo xvfb-run -a timeout 30 "$BIN" -f "$T/d.xml" --render-png="$T/r.png" >/dev/null 2>"$T/err"
[ -s "$T/r.png" ] || { echo "ECHEC : aucun rendu ($(grep -av 'xapp\|libEGL' "$T/err" | head -2 | tr '\n' ' '))"; exit 1; }
python3 -I - "$IMG" "$T/r.png" <<'PY' || exit 1
import struct, sys
def taille(p):
    with open(p, "rb") as f:
        d = f.read(24)
    return struct.unpack(">II", d[16:24])
(iw, ih), (rw, rh) = taille(sys.argv[1]), taille(sys.argv[2])
if rw >= iw and rh >= ih:
    print("OK : <pixmap> à la taille de l'image (image %d × %d, rendu %d × %d)" % (iw, ih, rw, rh))
    sys.exit(0)
print("ECHEC : rendu %d × %d plus petit que l'image %d × %d" % (rw, rh, iw, ih))
sys.exit(1)
PY
