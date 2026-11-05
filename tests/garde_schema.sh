#!/bin/bash
# SPDX-License-Identifier: GPL-2.0-or-later
# garde_schema.sh — sermo.schema.json est-il bien celui que le code produit, et dit-il
# tout ce que les dialogues du dépôt emploient ?
#
#   1. Régénération : outils/schema/generer.py doit produire EXACTEMENT le fichier
#      commité (sinon quelqu'un a touché le code ou le fichier sans régénérer).
#   2. Couverture : chaque balise employée par les dialogues du dépôt (examples/,
#      tests/xml/, sermoia-mcp/examples/) est connue du schéma. Les attributs
#      employés et lus par aucun port (hors gtk3/gtk4, qui acceptent toute
#      propriété GObject) sont comptés et listés, sans faire échouer : c'est un
#      état, pas une faute.
#   3. Témoin : un dialogue fabriqué avec une balise inventée doit être vu.
#
# Codes : 0 = schéma à jour et couvrant · 1 = divergence ou balise inconnue · 77 = outil manquant
set -u
RACINE=$(CDPATH= cd -- "$(dirname -- "$0")/.." && pwd)
cd "$RACINE" || exit 2
command -v python3 >/dev/null || { echo "IGNORÉ : python3 manquant" >&2; exit 77; }
[ -f sermo.schema.json ] || { echo "ECHEC : sermo.schema.json absent (outils/schema/generer.py)"; exit 1; }
mkdir -p "${TMPDIR:-$RACINE/_build}" 2>/dev/null   # un clone neuf n'a pas _build/ (vu le 2026-11-05 sur bancs-sur-paquets)
T=$(mktemp -d "${TMPDIR:-$RACINE/_build}/garde_schema.XXXXXX") || exit 77
trap 'rm -rf "$T"' EXIT
ecarts=0

# ── 1. Régénération à l'identique (le commit, lui, peut différer : on l'efface des deux côtés)
python3 -I outils/schema/generer.py "$RACINE" --sortie "$T/regen.json" >/dev/null || { echo "ECHEC : la génération échoue"; exit 1; }
sans_commit() { python3 -I -c 'import json,sys; d=json.load(open(sys.argv[1])); d.pop("commit",None); print(json.dumps(d,ensure_ascii=False,sort_keys=True))' "$1"; }
if [ "$(sans_commit sermo.schema.json)" != "$(sans_commit "$T/regen.json")" ]; then
    echo "ECHEC : sermo.schema.json n'est plus ce que le code produit — régénérer : python3 -I outils/schema/generer.py"
    diff <(python3 -I -m json.tool sermo.schema.json) <(python3 -I -m json.tool "$T/regen.json") | head -20
    ecarts=$((ecarts + 1))
else
    echo "  ✔ régénération identique (hors commit)"
fi

# ── 2. Couverture des dialogues du dépôt ─────────────────────────────────────
python3 -I - "$RACINE" "$T" <<'PY' || ecarts=$((ecarts + 1))
import glob, json, os, re, sys
R, T = sys.argv[1], sys.argv[2]
d = json.load(open(os.path.join(R, "sermo.schema.json"), encoding="utf-8"))
connues = set(d["widgets"]) | set(d["contenu"]) | set(d["contenu_alias"]) | set(d["controle"]) | set(d["herites"])
connues |= {a for w in d["widgets"].values() for a in w["alias"]} | {"button"}
lus = {w: set(v["attributs_lus"]) for w, v in d["widgets"].items()}
communs = set(d["attributs_communs"]) | set(d["attributs_hors_widget"])
fichiers = sorted(glob.glob(os.path.join(R, "examples", "**", "*.xml"), recursive=True)) + \
    sorted(glob.glob(os.path.join(R, "tests", "xml", "*.xml"))) + \
    sorted(glob.glob(os.path.join(R, "sermoia-mcp", "examples", "**", "*.xml"), recursive=True))
inconnues = {}
attrs_inconnus = {}
RX_TAG = re.compile(r"<([a-z_][a-z_0-9]*)(\s[^>]*)?>")
RX_ATTR = re.compile(r'\b([a-zA-Z_][a-zA-Z0-9_-]*)="')
for f in fichiers:
    s = open(f, encoding="utf-8", errors="replace").read()
    s = re.sub(r"<!--.*?-->", "", s, flags=re.S)
    # Le texte des balises de contenu n'est pas du sermo : un <label> peut porter du
    # balisage Pango (<b>, <big>, <span>), un <input> une commande avec des < >.
    s = re.sub(r"<(label|item|default|input|output|action)\b[^>]*>.*?</\1>", "<\\1></\\1>", s, flags=re.S)
    for m in RX_TAG.finditer(s):
        tag, reste = m.group(1), m.group(2) or ""
        if tag not in connues:
            inconnues.setdefault(tag, []).append(os.path.relpath(f, R))
        elif tag in lus:
            for a in RX_ATTR.findall(reste):
                if a not in lus[tag] and a not in communs:
                    attrs_inconnus.setdefault("%s@%s" % (a, tag), set()).add(os.path.relpath(f, R))
print("  dialogues lus : %d" % len(fichiers))
if inconnues:
    print("ECHEC : balise(s) employée(s) par le dépôt et absente(s) du schéma :")
    for t, fs in sorted(inconnues.items()):
        print("    <%s> : %s%s" % (t, ", ".join(fs[:3]), " …" if len(fs) > 3 else ""))
    sys.exit(1)
print("  ✔ toutes les balises employées sont dans le schéma")
if attrs_inconnus:
    print("  état : %d attribut(s)@widget employé(s) et lu(s) par aucun port hors gtk3/gtk4 (propriétés GTK libres) :" % len(attrs_inconnus))
    for k, fs in sorted(attrs_inconnus.items())[:15]:
        print("    %s (%d fichier(s))" % (k, len(fs)))
    if len(attrs_inconnus) > 15:
        print("    … et %d autre(s)" % (len(attrs_inconnus) - 15))
PY

# ── 3. Témoin : une balise inventée doit être vue ────────────────────────────
python3 -I - "$RACINE" <<'PY' && echo "  ✔ témoin : <bouton> inventé est vu absent du schéma" || { echo "ECHEC : le témoin <bouton> passe pour connu"; ecarts=$((ecarts + 1)); }
import json, os, sys
d = json.load(open(os.path.join(sys.argv[1], "sermo.schema.json"), encoding="utf-8"))
connues = set(d["widgets"]) | set(d["contenu"]) | set(d["controle"]) | set(d["herites"])
sys.exit(0 if "bouton" not in connues and "button" in connues else 1)
PY

[ "$ecarts" -eq 0 ] && { echo "garde_schema : OK — schéma régénérable à l'identique, dialogues du dépôt couverts, témoin vu"; exit 0; }
echo "garde_schema : $ecarts écart(s)"; exit 1
