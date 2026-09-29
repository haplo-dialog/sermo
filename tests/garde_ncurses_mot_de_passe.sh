#!/bin/bash
# SPDX-License-Identifier: GPL-2.0-or-later
# tests/garde_ncurses_mot_de_passe.sh — un <password> ne s'affiche pas en clair.
#
# POURQUOI CE BANC EXISTE
#
# Le backend terminal éditait <password> avec echo() + getnstr() : le mot de
# passe tapé s'affichait EN CLAIR, lisible par-dessus l'épaule et dans tout
# enregistrement du terminal, alors que les six backends graphiques le masquaient.
# Le banc de comportement ne pouvait pas le voir : il tourne en mode batch, sans
# terminal, et ne passe jamais par la saisie.
#
# Ce banc lance le binaire dans un VRAI pseudo-terminal, tape un secret dans le
# champ, et lit tout ce que le programme a écrit à l'écran AVANT de sortir. Le
# secret ne doit pas y être ; il doit en revanche sortir dans la variable.
# Témoin : le même scénario sur un <entry>, qui DOIT afficher le texte tapé —
# sans quoi le banc ne saurait pas voir un secret affiché.
#
# Usage : garde_ncurses_mot_de_passe.sh <chemin/vers/ncursessermo>
# Codes : 0 = masqué · 1 = affiché en clair (ou témoin raté) · 2 = usage/outil

set -u
BIN="${1:-}"
[[ -x "$BIN" ]] || { echo "usage: $0 <chemin/vers/ncursessermo>" >&2; exit 2; }
command -v python3 >/dev/null || { echo "outil manquant : python3" >&2; exit 2; }

exec python3 - "$BIN" <<'PY'
import os, pty, select, sys, time

BIN = sys.argv[1]
SECRET = "Zq7-temoin-mdp"

def lire(fd, duree):
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
    return sortie

def scenario(balise):
    xml = ('<window title="garde"><vbox><%s><variable>CHAMP</variable></%s>'
           '<button ok></button></vbox></window>' % (balise, balise))
    # <button ok> sans action : il doit fermer le dialogue (EXIT="OK"), comme
    # dans gtkdialog et les six autres backends.
    pid, fd = pty.fork()
    if pid == 0:
        env = dict(os.environ, TERM="xterm", LANG="C.UTF-8", LINES="24", COLUMNS="80", DIALOG=xml)
        env.pop("SERMO_NCURSES_BATCH", None)
        os.execvpe(BIN, [BIN, "--program=DIALOG"], env)
    ecran = lire(fd, 1.5)                                   # premier dessin
    os.write(fd, b"\r"); ecran += lire(fd, 0.6)             # Entrée : éditer le champ
    os.write(fd, SECRET.encode()); ecran += lire(fd, 0.8)   # taper le secret
    os.write(fd, b"\r"); ecran += lire(fd, 0.8)             # valider la saisie
    os.write(fd, b"\t"); ecran += lire(fd, 0.5)             # aller au bouton OK
    os.write(fd, b"\r"); apres = lire(fd, 2.5)              # sortir
    try:
        os.kill(pid, 9)
    except ProcessLookupError:
        pass
    try:
        os.waitpid(pid, 0)
    except ChildProcessError:
        pass
    return ecran, apres

# Témoin : un <entry> affiche ce qu'on tape. Si le banc ne le voit pas, il ne
# prouverait rien sur <password>.
ecran, apres = scenario("entry")
if SECRET.encode() not in ecran:
    print("ÉCHEC DU TÉMOIN : le texte tapé dans un <entry> n'apparaît pas à l'écran ;"
          " le banc ne saurait pas voir un mot de passe affiché.")
    sys.exit(1)

ecran, apres = scenario("password")
if SECRET.encode() in ecran:
    print("ÉCHEC : le mot de passe tapé s'affiche en clair dans le terminal.")
    sys.exit(1)
if b'EXIT="OK"' not in apres:
    print("ÉCHEC : le bouton OK (sans action) ne ferme pas le dialogue dans le terminal.")
    sys.exit(1)
if ('CHAMP="%s"' % SECRET).encode() not in apres:
    print("ÉCHEC : le mot de passe n'est pas exporté dans sa variable après la saisie masquée.")
    sys.exit(1)
print("garde_ncurses_mot_de_passe : OK — saisie masquée, valeur exportée, bouton OK qui ferme (témoin <entry> vu).")
PY
