#!/bin/sh
# verifie-facade.sh — l'arbre public ne doit porter AUCUNE identité nominative,
# hors les lignes de clôture déjà publiées.
#
# POURQUOI. Ce dépôt est public sous l'identité de rôle
# « haplo-dialog <devel@haplo-dialog.fr> ». Les 93 commits, toutes les
# étiquettes, debian/copyright et les 7 descriptions de release la respectent.
# Deux fichiers ne la respectaient pas : les debian/changelog, dont les lignes
# de clôture portaient une adresse nominative — et elles sont parties dans les
# .deb publiés, où « apt changelog » les affiche.
#
# La dérive s'est faite SEULE : le dépôt privé porte les deux formes, personne
# ne l'a vue. Ce que rien ne contrôle dérive.
#
# LES ENTRÉES DÉJÀ PUBLIÉES NE SONT PAS RÉÉCRITES (décision de SC, 2026-08-31).
# Une ligne de clôture dit qui a fait cette entrée, ce jour-là : la réécrire
# serait falsifier un registre, et les .deb correspondants sont téléchargés.
# On plafonne donc : les anciennes sont acquises, aucune NOUVELLE n'est admise.
#
# Porte de sortie : HAPLO_FACADE_OK=1 pour passer outre en connaissance de
# cause. Elle est bruyante exprès.
#
# Codes : 0 conforme · 1 une identité nominative est apparue.

set -u
ICI=$(CDPATH= cd -- "$(dirname -- "$0")/.." && pwd)
cd "$ICI" || exit 1

# Motifs interdits. haplo-linux.fr en fait partie : ce projet est autonome, il
# ne doit pas se relier au reste par une URL oubliée.
# ⚠️ « S\. Cage » exigeait une ESPACE apres le point : « S.Cage » passait donc
# au travers, et c'est exactement sous cette forme que le nom reel a ete
# publie dans quatre fichiers du port Qt 6 le 2026-09-02. Le motif accepte
# desormais les deux graphies. « Marques » est ajoute : l'ancien binome n'a
# rien a faire dans un depot public, sous aucune forme.
MOTIFS='s\.cage@lucubratio|lucubratio|S\.? ?Cage|D\.? ?Marques|Marques|haplo-seb|/home/akej|haplo-linux\.fr'

# Plafond des lignes de clôture nominatives déjà publiées, par fichier.
PLAFOND="$ICI/tests/facade-plafond"

echecs=0
dire() { echecs=$((echecs+1)); printf 'ÉCHEC  %s\n' "$1"; }

# ── 1. Les fichiers SUIVIS PAR GIT, hors changelog (traités au point 2) ─────
# On interroge git, PAS le disque. Un `grep -r` verrait config.log, config.status
# et _build/ — ignorés par .gitignore, donc jamais publiés : du bruit qui ferait
# échouer ce contrôle à vie. Et selon le shell, `grep` peut être un ugrep qui
# respecte .gitignore ou le GNU grep qui l'ignore : les deux ne donnent pas le
# même résultat. `git ls-files` ne dépend d'aucun des deux et répond exactement
# à la question posée — qu'est-ce qui est publié.
if ! git rev-parse --git-dir >/dev/null 2>&1; then
    echo "ÉCHEC : pas un dépôt git — impossible de savoir ce qui est publié." >&2
    exit 1
fi
sales=$(git ls-files -z \
        | xargs -0 grep -lIE "$MOTIFS" 2>/dev/null \
        | grep -v -e 'debian/changelog$' -e 'tests/verifie-facade\.sh$')
if [ -n "$sales" ]; then
    dire "identité nominative dans l'arbre public :"
    printf '%s\n' "$sales" | sed 's/^/         /'
    printf '%s\n' "$sales" | while read -r f; do
        grep -nIE "$MOTIFS" "$f" | head -2 | sed "s|^|         $f:|"
    done
fi

# ── 2. Les changelog : aucune ligne de clôture nominative EN PLUS ───────────
if [ ! -f "$PLAFOND" ]; then
    dire "plafond absent : $PLAFOND — impossible de distinguer l'ancien du neuf"
else
    for f in $(find . -path ./.git -prune -o -name changelog -path '*debian*' -print | sed 's|^\./||'); do
        n=$(grep -cE "^ -- .*($MOTIFS)" "$f" 2>/dev/null || echo 0)
        p=$(awk -v k="$f" '$1==k {print $2}' "$PLAFOND")
        [ -n "$p" ] || p=0
        if [ "$n" -gt "$p" ]; then
            dire "$f : $n ligne(s) de clôture nominative(s), plafond $p — une NOUVELLE est apparue"
            printf '         attendu : -- haplo-dialog <devel@haplo-dialog.fr>\n'
        else
            printf 'ok     %s : %s/%s ligne(s) historique(s), aucune nouvelle\n' "$f" "$n" "$p"
        fi
    done
fi

# ── 3. L'auteur du dernier commit ───────────────────────────────────────────
if [ -d .git ]; then
    for champ in '%an <%ae>' '%cn <%ce>'; do
        q=$(git log -1 --format="$champ" 2>/dev/null)
        case "$q" in
            'haplo-dialog <devel@haplo-dialog.fr>') ;;
            *) dire "dernier commit signé « $q » — attendu haplo-dialog <devel@haplo-dialog.fr>" ;;
        esac
    done
    printf 'ok     dernier commit : %s\n' "$(git log -1 --format='%an <%ae>')"
fi

echo
if [ "$echecs" -eq 0 ]; then
    echo "Façade publique intacte."
    exit 0
fi
if [ "${HAPLO_FACADE_OK:-}" = 1 ]; then
    printf '\n⚠️  %s manquement(s) IGNORÉ(S) par HAPLO_FACADE_OK=1.\n' "$echecs"
    printf '⚠️  Une identité nominative va devenir PUBLIQUE et ne pourra plus être retirée.\n'
    exit 0
fi
printf '\n%s manquement(s). Corriger, ou passer outre en connaissance de cause :\n' "$echecs"
printf '  HAPLO_FACADE_OK=1 %s\n' "$0"
exit 1
