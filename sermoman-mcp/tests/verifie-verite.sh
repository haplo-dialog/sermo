#!/bin/sh
# SPDX-License-Identifier: GPL-2.0-or-later
# verifie-verite.sh — la documentation servie aux IA dit-elle VRAI sur le code ?
#
# verifie-exemples.sh prouve que la syntaxe documentée s'analyse. Il ne prouve
# RIEN sur le fond : le mensonge « safe_system impose une liste blanche » serait
# passé au travers, et il est passé au travers pendant des mois.
#
# Ce banc-ci relie chaque affirmation forte de data/ à un FAIT du code source de
# libsermocore. Sans les sources il s'arrête en 77, jamais en 0.
#
#   SERMO_SRC=/chemin/vers/sermo ./tests/verifie-verite.sh
#   SERMOMAN_REF=/chemin/copie/reference-xml.txt …   (autre référence à éprouver ;
#                                défaut : data/reference-xml.txt)
#
# Codes : 0 tout concorde · 1 une affirmation ne colle plus au code ·
#         77 sources de libsermocore introuvables, rien n'a été vérifié.

set -u
ICI=$(CDPATH= cd -- "$(dirname -- "$0")/.." && pwd)   # .../sermoman-mcp
REF="${SERMOMAN_REF:-$ICI/data/reference-xml.txt}"
[ -f "$REF" ] || { echo "IGNORÉ : référence introuvable : $REF. Rien n'a été vérifié." >&2; exit 77; }
GUIDE="$ICI/data/guide.md"

# Racine du dépôt sermo : SERMO_SRC, sinon le parent (sermoman-mcp y vit).
SRC="${SERMO_SRC:-}"
[ -n "$SRC" ] || for c in "$ICI/.." "$ICI/../.."; do
    [ -d "$c/libsermocore" ] && { SRC=$(CDPATH= cd -- "$c" && pwd); break; }
done
if [ -z "$SRC" ] || [ ! -d "$SRC/libsermocore" ]; then
    echo "IGNORÉ : sources de libsermocore introuvables — poser SERMO_SRC. Rien n'a été vérifié." >&2
    exit 77
fi

SAFE="$SRC/libsermocore/src/safe_exec.c"
LEXER="$SRC/libsermocore/src/gtkdialog_lexer.l"
CMAKE="$SRC/libsermocore/CMakeLists.txt"
for f in "$SAFE" "$LEXER"; do
    [ -f "$f" ] || { echo "IGNORÉ : $f absent. Rien n'a été vérifié." >&2; exit 77; }
done
VERSION=$(sed -n 's/.*project(sermocore VERSION \([0-9.]*\).*/\1/p' "$CMAKE" | head -1)

