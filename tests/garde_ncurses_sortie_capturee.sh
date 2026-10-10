#!/bin/bash
# SPDX-License-Identifier: GPL-2.0-or-later
# tests/garde_ncurses_sortie_capturee.sh — le port terminal garde son écran
# quand sa sortie est capturée.
#
# POURQUOI CE BANC EXISTE
#
# Un script gtkdialog lit presque toujours les variables du dialogue ainsi :
#
#     eval "$(ncursessermo --program=MAIN_DIALOG)"
#
# La sortie standard est alors un tube, pas un terminal. Le port terminal en
# concluait « pas d'écran » : il n'ouvrait rien et rendait EXIT="abort" en
# quelques centièmes de seconde, alors que l'utilisateur était bien assis devant
# un terminal. Les ports graphiques, eux, ouvrent leur fenêtre dans ce cas.
# Le banc de comportement ne pouvait pas le voir : il tourne sans terminal.
#
# Ce banc lance le binaire dans un VRAI pseudo-terminal, sortie capturée, et
# vérifie que l'écran est dessiné sur le terminal, que la capture ne reçoit que
# les variables, et que le terminal est rendu dans son état normal.
# Témoin : le même dialogue sans capture, qui DOIT montrer son écran — sans
# quoi le banc ne saurait pas voir un écran.
# Le mode sans écran reste voulu dans deux cas, vérifiés aussi : la variable
# SERMO_NCURSES_BATCH, et une entrée standard qui n'est pas un terminal.
#
# Usage : garde_ncurses_sortie_capturee.sh <chemin/vers/ncursessermo>
# Codes : 0 = conforme · 1 = écart (ou témoin raté) · 2 = usage/outil

set -u
BIN="${1:-}"
[[ -x "$BIN" ]] || { echo "usage: $0 <chemin/vers/ncursessermo>" >&2; exit 2; }
command -v python3 >/dev/null || { echo "outil manquant : python3" >&2; exit 2; }
BIN="$(readlink -f "$BIN")"
T="$(mktemp -d)"; trap 'rm -rf "$T"' EXIT

exec python3 - "$BIN" "$T" <<'PY'
import os, pty, select, sys, time

BIN, TMP = sys.argv[1], sys.argv[2]
TITRE = "garde-capture-Zq7"
TEXTE = "valeur-temoin-42"
FORMULAIRE = ('<window title="%s"><vbox><entry><variable>CHAMP</variable></entry>'
              '<button ok></button></vbox></window>' % TITRE)
MINUTERIE = ('<window title="%s"><vbox><text><label>attente</label></text>'
             '<timer interval="1"><variable>T</variable><action>exit:fin</action></timer>'
             '</vbox></window>' % TITRE)
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

def lancer(commande, dialogue, env_plus=None):
    pid, fd = pty.fork()
    if pid == 0:
        env = dict(os.environ, TERM="xterm", LANG="C.UTF-8", LINES="24", COLUMNS="80",
                   DIALOG=dialogue, BIN=BIN, TMP=TMP)
        env.pop("SERMO_NCURSES_BATCH", None)
        env.update(env_plus or {})
        os.execvpe("/bin/sh", ["/bin/sh", "-c", commande], env)
    return pid, fd

def finir(pid):
    try:
        os.kill(pid, 9)
    except ProcessLookupError:
        pass
    try:
        os.waitpid(pid, 0)
    except ChildProcessError:
        pass

def remplir_et_valider(fd):
    """Tape TEXTE dans le champ, puis active le bouton OK."""
    os.write(fd, b"\r"); lire(fd, 0.6)              # Entrée : éditer le champ
    os.write(fd, TEXTE.encode()); lire(fd, 0.8)     # taper le texte
    os.write(fd, b"\r"); lire(fd, 0.8)              # valider la saisie
    os.write(fd, b"\t"); lire(fd, 0.5)              # aller au bouton OK
    os.write(fd, b"\r")                             # sortir

def entre(marque, texte):
    """Ce que la commande a écrit entre MARQUE<< et >>, ou None."""
    debut, fin_ = ("%s<<" % marque).encode(), b">>"
    i = texte.rfind(debut)
    if i < 0:
        return None
    j = texte.find(fin_, i)
    return texte[i + len(debut):j] if j >= 0 else None

def terminal_rendu(apres):
    """Le terminal est-il revenu à l'état normal (écho et mode ligne) ?"""
    etat = entre("STTY", apres)
    if etat is None:
        return False
    mots = etat.split()
    return b"echo" in mots and b"icanon" in mots

SUITE = '; printf "STTY<<%s>>\\n" "$(stty -a)"'

# ── Témoin : sans capture, l'écran se dessine ───────────────────────────────
pid, fd = lancer('"$BIN" --program=DIALOG' + SUITE, FORMULAIRE)
ecran = lire(fd, 3, TITRE.encode())
remplir_et_valider(fd)
apres = lire(fd, 4, b"STTY<<") + lire(fd, 1)
finir(pid)
if TITRE.encode() not in ecran:
    print("ÉCHEC DU TÉMOIN : sans capture, le titre du dialogue n'apparaît pas à l'écran ;"
          " le banc ne saurait pas voir un écran.")
    sys.exit(1)
if ('CHAMP="%s"' % TEXTE).encode() not in apres or b'EXIT="OK"' not in apres:
    print("ÉCHEC DU TÉMOIN : sans capture, le dialogue ne rend pas CHAMP et EXIT=\"OK\".")
    sys.exit(1)

