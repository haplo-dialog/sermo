#!/bin/bash
# SPDX-License-Identifier: GPL-2.0-or-later
# tests/garde_ncurses_sortie_terminal.sh — sans capture, les variables de sortie
# restent à l'écran une fois le dialogue fermé.
#
# POURQUOI CE BANC EXISTE
#
# Lancé dans un terminal, sortie NON capturée :
#
#     ncursessermo --file=dialogue.xml
#
# le port terminal dessine sur l'écran alterné, et les variables (NOM="…",
# EXIT="…") s'écrivent sur ce même terminal. Le cœur les écrivait pendant que
# l'écran alterné était encore ouvert : endwin() n'arrivait qu'ensuite, à la
# sortie du programme, et les variables partaient avec l'écran alterné. Une fois
# le dialogue fermé, le terminal ne montrait plus rien (sortie par minuterie,
# et tout « exit:… » joué par le cœur).
#
# Ce banc lance le binaire dans un VRAI pseudo-terminal et lit le flux brut :
# les variables doivent arriver APRÈS la séquence qui quitte l'écran alterné
# (ESC [ ? 1049 l), sans réouverture ensuite — sinon un terminal les efface.
# Témoin : le flux doit montrer l'entrée et la sortie de l'écran alterné, sans
# quoi le banc ne saurait pas voir le défaut.
#
# Usage : garde_ncurses_sortie_terminal.sh <chemin/vers/ncursessermo>
# Codes : 0 = conforme · 1 = écart (ou témoin raté) · 2 = usage/outil

set -u
BIN="${1:-}"
[[ -x "$BIN" ]] || { echo "usage: $0 <chemin/vers/ncursessermo>" >&2; exit 2; }
command -v python3 >/dev/null || { echo "outil manquant : python3" >&2; exit 2; }
BIN="$(readlink -f "$BIN")"

exec python3 - "$BIN" <<'PY'
import os, pty, select, sys, time

BIN = sys.argv[1]
TITRE = "garde-terminal-Kp3"
DEFAUT = "valeur-temoin-17"
MINUTERIE = ('<window title="%s"><vbox>'
             '<entry><variable>NOM</variable><default>%s</default></entry>'
             '<timer visible="false" interval="1"><action>exit:fin</action></timer>'
             '</vbox></window>' % (TITRE, DEFAUT))
BOUTON = ('<window title="%s"><vbox>'
          '<entry><variable>NOM</variable><default>%s</default></entry>'
          '<button ok></button>'
          '</vbox></window>' % (TITRE, DEFAUT))
ENTREE, SORTIE = b"\x1b[?1049h", b"\x1b[?1049l"
echecs = []

def lire(fd, duree, jusqua=None):
    """Lit le terminal pendant `duree` s, ou jusqu'à voir `jusqua`."""
    fin, sortie = time.time() + duree, b""
    while time.time() < fin:
        prets, _, _ = select.select([fd], [], [], 0.05)
        if prets:
            try:
                bloc = os.read(fd, 65536)
            except OSError:
                break
            if not bloc:
                break
            sortie += bloc
            if jusqua and jusqua in sortie:
                break
    return sortie

def jouer(dialogue, touches=b""):
    """Lance le dialogue sans capture ; rend tout le flux du terminal."""
    pid, fd = pty.fork()
    if pid == 0:
        env = dict(os.environ, TERM="xterm", LANG="C.UTF-8", LINES="24", COLUMNS="80",
                   DIALOG=dialogue, BIN=BIN)
        env.pop("SERMO_NCURSES_BATCH", None)
        os.execvpe("/bin/sh", ["/bin/sh", "-c", '"$BIN" --program=DIALOG; echo FINI-Kp3'], env)
    flux = lire(fd, 3, TITRE.encode())
    for t in touches:
        os.write(fd, bytes([t])); flux += lire(fd, 0.6)
    flux += lire(fd, 6, b"FINI-Kp3") + lire(fd, 0.3)
    try:
        os.kill(pid, 9)
    except ProcessLookupError:
        pass
    try:
        os.waitpid(pid, 0)
    except ChildProcessError:
        pass
    return flux

def verifier(nom, flux, exit_attendu):
    if TITRE.encode() not in flux:
        echecs.append("%s : aucun écran — le dialogue ne s'est pas dessiné" % nom)
        return
    if b"FINI-Kp3" not in flux:
        echecs.append("%s : le dialogue n'est pas sorti de lui-même" % nom)
        return
    if ENTREE not in flux or SORTIE not in flux:
        echecs.append("%s : ÉCHEC DU TÉMOIN — le flux ne montre pas l'écran alterné ;"
                      " le banc ne saurait pas voir le défaut" % nom)
        return
    i = flux.rfind(SORTIE)
    apres = flux[i + len(SORTIE):]
    for attendu in (('NOM="%s"' % DEFAUT).encode(), ('EXIT="%s"' % exit_attendu).encode()):
        if attendu not in flux:
            echecs.append("%s : %s n'est pas écrit du tout" % (nom, attendu.decode()))
        elif attendu not in apres:
            echecs.append("%s : %s est écrit DANS l'écran alterné — effacé à la fermeture"
                          % (nom, attendu.decode()))
    if ENTREE in apres:
        echecs.append("%s : l'écran alterné est rouvert après les variables" % nom)

# 1. Une minuterie qui sort (exit:fin) : action jouée par le cœur, sans touche.
verifier("minuterie", jouer(MINUTERIE), "fin")
# 2. Le bouton OK (Tab puis Entrée) : le port rend l'écran lui-même avant
#    l'export ; ce chemin marchait déjà, il ne doit pas régresser.
verifier("bouton OK", jouer(BOUTON, b"\t\r"), "OK")

if echecs:
    for e in echecs:
        print("ÉCHEC : " + e)
    sys.exit(1)
print("garde_ncurses_sortie_terminal : OK — sans capture, les variables arrivent après la"
      " sortie de l'écran alterné (minuterie, bouton OK ; témoin écran alterné vu).")
PY