echecs=0; joues=0
ko() { echecs=$((echecs+1)); printf 'ÉCHEC  %s\n' "$1"; [ $# -gt 1 ] && printf '       %s\n' "$2"; }
ok() { printf 'ok     %s\n' "$1"; }
essai() { joues=$((joues+1)); }

# ── 1. La liste blanche est-elle TOUJOURS éteinte par défaut dans le code ? ──
essai
if grep -q 'aucune restriction' "$SAFE"; then
    ok "safe_system : sans SERMO_ALLOWED_CMDS, le code n'impose aucune restriction"
else
    ko "le défaut de safe_system a CHANGÉ dans le code" \
       "« aucune restriction » a disparu de $SAFE — relire la section SÉCURITÉ de data/"
fi

# ── 2. La doc ne doit JAMAIS réenseigner que la liste blanche est le défaut ──
# On cible la vieille formule fausse « impose une liste blanche » ; la phrase
# honnête « PAS de liste blanche imposée par défaut » ne la déclenche pas.
essai
if grep -qiE 'impose une liste blanche' "$REF" "$GUIDE"; then
    ko "la vieille affirmation fausse (« impose une liste blanche ») est revenue dans la doc"
else
    ok "la doc n'enseigne pas « liste blanche par défaut » (le vrai défaut = permissif)"
fi

# ── 3. La doc affirme POSITIVEMENT le vrai défaut permissif ─────────────────
essai
if grep -qiE 'pas de liste blanche.*(défaut|imposée)|non posée par défaut|aucune restriction' "$GUIDE" "$REF"; then
    ok "la doc dit clairement qu'il n'y a pas de liste blanche par défaut"
else
    ko "la doc ne dit nulle part que le défaut est permissif" \
       "ajouter, comme le code, que sans SERMO_ALLOWED_CMDS rien n'est restreint"
fi

# ── 4. Les deux verrous documentés existent-ils dans le code ? ──────────────
for v in SERMO_ALLOWED_CMDS SERMO_NO_SHELL_FALLBACK; do
    essai
    if grep -q "$v" "$SAFE"; then
        grep -q "$v" "$REF" "$GUIDE" && ok "$v : dans le code et dans la doc" \
            || ko "$v existe dans le code mais la doc ne le documente pas"
    else
        grep -q "$v" "$REF" "$GUIDE" \
            && ko "la doc documente $v, ABSENT du code" "cherché dans $SAFE" \
            || ok "$v : absent des deux, cohérent"
    fi
done

# ── 4 bis. La limite des <input> (2.7.3) : documentée, et dans le code ────
# La doc l'annonce à 16 Mio ; le code doit porter la même valeur par défaut et
# lire la même variable.
INPUT="$SRC/libsermocore/include/sermo_input.h"
INPUTC="$SRC/libsermocore/src/sermo_input.c"
essai
if [ ! -f "$INPUT" ] || [ ! -f "$INPUTC" ]; then
    grep -q 'SERMO_INPUT_MAX' "$REF" "$GUIDE" \
        && ko "la doc documente SERMO_INPUT_MAX, mais sermo_input.h/.c sont ABSENTS du code" \
        || ok "SERMO_INPUT_MAX : absent des deux, cohérent"
elif ! grep -q 'getenv("SERMO_INPUT_MAX")' "$INPUTC"; then
    ko "SERMO_INPUT_MAX n'est plus lue par $INPUTC"
elif ! grep -qE 'SERMO_INPUT_MAX_DEFAUT +\(\(size_t\) 16 \* 1024 \* 1024\)' "$INPUT"; then
    ko "la limite par défaut du code n'est plus 16 Mio, la doc l'annonce" "relire $INPUT"
elif ! grep -q 'SERMO_INPUT_MAX' "$REF" || ! grep -q 'SERMO_INPUT_MAX' "$GUIDE"; then
    ko "SERMO_INPUT_MAX existe dans le code mais la doc ne la documente pas partout"
else
    ok "SERMO_INPUT_MAX : lue par le code, 16 Mio par défaut, comme le dit la doc"
fi

# ── 5. Chaque balise du lexer est-elle décrite par la « RÉFÉRENCE COMPLÈTE » ?
# Les sous-éléments et attributs sont documentés ailleurs : on les exclut par
# une liste EXPLICITE, pour qu'une balise-widget oubliée soit signalée.
essai
HORS='action default height input item label output radio sensitive separator variable visible width'
manquantes=""
lues=0
for t in $(grep -oE '^\\<[a-z0-9]+\\>' "$LEXER" | sed 's/\\<//;s/\\>//' | sort -u); do
    lues=$((lues+1))
    case " $HORS " in *" $t "*) continue ;; esac
    grep -qE "^ +<$t>" "$REF" || manquantes="$manquantes $t"
done
if [ -z "$manquantes" ]; then
    ok "toutes les balises-widgets du lexer sont décrites ($lues entrées lues)"
else
    ko "balise(s) du lexer absente(s) de la « RÉFÉRENCE COMPLÈTE » :$manquantes"
fi

# ── 6. Chaque source et cible du lexer est-elle dans la référence ? ─────────
# Le test 5 ne lit que les balises d'un mot (« \<text\> ») : les formes à deux
# mots « <input modbus> », « <action mqtt> »… lui échappent. On les extrait du
# lexer (règles « \<input\ X\> » et « \<action\ X\> ») et on exige chacune,
# écrite telle quelle, dans la référence. Les cinq familles attendues doivent
# être trouvées : un lexer réécrit autrement rendrait sinon un vert muet.
essai
formes=$(grep -oE '^\\<(input|action)\\ [a-z]+\\>' "$LEXER" | sed 's/\\//g' | sort -u)
absentes=""; familles_vues=""; n_formes=0
for x in modbus opcua mqtt mavlink media; do
    printf '%s\n' "$formes" | grep -qE "^<(input|action) $x>\$" && familles_vues="$familles_vues $x"
done
while IFS= read -r f; do
    [ -n "$f" ] || continue
    n_formes=$((n_formes+1))
    grep -qF "$f" "$REF" || absentes="$absentes « $f »"
done <<EOF_FORMES
$formes
EOF_FORMES
if [ "$familles_vues" != " modbus opcua mqtt mavlink media" ]; then
    ko "sources et cibles : le lexer ne donne pas les cinq familles attendues (vues :${familles_vues:- aucune})" \
       "relire l'extraction dans $LEXER"
elif [ -n "$absentes" ]; then
    ko "forme(s) du lexer absente(s) de la référence :$absentes" "ajouter à la section SOURCES ET CIBLES de $REF"
else
    ok "sources et cibles : les $n_formes formes du lexer sont dans la référence"
fi

# L'inventaire des scripts (data/scripts.md) suit les en-têtes des scripts.
if [ -d "$SRC/examples/metiers" ]; then
    essai
    if m=$(python3 -I "$ICI/outils/inventaire-scripts.py" --verifier 2>&1); then ok "$m"; else ko "$m"; fi
fi

echo
[ -n "$VERSION" ] && printf 'Version du cœur (libsermocore) : %s\n' "$VERSION"
printf '%s vérification(s), %s échec(s). Source : %s\n' "$joues" "$echecs" "$SRC/libsermocore"
[ "$joues" -gt 0 ] || { echo "ÉCHEC : rien n'a été vérifié." >&2; exit 1; }
[ "$echecs" -eq 0 ] || exit 1
echo "La documentation servie concorde avec le code."
