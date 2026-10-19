#!/bin/bash
# SPDX-License-Identifier: GPL-2.0-or-later
# tests/garde_spdx_scripts.sh — haplo-dialog — 2026 — GPL-2.0-or-later
#
# garde_spdx.sh ne lit que le C (.c .h .cc .cpp .hpp .l .y). Les SCRIPTS (.sh,
# .py, exécutables à shebang) n'étaient contrôlés par aucun banc, et c'est ainsi
# que six exemples en CC0 sont restés déclarés GPL-2+ dans debian/copyright
# (bloc « examples/* ») du 17 au 18/10/2026. Ce banc compare, pour chaque script
# suivi par git :
#   - son en-tête SPDX (5 premières lignes)
#   - la licence que debian/copyright lui attribue (DEP-5 : le DERNIER bloc
#     « Files: » qui le désigne l'emporte ; « * » y traverse les « / »).
# Règles :
#   1 — en-tête présent : il doit dire la même licence que debian/copyright ;
#   2 — en-tête absent : accepté SEULEMENT si le bloc qui couvre le fichier est
#       l'héritage gtkdialog (Copyright László Pere ou Thunor) — ces fichiers
#       viennent de l'amont et n'ont jamais porté d'en-tête ; en ajouter un serait
#       choisir une licence à la place de leurs auteurs.
# Pas de liste d'exceptions à entretenir : c'est debian/copyright qui fait foi.
# Témoin : un faux dépôt où l'en-tête contredit le copyright doit être refusé.
# Usage : garde_spdx_scripts.sh [racine du dépôt]
set -u
RACINE=$(CDPATH= cd -- "${1:-$(dirname -- "$0")/..}" && pwd) || exit 2
command -v python3 >/dev/null || { echo "outil manquant : python3" >&2; exit 77; }
command -v git >/dev/null || { echo "outil manquant : git" >&2; exit 77; }

verifier() {   # verifier <racine> → 0 si conforme ; liste les fautes sur la sortie
    python3 -I - "$1" <<'PY'
import fnmatch, os, re, subprocess, sys
racine = sys.argv[1]
NOMS = {"GPL-2+": "GPL-2.0-or-later", "GPL-2": "GPL-2.0-only", "GPL-3+": "GPL-3.0-or-later",
        "LGPL-2+": "LGPL-2.0-or-later", "LGPL-2.1+": "LGPL-2.1-or-later", "LGPL-3+": "LGPL-3.0-or-later",
        "Expat": "MIT"}
def norme(l):
    l = l.strip()
    return NOMS.get(l, l)

# Lecture DEP-5 : blocs « Files: » (lignes de suite indentées), Copyright, License.
blocs, courant, cle = [], None, None
for ligne in open(os.path.join(racine, "debian/copyright"), encoding="utf-8"):
    ligne = ligne.rstrip("\n")
    if not ligne.strip():
        courant, cle = None, None
        continue
    if ligne[0] in " \t":
        if courant is not None and cle:
            courant[cle] += " " + ligne.strip()
        continue
    k, _, v = ligne.partition(":")
    if k == "Files":
        courant = {"Files": v.strip(), "Copyright": "", "License": ""}
        blocs.append(courant)
        cle = "Files"
    elif courant is not None and k in ("Copyright", "License"):
        courant[k] = v.strip()
        cle = k
    else:
        cle = None
for b in blocs:
    b["motifs"] = b["Files"].split()
    b["licence"] = norme(b["License"].split("\n")[0]) if b["License"] else ""
    b["heritage"] = bool(re.search(r"László Pere|Thunor", b["Copyright"]))

def bloc_de(f):
    trouve = None
    for b in blocs:
        if any(fnmatch.fnmatchcase(f, m) for m in b["motifs"]):
            trouve = b
    return trouve

suivis = subprocess.run(["git", "-C", racine, "ls-files", "-z"], capture_output=True, check=True).stdout.split(b"\0")
fautes = vus = 0
for brut in suivis:
    f = brut.decode("utf-8", "surrogateescape")
    if not f or "/tiers/" in f or "/imgui/" in f or f.startswith("debian/"):
        continue
    chemin = os.path.join(racine, f)
    if not os.path.isfile(chemin) or os.path.islink(chemin):
        continue
    with open(chemin, "rb") as fh:
        tete = fh.read(4096)
    script = f.endswith((".sh", ".py")) or (tete.startswith(b"#!") and os.access(chemin, os.X_OK))
    if not script:
        continue
    vus += 1
    m = re.search(rb"SPDX-License-Identifier:\s*([^\s*]+)", b"\n".join(tete.split(b"\n")[:5]))
    b = bloc_de(f)
    attendu = b["licence"] if b else "(aucun bloc)"
    if m:
        dit = m.group(1).decode("ascii", "replace")
        if dit != attendu:
            print("  ✘ %s : en-tête %s, debian/copyright %s" % (f, dit, attendu))
            fautes += 1
    elif not (b and b["heritage"]):
        print("  ✘ %s : aucun en-tête SPDX, et ce n'est pas un fichier hérité de gtkdialog (bloc : %s)" % (f, b["Files"][:60] if b else "aucun"))
        fautes += 1
print("VUS=%d FAUTES=%d" % (vus, fautes))
sys.exit(1 if fautes else 0)
PY
}

