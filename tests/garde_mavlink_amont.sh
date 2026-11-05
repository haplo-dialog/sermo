#!/bin/bash
# SPDX-License-Identifier: GPL-2.0-or-later
# garde_mavlink_amont.sh — les en-têtes MAVLink embarqués sont-ils ceux dits,
# et l'amont a-t-il bougé depuis ?
#
# POURQUOI CE BANC EXISTE
#
# sermo-source/tiers/mavlink/ est une copie figée de mavlink/c_library_v2
# (commit noté dans LISEZMOI). Rien ne la met à jour : ni apt, ni la
# construction. Un audit extérieur (2026-11-01) l'a relevé : une copie figée
# vieillit en silence. Ce banc rend le vieillissement VISIBLE, à jouer avant
# chaque publication. Il ne met rien à jour : décider reste un geste humain.
#
# Deux mesures :
#   1. INTÉGRITÉ — l'archive du commit noté est téléchargée, son SHA-256 comparé
#      à celui du LISEZMOI, puis chaque fichier embarqué comparé octet à octet.
#      « Copié sans modification » devient une mesure, pas une phrase.
#   2. ÉCART — la tête de c_library_v2 est lue (git ls-remote). Si elle diffère
#      du commit noté, l'API GitHub donne le nombre de commits, la date du
#      dernier et les fichiers touchés dans les dialectes embarqués.
#
# Verdicts : 0 = copie intègre ET amont inchangé ; 1 = copie altérée, ou amont
# en avance (ÉCART, à décider avant de publier) ; 77 = pas de réseau, rien
# vérifié. Pas dans ci/bancs.sh : l'amont bouge toutes les semaines, la CI
# serait rouge pour toujours. C'est un banc de PUBLICATION.
#
# Usage : garde_mavlink_amont.sh [racine]        (défaut : la racine du dépôt)
#         SANS_RESEAU=1 … : saute les mesures réseau (sort 77 après la lecture du LISEZMOI)

set -u
RACINE="${1:-$(cd "$(dirname "$0")/.." && pwd)}"
cd "$RACINE" || exit 2
TIERS="sermo-source/tiers/mavlink"
LISEZMOI="$TIERS/LISEZMOI"
DEPOT="https://github.com/mavlink/c_library_v2"
API="https://api.github.com/repos/mavlink/c_library_v2"

for t in git curl python3 sha256sum tar cmp; do
    command -v "$t" >/dev/null || { echo "outil manquant : $t" >&2; exit 77; }
done
[ -f "$LISEZMOI" ] || { echo "ECHEC : $LISEZMOI absent"; exit 1; }

COMMIT=$(sed -n 's/^Commit *: *\([0-9a-f]\{40\}\).*/\1/p' "$LISEZMOI" | head -1)
SHA=$(sed -n 's/^SHA-256 *: *\([0-9a-f]\{64\}\).*/\1/p' "$LISEZMOI" | head -1)
[ -n "$COMMIT" ] || { echo "ECHEC : LISEZMOI ne donne pas le commit amont (ligne « Commit : <40 hex> »)"; exit 1; }
[ -n "$SHA" ]    || { echo "ECHEC : LISEZMOI ne donne pas le SHA-256 de l'archive"; exit 1; }
echo "copie embarquée : commit ${COMMIT:0:12} (LISEZMOI), $(git ls-files -- "$TIERS" | grep -c '\.h$') en-têtes suivis"

[ "${SANS_RESEAU:-0}" = 1 ] && { echo "IGNORÉ : SANS_RESEAU=1, intégrité et écart non mesurés" >&2; exit 77; }

T=$(mktemp -d); trap 'rm -rf "$T"' EXIT
ecarts=0

# ── 1. Intégrité : l'archive du commit noté, fichier par fichier ─────────────
ARCH="https://codeload.github.com/mavlink/c_library_v2/tar.gz/$COMMIT"
if ! curl -fsSL --max-time 120 -o "$T/amont.tar.gz" "$ARCH"; then
    echo "IGNORÉ : archive amont injoignable ($ARCH) — rien vérifié" >&2; exit 77
