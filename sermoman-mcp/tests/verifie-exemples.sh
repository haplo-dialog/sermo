#!/bin/sh
# SPDX-License-Identifier: GPL-2.0-or-later
# verifie-exemples.sh — rejoue CHAQUE bout de XML documenté dans data/ contre
# les binaires RÉELS des backends, et échoue si l'un d'eux ne passe pas
# l'analyseur.
#
# Pourquoi ce banc existe. Le 2026-08-30, plusieurs formes enseignées par la
# référence étaient des ERREURS DE SYNTAXE : <frame><label>, <notebook><label>,
# <expander><label>, <pixmap><filename>, <table><column-header>. L'exemple
# phare du guide ne démarrait pas. Personne ne l'avait vu parce que rien ne
# rejouait la documentation.
#
# ⛔ Ce banc REFUSE de réussir s'il n'a rien testé : un banc muet qui rend 0
#    est pire que pas de banc.
#
# Cible : le sermo MODULAIRE local. On découvre les binaires des backends dans
# ../sermo-backend-*/_build/, ou on les prend dans SERMO_BINS / le PATH.

set -u
ICI=$(CDPATH= cd -- "$(dirname -- "$0")/.." && pwd)   # .../sermoman-mcp
ROOT=$(CDPATH= cd -- "$ICI/.." && pwd)                # racine du dépôt sermo
TMP=$(mktemp -d); trap 'rm -rf "$TMP"' EXIT

# ── Découverte des binaires ─────────────────────────────────────────────────
# Le cœur (libsermocore) est commun aux backends : n'importe lequel analyse le
# même XML. On en essaie plusieurs quand ils sont là, pour couvrir large.
BINS="${SERMO_BINS:-}"
if [ -z "$BINS" ]; then
    for b in gtk3 gtk4 qt6 fltk1 efl1 sdl3 ncurses; do
        p="$ROOT/sermo-backend-$b/_build/${b}sermo"
        [ -x "$p" ] && BINS="$BINS $p"
    done
    # Repli : binaires installés sur le PATH.
    if [ -z "$BINS" ]; then
        for b in gtk3sermo gtk4sermo qt6sermo fltk1sermo efl1sermo sdl3sermo ncursessermo; do
            command -v "$b" >/dev/null 2>&1 && BINS="$BINS $b"
        done
    fi
fi
if [ -z "$BINS" ]; then
    echo "IGNORÉ : aucun binaire de backend trouvé (construire un ../sermo-backend-*/_build/*sermo, ou poser SERMO_BINS). Rien n'a été vérifié." >&2
    exit 77
fi

# Ports EXIGÉS (optionnel) : sans ça, un backend manquant ferait passer le banc
# au vert en ne testant qu'une partie du périmètre, en silence.
for exige in ${SERMO_PORTS_REQUIS:-}; do
    case " $BINS " in
        *"$exige"*) ;;
        *) echo "ÉCHEC : $exige est exigé (SERMO_PORTS_REQUIS) et introuvable." >&2
           exit 1 ;;
    esac
done
command -v xvfb-run >/dev/null 2>&1 || {
    echo "IGNORÉ : xvfb-run absent — rien n'a été vérifié." >&2; exit 77; }

# ── Extraction : <window>…</window> complets + fragments indentés de la
#    référence, qu'on enveloppe dans un <window> minimal. Balaye TOUT data/. ──
python3 - "$ICI" "$TMP" <<'PYEOF'
import glob, io, os, re, sys
racine, tmp = sys.argv[1], sys.argv[2]
n = 0

def ecrire(txt):
    global n
    io.open(os.path.join(tmp, "c%03d.xml" % n), "w", encoding="utf-8").write(txt)
    n += 1

fichiers = sorted(glob.glob(os.path.join(racine, "data", "*.md"))) \
         + [os.path.join(racine, "data", "reference-xml.txt")]
for chemin in fichiers:
    if not os.path.exists(chemin):
        continue
    lignes = io.open(chemin, encoding="utf-8").read().split("\n")

    # 1) blocs <window>…</window> complets, reconstruits ligne à ligne : on
    #    n'ouvre que sur une ligne commençant par <window, on ferme sur
    #    </window>. Une ligne intermédiaire qui n'est pas du XML annule le bloc
    #    (sinon on avale la prose qui cite des balises).
    buf = None
    for l in lignes:
        t = l.strip()
        if buf is None:
            if t.startswith("<window"):
                buf = [t]
                if "</window>" in t:
                    ecrire(" ".join(buf)); buf = None
            continue
        if t and not t.startswith("<") and not t.startswith("/") \
           and "</" not in t and ">" not in t:
            buf = None            # de la prose : ce n'était pas un exemple
            continue
        buf.append(t)
        if "</window>" in t:
            ecrire(" ".join(buf)); buf = None

    # 2) fragments : suites de lignes indentées (≥ 8 espaces) dont CHACUNE est
    #    du XML — le style des extraits de la référence. On les enveloppe.
    if chemin.endswith(".txt"):
        bloc = []
        for l in lignes:
            t = l.strip()
            if l.startswith("        ") and t.startswith("<"):
                bloc.append(t)
                continue
            if bloc:
                frag = " ".join(bloc)
                if not frag.startswith("<window") and re.search(r"</[a-z]+>", frag):
                    ecrire("<window>" + frag + "</window>")
                bloc = []
        if bloc:
            frag = " ".join(bloc)
            if not frag.startswith("<window") and re.search(r"</[a-z]+>", frag):
                ecrire("<window>" + frag + "</window>")
print(n)
PYEOF
CAS=$(ls "$TMP"/*.xml 2>/dev/null | wc -l)
if [ "$CAS" -eq 0 ]; then
    echo "ÉCHEC : aucun exemple extrait — l'extracteur est cassé, ou data/ est vide." >&2
    exit 1
fi

joues=0; echecs=0
for f in "$TMP"/*.xml; do
    for b in $BINS; do
        MAIN_DIALOG="$(cat "$f")" xvfb-run -a timeout 20 "$b" \
            --program=MAIN_DIALOG --print-ir >/dev/null 2>"$TMP/err"
        rc=$?
        joues=$((joues + 1))
        if [ "$rc" -ne 0 ]; then
            echecs=$((echecs + 1))
            printf 'ÉCHEC  %-28s %s\n' "$(basename "$b")" "$(basename "$f")"
            sed 's/^/         /' "$TMP/err" | head -3
            sed 's/^/         | /' "$f" | head -6
        fi
    done
done

printf '%s exemple(s) documenté(s), %s exécution(s), %s échec(s).\n' \
    "$CAS" "$joues" "$echecs"
printf 'Backends :%s\n' "$(for b in $BINS; do printf ' %s' "$(basename "$b")"; done)"
[ "$joues" -gt 0 ] || { echo "ÉCHEC : rien n'a été joué." >&2; exit 1; }
[ "$echecs" -eq 0 ] || exit 1
echo "Tous les exemples de la documentation passent l'analyseur."
