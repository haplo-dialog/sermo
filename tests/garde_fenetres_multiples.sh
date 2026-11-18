#!/bin/bash
# SPDX-License-Identifier: GPL-2.0-or-later
# tests/garde_fenetres_multiples.sh — haplo-dialog — 2026
#
# launch: ouvre une seconde fenêtre, closewindow: la ferme, launch: la rouvre
# (manuel §9.2). Jusqu'à la 3.3.0, hors gtk3 et gtk4 :
#   - qt6   : launch: relançait QApplication::exec(), que Qt refuse ; le
#             dialogue sortait aussitôt en EXIT="abort" ;
#   - fltk1 : closewindow: retirait les variables mais laissait la fenêtre à
#             l'écran, ses minuteries en marche ;
#   - efl1  : la seconde fenêtre s'empilait DANS la première (une seule fenêtre
#             native), et closewindow: ne fermait rien ;
#   - sdl3  : launch: ouvrait une seconde boucle de rendu au milieu d'une
#             image, et closewindow: laissait le dialogue figé jusqu'au délai ;
#   - lvgl  : launch: rappelait la boucle au milieu d'un tour de LVGL, qui le
#             refuse ; le dialogue restait figé ;
#   - ncurses, html : la boucle de la fenêtre lancée ne servait qu'elle ; la
#             première était perdue, et sans terminal le dialogue sortait en
#             EXIT="abort" (ou sur « syntax error » au second launch:).
#
# Mesure, sans personne pour cliquer : une minuterie coche une case dont
# l'action est launch:SEC (une minuterie n'a pas le droit de lancer
# elle-même) ; SEC se ferme par sa propre minuterie ; une seconde case la
# relance ; la fenêtre principale sort, SEC ouverte. Le banc regarde À L'ÉCRAN
# (xdotool, xprop) que SEC apparaît, disparaît, réapparaît, et que la
# principale est toujours là quand SEC est fermée ; puis compare les variables
# rendues à celles de l'étalon gtk3sermo.
# sdl3 et lvgl n'ont qu'une fenêtre native : la fenêtre lancée y prend la place
# de la principale, qui revient à sa fermeture. Le banc n'y exige donc pas les
# deux titres en même temps.
#
# Sans écran (html, ncurses et lvgl sans terminal ni affichage), il n'y a pas
# d'horloge : chaque minuterie joue une fois, dans l'ordre où elle partirait.
# Le même dialogue doit y rendre les mêmes variables ; la page html reste celle
# de la fenêtre principale.
# ncurses dans un terminal : joué dans un vrai pseudo-terminal, la fenêtre du
# dessus occupe l'écran ; le banc lit les titres dessinés au fil du temps.
#
# Usage : garde_fenetres_multiples.sh <binaire d'un port>
# Codes : 0 = tient · 1 = défaut · 2 = outil ou binaire manquant
set -u
BIN="${1:?usage: garde_fenetres_multiples.sh /chemin/vers/binaire}"
[ -x "$BIN" ] || { echo "binaire introuvable : $BIN" >&2; exit 2; }
BIN=$(CDPATH= cd -- "$(dirname -- "$BIN")" && pwd)/$(basename -- "$BIN")
PORT=$(basename -- "$BIN")
case "$PORT" in
    htmlsermo)    outils="timeout" ;;
    ncursessermo) outils="timeout python3" ;;
    *)            outils="xvfb-run xdotool xprop timeout" ;;
esac
for t in $outils; do
    command -v "$t" >/dev/null || { echo "outil manquant : $t" >&2; exit 2; }
done
T=$(mktemp -d) || exit 2; trap 'rm -rf "$T"' EXIT
cd "$T" || exit 2

