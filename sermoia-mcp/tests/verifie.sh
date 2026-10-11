#!/bin/bash
# SPDX-License-Identifier: GPL-2.0-or-later
# Copyright (C) 2026 S. Cage
# haplo-dialog <devel@haplo-dialog.fr>
#
# verifie.sh — sermoia-mcp dit-il vrai, et répond-il ?
#
# Cinq contrôles, chacun rejoué sur le code et les binaires réels :
#   1. le schéma servi par sermo_schema nomme exactement les balises de widget
#      que l'analyseur du cœur reconnaît (libsermocore/src/gtkdialog_lexer.l) —
#      ni balise inventée, ni balise oubliée ;
#   2. chaque exemple du catalogue (examples/) s'analyse (--print-ir, code 0) ;
#   3. le serveur parle MCP : initialize (version du fichier VERSION, version
#      du protocole du client reprise, instructions), six outils annotés,
#      sermo_check juste/faux sans bruit GTK, sermo_examples par mot et par
#      phrase, sermo_vote (sujet hors catalogue refusé, note assainie), ping,
#      codes d'erreur JSON-RPC ; sermo_render ne laisse RIEN du backend sur la
#      sortie standard (htmlsermo écrit ses VAR="…") et n'EXÉCUTE aucun <input> ;
#   4. sermo_render rend une image PNG avec un port à fenêtre (petit dialogue),
#      toujours sans exécuter les commandes du XML ;
#   5. un dialogue jamais fermé est coupé au délai de sermo_run, sans survivant,
#      et le serveur répond encore après.
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

# ── 3. Protocole MCP et outils ────────────────────────────────────────────────
# Un faux binaire qui ne rend jamais la main (délais). Posé sous _build, jamais
# sous /tmp : un banc n'exécute rien depuis /tmp.
ESSAIS="$ICI/_build/essais"; mkdir -p "$ESSAIS"
printf '#!/bin/sh\nsleep 120 &\nsleep 120\n' > "$ESSAIS/bloque.sh"; chmod +x "$ESSAIS/bloque.sh"
TEMOIN="$T/temoin-rendu"
python3 -I - "$TEMOIN" > "$T/req.jsonl" <<'PY'
import json,sys
ok='<window><vbox><entry><variable>E</variable></entry></vbox></window>'
faux='<window><vbox><entry><variable>E</variable></vbox></window>'
piege='<window><vbox><text><input>touch %s</input></text><text><label>x</label></text></vbox></window>' % sys.argv[1]
def appel(i,nom,args): return {"jsonrpc":"2.0","id":i,"method":"tools/call","params":{"name":nom,"arguments":args}}
m=[{"jsonrpc":"2.0","id":1,"method":"initialize","params":{"protocolVersion":"2025-06-18","capabilities":{},"clientInfo":{"name":"verifie","version":"0"}}},
   {"jsonrpc":"2.0","method":"notifications/initialized"},
   {"jsonrpc":"2.0","id":2,"method":"tools/list"},
   appel(3,"sermo_check",{"xml":ok}),
   appel(4,"sermo_check",{"xml":faux}),
   appel(5,"sermo_examples",{"task":"calendar"}),
   appel(6,"sermo_vote",{"example":"calendar/calendar.xml","helpful":True,"note":"tab\tdedans"}),
   appel(7,"sermo_render",{"xml":piege}),
   {"jsonrpc":"2.0","id":8,"method":"ping"},
   {"jsonrpc":"2.0","id":9,"method":"pas/une/methode"},
   appel(10,"sermo_inexistant",{}),
   appel(11,"sermo_examples",{"task":"formulaire de contact","limit":1}),
   appel(12,"sermo_vote",{"example":"../../etc","helpful":True}),
   appel(13,"sermo_check",{"xml":5})]
for x in m: print(json.dumps(x))
print('{pas du json')
print('[1,2]')
PY
RBIN="$RACINE/sermo-backend-html/_build/htmlsermo"; [ -x "$RBIN" ] || RBIN="$BIN"
SERMO_BIN="$BIN" SERMO_RENDER_BIN="$RBIN" SERMO_EXAMPLES_DIR="$ICI/examples" SERMO_VOTE_FILE="$T/votes.tsv" \
    env -u DISPLAY -u WAYLAND_DISPLAY timeout 120 "$SERVEUR" < "$T/req.jsonl" > "$T/rep.jsonl" 2>/dev/null
