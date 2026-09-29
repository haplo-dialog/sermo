#!/usr/bin/env python3
# SPDX-License-Identifier: GPL-2.0-or-later
# sermoman-mcp — serveur MCP LOCAL (stdio) exposant la documentation publique
# de sermo à un assistant IA.
#
# sermo est une boîte à dialogues pilotée par XML (lignée gtkdialog) : un cœur
# unique « libsermocore » et sept backends de rendu (gtk3, gtk4, qt6, fltk1,
# efl1, sdl3, ncurses). Ce serveur donne à l'assistant une connaissance précise
# de la syntaxe XML, de l'architecture, de l'histoire et de l'écosystème du
# projet, pour qu'il aide à écrire des dialogues corrects sans inventer de balise.
#
# Principes :
#   - transport stdio uniquement : aucun port, aucun accès réseau (entrant ni
#     sortant) ;
#   - LECTURE SEULE : n'exécute rien, n'écrit rien, ne publie rien ;
#   - contenu 100 % embarqué, dérivé de la documentation publique du projet.
# Sans dépendance : uniquement la bibliothèque standard Python.

import json
import os
import sys

HERE = os.path.dirname(os.path.abspath(__file__))
DATA = os.path.join(HERE, "data")


def _version():
    """Version annoncée aux clients MCP, lue dans le fichier VERSION du dépôt.

    Elle était écrite en dur (« 1.0.0 ») et n'a jamais bougé : un client qui
    interrogeait ce serveur n'apprenait rien de la version qu'il avait en face.
    VERSION est la seule source du dépôt — voir VERSIONING.md.
    """
    try:
        with open(os.path.join(HERE, os.pardir, "VERSION"), encoding="utf-8") as f:
            return f.read().strip() or "inconnue"
    except OSError:
        return "inconnue"


VERSION = _version()
EXAMPLES_DIR = os.path.join(DATA, "examples")


def _load(name):
    try:
        with open(os.path.join(DATA, name), encoding="utf-8") as f:
            return f.read()
    except OSError:
        return ""


# Corpus documentaire embarqué. Chaque entrée est servie telle quelle par son
# outil dédié et balayée par la recherche plein texte.
REFERENCE = _load("reference-xml.txt")
GUIDE = _load("guide.md")
ARCHITECTURE = _load("architecture.md")
HERITAGE = _load("heritage.md")
ECOSYSTEM = _load("ecosystem.md")

# Corpus indexé pour la recherche : (étiquette, texte).
CORPUS = [
    ("référence", REFERENCE),
    ("guide", GUIDE),
    ("architecture", ARCHITECTURE),
    ("héritage", HERITAGE),
    ("écosystème", ECOSYSTEM),
]

# Exemple canonique servi par défaut ; les autres exemples curés vivent à côté.
EXAMPLE = _load(os.path.join("examples", "formulaire.xml"))


def _example_index():
    """Liste les exemples curés disponibles dans data/examples (nom → chemin).

    Un exemple est soit un fichier .xml/.sh direct, soit un sous-dossier
    (exemple multi-fichiers) dont on retient le point d'entrée."""
    index = {}
    try:
        entries = sorted(os.listdir(EXAMPLES_DIR))
    except OSError:
        return index
    for entry in entries:
        path = os.path.join(EXAMPLES_DIR, entry)
        if os.path.isfile(path):
            index[os.path.splitext(entry)[0]] = path
        elif os.path.isdir(path):
            index[entry] = path
    return index


def _example_entry_script(dir_path, name):
    """Point d'entrée d'un exemple multi-fichiers : fichier homonyme, puis
    « main », puis le premier fichier contenant <window>, sinon le premier."""
    for cand in (name, os.path.splitext(name)[0], "main"):
        p = os.path.join(dir_path, cand)
        if os.path.isfile(p):
            return p
    files = sorted(f for f in os.listdir(dir_path)
                   if os.path.isfile(os.path.join(dir_path, f)))
    # Un seul script de lancement : c'est lui qu'on montre. Sans cette règle,
    # l'exemple glade présentait son fichier de fonctions (1er par ordre
    # alphabétique), puis son interface GTK 4.
    scripts = [f for f in files if f.endswith(".sh")]
    if len(scripts) == 1:
        return os.path.join(dir_path, scripts[0])
    for f in files:
        p = os.path.join(dir_path, f)
        try:
            with open(p, encoding="utf-8", errors="replace") as fh:
                if "<window" in fh.read():
                    return p
        except OSError:
            continue
    return os.path.join(dir_path, files[0]) if files else None