export PRINC='<window title="sermo-fen-P"><vbox><entry><variable>E</variable><default>princ</default></entry>
<checkbox><label>un</label><variable>C</variable><action>launch:SEC</action></checkbox>
<checkbox><label>deux</label><variable>C2</variable><action>launch:SEC</action></checkbox>
<timer visible="false"><variable>T</variable><action>disable:T</action><action>activate:C</action></timer>
<timer interval="7" visible="false"><variable>T2</variable><action>disable:T2</action><action>activate:C2</action></timer>
<timer interval="11" visible="false"><variable>F</variable><action>exit:fin</action></timer>
</vbox><variable>PRINC</variable></window>'
# SEC se ferme à son premier tic seulement : relancée, elle reste ouverte.
export SEC='<window title="sermo-fen-S"><vbox><entry><variable>S</variable><default>sec</default></entry>
<timer interval="3" visible="false"><variable>TS</variable><action>sh -c "echo t >> tics-sec"</action>
<action condition="command_is_true([ $(wc -l < tics-sec) = 1 ] &amp;&amp; echo true)">closewindow:SEC</action></timer>
</vbox><variable>SEC</variable></window>'

echec=0
dire() { echo "ECHEC : $*"; echec=1; }

# Les variables rendues (fichier « sortie »), comparées à l'étalon gtk3sermo.
variables() {
    grep -a '^[A-Za-z_][A-Za-z0-9_]*=' sortie | LC_ALL=C sort > rendu
    LC_ALL=C sort > attendu <<'FIN'
C="true"
C2="true"
E="princ"
EXIT="fin"
F="true"
PRINC=""
S="sec"
SEC=""
T="false"
T2="false"
TS="true"
FIN
    if ! diff attendu rendu > ecart; then
        dire "$1 : variables rendues différentes de l'étalon gtk3sermo :"
        sed 's/^/  /' ecart | head -12
    fi
}

# ─── Sans écran : pas d'horloge, chaque minuterie une fois ──────────────────
sans_ecran() {
    : > tics-sec
    SERMO_NCURSES_BATCH=1 SERMO_LVGL_BATCH=1 SERMO_HTML_OUT="$T/page.html" \
        timeout 30 "$BIN" --program=PRINC </dev/null > sortie 2> err
    code=$?
    [ "$code" = 0 ] || dire "sans écran : code de sortie $code ($(grep -av '^safe_' err | head -1))"
    [ "$(wc -l < tics-sec)" = 2 ] \
        || dire "sans écran : la seconde fenêtre a tiqué $(wc -l < tics-sec) fois (2 attendues : lancée, fermée, relancée)"
    variables "sans écran"
}

# ─── ncurses dans un pseudo-terminal ─────────────────────────────────────────
# Les titres n'ont aucun caractère commun : ncurses n'écrit que les cases qui
# changent, « sermo-fen-P » → « sermo-fen-S » ne ferait passer qu'une lettre.
en_terminal() {
    : > tics-sec
    python3 - "$BIN" <<'PY' > etapes
import os, pty, re, select, sys, time
env = dict(os.environ, TERM="xterm", LANG="C.UTF-8", LINES="24", COLUMNS="80", BIN=sys.argv[1])
env.pop("SERMO_NCURSES_BATCH", None)
env["PRINC"] = env["PRINC"].replace("sermo-fen-P", "abcdef")
env["SEC"] = env["SEC"].replace("sermo-fen-S", "UVWXYZ")
pid, fd = pty.fork()
if pid == 0:
    os.execvpe("/bin/sh", ["/bin/sh", "-c", '"$BIN" --program=PRINC; echo FINI-$?'], env)
debut, flux, vus = time.time(), b"", []
while time.time() - debut < 30 and b"FINI-" not in flux:
    if select.select([fd], [], [], 0.05)[0]:
        try:
            bloc = os.read(fd, 65536)
        except OSError:
            break
        if not bloc:
            break
        flux += bloc
        for titre, nom in ((b"abcdef", "P"), (b"UVWXYZ", "S")):
            if titre in bloc and (not vus or vus[-1] != nom):
                vus.append(nom)
try:
    os.kill(pid, 9)
except ProcessLookupError:
    pass
print("titres " + "".join(vus))
fin = flux[flux.rfind(b"\x1b[?1049l"):].decode(errors="replace").replace("\r", "")
print("code " + (fin.split("FINI-")[1].split()[0] if "FINI-" in fin else "aucun"))
# la première variable suit la séquence qui rend l'écran, sur la même ligne
open("sortie", "w").write("\n".join(re.findall(r'[A-Za-z_][A-Za-z0-9_]*="[^"\n]*"', fin)) + "\n")
PY
    grep -q '^titres PSPS$' etapes \
        || dire "en terminal : écrans vus « $(sed -n 's/^titres //p' etapes) » (PSPS attendu : principale, lancée, principale, relancée)"
    grep -q '^code 0$' etapes || dire "en terminal : $(grep '^code' etapes)"
    variables "en terminal"
}