# ── Témoin de sensibilité : un faux dépôt fautif doit être refusé ────────────
T=$(mktemp -d); trap 'rm -rf -- "$T"' EXIT
mkdir -p "$T/debian" "$T/examples/neuf" "$T/examples/vieux"
cat > "$T/debian/copyright" <<'DEP5'
Format: https://www.debian.org/doc/packaging-manuals/copyright-format/1.0/

Files: *
Copyright: 2026 S. Cage
License: GPL-2+

Files: examples/*
Copyright: 2011-2013 Thunor
License: GPL-2+
DEP5
printf '#!/bin/sh\n# SPDX-License-Identifier: CC0-1.0\n' > "$T/examples/neuf/a.sh"      # contredit le copyright
printf '#!/bin/sh\necho hérité\n' > "$T/examples/vieux/b.sh"                          # hérité : permis sans en-tête
printf '#!/bin/sh\necho sans\n' > "$T/c.sh"                                            # ni en-tête ni héritage
( cd "$T" && git init -q && git add -A ) || exit 2
sortie=$(verifier "$T"); rc=$?
[[ $rc == 1 && "$sortie" == *"examples/neuf/a.sh : en-tête CC0-1.0, debian/copyright GPL-2.0-or-later"* \
   && "$sortie" == *"c.sh : aucun en-tête SPDX"* && "$sortie" != *"vieux/b.sh"* && "$sortie" == *"FAUTES=2"* ]] \
    || { echo "ÉCHEC — témoin : le banc ne voit pas les fautes fabriquées (rc=$rc) :"; echo "$sortie"; exit 1; }
echo "  ✔ témoin : en-tête contredit et en-tête manquant détectés, fichier hérité accepté"

# ── Le vrai dépôt ───────────────────────────────────────────────────────────
sortie=$(verifier "$RACINE"); rc=$?
vus=$(sed -n 's/^VUS=\([0-9]*\).*/\1/p' <<<"$sortie")
[[ "${vus:-0}" -gt 100 ]] || { echo "ÉCHEC — seulement ${vus:-0} scripts vus : le banc ne regarde pas le dépôt"; exit 1; }
if [[ $rc != 0 ]]; then
    echo "ÉCHEC — licence des scripts : en-tête SPDX et debian/copyright ne disent pas la même chose"
    grep '✘' <<<"$sortie"
    echo "Corriger l'un ou l'autre ; debian/copyright est ce que le paquet annonce."
    exit 1
fi
echo "garde_spdx_scripts : OK — $vus scripts, en-têtes et debian/copyright d'accord (fichiers hérités de gtkdialog sans en-tête acceptés)"
