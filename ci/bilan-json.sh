#!/bin/bash
# SPDX-License-Identifier: GPL-2.0-or-later
# ci/bilan-json.sh — un seul fichier de chiffres pour tout ce que les bancs ont mesuré.
#
# POURQUOI
#
# Le site (pages des ports, « Ce que les bancs vérifient ») et BILAN_SANTE.md
# (tableau « Par backend ») recopiaient à la main les chiffres des bancs : 55 cas
# XML, 78 de comportement, 8 garanties… Recopiés, ils vieillissent : le 2026-11-03,
# le bilan disait encore 61 cas et 176 bancs quand la CI en jouait 78 et 265.
# Ici, les chiffres sont LUS dans les journaux que ci/bancs.sh vient d'écrire,
# et posés dans un JSON que le site et le bilan reprennent tel quel. Un chiffre,
# une source.
#
# ENTRÉES (dossier des journaux, défaut _journaux/bancs/) :
#   verdicts.tsv            écrit par ci/bancs.sh : nom ⇥ verdict ⇥ rc ⇥ dernière ligne
#   <port>_<banc>.log       le journal complet de chaque banc d'un port
#   comportement_*.log      une ligne par port : « gtk3  ✅ 78 cas · 78 au vert · … »
# SORTIES :
#   <dossier>/../bilan.json et, sur la sortie standard, UNE ligne « BILAN-JSON: {…} »
#   que la trace publique du job de CI garde (les artefacts demandent un compte,
#   la trace non : le site se sert de cette ligne).
#
# Usage : ci/bilan-json.sh [dossier-des-journaux] [commit]
# Codes : 0 = JSON écrit · 1 = pas de journaux · 2 = outil manquant

set -u
J="${1:-$(cd "$(dirname "$0")/.." && pwd)/_journaux/bancs}"
COMMIT="${2:-$(git -C "$(dirname "$0")/.." rev-parse --short HEAD 2>/dev/null || echo inconnu)}"
command -v python3 >/dev/null || { echo "outil manquant : python3" >&2; exit 2; }
[ -d "$J" ] || { echo "pas de journaux : $J" >&2; exit 1; }
SORTIE="$(cd "$J/.." && pwd)/bilan.json"

python3 -I - "$J" "$COMMIT" "$SORTIE" <<'PY'
import json, os, re, sys, unicodedata, datetime

J, COMMIT, SORTIE = sys.argv[1], sys.argv[2], sys.argv[3]
PORTS = ["gtk3", "gtk4", "qt6", "fltk1", "efl1", "sdl3", "lvgl", "ncurses", "html"]
ANSI = re.compile(r"\x1b\[[0-9;]*[a-zA-Z]")


def journal(nom):
    """Le nom du journal, comme ci/bancs.sh le fabrique (iconv TRANSLIT puis tr -c 'A-Za-z0-9.-' '_')."""
    ascii_ = unicodedata.normalize("NFKD", nom).encode("ascii", "ignore").decode()
    return os.path.join(J, re.sub(r"[^A-Za-z0-9.-]", "_", ascii_) + ".log")


def lignes(chemin):
    try:
        with open(chemin, encoding="utf-8", errors="replace") as f:
            return [ANSI.sub("", l.rstrip("\n")) for l in f]
    except OSError:
        return []


# ── 1. Les verdicts de ci/bancs.sh ───────────────────────────────────────────
verdicts = {}          # nom → (verdict, rc, fin)
ordre = []
for l in lignes(os.path.join(J, "verdicts.tsv")):
    if not l.strip():
        continue
    champs = l.split("\t")
    while len(champs) < 4:
        champs.append("")
    verdicts[champs[0]] = (champs[1].strip(), champs[2], champs[3])
    ordre.append(champs[0])
joues = sum(1 for _, rc, _ in verdicts.values() if rc != "-")      # « binaire absent » : rc « - », pas joué
echecs = sum(1 for v, _, _ in verdicts.values() if v != "OK")
sans_verdicts = not verdicts


def verdict(nom):
    if nom in verdicts:
        return verdicts[nom][0]
    return "non enregistré" if os.path.exists(journal(nom)) else "absent"


def chiffre(nom, motif, groupes):
    """Les nombres du motif, lus dans la DERNIÈRE ligne du journal qui le porte."""
    for l in reversed(lignes(journal(nom))):
        m = re.search(motif, l)
        if m:
            return {g: int(m.group(i + 1)) for i, g in enumerate(groupes)}
    return {}