# ── 1. Sortie capturée par $(…) : l'écran reste, la capture ne reçoit que les variables
pid, fd = lancer('x=$("$BIN" --program=DIALOG); printf "CAPTURE<<%s>>\\n" "$x"' + SUITE, FORMULAIRE)
ecran = lire(fd, 3, TITRE.encode())
if TITRE.encode() not in ecran:
    echecs.append("capture $(…) : aucun écran — le dialogue ne s'ouvre pas quand sa sortie est capturée")
else:
    remplir_et_valider(fd)
apres = lire(fd, 4, b"STTY<<") + lire(fd, 1)
finir(pid)
capture = entre("CAPTURE", ecran + apres)
if capture is None:
    echecs.append("capture $(…) : la commande n'a rien rendu")
else:
    if TITRE.encode() in ecran:
        if ('CHAMP="%s"' % TEXTE).encode() not in capture:
            echecs.append("capture $(…) : CHAMP n'est pas dans la sortie capturée")
        if b'EXIT="OK"' not in capture:
            echecs.append("capture $(…) : EXIT=\"OK\" n'est pas dans la sortie capturée")
        if not terminal_rendu(apres):
            echecs.append("capture $(…) : le terminal n'est pas rendu à l'état normal (écho, mode ligne)")
    if b"\x1b" in capture:
        echecs.append("capture $(…) : des séquences d'écran sont passées dans la sortie capturée")

# ── 2. Sortie envoyée dans un fichier : même attente ────────────────────────
pid, fd = lancer('"$BIN" --program=DIALOG > "$TMP/sortie"; echo FINI' + SUITE, FORMULAIRE)
ecran = lire(fd, 3, TITRE.encode())
if TITRE.encode() not in ecran:
    echecs.append("sortie dans un fichier : aucun écran")
else:
    remplir_et_valider(fd)
apres = lire(fd, 4, b"STTY<<") + lire(fd, 1)
finir(pid)
try:
    with open(os.path.join(TMP, "sortie"), "rb") as f:
        fichier = f.read()
except OSError:
    fichier = b""
if TITRE.encode() in ecran:
    if ('CHAMP="%s"' % TEXTE).encode() not in fichier or b'EXIT="OK"' not in fichier:
        echecs.append("sortie dans un fichier : CHAMP et EXIT=\"OK\" ne sont pas dans le fichier")
    if not terminal_rendu(apres):
        echecs.append("sortie dans un fichier : le terminal n'est pas rendu à l'état normal")
if b"\x1b" in fichier:
    echecs.append("sortie dans un fichier : des séquences d'écran sont dans le fichier")

# ── 3. Une minuterie qui sort (exit:fin), sortie capturée : terminal rendu ──
pid, fd = lancer('x=$("$BIN" --program=DIALOG); printf "CAPTURE<<%s>>\\n" "$x"' + SUITE, MINUTERIE)
ecran = lire(fd, 3, TITRE.encode())
apres = lire(fd, 6, b"STTY<<") + lire(fd, 1)
finir(pid)
capture = entre("CAPTURE", ecran + apres)
if TITRE.encode() not in ecran:
    echecs.append("minuterie : aucun écran")
elif capture is None or b'EXIT="fin"' not in capture:
    echecs.append("minuterie : EXIT=\"fin\" n'est pas dans la sortie capturée")
elif b"\x1b" in capture:
    echecs.append("minuterie : des séquences d'écran sont passées dans la sortie capturée")
elif not terminal_rendu(apres):
    echecs.append("minuterie : le terminal n'est pas rendu à l'état normal après la sortie")

# ── 4. Le mode sans écran reste voulu : SERMO_NCURSES_BATCH ─────────────────
pid, fd = lancer('x=$("$BIN" --program=DIALOG); printf "CAPTURE<<%s>>\\n" "$x"', FORMULAIRE,
                 {"SERMO_NCURSES_BATCH": "1"})
tout = lire(fd, 4, b">>")
finir(pid)
capture = entre("CAPTURE", tout)
if TITRE.encode() in tout:
    echecs.append("SERMO_NCURSES_BATCH=1 : un écran s'ouvre alors que le mode sans écran est demandé")
if capture is None or b'EXIT="abort"' not in capture:
    echecs.append("SERMO_NCURSES_BATCH=1 : le dialogue ne rend pas EXIT=\"abort\" de lui-même")

# ── 5. … et une entrée standard qui n'est pas un terminal (les bancs, cron) ─
pid, fd = lancer('x=$("$BIN" --program=DIALOG </dev/null); printf "CAPTURE<<%s>>\\n" "$x"', FORMULAIRE)
tout = lire(fd, 4, b">>")
finir(pid)
capture = entre("CAPTURE", tout)
if TITRE.encode() in tout:
    echecs.append("entrée </dev/null : un écran s'ouvre alors que personne ne peut taper")
if capture is None or b'EXIT="abort"' not in capture:
    echecs.append("entrée </dev/null : le dialogue ne rend pas EXIT=\"abort\" de lui-même")

if echecs:
    for e in echecs:
        print("ÉCHEC : " + e)
    sys.exit(1)
print("garde_ncurses_sortie_capturee : OK — écran gardé sous $(…) et vers un fichier, capture"
      " propre, terminal rendu, modes sans écran intacts (témoin sans capture vu).")
PY
