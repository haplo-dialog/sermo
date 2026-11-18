#!/bin/bash
# SPDX-License-Identifier: GPL-2.0-or-later
# sermoman-mcp/tests/verifie-protocole.sh — haplo-dialog — 2026
#
# Le serveur survit à ce qu'un client peut lui envoyer de pire, et répond selon
# JSON-RPC : JSON qui n'est pas un objet (-32600), ligne hors UTF-8 ou JSON
# cassé (-32700), « params » qui n'est pas un objet (-32602), méthode inconnue
# (-32601), ping ({}), notification (aucune réponse). Avant la 3.3.0, un tableau
# JSON ou un octet 0xFF le faisaient mourir sur une exception.
# Une ligne de plus de 8 Mio est refusée sans être gardée en mémoire.
#
# Codes : 0 = tout tient · 1 = un défaut · 77 = python3 absent
set -u
ICI=$(CDPATH= cd -- "$(dirname -- "$0")" && pwd)
command -v python3 >/dev/null || { echo "IGNORÉ : python3 manquant" >&2; exit 77; }
T=$(mktemp -d) || exit 77; trap 'rm -rf "$T"' EXIT
printf '%b' '[1,2]\n"texte"\n\xff\xfe\n{cassé\n{"jsonrpc":"2.0","id":1,"method":"tools/call","params":[1]}\n{"jsonrpc":"2.0","method":"notifications/initialized"}\n{"jsonrpc":"2.0","id":2,"method":"inconnue"}\n{"jsonrpc":"2.0","id":3,"method":"ping"}\n{"jsonrpc":"2.0","id":4,"method":"tools/list"}\n' \
    | timeout 30 python3 -I "$ICI/../server.py" >"$T/sortie" 2>"$T/erreurs"
rc=$?
python3 -I - "$T/sortie" "$rc" <<'PY' || { echo "ECHEC : sermoman-mcp ne tient pas le protocole"; sed 's/^/  /' "$T/erreurs" | tail -5; exit 1; }
import json, sys
l = [json.loads(x) for x in open(sys.argv[1], encoding="utf-8")]
codes = [x.get("error", {}).get("code") for x in l]
attendu = [-32600, -32600, -32700, -32700, -32602, -32601, None, None]
ok = (sys.argv[2] == "0" and codes == attendu and l[6] == {"jsonrpc": "2.0", "id": 3, "result": {}}
      and l[7]["id"] == 4 and len(l[7]["result"]["tools"]) > 0)
if not ok:
    print("  code de sortie", sys.argv[2], "· codes", codes, "· attendu", attendu)
sys.exit(0 if ok else 1)
PY

# Une ligne sans fin : 300 Mo sans saut de ligne, sous 256 Mo de mémoire. Avant
# la 3.3.0 le serveur l'accumulait et mourait (MemoryError) ; il la refuse
# (-32600) sans la garder, et répond à la requête qui suit.
{ head -c 300000000 /dev/zero | tr '\0' a; printf '\n{"jsonrpc":"2.0","id":7,"method":"ping"}\n'; } \
    | ( ulimit -v 262144; timeout 60 python3 -I "$ICI/../server.py" ) >"$T/longue" 2>"$T/erreurs"
rc=$?
python3 -I - "$T/longue" "$rc" <<'PY' || { echo "ECHEC : sermoman-mcp ne borne pas la taille d'une ligne"; sed 's/^/  /' "$T/erreurs" | tail -3; exit 1; }
import json, sys
l = [json.loads(x) for x in open(sys.argv[1], encoding="utf-8")]
ok = (sys.argv[2] == "0" and len(l) == 2 and l[0].get("error", {}).get("code") == -32600
      and l[1] == {"jsonrpc": "2.0", "id": 7, "result": {}})
if not ok:
    print("  code de sortie", sys.argv[2], "·", len(l), "réponse(s)")
sys.exit(0 if ok else 1)
PY
echo "OK : sermoman-mcp survit aux requêtes invalides (8 réponses justes, une notification sans réponse) et refuse une ligne de 300 Mo sans la garder"
