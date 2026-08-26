#!/bin/bash
# garde_pas_de_poste.sh — aucun horodatage de poste ne doit entrer dans le dépôt public.
#
# POURQUOI CE BANC EXISTE
#
# Le dépôt privé haplo-dialog applique l'ordonnance du double horodatage : chaque
# fichier .md y porte, en deux premières lignes, « maj : … · poste <hostname> » et
# « réindex : … ». C'est utile là-bas, et un hook pre-commit l'entretient.
#
# Mais ce dépôt-ci est PUBLIC, et il est alimenté par COPIE DE FICHIERS depuis le
# privé. Une copie faite sans précaution y ferait entrer 46 fichiers portant le nom
# de la machine de travail.
#
# Ce n'est pas théorique. En août 2026, le nom réel et l'adresse professionnelle du
# mainteneur ont fuité dans l'historique de ce dépôt. Réécrire l'historique n'a PAS
# suffi : GitLab a continué de servir les anciens commits par leur empreinte, et la
# liste publique des pipelines publiait ces empreintes. Il a fallu SUPPRIMER le
# projet et le recréer. Le coût d'une fuite ici se compte en journées.
#
# Ce banc est donc une barrière à l'entrée, pas un contrôle de confort.
#
# Usage : garde_pas_de_poste.sh [racine]   (défaut : la racine du dépôt)

set -u
RACINE="${1:-$(cd "$(dirname "$0")/.." && pwd)}"
[ -d "$RACINE" ] || { echo "usage: $0 [racine]" >&2; exit 2; }

# grep -r SANS -a saute les fichiers en ISO-8859-1 en les prenant pour du binaire :
# il rendrait « 0 trouvé » sur un fichier fautif. C'est arrivé trois fois cette
# semaine. Toujours -a.
# Le motif vise la FORME EXACTE de l'ordonnance, pas le mot « poste » tout seul :
# tests/fuzz/fuzz.py contient « poste sans instrumentation » dans une phrase
# française ordinaire, et une première version de ce banc le signalait. Un banc qui
# crie pour rien finit ignoré, donc on exige le séparateur « · » et le contexte
# « maj : <date> », ou la ligne « réindex : ».
MOTIFS='maj :[[:space:]]+[0-9]{4}-[0-9]{2}-[0-9]{2}[^·]*· poste |^<div align="right"><sub><code>réindex :'

trouves=$(grep -rlaE "$MOTIFS" "$RACINE" \
            --exclude-dir=.git --exclude="$(basename "$0")" 2>/dev/null || true)

# ── Témoin de sensibilité ───────────────────────────────────────────────────
# Sans lui, un banc devenu aveugle resterait vert pour toujours. On fabrique un
# fichier fautif et on exige que la recherche le trouve.
TMP="$(mktemp -d)"; trap 'rm -rf "$TMP"' EXIT
printf '<div align="right"><sub><code>maj :     2026-01-01 00:00:00 · poste temoin</code></sub></div>\n' \
    > "$TMP/temoin.md"
if ! grep -rlaE "$MOTIFS" "$TMP" >/dev/null 2>&1; then
    echo "ÉCHEC — le témoin de sensibilité n'est pas détecté : ce banc ne mesure rien." >&2
    exit 1
fi
echo "  ✔ témoin de sensibilité : un fichier fautif est bien détecté"

if [ -n "$trouves" ]; then
    echo >&2
    echo "ÉCHEC — un horodatage de poste est entré dans le dépôt PUBLIC :" >&2
    printf '%s\n' "$trouves" | sed "s|^$RACINE/|  ✘ |" >&2
    cat >&2 <<'MSG'

Ces deux lignes viennent du dépôt privé, où l'ordonnance du double horodatage
les impose. Elles n'ont rien à faire ici : elles publient le nom de la machine
de travail.

Retirer les deux premières lignes de chaque fichier cité avant de committer.
Et si c'est arrivé par une copie de fichiers depuis le privé : la copie doit
filtrer ces en-têtes, à chaque fois.
MSG
    exit 1
fi

echo "garde_pas_de_poste : OK — aucun horodatage de poste dans le dépôt."
exit 0