case "$PORT" in
    htmlsermo)
        sans_ecran
        grep -q 'sermo-fen-P' page.html 2>/dev/null || dire "la page n'est pas celle de la fenêtre principale"
        grep -q 'sermo-fen-S' page.html 2>/dev/null && dire "la fenêtre lancée a écrasé la page de la principale"
        [ "$echec" = 0 ] || exit 1
        echo "OK : launch:, closewindow:, launch: sans écran — mêmes variables que l'étalon, page de la principale ($PORT)"
        exit 0 ;;
    ncursessermo)
        sans_ecran
        en_terminal
        [ "$echec" = 0 ] || { sed 's/^/  étape : /' etapes; exit 1; }
        echo "OK : launch:, closewindow:, launch: — sans terminal et dans un terminal, la seconde fenêtre s'ouvre, se ferme et revient ($PORT)"
        exit 0 ;;
    lvglsermo)
        sans_ecran ;;
esac

GSK_RENDERER=cairo GTK_A11Y=none \
xvfb-run -a -s '-screen 0 1024x768x24 -noreset' timeout 60 sh -c '
    command -v xfwm4 >/dev/null 2>&1 && { xfwm4 --compositor=off >/dev/null 2>&1 & sleep 1; }
    # Titres des fenêtres visibles (_NET_WM_NAME : sdl3 ne pose pas WM_NAME).
    titres() {
        # sdl3 échappe à la recherche par nom : on ajoute celle par classe.
        for w in $(xdotool search --onlyvisible --name "" 2>/dev/null) \
                 $(xdotool search --onlyvisible --class "$(basename "$1")" 2>/dev/null); do
            xprop -id "$w" _NET_WM_NAME WM_NAME 2>/dev/null
        done | grep -o "sermo-fen-[PS]" | sort -u | tr -d "\n"
    }
    # attendre <motif attendu dans titres> <étape> : 20 s au plus
    attendre() {
        for i in $(seq 1 100); do
            t=$(titres "$bin")
            case "$1" in
                avec) case "$t" in *sermo-fen-S*) echo "$2 $t" >> etapes; return 0 ;; esac ;;
                sans) case "$t" in *sermo-fen-S*) ;; *sermo-fen-P*) echo "$2 $t" >> etapes; return 0 ;; esac ;;
            esac
            kill -0 "$pid" 2>/dev/null || break
            sleep 0.2
        done
        echo "$2 MANQUE $(titres "$bin")" >> etapes; return 1
    }
    : > tics-sec; : > etapes
    bin=$1
    "$1" --program=PRINC > sortie 2> err &
    pid=$!
    attendre avec ouverte && attendre sans fermee && attendre avec rouverte
    wait "$pid"; echo "code $?" >> etapes
' sh "$BIN"

grep -q '^ouverte sermo-fen-' etapes  || dire "la seconde fenêtre n'est jamais apparue ($(grep '^ouverte' etapes))"
grep -q '^fermee sermo-fen-P$' etapes || dire "closewindow: n'a pas laissé la principale seule à l'écran ($(grep '^fermee' etapes))"
grep -q '^rouverte sermo-fen-' etapes || dire "la seconde fenêtre n'est pas revenue au second launch: ($(grep '^rouverte' etapes))"
case "$PORT" in
    sdl3sermo|lvglsermo) ;;   # une seule fenêtre native, voir l'en-tête
    *) grep -q '^ouverte sermo-fen-Psermo-fen-S$' etapes \
           || dire "les deux fenêtres n'étaient pas à l'écran ensemble ($(grep '^ouverte' etapes))" ;;
esac
grep -q '^code 0$' etapes || dire "code de sortie : $(grep '^code' etapes)"
variables "à l'écran"
[ "$echec" = 0 ] || { sed 's/^/  étape : /' etapes; exit 1; }
echo "OK : launch:, closewindow:, launch: — la seconde fenêtre s'ouvre, se ferme et revient ($PORT)"
