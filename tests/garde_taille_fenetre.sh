#!/bin/bash
# SPDX-License-Identifier: GPL-2.0-or-later
# garde_taille_fenetre.sh — une fenêtre qui tient dans la taille demandée la garde.
#
# POURQUOI CE BANC EXISTE
#
# La galerie du site a publié pendant des semaines une image vide de 560x1530
# pour un dialogue qui en demande 560x420. Trois défauts se combinaient dans le
# port sdl3, et aucun banc ne pouvait les voir : les bancs XML et de
# comportement lisent les VALEURS exportées, jamais la fenêtre.
#
#   1. le rendu PNG redimensionnait à chaque frame de chauffe, douze fois ;
#      un enfant space-expand prend la hauteur restante, donc agrandir
#      augmente ce qu'il réclame — la mesure s'emballait ;
#   2. dans un hbox, le premier enfant extensible prenait toute la largeur et
#      ses frères disparaissaient ;
#   3. dans une vbox, un enfant extensible prenait toute la hauteur et ses
#      frères d'après étaient dessinés sous le bord de la fenêtre.
#
# Le témoin ci-dessous porte les trois cas : deux cadres extensibles côte à
# côte, puis deux frères en dessous. Son contenu TIENT dans la taille demandée
# — un débordement légitime agrandirait la fenêtre chez l'étalon aussi (mesuré
# le 2026-09-20 : gtk3 rend 400x980 pour 400x200 demandé et 25 boutons).
#
# Usage : garde_taille_fenetre.sh <binaire> [tolérance %]   (défaut : 5)

set -u
BIN="${1:-}"
[ -n "$BIN" ] && [ -x "$BIN" ] || { echo "usage: $0 <binaire> [tolérance %]" >&2; exit 2; }
TOL="${2:-5}"
NOM=$(basename "$BIN")
LARG=500
HAUT=320

T=$(mktemp -d) || exit 2
trap 'rm -rf "$T"' EXIT
cat > "$T/temoin.xml" <<XML
<window title="temoin" default-width="$LARG" default-height="$HAUT">
  <vbox space-expand="true" space-fill="true">
    <hbox space-expand="true" space-fill="true">
      <frame label="G" space-expand="true" space-fill="true"><vbox><text><label>gauche</label></text></vbox></frame>
      <frame label="D" space-expand="true" space-fill="true"><vbox><text><label>droite</label></text></vbox></frame>
    </hbox>
    <frame label="bandeau"><hbox><text><label>en bas</label></text></hbox></frame>
    <hbox><button ok></button></hbox>
  </vbox>
</window>
XML

case "$NOM" in
    sdl3sermo) SDL_VIDEODRIVER=offscreen timeout 40 "$BIN" --file="$T/temoin.xml" --render-png="$T/t.png" >/dev/null 2>&1 ;;
    efl1sermo) ELM_ENGINE=buffer       timeout 40 "$BIN" --file="$T/temoin.xml" --render-png="$T/t.png" >/dev/null 2>&1 ;;
    *)         timeout 40 xvfb-run -a  "$BIN" --file="$T/temoin.xml" --render-png="$T/t.png" >/dev/null 2>&1 ;;
esac

[ -s "$T/t.png" ] || { echo "ECHEC : $NOM n'a produit aucune image" >&2; exit 1; }
LU=$(identify -format '%w %h' "$T/t.png" 2>/dev/null)
[ -n "$LU" ] || { echo "ECHEC : image illisible" >&2; exit 1; }
W=${LU% *}; H=${LU#* }

# ⛔ Une image quasi unie est vide : la bonne taille ne suffit pas à dire que le
#    dialogue est dessiné. C'est exactement ce que la galerie publiait — 560x1530
#    et 31 couleurs, pas un widget.
#
#    Le seuil est GROSSIER, et volontairement bas : c'est un garde-fou contre
#    l'image vide, pas une mesure de qualité. Il est calé sur des mesures du
#    2026-09-20, thèmes compris : image vide 31 · efl1 correct 48 (thème sombre,
#    peu de teintes) · fltk1 219 · gtk4 316 · gtk3 318 · sdl3 444. Un premier
#    essai à 60 accusait efl1 à tort — vérifié à l'œil, son rendu était juste.
SEUIL_COULEURS=40
K=$(identify -format '%k' "$T/t.png" 2>/dev/null || echo 0)

ecarts=0
maxw=$(( LARG + LARG * TOL / 100 )); maxh=$(( HAUT + HAUT * TOL / 100 ))
[ "$W" -le "$maxw" ] || { echo "  largeur $W, demandée $LARG (toléré $maxw)" >&2; ecarts=$((ecarts+1)); }
[ "$H" -le "$maxh" ] || { echo "  hauteur $H, demandée $HAUT (toléré $maxh)" >&2; ecarts=$((ecarts+1)); }
[ "$W" -ge $(( LARG / 2 )) ] || { echo "  largeur $W : moins de la moitié du demandé" >&2; ecarts=$((ecarts+1)); }
[ "$H" -ge $(( HAUT / 2 )) ] || { echo "  hauteur $H : moins de la moitié du demandé" >&2; ecarts=$((ecarts+1)); }
[ "$K" -ge "$SEUIL_COULEURS" ] || { echo "  $K couleurs (seuil $SEUIL_COULEURS) : image quasi unie, le dialogue n'est pas dessiné" >&2; ecarts=$((ecarts+1)); }

if [ "$ecarts" -gt 0 ]; then
    echo "ECHEC : $NOM rend ${W}x${H} en $K couleurs pour ${LARG}x${HAUT} demandés" >&2
    exit 1
fi
echo "OK : $NOM rend ${W}x${H} ($K couleurs) pour ${LARG}x${HAUT} demandés"
