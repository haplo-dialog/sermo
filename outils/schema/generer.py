#!/usr/bin/env python3
# SPDX-License-Identifier: GPL-2.0-or-later
# outils/schema/generer.py — sermo.schema.json, le langage XML de sermo lu DANS LE CODE.
#
# POURQUOI
#
# Un éditeur ou un assistant IA a besoin de la liste des balises, de ce que chaque
# widget accepte, des actions et des signaux. Écrite à la main, cette liste diverge
# (sermoia-mcp en servait une, tapée dans le C, que son banc ne comparait qu'aux
# balises). Ici, TOUT est extrait des sources, et un banc (tests/garde_schema.sh)
# refuse un sermo.schema.json qui ne serait plus celui que ce script produit.
#
# D'OÙ VIENT CHAQUE CHOSE
#   balises                 libsermocore/src/gtkdialog_lexer.l   (motifs \<nom\>, \<nom[ ]+)
#   alias                   idem (motif dont le jeton rendu est une autre balise)
#   balises de contenu      libsermocore/src/gtkdialog_parser.y  (règle « attr »)
#   conteneurs              idem (règle « widget » : jeton suivi de « wlist »)
#   boutons de stock        lexer (\<button ok\> …)
#   sources et cibles       lexer (<input modbus …>, <action mqtt …>)
#   préfixes d'action       libsermocore/src/stringman.c (tableau prefixes[])
#   signaux                 libsermocore/src/signals.c (widget_signal_executor(…, "nom"))
#   attributs par widget    sermo-backend-<port>/src/widget_<nom>.c* (get_tag_attribute(…, "nom")),
#                           port par port ; libsermocore/src/*.c pour les attributs communs
#   attributs libres        sermo-backend-gtk3|gtk4/src/tag_set_property.c : toute propriété
#                           GObject du widget GTK est acceptée telle quelle
#   familles                la seule part éditoriale (FAMILLES ci-dessous) ; un widget
#                           sans famille fait ÉCHOUER la génération, rien ne se perd en silence
#
# CE QUE CE SCHÉMA N'EST PAS : un XSD. Le dialecte hérité de gtkdialog n'est pas du
# XML strict (mesuré le 2026-11-04 : 266 des 294 dialogues du dépôt, <button ok> et
# <input file> en tête, ne passent pas un analyseur XML). Le validateur de référence
# reste l'analyseur du cœur : sermoia-mcp « sermo_check », ou « <port>sermo --print-ir ».
#
# Usage : outils/schema/generer.py [racine-du-dépôt] [--sortie fichier | -]
import json
import os
import re
import subprocess
import sys

RACINE = os.path.abspath(sys.argv[1]) if len(sys.argv) > 1 and not sys.argv[1].startswith("-") else \
    os.path.abspath(os.path.join(os.path.dirname(__file__), "..", ".."))
SORTIE = os.path.join(RACINE, "sermo.schema.json")
if "--sortie" in sys.argv:
    SORTIE = sys.argv[sys.argv.index("--sortie") + 1]
PORTS = ["gtk3", "gtk4", "qt6", "fltk1", "efl1", "sdl3", "lvgl", "ncurses", "html"]

# Jetons du lexer qui ne sont pas des balises de widget : contenu, contrôle, hérités.
# (La même liste que sermoia-mcp/tests/verifie.sh.)
NON_WIDGETS = {"action", "command", "default", "gvim", "height", "if", "input", "item", "label",
               "output", "radio", "sensitive", "show", "variable", "visible", "while", "width"}
CONTROLE = ["if", "while", "command", "show"]
HERITES = ["gvim", "radio"]          # reconnus par l'analyseur, hérités de gtkdialog, non documentés

