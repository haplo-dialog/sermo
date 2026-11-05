#!/bin/bash
# SPDX-License-Identifier: GPL-2.0-or-later
# ci/bilan-md.sh — le tableau « Par backend » de BILAN_SANTE.md, écrit depuis bilan.json.
#
# BILAN_SANTE.md recopiait ses chiffres à la main (le 2026-11-03 : « 61 cas »,
# « 176 bancs » quand la CI en jouait 78 et 265). Ici le bloc entre les deux
# repères invisibles
#     [//]: # (bilan:debut)   …   [//]: # (bilan:fin)
# est RÉÉCRIT depuis _journaux/bilan.json (ci/bilan-json.sh, à la fin de
# ci/bancs.sh). Le reste du fichier — les éditions datées, écrites par le
# mainteneur — n'est pas touché. Sans repères, le bloc est posé après le titre.
# Les repères sont des définitions de lien vides : GitHub et le site (markdown-it)
# ne les affichent pas. Geste du mainteneur, avant une publication :
#     bash ci/bancs.sh && bash ci/bilan-md.sh
#
# Usage : ci/bilan-md.sh [bilan.json] [BILAN_SANTE.md]
# Codes : 0 = bloc écrit (ou déjà identique) · 1 = entrée absente ou illisible · 2 = outil manquant

set -u
RACINE=$(CDPATH= cd -- "$(dirname -- "$0")/.." && pwd)
BILAN="${1:-$RACINE/_journaux/bilan.json}"
MD="${2:-$RACINE/BILAN_SANTE.md}"
command -v python3 >/dev/null || { echo "outil manquant : python3" >&2; exit 2; }
[ -f "$BILAN" ] || { echo "pas de bilan : $BILAN (jouer ci/bancs.sh d'abord)" >&2; exit 1; }
[ -f "$MD" ] || { echo "pas de fichier : $MD" >&2; exit 1; }

python3 -I - "$BILAN" "$MD" <<'PY'
import json, re, sys

bilan = json.load(open(sys.argv[1], encoding="utf-8"))
md_path = sys.argv[2]
texte = open(md_path, encoding="utf-8").read()
DEBUT, FIN = "[//]: # (bilan:debut)", "[//]: # (bilan:fin)"
PORTS = ["gtk3", "gtk4", "qt6", "fltk1", "efl1", "sdl3", "lvgl", "ncurses", "html"]
P = bilan["ports"]


def coche(b):
    v = b.get("verdict", "absent")
    return {"OK": "✅", "absent": "—", "non enregistré": "?"}.get(v, "❌")


def cellule(port, cle, detail=lambda b: ""):
    b = P.get(port, {}).get(cle)
    if not b or b.get("verdict") == "absent":
        return "—"
    d = detail(b)
    return coche(b) + (" " + d if d else "")


etalon = P.get("gtk3", {})
n_xml = etalon.get("xml", {}).get("verts", 0) + etalon.get("xml", {}).get("rouges", 0) + etalon.get("xml", {}).get("sautes", 0) or "?"
n_comp = etalon.get("comportement", {}).get("cas", "?")
n_durc = etalon.get("durcissement", {}).get("garanties", "?")
n_ech = etalon.get("echappement", {}).get("cas", "?")
n_clic = etalon.get("clic", {}).get("widgets", "?")

lignes = [DEBUT, "",
          "## Dernière passe des bancs — commit `%s`, le %s" % (bilan.get("commit", "?"), bilan.get("date", "?")), "",
          "*Bloc écrit par `ci/bilan-md.sh` depuis `_journaux/bilan.json` (`ci/bilan-json.sh`, en fin de",
          "`ci/bancs.sh`) : ne pas le corriger à la main, rejouer les bancs. Les éditions datées qui",
          "suivent sont écrites par le mainteneur et gardent leurs chiffres.*", ""]