ATTENDUE=$(tr -d '[:space:]' < "$RACINE/VERSION")
verdict=$(python3 -I - "$T/rep.jsonl" "$ATTENDUE" <<'PY'
import json,sys
r={}; e={}; sans_id=[]; err=[]
for l in open(sys.argv[1]):
    try: d=json.loads(l)
    except ValueError:
        err.append(f"ligne non JSON sur le canal MCP : {l.strip()[:60]!r}"); continue
    if d.get('id') is None: sans_id.append(d.get('error',{}).get('code'))
    elif 'error' in d: e[d['id']]=d['error']['code']
    else: r[d['id']]=d.get('result',{})
def texte(i): return ' '.join(c.get('text','') for c in r.get(i,{}).get('content',[]))
init=r.get(1,{})
v=init.get('serverInfo',{}).get('version')
if v!=sys.argv[2]: err.append(f"initialize : version {v!r}, VERSION dit {sys.argv[2]!r}")
if init.get('protocolVersion')!='2025-06-18': err.append(f"initialize ne reprend pas la version du client : {init.get('protocolVersion')!r}")
if 'sermo_check' not in init.get('instructions',''): err.append("initialize sans instructions")
outils=r.get(2,{}).get('tools',[])
if sorted(t['name'] for t in outils)!=sorted(['sermo_schema','sermo_check','sermo_run','sermo_render','sermo_examples','sermo_vote']): err.append(f"tools/list : {[t['name'] for t in outils]}")
ann={t['name']:t.get('annotations',{}) for t in outils}
if ann.get('sermo_run',{}).get('readOnlyHint',True) or not ann.get('sermo_render',{}).get('readOnlyHint'): err.append("annotations readOnlyHint fausses")
if r.get(3,{}).get('isError') or not texte(3).startswith('OK'): err.append("sermo_check refuse un XML juste")
if not r.get(4,{}).get('isError') or not texte(4).startswith('ERREUR'): err.append("sermo_check accepte un XML faux")
if 'Gtk-Message' in texte(4): err.append("sermo_check laisse le bruit GTK dans le diagnostic")
if 'calendar' not in texte(5): err.append("sermo_examples ne sert pas le sujet calendar")
if r.get(6,{}).get('isError'): err.append("sermo_vote a échoué")
if 7 not in r: err.append("sermo_render n'a pas répondu")
if r.get(8)!={}: err.append(f"ping : {r.get(8)!r}")
if e.get(9)!=-32601: err.append(f"méthode inconnue : code {e.get(9)}")
if e.get(10)!=-32602: err.append(f"outil inconnu : code {e.get(10)}")
if 'formulaire/' not in texte(11): err.append("sermo_examples ne trouve pas « formulaire de contact »")
if not r.get(12,{}).get('isError'): err.append("sermo_vote accepte un sujet hors catalogue")
if not r.get(13,{}).get('isError'): err.append("sermo_check accepte un 'xml' qui n'est pas une chaîne")
if sans_id!=[-32700,-32600]: err.append(f"JSON illisible / tableau : codes {sans_id}")
print('\n'.join(err) if err else 'OK')
PY
)
if [ "$verdict" = OK ]; then
    echo "protocole : initialize ($ATTENDUE), 6 outils annotés, check juste/faux, examples (mot et phrase), vote, render ($(basename "$RBIN")), ping, codes d'erreur"
else
    while IFS= read -r l; do ko "$l"; done <<< "$verdict"
fi
grep -q $'^calendar\t1\ttab dedans$' "$T/votes.tsv" 2>/dev/null || ko "le vote n'a pas été écrit (ou la tabulation de la note a traversé)"
[ -e "$TEMOIN" ] && ko "sermo_render a EXÉCUTÉ un <input> du XML"

# ── 4. Rendu en image : un port à fenêtre, commandes bloquées ─────────────────
IMG=""
for p in qt6 sdl3 gtk3 lvgl; do
    c="$RACINE/sermo-backend-$p/_build/${p}sermo"; [ -x "$c" ] && { IMG=$c; break; }
done
case "$(basename "${IMG:-x}")" in
    qt6sermo|sdl3sermo) peut=1 ;;
    *) command -v xvfb-run >/dev/null 2>&1 && [ -n "$IMG" ] && peut=1 || peut=0 ;;
esac
if [ "$peut" = 1 ]; then
    rm -f "$TEMOIN"
    sed -n 8p "$T/req.jsonl" | SERMO_BIN="$BIN" SERMO_RENDER_BIN="$IMG" timeout 120 "$SERVEUR" > "$T/img.jsonl" 2>/dev/null
    python3 -I - "$T/img.jsonl" <<'PY' && echo "rendu : image PNG par $(basename "$IMG") (petit dialogue accepté)" || ko "sermo_render n'a pas rendu d'image avec $(basename "$IMG")"
import json,sys,base64
d=json.loads(open(sys.argv[1]).readline())
img=[x for x in d["result"]["content"] if x["type"]=="image"]
sys.exit(0 if img and base64.b64decode(img[0]["data"])[:8]==b"\x89PNG\r\n\x1a\n" else 1)
PY
    [ -e "$TEMOIN" ] && ko "sermo_render ($(basename "$IMG")) a EXÉCUTÉ un <input> du XML"
else
    echo "rendu : image NON vérifiée (aucun port à fenêtre construit, ou xvfb-run absent)"
fi

# ── 5. Délais : un dialogue jamais fermé ne fige pas le serveur ───────────────
debut=$(date +%s)
printf '%s\n' '{"jsonrpc":"2.0","id":1,"method":"tools/call","params":{"name":"sermo_run","arguments":{"xml":"<window/>","timeout":5}}}' \
    '{"jsonrpc":"2.0","id":2,"method":"ping"}' \
    | SERMO_BIN="$ESSAIS/bloque.sh" DISPLAY=:99 timeout 60 "$SERVEUR" > "$T/delai.jsonl" 2>/dev/null
duree=$(( $(date +%s) - debut ))
python3 -I - "$T/delai.jsonl" <<'PY' && [ "$duree" -lt 20 ] && echo "délai : sermo_run coupé à 5 s (${duree} s en tout), le serveur répond ensuite" || ko "délai de sermo_run non tenu (${duree} s)"
import json,sys
l=[json.loads(x) for x in open(sys.argv[1])]
ok=len(l)==2 and l[0]["result"].get("isError") and "timeout" in l[0]["result"]["content"][0]["text"] and l[1]["result"]=={}
sys.exit(0 if ok else 1)
PY
pgrep -f "$ESSAIS/bloque.sh" >/dev/null && ko "des processus du faux binaire survivent au délai"

[ "$ecarts" -eq 0 ] && { echo "verifie : OK"; exit 0; }
echo "verifie : $ecarts écart(s)"; exit 1