def _get_example(name):
    index = _example_index()
    if not name:
        listing = ", ".join(sorted(index)) or "(aucun)"
        header = ("Exemple canonique (formulaire de saisie). Autres exemples "
                  "disponibles via `sermo_example` avec un nom :\n  %s\n\n"
                  "----------\n\n" % listing)
        return header + (EXAMPLE or "(exemple indisponible)")
    key = os.path.splitext(name.strip())[0]
    path = index.get(key) or index.get(name.strip())
    if not path:
        return ("Exemple « %s » introuvable. Disponibles : %s"
                % (name, ", ".join(sorted(index)) or "(aucun)"))
    if os.path.isdir(path):
        entry = _example_entry_script(path, key)
        others = sorted(f for f in os.listdir(path)
                        if entry and os.path.join(path, f) != entry)
        note = ("Exemple multi-fichiers « %s ». Point d'entrée ci-dessous ; "
                "fichiers annexes : %s\n\n----------\n\n"
                % (key, ", ".join(others) or "aucun"))
        try:
            with open(entry, encoding="utf-8", errors="replace") as fh:
                return note + fh.read()
        except OSError:
            return "Exemple « %s » illisible." % name
    try:
        with open(path, encoding="utf-8", errors="replace") as fh:
            return fh.read()
    except OSError:
        return "Exemple « %s » illisible." % name


REPORT = (
    "Pour signaler un bug ou proposer une amélioration de sermo, ouvrez une "
    "« issue » sur le dépôt GitLab du projet :\n"
    "  https://gitlab.com/haplo-dialog/sermo/-/issues\n"
    "Le suivi des tickets demande un compte GitLab. Sans compte, le même "
    "rapport est le bienvenu par courriel à devel@haplo-dialog.fr.\n"
    "Décrivez :\n"
    "  1. la version (sortie de `sermo --version`, ou celle du backend : "
    "`gtk3sermo --version`, `qt6sermo --version`, etc.) ;\n"
    "  2. le backend utilisé (gtk3, gtk4, qt6, fltk1, efl1, sdl3 ou ncurses) ;\n"
    "  3. le script XML minimal qui reproduit le problème ;\n"
    "  4. le comportement attendu et le comportement observé.\n"
    "Ce serveur ne publie rien lui-même : il vous aide seulement à préparer un "
    "signalement clair."
)