# ── 2. Comportement : un journal pour tous les ports, une ligne par port ──────
comportement = {}
for nom in ("comportement (tous les ports)", "comportement (ports construits)"):
    for l in lignes(journal(nom)):
        m = re.match(r"^(\w+)\s+\S+\s+(\d+) cas · (\d+) au vert · (\d+) en écart · (\d+) bloqué", l)
        if m and m.group(1) in PORTS:
            cas, verts, ecarts, bloques = (int(m.group(i)) for i in (2, 3, 4, 5))
            # Le banc est commun aux neuf ports : son verdict dit « au moins un port en
            # écart ». Pour CE port, c'est sa ligne qui fait foi (parité ou non) ; un
            # banc figé ou absent garde son verdict.
            v = verdict(nom)
            if v in ("OK", "ÉCHEC", "non enregistré"):
                v = "OK" if (ecarts == 0 and bloques == 0 and verts == cas) else "ÉCHEC"
            comportement[m.group(1)] = {"verdict": v, "cas": cas, "verts": verts, "ecarts": ecarts, "bloques": bloques}

# ── 3. Par port ──────────────────────────────────────────────────────────────
ports = {}
for p in PORTS:
    fiche = {}
    n = f"{p} xml"
    fiche["xml"] = {"verdict": verdict(n), **chiffre(n, r"(\d+) PASS \| (\d+) FAIL \| (\d+) SKIP", ("verts", "rouges", "sautes"))}
    fiche["comportement"] = comportement.get(p, {"verdict": "absent"})
    n = f"{p} durcissement"
    fiche["durcissement"] = {"verdict": verdict(n), **chiffre(n, r"OK : (\d+) garanties", ("garanties",))}
    n = f"{p} glade"
    etat = next((("chargé" if "chargé" in l else "refusé") for l in lignes(journal(n)) if "fichier chargé" in l or "option refusée" in l), None)
    fiche["glade"] = {"verdict": verdict(n), **({"etat": etat} if etat else {})}
    n = f"{p} terminal"
    etat = next((("VTE" if "VTE" in l else "non promis") for l in lignes(journal(n)) if "annonce VTE" in l or "non promis" in l), None)
    fiche["terminal"] = {"verdict": verdict(n), **({"etat": etat} if etat else {})}
    if p != "ncurses":
        n = f"{p} échappement"
        fiche["echappement"] = {"verdict": verdict(n), **chiffre(n, r"(\d+) cas, aucune exécution", ("cas",))}
    if p not in ("ncurses", "html"):
        n = f"{p} clic widgets"
        fiche["clic"] = {"verdict": verdict(n), **chiffre(n, r"(\d+) widgets, aucun plantage", ("widgets",))}
        n = f"{p} exemples réels"
        ex = {"verdict": verdict(n), **chiffre(n, r"(\d+) exemples : (\d+) OK · (\d+) CRASH · (\d+) SYNTAX · (\d+) sans fenêtre",
                                                ("joues", "ok", "crash", "syntax", "sans_fenetre"))}
        non_joues = [re.sub(r"\s+", " ", l.strip()) for l in lignes(journal(n)) if "(non joué" in l]
        reserves = sum(1 for l in lignes(journal(n)) if "réservé" in l)
        if non_joues:
            ex["non_joues"] = non_joues
        if reserves:
            ex["reserves"] = reserves
        fiche["exemples"] = ex
    if p == "ncurses":
        n = f"{p} mot de passe"
        fiche["mot_de_passe"] = {"verdict": verdict(n)}
    # Tout le reste des bancs de ce port : le verdict seul.
    pris = {f"{p} xml", f"{p} durcissement", f"{p} glade", f"{p} terminal", f"{p} échappement",
            f"{p} clic widgets", f"{p} exemples réels", f"{p} mot de passe"}
    autres = {nom[len(p) + 1:]: verdicts[nom][0] for nom in ordre if nom.startswith(p + " ") and nom not in pris}
    if autres:
        fiche["autres"] = autres
    ports[p] = fiche

bilan = {"commit": COMMIT, "date": datetime.date.today().isoformat(),
         "bancs": {"joues": joues, "echecs": echecs} if not sans_verdicts else {"note": "verdicts.tsv absent : journaux seuls"},
         "ports": ports}
with open(SORTIE, "w", encoding="utf-8") as f:
    json.dump(bilan, f, ensure_ascii=False, indent=1)
    f.write("\n")
print("BILAN-JSON: " + json.dumps(bilan, ensure_ascii=False, separators=(",", ":")))
PY
