#!/usr/bin/python3 -I
# SPDX-License-Identifier: GPL-2.0-or-later
# inventaire-scripts.py — écrit sermoman-mcp/data/scripts.md, l'inventaire des
# scripts métier (examples/metiers) et des boîtes à outils (examples/boites-a-outils),
# lu dans l'en-tête de chaque script (Métier ou Boîte, Besoin, Enrobe, Ports,
# Terminal). Rien n'est écrit à la main : l'inventaire suit les scripts.
#
# Usage : sermoman-mcp/outils/inventaire-scripts.py [--verifier]
#   --verifier : n'écrit rien ; code 1 si data/scripts.md n'est plus à jour.
import glob
import os
import re
import sys

ICI = os.path.dirname(os.path.dirname(os.path.abspath(__file__)))   # .../sermoman-mcp
RACINE = os.path.dirname(ICI)
CIBLE = os.path.join(ICI, "data", "scripts.md")
CHAMPS = ("Métier", "Boîte", "Besoin", "Enrobe", "Ports", "Terminal")

BOITES = {
    "systeme": "Système — régler son poste",
    "serveur": "Serveur — tenir un serveur, souvent par SSH",
    "administrateur": "Administrateur — administrer une machine Debian",
    "utilisateur": "Utilisateur — outils du quotidien, sans connaissances techniques",
}


def entete(chemin):
    """Les champs de l'en-tête ; une valeur peut continuer sur les lignes « #   … »."""
    champs, courant = {}, None
    for ligne in open(chemin, encoding="utf-8"):
        if not ligne.startswith("#"):
            break
        m = re.match(r"# (\S+)\s*:\s*(.*)$", ligne.rstrip("\n"))
        if m and m.group(1) in CHAMPS + ("Usage",):
            courant = m.group(1)
            champs[courant] = m.group(2).strip()
        elif courant and re.match(r"#\s{3,}\S", ligne) and courant != "Usage":
            champs[courant] += " " + ligne[1:].strip()
        else:
            courant = None
    return champs


def ligne(rel, c):
    return (f"- **`{rel}`** — {c.get('Besoin', '?')}\n"
            f"  - enrobe : {c.get('Enrobe', '?')}\n"
            f"  - ports : {c.get('Ports', '?')} ; terminal : {c.get('Terminal', '?')}\n")


def produire():
    out = ["# Scripts prêts à l'emploi : scripts métier et boîtes à outils\n",
           "Inventaire écrit par `sermoman-mcp/outils/inventaire-scripts.py` à partir de",
           "l'en-tête de chaque script : ne pas le modifier à la main.\n",
           "Ce sont des exemples complets (domaine public, CC0-1.0) ET le test de réussite",
           "des neuf ports : `examples/metiers/verifie.sh` joue chacun de leurs dialogues",
           "sur chaque port et compare les valeurs à gtk3sermo. Règles d'écriture :",
           "`examples/metiers/CONVENTION.md`. Installés par le paquet `sermo-doc` dans",
           "`/usr/share/doc/sermo-doc/examples/`. Leurs dialogues sont servis par",
           "sermoia-mcp (`sermo_examples`, sujets `metier-…` et `boite-…`).\n",
           "Lancer sans risque : `<script> gtk3sermo` (le 1er argument est le port). Par",
           "défaut rien n'est modifié : la commande est affichée (`à blanc : …`) ;",
           "`SERMO_A_BLANC=0` pour agir. Aucun script n'appelle `sudo` ni `pkexec`.\n"]
    n = 0
    out.append("## Scripts métier (examples/metiers)\n")
    for d in sorted(glob.glob(os.path.join(RACINE, "examples/metiers/[0-9][0-9]-*"))):
        scripts = sorted(glob.glob(os.path.join(d, "[0-9]-*.sh")))
        if not scripts:
            continue
        metier = entete(scripts[0]).get("Métier", os.path.basename(d))
        out.append(f"### {metier} (`{os.path.basename(d)}`)\n")
        for s in scripts:
            out.append(ligne(os.path.relpath(s, RACINE), entete(s))); n += 1
        out.append("")
    out.append("## Boîtes à outils (examples/boites-a-outils)\n")
    out.append("Chaque boîte : un `lanceur.sh` qui présente les dix outils, puis rouvre le menu.\n")
    for b, titre in BOITES.items():
        d = os.path.join(RACINE, "examples/boites-a-outils", b)
        scripts = sorted(glob.glob(os.path.join(d, "[0-9][0-9]-*.sh")))
        if not scripts:
            continue
        out.append(f"### {titre} (`{b}`)\n")
        lanceur = os.path.join(d, "lanceur.sh")
        if os.path.exists(lanceur):
            out.append(ligne(os.path.relpath(lanceur, RACINE), entete(lanceur))); n += 1
        for s in scripts:
            out.append(ligne(os.path.relpath(s, RACINE), entete(s))); n += 1
        out.append("")
    out.append(f"({n} scripts.)\n")
    return "\n".join(out)


texte = produire()
if "--verifier" in sys.argv[1:]:
    actuel = open(CIBLE, encoding="utf-8").read() if os.path.exists(CIBLE) else ""
    if actuel != texte:
        print("inventaire des scripts PÉRIMÉ (relancer sermoman-mcp/outils/inventaire-scripts.py)")
        sys.exit(1)
    print("inventaire des scripts : à jour")
    sys.exit(0)
open(CIBLE, "w", encoding="utf-8").write(texte)
print(f"écrit : {os.path.relpath(CIBLE, RACINE)}")
