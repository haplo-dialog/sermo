#!/bin/bash
# SPDX-License-Identifier: GPL-2.0-or-later
# tests/garde_render_jauge.sh — haplo-dialog — 2026 — GPL-2.0-or-later
#
# Une <levelbar range-min="0" range-max="100"> dessine sa valeur DANS cet
# intervalle.
#
# Trouvé le 2026-10-18 en relisant les pages de manuel : gtk4sermo lisait
# « min »/« max » et ignorait « range-min »/« range-max » (les noms de
# l'étalon gtk3sermo et des sept autres ports). Une jauge 0..100 restait sur
# 0..1 : à 25, la barre était PLEINE. Les bancs de comportement n'en voyaient
# rien : la valeur exportée, elle, était juste.
#
# Le garde rend la même jauge à 25 puis à 100 : les deux images doivent
# DIFFÉRER. Deux images identiques disent que l'intervalle est ignoré (25 et
# 100 sont tous deux « pleins » sur 0..1). On compte les pixels qui changent
# (ImageMagick compare). TÉMOIN : les deux images existent.
#
# Usage : garde_render_jauge.sh <binaire>
# Codes : 0 = l'intervalle est suivi · 1 = un défaut · 2 = outil ou binaire manquant
set -u
BIN="${1:?usage: garde_render_jauge.sh /chemin/vers/binaire}"
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
    export GARDE_JAUGE="<window default-width=\"300\"><vbox><levelbar range-min=\"0\" range-max=\"100\"><input>echo $1</input><variable>J</variable></levelbar><button ok></button></vbox></window>"
    ( cd "$T" && env "${ENV_PRE[@]}" xvfb-run -a timeout 30 "$BIN" --program=GARDE_JAUGE --render-png="$2" ) >/dev/null 2>&1
}
rendre 25 "$T/quart.png"
rendre 100 "$T/plein.png"
for f in quart plein; do
    [ -s "$T/$f.png" ] || { echo "ECHEC : --render-png n'a pas écrit $f.png"; exit 1; }
done
SEUIL=40
n=$(compare -metric AE -fuzz 10% "$T/quart.png" "$T/plein.png" null: 2>&1 | sed 's/[ (].*//')
n=${n%%.*}
case "$n" in ''|*[!0-9]*) echo "ECHEC : comparaison impossible ($n)"; exit 1 ;; esac
if [ "$n" -lt "$SEUIL" ]; then
    echo "ECHEC : entre la jauge à 25 et à 100 (sur 0..100), seuls $n pixels changent (seuil $SEUIL) — range-min/range-max ignorés"
    exit 1
fi
echo "OK : entre la jauge à 25 et à 100 (sur 0..100), $n pixels changent (seuil $SEUIL)"
