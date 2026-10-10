#!/bin/bash
# SPDX-License-Identifier: GPL-2.0-or-later
# Copyright (C) 2026 S. Cage
# haplo-dialog <devel@haplo-dialog.fr>
#
# verifie.sh — sermoia-mcp dit-il vrai, et répond-il ?
#
# Trois contrôles, chacun rejoué sur le code et les binaires réels :
#   1. le schéma servi par sermo_schema nomme exactement les balises de widget
#      que l'analyseur du cœur reconnaît (libsermocore/src/gtkdialog_lexer.l) —
#      ni balise inventée, ni balise oubliée ;
#   2. chaque exemple du catalogue (examples/) s'analyse (--print-ir, code 0) ;
#   3. le serveur parle MCP : initialize annonce la version du fichier VERSION,
#      tools/list donne les six outils, sermo_check distingue un XML juste d'un
#      XML faux, sermo_examples et sermo_vote répondent, et sermo_render ne
#      laisse RIEN du backend sur la sortie standard (le canal MCP) — éprouvé
#      avec htmlsermo, qui ignore --render-png et écrit ses VAR="…".
#
# Usage : SERMO_BIN=<binaire sermo> tests/verifie.sh
#         (défaut : le premier port construit sous ../sermo-backend-*/_build)
# Codes : 0 tout concorde · 1 un écart · 77 binaire ou serveur absent, rien vérifié.

set -u
ICI=$(CDPATH= cd -- "$(dirname -- "$0")/.." && pwd)     # .../sermoia-mcp
RACINE=$(CDPATH= cd -- "$ICI/.." && pwd)
SERVEUR="$ICI/_build/sermoia-mcp"
T=$(mktemp -d); trap 'rm -rf "$T"' EXIT
ecarts=0
ko() { echo "ECHEC : $*"; ecarts=$((ecarts + 1)); }

BIN="${SERMO_BIN:-}"
if [ -z "$BIN" ]; then
    for p in ncurses html gtk3 qt6 sdl3 lvgl fltk1 efl1 gtk4; do
        c="$RACINE/sermo-backend-$p/_build/${p}sermo"
        [ -x "$c" ] && { BIN=$c; break; }
    done
fi
[ -n "$BIN" ] && [ -x "$BIN" ] || { echo "IGNORÉ : aucun binaire sermo (SERMO_BIN). Rien vérifié." >&2; exit 77; }
[ -x "$SERVEUR" ] || { echo "IGNORÉ : serveur non construit ($SERVEUR). Rien vérifié." >&2; exit 77; }

# ── 1. Schéma ↔ analyseur ──────────────────────────────────────────────────────
LEXER="$RACINE/libsermocore/src/gtkdialog_lexer.l"
# Jetons du lexer qui ne sont PAS des balises de widget (contenu, attributs,
# instructions de contrôle, sorties).
NON_WIDGETS=" action command default gvim height if input item label output radio sensitive show variable visible while width "
grep -aoE '"<[a-z]+' "$LEXER" | tr -d '"<' | sort -u > "$T/lexer"
[ -s "$T/lexer" ] || { echo "ECHEC : aucune balise lue dans $LEXER" >&2; exit 1; }
while read -r b; do
    case "$NON_WIDGETS" in *" $b "*) ;; *) echo "$b" ;; esac
done < "$T/lexer" > "$T/lexer-widgets"
python3 -I - "$ICI/src/sermoia-mcp.c" > "$T/schema" <<'EOF'
import json,re,sys
s=open(sys.argv[1],encoding='utf-8').read()
m=re.search(r'static const char \*SCHEMA_JSON =\n(.*?);\n',s,re.S)
txt=''.join(re.findall(r'"((?:[^"\\]|\\.)*)"',m.group(1)))
d=json.loads(txt.encode().decode('unicode_escape'))
for k in ('conteneurs','affichage','saisie','selection','boutons','menus','divers'):
    for b in d[k]: print(b)
EOF
sort -u "$T/schema" -o "$T/schema"
[ -s "$T/schema" ] || ko "schéma illisible dans src/sermoia-mcp.c"
inventees=$(comm -23 "$T/schema" "$T/lexer")
oubliees=$(comm -13 "$T/schema" "$T/lexer-widgets")
[ -z "$inventees" ] || ko "le schéma nomme des balises que l'analyseur ignore : $(echo $inventees)"
[ -z "$oubliees" ] || ko "le schéma oublie des balises de l'analyseur : $(echo $oubliees)"
echo "schéma : $(wc -l < "$T/schema") balises, toutes connues de l'analyseur ($(wc -l < "$T/lexer-widgets") balises de widget)"

