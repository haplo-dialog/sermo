#!/bin/bash
# SPDX-License-Identifier: GPL-2.0-or-later
# garde_badges.sh — les badges des README disent-ils encore quelque chose de vrai ?
#
# Les badges (pipeline GitLab, dernière version) sont LUS en direct par la forge
# et par shields.io : rien n'est écrit à la main. Mais une adresse peut mourir
# en silence, et un badge mort reste une image : « pipeline : unknown »,
# « release : project not found », « no status ». Ce banc télécharge chaque
# badge des deux README et refuse : un code autre que 200, un contenu qui n'est
# pas un SVG, ou un texte qui avoue l'absence de donnée. Les deux README doivent
# porter les mêmes badges (adresses identiques).
#
# Pas dans ci/bancs.sh : il dépend de services tiers. Banc de PUBLICATION, comme
# garde_mavlink_amont.sh. Codes : 0 = tout vit · 1 = un badge mort ou des README
# désaccordés · 77 = sans réseau (SANS_RESEAU=1) ou outil manquant.
set -u
RACINE="${1:-$(cd "$(dirname "$0")/.." && pwd)}"
cd "$RACINE" || exit 2
for t in curl grep sed; do command -v "$t" >/dev/null || { echo "outil manquant : $t" >&2; exit 77; }; done
MORT='not found|no status|unknown|invalid|inaccessible|not available|error'

badges() { grep -o '!\[[^]]*\]([^)]*)' "$1" | sed 's/^!\[[^]]*\](\(.*\))$/\1/' | grep -E 'badges/|shields\.io'; }
FR=$(badges README.md); EN=$(badges README.en.md)
[ -n "$FR" ] || { echo "ECHEC : aucun badge dans README.md"; exit 1; }
if [ "$FR" != "$EN" ]; then echo "ECHEC : README.md et README.en.md n'ont pas les mêmes badges"; printf '  fr : %s\n' $FR; printf '  en : %s\n' $EN; exit 1; fi
echo "badges : $(printf '%s\n' "$FR" | wc -l) dans chaque README, mêmes adresses"
[ "${SANS_RESEAU:-0}" = 1 ] && { echo "IGNORÉ : SANS_RESEAU=1, badges non téléchargés" >&2; exit 77; }

T=$(mktemp -d); trap 'rm -rf "$T"' EXIT
ecarts=0
verifie() {  # verifie <url> → 0 vivant, 1 mort ; écrit la raison
    local u=$1 code type texte
    code=$(curl -sL --max-time 30 -o "$T/b.svg" -w '%{http_code} %{content_type}' "$u" 2>/dev/null) || { echo "  injoignable : $u"; return 2; }
    type=${code#* }; code=${code%% *}
    texte=$(tr -d '\n' < "$T/b.svg" | sed 's/<[^>]*>/ /g; s/  */ /g')
    if [ "$code" != 200 ]; then echo "  MORT (HTTP $code) : $u"; return 1; fi
    case "$type" in image/svg+xml*) ;; *) echo "  MORT (pas un SVG : $type) : $u"; return 1 ;; esac
    if printf '%s' "$texte" | grep -qiE "$MORT"; then echo "  MORT (le badge avoue : « $(printf '%s' "$texte" | grep -oiE "[a-z ]*($MORT)" | head -1 | sed 's/^ *//') ») : $u"; return 1; fi
    echo "  vivant : $u — « $(printf '%s' "$texte" | sed 's/^ *//; s/ *$//' | cut -c1-60) »"
}
while IFS= read -r u; do
    verifie "$u"; rc=$?
    [ "$rc" -eq 2 ] && { echo "IGNORÉ : réseau indisponible, rien vérifié" >&2; exit 77; }
    [ "$rc" -eq 1 ] && ecarts=$((ecarts + 1))
done <<< "$FR"
# Témoin : un badge d'un projet qui n'existe pas doit être vu mort (sinon ce banc ne mord pas).
if verifie 'https://img.shields.io/gitlab/v/release/haplo-dialog%2Fnexistepas' >/dev/null; then
    echo "ECHEC : le témoin (projet inexistant) passe pour vivant"; ecarts=$((ecarts + 1))
else echo "  témoin : projet inexistant vu mort"; fi
[ "$ecarts" -eq 0 ] && { echo "garde_badges : OK — $(printf '%s\n' "$FR" | wc -l) badges vivants, témoin vu"; exit 0; }
echo "garde_badges : $ecarts badge(s) mort(s)"; exit 1