TOOLS = [
    {
        "name": "sermo_reference",
        "description": "Référence complète de la syntaxe XML de sermo (widgets, "
                       "attributs communs, sous-éléments), commune aux sept "
                       "backends.",
        "inputSchema": {"type": "object", "properties": {}},
    },
    {
        "name": "sermo_guide",
        "description": "Guide de prise en main : le cœur et les backends, "
                       "installation, compatibilité gtkdialog, sécurité, licences.",
        "inputSchema": {"type": "object", "properties": {}},
    },
    {
        "name": "sermo_architecture",
        "description": "Architecture modulaire : le cœur libsermocore, les sept "
                       "backends, le contrat cœur↔backend (sermo_be_*), les "
                       "variantes de build, le packagage et les bancs de test.",
        "inputSchema": {"type": "object", "properties": {}},
    },
    {
        "name": "sermo_heritage",
        "description": "La vie de sermo : lignée gtkdialog → haplo-dialog → "
                       "sermo, historique des versions, feuille de route et "
                       "bilan de santé.",
        "inputSchema": {"type": "object", "properties": {}},
    },
    {
        "name": "sermo_ecosystem",
        "description": "Les outils autour de sermo (sermoman-mcp, le binaire "
                       "et ses options de vérification) : quel outil pour quoi.",
        "inputSchema": {"type": "object", "properties": {}},
    },
    {
        "name": "sermo_search",
        "description": "Recherche plein texte dans toute la documentation "
                       "embarquée (référence, guide, architecture, héritage, "
                       "écosystème). Renvoie les passages pertinents avec leur "
                       "contexte et leur source.",
        "inputSchema": {
            "type": "object",
            "properties": {"query": {"type": "string",
                                     "description": "terme ou widget à rechercher"}},
            "required": ["query"],
        },
    },
    {
        "name": "sermo_example",
        "description": "Un exemple complet et fonctionnel de dialogue. Sans "
                       "argument : l'exemple canonique (formulaire) et la liste "
                       "des exemples curés disponibles. Avec « name » : le "
                       "contenu de cet exemple.",
        "inputSchema": {
            "type": "object",
            "properties": {"name": {"type": "string",
                                    "description": "nom d'un exemple curé "
                                                   "(optionnel)"}},
        },
    },
    {
        "name": "sermo_how_to_report",
        "description": "Comment signaler un bug ou proposer une amélioration "
                       "(via les issues GitLab du projet).",
        "inputSchema": {"type": "object", "properties": {}},
    },
]


def _search(query):
    q = query.lower().strip()
    if not q:
        return "Indiquez un terme à rechercher."
    hits = []
    for label, text in CORPUS:
        if not text:
            continue
        lines = text.splitlines()
        for i, line in enumerate(lines):
            if q in line.lower():
                ctx = "\n".join(lines[max(0, i - 2):i + 3])
                hits.append("[%s]\n%s" % (label, ctx))
                if len(hits) >= 15:
                    break
        if len(hits) >= 15:
            break
    return "\n\n---\n\n".join(hits) if hits else "Aucun résultat pour « %s »." % query


def _call_tool(name, args):
    if name == "sermo_reference":
        return REFERENCE or "(référence indisponible)"
    if name == "sermo_guide":
        return GUIDE or "(guide indisponible)"
    if name == "sermo_architecture":
        return ARCHITECTURE or "(architecture indisponible)"
    if name == "sermo_heritage":
        return HERITAGE or "(héritage indisponible)"
    if name == "sermo_ecosystem":
        return ECOSYSTEM or "(écosystème indisponible)"
    if name == "sermo_example":
        return _get_example(str(args.get("name", "")))
    if name == "sermo_how_to_report":
        return REPORT
    if name == "sermo_search":
        return _search(str(args.get("query", "")))
    raise ValueError("outil inconnu : %s" % name)


def _send(obj):
    sys.stdout.write(json.dumps(obj, ensure_ascii=False) + "\n")
    sys.stdout.flush()


def _respond(rid, result):
    _send({"jsonrpc": "2.0", "id": rid, "result": result})


def _error(rid, code, message):
    _send({"jsonrpc": "2.0", "id": rid, "error": {"code": code, "message": message}})


def main():
    for line in sys.stdin:
        line = line.strip()
        if not line:
            continue
        try:
            req = json.loads(line)
        except json.JSONDecodeError:
            continue
        method = req.get("method")
        rid = req.get("id")
        if method == "initialize":
            _respond(rid, {
                "protocolVersion": "2024-11-05",
                "capabilities": {"tools": {}},
                "serverInfo": {"name": "sermoman-mcp", "version": VERSION},
            })
        elif method == "notifications/initialized":
            pass  # notification : aucune réponse attendue
        elif method == "tools/list":
            _respond(rid, {"tools": TOOLS})
        elif method == "tools/call":
            params = req.get("params", {})
            try:
                text = _call_tool(params.get("name"), params.get("arguments") or {})
                _respond(rid, {"content": [{"type": "text", "text": text}]})
            except Exception as exc:  # noqa: BLE001 — renvoyé proprement au client
                _error(rid, -32602, str(exc))
        elif rid is not None:
            _error(rid, -32601, "méthode inconnue : %s" % method)


if __name__ == "__main__":
    main()