b = bilan.get("bancs", {})
if "joues" in b:
    lignes += ["```sh", "bash ci/bancs.sh          # %d bancs : %d verts, %d en échec" % (b["joues"], b["joues"] - b["echecs"], b["echecs"]), "```", ""]
else:
    lignes += ["*%s*" % b.get("note", "verdicts non enregistrés"), ""]
lignes += ["### Par backend", "",
           "| Backend | XML (%s) | Comportement (%s) | Durcissement (%s) | `--glade-xml` | `<terminal>` | Échappement (%s) | Clic (%s) | Exemples réels |"
           % (n_xml, n_comp, n_durc, n_ech, n_clic),
           "|---|:--:|:--:|:--:|:--:|:--:|:--:|:--:|:--:|"]
for p in PORTS:
    # Un port sans aucun banc (binaire absent, port non demandé) n'a pas de ligne.
    if p not in P or all(isinstance(b, dict) and b.get("verdict") == "absent" for k, b in P[p].items() if k != "autres") and "autres" not in P[p]:
        continue
    nom = "gtk3 (étalon)" if p == "gtk3" else p
    xml = cellule(p, "xml", lambda b: "%s/%s" % (b.get("verts", "?"), b.get("verts", 0) + b.get("rouges", 0) + b.get("sautes", 0)) if "verts" in b else "")
    comp = cellule(p, "comportement", lambda b: "%s/%s" % (b.get("verts", "?"), b.get("cas", "?")) if "cas" in b else "")
    durc = cellule(p, "durcissement", lambda b: "%s/%s" % (b.get("garanties", "?"), b.get("garanties", "?")) if "garanties" in b else "")
    glade = cellule(p, "glade", lambda b: b.get("etat", ""))
    term = cellule(p, "terminal", lambda b: b.get("etat", ""))
    ech = cellule(p, "echappement", lambda b: "%s/%s" % (b.get("cas", "?"), b.get("cas", "?")) if "cas" in b else "")
    clic = cellule(p, "clic", lambda b: "%s/%s" % (b.get("widgets", "?"), b.get("widgets", "?")) if "widgets" in b else "")

    def ex(b):
        if "joues" not in b:
            return ""
        s = "%s/%s" % (b.get("ok", "?"), b["joues"])
        nj = b.get("non_joues", [])
        if nj:
            outils = [re.sub(r".*: (\S+) absent.*", r"\1", l) for l in nj]
            s += " ; %d non joué(s) faute d'outil (%s)" % (len(nj), ", ".join(outils))
        return s
    exemples = cellule(p, "exemples", ex)
    if p == "ncurses":
        term += " · mot de passe " + coche(P[p].get("mot_de_passe", {"verdict": "absent"}))
    lignes.append("| %s | %s | %s | %s | %s | %s | %s | %s | %s |" % (nom, xml, comp, durc, glade, term, ech, clic, exemples))
lignes += ["", "Légende : ✅ banc vert · ❌ banc rouge, figé ou qui n'a rien vérifié · — pas de banc pour ce port",
           "(ncurses est un terminal, html écrit une page) · ? journal présent, verdict non enregistré.", "", FIN]
bloc = "\n".join(lignes) + "\n"

if DEBUT in texte and FIN in texte:
    avant, reste = texte.split(DEBUT, 1)
    _, apres = reste.split(FIN, 1)
    nouveau = avant + bloc.rstrip("\n") + apres
else:
    m = re.match(r"(# [^\n]*\n)", texte)
    if not m:
        sys.exit("BILAN_SANTE.md sans titre de niveau 1 : repères à poser à la main")
    nouveau = m.group(1) + "\n" + bloc + texte[m.end():].lstrip("\n")
if nouveau != texte:
    open(md_path, "w", encoding="utf-8").write(nouveau)
    print("bilan-md : bloc écrit dans %s (%d ports)" % (md_path, sum(1 for p in PORTS if p in P)))
else:
    print("bilan-md : bloc déjà à jour")
PY