fi
vu=$(sha256sum "$T/amont.tar.gz" | cut -d' ' -f1)
if [ "$vu" != "$SHA" ]; then
    echo "ECHEC : SHA-256 de l'archive $vu ≠ LISEZMOI $SHA"; ecarts=$((ecarts + 1))
fi
mkdir "$T/x" && tar -xzf "$T/amont.tar.gz" -C "$T/x" || { echo "ECHEC : archive illisible"; exit 1; }
SRC=$(find "$T/x" -mindepth 1 -maxdepth 1 -type d | head -1)
n=0; alt=0; absents=0
while IFS= read -r f; do
    [ -n "$f" ] || continue
    rel="${f#"$TIERS"/}"
    case "$rel" in LISEZMOI|COPYING.generateur) continue ;; esac
    n=$((n + 1))
    if [ ! -f "$SRC/$rel" ]; then
        echo "  $rel : absent de l'archive amont"; absents=$((absents + 1))
    elif ! cmp -s "$f" "$SRC/$rel"; then
        echo "  $rel : DIFFÈRE de l'archive amont"; alt=$((alt + 1))
    fi
done <<< "$(git ls-files -- "$TIERS")"
if [ "$alt" -eq 0 ] && [ "$absents" -eq 0 ] && [ "$vu" = "$SHA" ]; then
    echo "intégrité : $n fichiers identiques à l'archive du commit noté (SHA-256 égal)"
else
    echo "ECHEC : copie embarquée altérée ($alt différent(s), $absents absent(s) sur $n)"; ecarts=$((ecarts + 1))
fi
# Témoin : un octet changé doit être vu. On altère une copie, pas le dépôt.
cp "$TIERS/checksum.h" "$T/temoin.h"; printf 'x' >> "$T/temoin.h"
cmp -s "$T/temoin.h" "$SRC/checksum.h" && { echo "ECHEC : le témoin altéré passe pour identique (cmp ne mord pas)"; ecarts=$((ecarts + 1)); }

# ── 2. Écart : la tête de l'amont ────────────────────────────────────────────
TETE=$(timeout 60 git ls-remote "$DEPOT" refs/heads/master 2>/dev/null | cut -f1)
if [ -z "$TETE" ]; then
    echo "IGNORÉ : tête de l'amont injoignable (git ls-remote $DEPOT)" >&2
    [ "$ecarts" -eq 0 ] && exit 77 || exit 1
fi
if [ "$TETE" = "$COMMIT" ]; then
    echo "écart : aucun, l'amont est au commit embarqué (${TETE:0:12})"
else
    if curl -fsSL --max-time 60 -o "$T/cmp.json" "$API/compare/$COMMIT...$TETE"; then
        python3 -I - "$T/cmp.json" "$TIERS" <<'PY'
import json, sys
d = json.load(open(sys.argv[1]))
cs = d.get("commits", [])
dates = [c["commit"]["committer"]["date"][:10] for c in cs]
fichiers = [f["filename"] for f in d.get("files", [])]
dial = [f for f in fichiers if f.split("/")[0] in ("common", "standard", "minimal") or "/" not in f]
print(f"ÉCART : l'amont est {d.get('ahead_by', len(cs))} commit(s) en avance "
      f"({dates[0] if dates else '?'} → {dates[-1] if dates else '?'}), "
      f"{len(fichiers)} fichier(s) touché(s), dont {len(dial)} dans les dialectes embarqués")
for f in dial[:12]:
    print(f"  {f}")
if len(dial) > 12:
    print(f"  … et {len(dial) - 12} autre(s)")
print("À décider avant de publier : rafraîchir la copie (LISEZMOI : commit, SHA-256, date) ou "
      "garder et le dire.")
PY
    else
        echo "ÉCART : l'amont est à ${TETE:0:12}, la copie à ${COMMIT:0:12} (détail API GitHub injoignable)"
    fi
    ecarts=$((ecarts + 1))
fi

[ "$ecarts" -eq 0 ] && { echo "garde_mavlink_amont : OK"; exit 0; }
echo "garde_mavlink_amont : $ecarts point(s) à régler"; exit 1
