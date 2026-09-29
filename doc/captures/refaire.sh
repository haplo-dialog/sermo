#!/bin/bash
# SPDX-License-Identifier: GPL-2.0-or-later
# doc/captures/refaire.sh — refait formulaire-sept-ports.png, l'image du README :
# le formulaire d'examples/showcase/01-formulaire.sh rendu par les sept ports.
#
# Les binaires viennent des paquets .deb donnés (ceux qu'on livre), sinon de
# l'arbre construit (sermo-backend-<port>/_build/). Tout se passe sous un écran
# virtuel : rien ne s'ouvre sur l'écran de la session.
#
# Rendu, port par port :
#   gtk3, gtk4, fltk1  --render-png, sous xvfb ;
#   efl1, sdl3         --render-png, plateforme hors écran ;
#   qt6                capture de la fenêtre sous xvfb : son --render-png ne
#                      tient pas encore compte de la taille demandée et rend
#                      parfois l'image d'une liste déroulante (TODO.md) ;
#   ncurses            capture d'un xterm sous xvfb.
# Thème clair demandé partout (SERMO_DARK=0, HOME vide) ; efl1 l'ignore.
#
# Usage : doc/captures/refaire.sh [dossier-des-paquets]
# Outils : xvfb-run, xdotool, xterm, ImageMagick (import, montage), dpkg-deb.

set -uo pipefail
ICI=$(CDPATH= cd -- "$(dirname -- "$0")" && pwd)
RACINE=$(CDPATH= cd -- "$ICI/../.." && pwd)
for t in xvfb-run xdotool xterm import montage convert identify; do
    command -v "$t" >/dev/null || { echo "outil manquant : $t" >&2; exit 2; }
done
PORTS=(gtk3 gtk4 qt6 fltk1 efl1 sdl3 ncurses)
T=$(mktemp -d); trap 'rm -rf "$T"' EXIT
mkdir -p "$T/home" "$T/png"

binaire() {  # binaire <port> : chemin du binaire à rendre
    if [ -n "${PAQUETS:-}" ]; then
        echo "$T/extrait/$1/usr/bin/${1}sermo"
    else
        echo "$RACINE/sermo-backend-$1/_build/${1}sermo"
    fi
}

if [ $# -ge 1 ]; then
    PAQUETS=$(readlink -f "$1")
    command -v dpkg-deb >/dev/null || { echo "outil manquant : dpkg-deb" >&2; exit 2; }
    for p in "${PORTS[@]}"; do
        deb=$(ls "$PAQUETS"/sermo-backend-"$p"_*.deb 2>/dev/null | head -1)
        [ -n "$deb" ] || { echo "paquet sermo-backend-$p absent de $PAQUETS" >&2; exit 2; }
        mkdir -p "$T/extrait/$p"
        dpkg-deb -x "$deb" "$T/extrait/$p" || { echo "extraction de $deb" >&2; exit 2; }
    done
fi
for p in "${PORTS[@]}"; do
    [ -x "$(binaire "$p")" ] || { echo "binaire absent : $(binaire "$p")" >&2; exit 2; }
done

# Le XML exact de l'exemple : le script est lancé avec un faux binaire qui écrit
# MAIN_DIALOG au lieu d'ouvrir la fenêtre.
printf '#!/bin/sh\nprintf "%%s" "$MAIN_DIALOG" > "%s"\n' "$T/formulaire.xml" > "$T/faux"
chmod +x "$T/faux"
sh "$RACINE/examples/showcase/01-formulaire.sh" "$T/faux"
[ -s "$T/formulaire.xml" ] || { echo "XML de l'exemple non extrait" >&2; exit 1; }

# fenetre <sortie> <classe> <commande…> : lance, attend la fenêtre, la photographie.
cat > "$T/fenetre.sh" <<'EOF'
#!/bin/sh
out="$1"; classe="$2"; shift 2
"$@" >/dev/null 2>&1 &
pid=$!
for i in $(seq 1 100); do
    wid=$(xdotool search --onlyvisible --class "$classe" 2>/dev/null | tail -1)
    [ -n "$wid" ] && break
    sleep 0.1
done
[ -n "$wid" ] || { echo "aucune fenêtre $classe" >&2; kill $pid; exit 1; }
sleep 1.5
import -window "$wid" "$out"
kill $pid 2>/dev/null; wait $pid 2>/dev/null
exit 0
EOF
chmod +x "$T/fenetre.sh"

ENV=(env HOME="$T/home" SERMO_DARK=0 LC_ALL="${LC_ALL:-fr_FR.UTF-8}")
XVFB=(xvfb-run -a -s '-screen 0 1024x768x24')
echecs=0
for p in "${PORTS[@]}"; do
    b=$(binaire "$p"); out="$T/png/$p.png"
    case "$p" in
        gtk3|gtk4|fltk1)
            "${ENV[@]}" GSK_RENDERER=cairo GTK_A11Y=none "${XVFB[@]}" \
                timeout 30 "$b" --render-png="$out" --file="$T/formulaire.xml" >/dev/null 2>&1 ;;
        efl1|sdl3)
            "${ENV[@]}" ELM_ENGINE=buffer SDL_VIDEODRIVER=offscreen \
                timeout 30 "$b" --render-png="$out" --file="$T/formulaire.xml" >/dev/null 2>&1 ;;
        qt6)
            "${ENV[@]}" QT_QPA_PLATFORM=xcb "${XVFB[@]}" \
                "$T/fenetre.sh" "$out" qt6sermo "$b" --file="$T/formulaire.xml" ;;
        ncurses)
            "${ENV[@]}" SERMO_NCURSES_THEME=clair "${XVFB[@]}" \
                "$T/fenetre.sh" "$out" xterm xterm -geometry 60x24+0+0 \
                -fa 'DejaVu Sans Mono' -fs 12 -bg white -fg black \
                -e "$b" --file="$T/formulaire.xml" ;;
    esac
    if [ -s "$out" ]; then
        echo "  ✔ $p : $(identify -format '%wx%h' "$out")"
    else
        echo "  ✘ $p : aucune image" >&2; echecs=$((echecs + 1))
    fi
done
[ "$echecs" -eq 0 ] || exit 1

( cd "$T/png" && montage -label '%t' "${PORTS[@]/%/.png}" -font DejaVu-Sans -pointsize 20 \
    -geometry '480x420+14+10>' -tile 4x2 -background white "$T/mosaique.png" ) || exit 1
convert "$T/mosaique.png" -strip -depth 8 -background white -alpha remove -alpha off \
    "PNG24:$ICI/formulaire-sept-ports.png" || exit 1
echo "écrit : $ICI/formulaire-sept-ports.png ($(identify -format '%wx%h' "$ICI/formulaire-sept-ports.png"))"
