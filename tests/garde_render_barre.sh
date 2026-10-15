#!/bin/bash
# SPDX-License-Identifier: GPL-2.0-or-later
# tests/garde_render_barre.sh — haplo-dialog — 2026 — GPL-2.0-or-later
#
# --render-png montre ce que la commande d'une <progressbar> a écrit.
#
# Trouvé le 2026-10-13 par la galerie du site : sous gtk3sermo, l'image de
# « Progression (doit afficher 65 %) » montrait une barre vide. gtk3 lit la
# commande dans un fil à part, et la capture partait avant la première ligne
# (puis avant la trame qui redessine la barre).
#
# Le garde rend deux fois la même fenêtre, la barre nourrie par « echo 0 » puis
# par « echo 65 » : les deux images doivent DIFFÉRER. Deux images identiques
# disent que la valeur n'est pas arrivée à l'écran. On compte les pixels qui
# changent (ImageMagick compare), pas les octets des fichiers. TÉMOIN : les
# deux images existent.
#
# Usage : garde_render_barre.sh <binaire>
# Codes : 0 = la barre est dessinée · 1 = un défaut · 2 = outil ou binaire manquant

set -u
BIN="${1:?usage: garde_render_barre.sh /chemin/vers/binaire}"
[[ -x "$BIN" ]] || { echo "binaire introuvable : $BIN" >&2; exit 2; }
BIN=$(readlink -f -- "$BIN")
for t in xvfb-run compare; do command -v "$t" >/dev/null || { echo "outil manquant : $t" >&2; exit 2; }; done

ENV_PRE=()
case "$(basename -- "$BIN")" in
    qt6sermo)  ENV_PRE=(QT_QPA_PLATFORM=xcb) ;;
    gtk4sermo) ENV_PRE=(GSK_RENDERER=cairo GTK_A11Y=none) ;;
    efl1sermo) ENV_PRE=(ELM_ENGINE=buffer) ;;
    sdl3sermo) ENV_PRE=(SDL_VIDEODRIVER=offscreen) ;;
esac

T=$(mktemp -d); trap 'rm -rf "$T"' EXIT
rendre() {   # rendre <valeur> <fichier.png>
    export GARDE_BARRE="<window><vbox><progressbar><input>echo $1</input><variable>PB</variable></progressbar><button ok></button></vbox></window>"
    ( cd "$T" && env "${ENV_PRE[@]}" xvfb-run -a timeout 30 "$BIN" --program=GARDE_BARRE --render-png="$2" ) >/dev/null 2>&1
}
rendre 0 "$T/zero.png"
rendre 65 "$T/soixante-cinq.png"
for f in zero soixante-cinq; do
    [ -s "$T/$f.png" ] || { echo "ECHEC : --render-png n'a pas écrit $f.png"; exit 1; }
done
# Compter les pixels qui changent (tolérance de 10 % sur la couleur), pas
# comparer les fichiers. Mesuré le 2026-10-13 sur gtk3 : barre vraiment
# dessinée, 141 pixels ; barre restée vide (ancien binaire), 3 pixels de bruit.
# Seuil : 40.
SEUIL=40
n=$(compare -metric AE -fuzz 10% "$T/zero.png" "$T/soixante-cinq.png" null: 2>&1 | sed 's/[ (].*//')
n=${n%%.*}
case "$n" in ''|*[!0-9]*) echo "ECHEC : comparaison impossible ($n)"; exit 1 ;; esac
if [ "$n" -lt "$SEUIL" ]; then
    echo "ECHEC : entre la barre à 0 % et à 65 %, seuls $n pixels changent (seuil $SEUIL) — la valeur lue n'est pas dessinée"
    exit 1
fi
echo "OK : entre la barre à 0 % et à 65 %, $n pixels changent (seuil $SEUIL)"
