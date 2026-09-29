#!/usr/bin/env python3
# SPDX-License-Identifier: GPL-2.0-or-later
# icon/engendrer-icones.py — les icônes d'application des ports, depuis le logo HD.
#
# POURQUOI UNE PASTILLE BLANCHE
#
# Le logo HD est un monogramme bleu marine. Posé tel quel, avec un fond
# transparent, il disparaît sur une barre de titre sombre — mesuré le 2026-09-19
# sur xfwm4 : contraste 1,3 contre le bandeau. Une icône d'application ne peut
# pas s'adapter au thème, contrairement au site qui l'inverse en blanc par une
# règle CSS. Elle est donc posée sur une pastille blanche arrondie, lisible sur
# un fond clair comme sur un fond sombre.
#
# Le logo source n'est pas carré (401,7 × 330,6) : il est centré dans un carré.
#
# Entrée  : icon/haplo-dialog-hd.svg, rendu en PNG par un navigateur (le SVG
#           porte deux dégradés que les convertisseurs en ligne de commande
#           rendent mal — voir la fiche « Aperçu SVG = navigateur »).
# Sortie  : sermo-backend-<port>/data/icons/hicolor/<taille>/apps/<port>sermo.png
#
# Usage : icon/engendrer-icones.py <logo-rendu.png>
#   où <logo-rendu.png> est le SVG rendu à 1024 px de large, fond transparent :
#   chromium --headless --window-size=1024,843 --default-background-color=00000000 \
#            --screenshot=logo.png icon/haplo-dialog-hd.svg
import os
import sys

from PIL import Image, ImageDraw

TAILLES = (16, 22, 24, 32, 48, 64, 128, 256)
PORTS = ("gtk3", "gtk4", "qt6", "fltk1", "efl1", "sdl3")   # ncurses n'a pas d'icône
RACINE = os.path.dirname(os.path.dirname(os.path.abspath(__file__)))

logo = Image.open(sys.argv[1]).convert("RGBA")
logo = logo.crop(logo.getbbox())          # on ne garde que le dessin


def icone(t):
    marge = max(1, round(t * 0.12))
    rayon = max(1, round(t * 0.22))
    im = Image.new("RGBA", (t, t), (0, 0, 0, 0))
    ImageDraw.Draw(im).rounded_rectangle([0, 0, t - 1, t - 1], radius=rayon, fill=(255, 255, 255, 255))
    dispo = t - 2 * marge
    l, h = logo.size
    f = min(dispo / l, dispo / h)
    petit = logo.resize((max(1, round(l * f)), max(1, round(h * f))), Image.LANCZOS)
    im.alpha_composite(petit, ((t - petit.width) // 2, (t - petit.height) // 2))
    return im


n = 0
for p in PORTS:
    for t in TAILLES:
        d = os.path.join(RACINE, "sermo-backend-%s/data/icons/hicolor/%dx%d/apps" % (p, t, t))
        os.makedirs(d, exist_ok=True)
        icone(t).save(os.path.join(d, "%ssermo.png" % p), optimize=True)
        n += 1
print("%d icônes : %d ports × %d tailles" % (n, len(PORTS), len(TAILLES)))