# ── 2. Catalogue d'exemples ────────────────────────────────────────────────────
n=0; mauvais=0
for x in "$ICI"/examples/*/*.xml; do
    [ -f "$x" ] || continue
    n=$((n + 1))
    if ! env -u DISPLAY -u WAYLAND_DISPLAY timeout 10 "$BIN" --file="$x" --print-ir >/dev/null 2>&1; then
        ko "exemple refusé par l'analyseur : ${x#$ICI/}"; mauvais=$((mauvais + 1))
    fi
done
[ "$n" -gt 0 ] || ko "catalogue vide ($ICI/examples)"
echo "exemples : $n analysés, $mauvais refusé(s) ($(basename "$BIN"))"

# ── 3. Protocole MCP ───────────────────────────────────────────────────────────
python3 -I - > "$T/req.jsonl" <<'EOF'
import json
ok='<window><vbox><entry><variable>E</variable></entry></vbox></window>'
faux='<window><vbox><entry><variable>E</variable></vbox></window>'
m=[{"jsonrpc":"2.0","id":1,"method":"initialize","params":{"protocolVersion":"2024-11-05","capabilities":{},"clientInfo":{"name":"verifie","version":"0"}}},
   {"jsonrpc":"2.0","method":"notifications/initialized"},
   {"jsonrpc":"2.0","id":2,"method":"tools/list"},
   {"jsonrpc":"2.0","id":3,"method":"tools/call","params":{"name":"sermo_check","arguments":{"xml":ok}}},
   {"jsonrpc":"2.0","id":4,"method":"tools/call","params":{"name":"sermo_check","arguments":{"xml":faux}}},
   {"jsonrpc":"2.0","id":5,"method":"tools/call","params":{"name":"sermo_examples","arguments":{"task":"calendar"}}},
   {"jsonrpc":"2.0","id":6,"method":"tools/call","params":{"name":"sermo_vote","arguments":{"example":"calendar/calendar.xml","helpful":True}}},
   {"jsonrpc":"2.0","id":7,"method":"tools/call","params":{"name":"sermo_render","arguments":{"xml":ok}}}]
for x in m: print(json.dumps(x))
EOF
RBIN="$RACINE/sermo-backend-html/_build/htmlsermo"; [ -x "$RBIN" ] || RBIN="$BIN"
SERMO_BIN="$BIN" SERMO_RENDER_BIN="$RBIN" SERMO_EXAMPLES_DIR="$ICI/examples" SERMO_VOTE_FILE="$T/votes.tsv" \
    env -u DISPLAY -u WAYLAND_DISPLAY timeout 60 "$SERVEUR" < "$T/req.jsonl" > "$T/rep.jsonl" 2>/dev/null
ATTENDUE=$(tr -d '[:space:]' < "$RACINE/VERSION")
verdict=$(python3 -I - "$T/rep.jsonl" "$ATTENDUE" <<'EOF'
import json,sys
r={}; err=[]
for l in open(sys.argv[1]):
    try: d=json.loads(l)
    except ValueError:
        err.append(f"ligne non JSON sur le canal MCP : {l.strip()[:60]!r}"); continue
    if 'id' in d: r[d['id']]=d.get('result',{})
v=r.get(1,{}).get('serverInfo',{}).get('version')
if v!=sys.argv[2]: err.append(f"initialize annonce la version {v!r}, VERSION dit {sys.argv[2]!r}")
outils=sorted(t['name'] for t in r.get(2,{}).get('tools',[]))
if outils!=sorted(['sermo_schema','sermo_check','sermo_run','sermo_render','sermo_examples','sermo_vote']): err.append(f"tools/list : {outils}")
if r.get(3,{}).get('isError'): err.append("sermo_check refuse un XML juste")
if not r.get(4,{}).get('isError'): err.append("sermo_check accepte un XML faux")
t5=r.get(5,{}).get('content',[{}])[0].get('text','')
if 'calendar' not in t5: err.append("sermo_examples ne sert pas le sujet calendar")
if r.get(6,{}).get('isError'): err.append("sermo_vote a échoué")
if 7 not in r: err.append("sermo_render n'a pas répondu")
print('\n'.join(err) if err else 'OK')
EOF
)
if [ "$verdict" = OK ]; then
    echo "protocole : initialize ($ATTENDUE), 6 outils, check juste/faux, examples, vote, render ($(basename "$RBIN"))"
else
    while IFS= read -r l; do ko "$l"; done <<< "$verdict"
fi
grep -q $'^calendar\t1' "$T/votes.tsv" 2>/dev/null || ko "le vote n'a pas été écrit dans SERMO_VOTE_FILE"

[ "$ecarts" -eq 0 ] && { echo "verifie : OK"; exit 0; }
echo "verifie : $ecarts écart(s)"; exit 1