# La seule part écrite à la main : à quelle famille appartient chaque widget.
FAMILLES = {
    "conteneurs": ["window", "vbox", "hbox", "frame", "notebook", "expander", "eventbox", "aspectframe",
                   "grid", "paned", "stack", "wizard", "toolbar", "flowbox", "overlay", "revealer"],
    "affichage": ["text", "image", "pixmap", "statusbar", "infobar", "drawingarea", "progressbar", "pulse",
                  "spinner", "levelbar"],
    "saisie": ["entry", "edit", "password", "searchentry", "spinbutton", "hscale", "vscale", "checkbox",
               "togglebutton", "radiobutton", "switch"],
    "selection": ["combobox", "comboboxtext", "comboboxentry", "chooser", "list", "tree", "table",
                  "colorbutton", "fontbutton", "filechooser", "calendar"],
    "boutons": ["button", "linkbutton", "menubutton"],
    "menus": ["menubar", "menu", "menuitem", "menuitemseparator"],
    "divers": ["terminal", "hseparator", "vseparator", "timer"],
}


def lire(chemin):
    with open(os.path.join(RACINE, chemin), encoding="utf-8", errors="replace") as f:
        return f.read()


# ── Lexer : balises, balises à attributs, alias, boutons de stock, sources ────
lexer = lire("libsermocore/src/gtkdialog_lexer.l")
balises = sorted(set(re.findall(r"^\\<([a-z_0-9]+)", lexer, re.M)))
a_attributs = set(re.findall(r"^\\<([a-z_0-9]+)\[ \]\+", lexer, re.M))
alias = {}
for m in re.finditer(r'^\\<([a-z_0-9]+)\\>\s*\{[^\n]*Token="<([a-z_0-9]+)>"', lexer, re.M):
    if m.group(1) != m.group(2):
        alias[m.group(1)] = m.group(2)
stock = re.findall(r"^\\<button\\ ([a-z]+)\\>", lexer, re.M)
sources = {}
for genre, nom in re.findall(r"^\\<(input|action|output)\\?[ \[\]+]*([a-z]+)", lexer, re.M):
    sources.setdefault(genre, [])
    if nom not in sources[genre]:
        sources[genre].append(nom)
widgets = [b for b in balises if b not in NON_WIDGETS and b not in alias]

# ── Grammaire : contenu, conteneurs ──────────────────────────────────────────
grammaire = lire("libsermocore/src/gtkdialog_parser.y")
m = re.search(r"^attr\n(.*?)\n\n", grammaire, re.M | re.S)
contenu = []
for alt in re.findall(r"^\s*\|\s*attr\s+([a-z]+)", m.group(1), re.M):
    nom = {"defaultvalue": "default"}.get(alt, alt)
    if nom not in contenu:
        contenu.append(nom)
# Un conteneur : un jeton suivi d'une liste de widgets (« wlist ») puis de son contenu
# (« attr ») — règles « window » et « wlist » de la grammaire.
m = re.search(r"^window\n(.*?)^widget\n", grammaire, re.M | re.S)
conteneurs = set()
for jeton in re.findall(r"(?:PART_)?([A-Z_]+)(?:\s+tagattr\s+'>')?\s+wlist\s+attr", m.group(1)):
    if jeton.lower() in widgets:
        conteneurs.add(jeton.lower())

# ── Actions, signaux ─────────────────────────────────────────────────────────
stringman = lire("libsermocore/src/stringman.c")
m = re.search(r"prefixes\[\]\s*=\s*\{(.*?)\};", stringman, re.S)
prefixes = re.findall(r'"([a-z]+)"', m.group(1))
signaux = sorted(set(re.findall(r'widget_signal_executor\([^;]*?"([a-z-]+)"', lire("libsermocore/src/signals.c"))))

# ── Attributs : par widget et par port, communs, libres ──────────────────────
RX_ATTR = re.compile(r'get_tag_attribute\s*\(\s*[^,]+,\s*"([a-zA-Z_-]+)"')
attributs = {w: {} for w in widgets}
hors_widget = {}
for p in PORTS:
    d = os.path.join(RACINE, "sermo-backend-%s" % p, "src")
    for f in sorted(os.listdir(d)):
        if not re.search(r"\.(c|cc|cpp)$", f):
            continue
        noms = sorted(set(RX_ATTR.findall(lire(os.path.join("sermo-backend-%s" % p, "src", f)))))
        if not noms:
            continue
        mw = re.match(r"widget_([a-z0-9_]+)\.", f)
        w = mw.group(1) if mw else None
        if w in attributs:
            for n in noms:
                attributs[w].setdefault(n, []).append(p)
        else:
            for n in noms:
                hors_widget.setdefault(n, []).append(p)
