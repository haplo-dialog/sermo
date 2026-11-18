#!/bin/bash
# SPDX-License-Identifier: GPL-2.0-or-later
# tests/garde_fermeture_fenetre.sh — haplo-dialog — 2026
#
# Fermer la fenêtre par sa croix rend EXIT="abort" et le code 0, sur tout port
# à fenêtre, comme gtk3sermo. Jusqu'à la 3.2.0, qt6, fltk1 et efl1 sortaient
# sans rien écrire : un script ne distinguait pas « fermé » de « planté ».
#
# La fermeture est DEMANDÉE (WM_DELETE_WINDOW, tests/outils/fermer-fenetres.py),
# sous Xvfb, sans gestionnaire de fenêtres.
#
# Usage : garde_fermeture_fenetre.sh <binaire>
# Codes : 0 = tient · 1 = défaut · 2 = outil ou binaire manquant
set -u
ICI=$(CDPATH= cd -- "$(dirname -- "$0")" && pwd)
BIN="${1:?usage: garde_fermeture_fenetre.sh /chemin/vers/binaire}"
[ -x "$BIN" ] || { echo "binaire introuvable : $BIN" >&2; exit 2; }
for t in xvfb-run python3 timeout; do command -v "$t" >/dev/null || { echo "outil manquant : $t" >&2; exit 2; }; done
T=$(mktemp -d) || exit 2; trap 'rm -rf "$T"' EXIT
printf '<window title="fermeture"><vbox><text><label>x</label></text><entry><variable>E</variable><default>v</default></entry></vbox></window>' >"$T/d.xml"
cat >"$T/jouer.sh" <<'JOUER'
"$1" -f "$2/d.xml" >"$2/sortie" 2>"$2/err" & p=$!
demande=0
for i in $(seq 1 60); do sleep 0.25; python3 -I "$3" 2>/dev/null && { demande=1; break; }; kill -0 $p 2>/dev/null || break; done
for i in $(seq 1 40); do kill -0 $p 2>/dev/null || break; sleep 0.25; done
if kill -0 $p 2>/dev/null; then kill $p; echo "vivant $demande" >"$2/rc"; else wait $p; echo "$? $demande" >"$2/rc"; fi
JOUER
env -u WAYLAND_DISPLAY QT_QPA_PLATFORM=xcb SDL_VIDEODRIVER=x11 GTK_A11Y=none GSK_RENDERER=cairo \
    timeout 60 xvfb-run -a bash "$T/jouer.sh" "$BIN" "$T" "$ICI/outils/fermer-fenetres.py" >/dev/null 2>&1
read -r rc demande <"$T/rc" 2>/dev/null || { echo "ECHEC : le dialogue n'a pas pu être joué"; exit 1; }
[ "$demande" = 1 ] || { echo "ECHEC : aucune fenêtre à fermer n'a été trouvée ($(head -2 "$T/err" | tr '\n' ' '))"; exit 1; }
[ "$rc" = vivant ] && { echo "ECHEC : la fenêtre fermée, le programme tourne encore"; exit 1; }
[ "$rc" = 0 ] || { echo "ECHEC : code de sortie $rc après fermeture (0 attendu)"; exit 1; }
[ "$(cat "$T/sortie")" = 'EXIT="abort"' ] || { echo "ECHEC : sortie [$(tr '\n' ' ' <"$T/sortie")] au lieu de EXIT=\"abort\""; exit 1; }
echo "OK : fenêtre fermée par sa croix : EXIT=\"abort\", code 0"