communs = sorted(set(n for f in os.listdir(os.path.join(RACINE, "libsermocore", "src")) if f.endswith(".c")
                     for n in RX_ATTR.findall(lire(os.path.join("libsermocore", "src", f)))))
libres = {p: "toute propriété GObject du widget GTK (tag_set_property.c)" for p in ("gtk3", "gtk4")
          if os.path.exists(os.path.join(RACINE, "sermo-backend-%s" % p, "src", "tag_set_property.c"))}

# ── Familles : chaque widget en a une, sinon on s'arrête ─────────────────────
famille_de = {w: f for f, ws in FAMILLES.items() for w in ws}
sans = [w for w in widgets if w not in famille_de]
en_trop = [w for w in famille_de if w not in widgets]
if sans or en_trop:
    sys.exit("FAMILLES à mettre à jour — widgets du lexer sans famille : %s ; dans FAMILLES mais plus dans le lexer : %s"
             % (sans or "aucun", en_trop or "aucun"))

# ── Version, commit ──────────────────────────────────────────────────────────
version = lire("VERSION").strip() if os.path.exists(os.path.join(RACINE, "VERSION")) else "?"
try:
    commit = subprocess.run(["git", "-C", RACINE, "rev-parse", "--short", "HEAD"], capture_output=True, text=True, check=True).stdout.strip()
except Exception:
    commit = "?"

schema = {
    "_": "Généré par outils/schema/generer.py depuis le code de sermo (lexer, grammaire, ports). Ne pas éditer : régénérer. tests/garde_schema.sh refuse un fichier qui diverge.",
    "langage": "sermo-xml",
    "version": version,
    "commit": commit,
    "xml_strict": False,
    "note_xml": "Dialecte hérité de gtkdialog : <button ok>, <input file>, <action mqtt …> portent des attributs nus ; ce n'est pas du XML strict. Le validateur de référence est l'analyseur du cœur : sermoia-mcp sermo_check, ou <port>sermo --print-ir.",
    "racine": "window",
    "ports": PORTS,
    "familles": {f: [w for w in ws] for f, ws in FAMILLES.items()},
    "widgets": {
        w: {
            "famille": famille_de[w],
            "conteneur": w in conteneurs,
            "attributs_de_balise": w in a_attributs,
            "alias": sorted(a for a, b in alias.items() if b == w),
            "attributs_lus": {n: ps for n, ps in sorted(attributs[w].items())},
        } for w in widgets
    },
    "boutons_stock": ["button %s" % s for s in stock],
    "contenu": contenu,
    "contenu_alias": {a: b for a, b in alias.items() if b in contenu},
    "sources": sources,
    "actions": {
        "forme": "<action [signal=\"…\"] [type=\"…\"]>PRÉFIXE:argument</action> ; sans préfixe, une commande shell (« command »).",
        "prefixes": prefixes,
    },
    "signaux": signaux,
    "attributs_communs": communs,
    "attributs_hors_widget": {n: sorted(set(ps)) for n, ps in sorted(hors_widget.items())},
    "attributs_libres": libres,
    "controle": CONTROLE,
    "herites": HERITES,
}
texte = json.dumps(schema, ensure_ascii=False, indent=1) + "\n"
if SORTIE == "-":
    sys.stdout.write(texte)
else:
    with open(SORTIE, "w", encoding="utf-8") as f:
        f.write(texte)
    print("%s : %d widgets (%d conteneurs), %d balises de contenu, %d préfixes d'action, %d signaux, %d attributs lus dans les ports"
          % (os.path.relpath(SORTIE, RACINE) if SORTIE.startswith(RACINE) else SORTIE, len(widgets), len(conteneurs),
             len(contenu), len(prefixes), len(signaux), sum(len(v) for v in attributs.values())))
