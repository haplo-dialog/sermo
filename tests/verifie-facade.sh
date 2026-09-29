#!/bin/bash
# SPDX-License-Identifier: GPL-2.0-or-later
# tests/verifie-facade.sh — ce qui sort ne porte ni trace de l'atelier, ni autre
# identité que celle du projet.
#
# POURQUOI CE CONTRÔLE EXISTE
#
# Un dépôt public se nettoie mal : un historique réécrit reste servi par
# empreinte, et ce qui a été cloné l'est pour de bon. Le contrôle doit donc
# passer AVANT, sur trois choses différentes : l'arbre, l'historique (messages,
# auteur ET committer, contenu de chaque version, étiquettes annotées), et les
# fichiers produits
# (binaires, .pc, paquets), qui embarquent parfois un chemin de la machine de
# construction.
#
# DEUX JEUX DE MOTIFS
#
#  - GÉNÉRIQUES, écrits ci-dessous : ils ne révèlent rien — en-tête
#    d'horodatage de poste, renvoi à un document de décision interne
#    (« ADR-00xx »), marque d'attribution dans un message de commit, chemin
#    personnel /home/<utilisateur>/.
#  - PRIVÉS, lus dans un fichier HORS du dépôt : noms, machines, outils internes.
#    Les écrire ici les publierait ; la version précédente de ce contrôle le
#    faisait. Fichier : $SERMO_FACADE_MOTIFS, sinon <git-common-dir>/facade-motifs
#    (jamais versionné). Une expression Perl par ligne ; # commente.
#
# USAGE
#   verifie-facade.sh [--strict]                    l'arbre suivi par git
#   verifie-facade.sh [--strict] --historique REF   tous les commits atteignables
#   verifie-facade.sh [--strict] --fichiers F…      des produits (.deb dépaquetés)
#
#   --strict       : échoue si les motifs privés sont absents (contrôle avant
#                    publication).
#   --sans-identite : ne contrôle pas l'identité des commits. C'est le mode de la
#                    CI : une contribution signée (DCO) garde le nom de son
#                    auteur, et n'a pas à porter celui du mainteneur.
#   Identité attendue des commits : $SERMO_FACADE_IDENTITE,
#   par défaut « s.cage <devel@haplo-dialog.fr> ».
#
# Codes : 0 conforme · 1 manquement, témoin raté ou motifs privés absents en
#         --strict · 2 usage ou outil manquant

set -u
LC_ALL=C; export LC_ALL
IDENTITE="${SERMO_FACADE_IDENTITE:-s.cage <devel@haplo-dialog.fr>}"

GENERIQUES=(
    'maj :[[:space:]]+[0-9]{4}-[0-9]{2}-[0-9]{2}[^<]*poste'
    'ADR-00[0-9]{2}'
    'Co-Authored-By:'
    '/home/(?!user/|utilisateur/)[a-z_][a-z0-9_-]*/'
)

usage() { sed -n '/^# USAGE/,/^# Codes/p' "$0" | sed 's/^# \{0,1\}//' >&2; exit 2; }

STRICT=0; MODE=arbre; REF=""; FICHIERS=(); IDENTITE_CONTROLEE=1
while [ $# -gt 0 ]; do
    case "$1" in
        --strict) STRICT=1 ;;
        --sans-identite) IDENTITE_CONTROLEE=0 ;;
        --historique) MODE=historique; REF="${2:-}"; [ -n "$REF" ] || usage; shift ;;
        --fichiers) MODE=fichiers; shift; FICHIERS=("$@"); break ;;
        -h|--help) usage ;;
        *) usage ;;
    esac
    shift
done
command -v git >/dev/null || { echo "outil manquant : git" >&2; exit 2; }
echo x | grep -qP 'x' 2>/dev/null || { echo "outil manquant : grep -P (PCRE)" >&2; exit 2; }

# ── motifs ──────────────────────────────────────────────────────────────────
charger_prives() {
    local f="${SERMO_FACADE_MOTIFS:-}"
    if [ -z "$f" ]; then
        local d; d=$(git rev-parse --git-common-dir 2>/dev/null) || return 1
        f="$d/facade-motifs"
    fi
    [ -f "$f" ] || return 1
    grep -v -e '^[[:space:]]*#' -e '^[[:space:]]*$' "$f"
}
MOTIFS=("${GENERIQUES[@]}")
PRIVES=0
if prives=$(charger_prives); then
    while IFS= read -r m; do MOTIFS+=("$m"); PRIVES=$((PRIVES+1)); done <<< "$prives"
fi
expr_motifs() { local IFS='|'; printf '(?:%s)' "${MOTIFS[*]}"; }
RE=$(expr_motifs)

# ── fonctions de fouille ────────────────────────────────────────────────────
MANQUEMENTS=0
signaler() { MANQUEMENTS=$((MANQUEMENTS+1)); printf '  ✘ %s\n' "$1"; }

fouiller_flux() {   # fouiller_flux <étiquette> : lit stdin, signale chaque ligne fautive
    local etiq="$1" n
    n=$(grep -a -n -o -P "$RE" | head -3 | cut -c1-120)
    [ -n "$n" ] && while IFS= read -r l; do signaler "$etiq:$l"; done <<< "$n"
    return 0
}

fouiller_arbre() {  # fouiller_arbre <racine> : fichiers suivis, sauf ce contrôle et la garde d'en-tête
    local racine="$1"
    while IFS= read -r -d '' f; do
        case "$f" in tests/verifie-facade.sh|tests/garde_pas_de_poste.sh) continue ;; esac
        [ -f "$racine/$f" ] || continue
        fouiller_flux "$f" < "$racine/$f"
    done < <(git -C "$racine" ls-files -z)
}

fouiller_historique() {  # fouiller_historique <dépôt> <ref>
    local depot="$1" ref="$2" c qui
    while IFS= read -r c; do
        [ "$IDENTITE_CONTROLEE" -eq 1 ] && for qui in '%an <%ae>' '%cn <%ce>'; do
            q=$(git -C "$depot" log -1 --format="$qui" "$c")
            [ "$q" = "$IDENTITE" ] || signaler "commit ${c:0:12} : identité « $q » (attendu « $IDENTITE »)"
        done
        # ⚠️ Jamais « commande | fouiller_flux » : le tube ferait tourner la fonction
        # dans un sous-shell, où le compte des manquements se perd. Le témoin l'a vu.
        fouiller_flux "message ${c:0:12}" < <(git -C "$depot" log -1 --format=%B "$c")
    done < <(git -C "$depot" rev-list "$ref")
    while IFS=' ' read -r type sha chemin; do
        [ "$type" = blob ] || continue
        case "$chemin" in tests/verifie-facade.sh|tests/garde_pas_de_poste.sh) continue ;; esac
        fouiller_flux "version ${sha:0:12} de ${chemin:-?}" < <(git -C "$depot" cat-file blob "$sha")
    done < <(git -C "$depot" rev-list --objects "$ref" | git -C "$depot" cat-file --batch-check='%(objecttype) %(objectname) %(rest)')
    # Étiquettes annotées : elles portent leur propre identité et leur propre
    # message, publiés avec elles. Une étiquette légère n'a rien à fouiller.
    while IFS= read -r t; do
        [ -n "$t" ] || continue
        [ "$(git -C "$depot" cat-file -t "refs/tags/$t")" = tag ] || continue
        q=$(git -C "$depot" for-each-ref --format='%(taggername) %(taggeremail)' "refs/tags/$t")
        [ "$IDENTITE_CONTROLEE" -eq 0 ] || [ "$q" = "$IDENTITE" ] || signaler "étiquette $t : identité « $q » (attendu « $IDENTITE »)"
        # sujet et corps seulement : le bloc de signature est du base64, où un
        # motif court pourrait apparaître par hasard
        fouiller_flux "étiquette $t" < <(git -C "$depot" for-each-ref --format='%(contents:subject)%0a%(contents:body)' "refs/tags/$t")
    done < <(git -C "$depot" tag --merged "$ref" 2>/dev/null)
}

fouiller_fichiers() {
    local f tmp
    for f in "$@"; do
        [ -e "$f" ] || { signaler "introuvable : $f"; continue; }
        case "$f" in
            *.deb)
                tmp=$(mktemp -d)
                dpkg-deb -R "$f" "$tmp" 2>/dev/null || { signaler "paquet illisible : $f"; rm -rf "$tmp"; continue; }
                while IFS= read -r -d '' g; do fouiller_flux "$(basename "$f"):${g#"$tmp"/}" < "$g"; done < <(find "$tmp" -type f -print0)
                rm -rf "$tmp" ;;
            *)  fouiller_flux "$f" < "$f" ;;
        esac
    done
}

# ── témoin : la fouille doit voir ce qu'elle cherche ───────────────────────
temoin() {
    local T; T=$(mktemp -d)
    git init -q "$T/d"
    printf 'renvoi ADR-0042\n' > "$T/d/a.txt"
    printf 'x \xe9 maj :     2026-01-01 00:00:00 \xb7 poste machine\n' > "$T/d/latin1.txt"
    git -C "$T/d" add . && git -C "$T/d" -c user.name=autre -c user.email=autre@exemple.invalid commit -q -m "essai

Co-Authored-By: quelqu'un"
    git -C "$T/d" rm -q a.txt && git -C "$T/d" -c user.name=autre -c user.email=autre@exemple.invalid commit -q -m "retrait"
    git -C "$T/d" -c user.name=autre -c user.email=autre@exemple.invalid tag -a v0 -m "voir ADR-0007"
    local avant=$MANQUEMENTS sortie
    sortie=$(MANQUEMENTS=0; IDENTITE_CONTROLEE=1; fouiller_historique "$T/d" HEAD >/dev/null; echo $MANQUEMENTS)
    rm -rf "$T"
    # attendus : 2 commits × 2 identités, 1 message, la version supprimée
    # (ADR), le fichier Latin-1 (en-tête de poste), l'étiquette (identité et
    # message)
    [ "$sortie" -ge 9 ] || { echo "ÉCHEC DU TÉMOIN : la fouille n'a vu que $sortie manquement(s) sur 9 fabriqués." >&2; exit 1; }
    MANQUEMENTS=$avant
}
temoin

# ── exécution ───────────────────────────────────────────────────────────────
if [ "$PRIVES" -eq 0 ]; then
    echo "motifs privés : absents — contrôle PARTIEL (motifs génériques seulement)."
    [ "$STRICT" -eq 1 ] && { echo "ÉCHEC : --strict exige les motifs privés (SERMO_FACADE_MOTIFS ou <git-common-dir>/facade-motifs)."; exit 1; }
else
    echo "motifs privés : $PRIVES chargé(s)."
fi

case "$MODE" in
    arbre)
        RACINE=$(git rev-parse --show-toplevel 2>/dev/null) || { echo "pas un dépôt git : impossible de savoir ce qui est publié" >&2; exit 2; }
        echo "fouille de l'arbre suivi : $RACINE"
        fouiller_arbre "$RACINE"
        [ "$IDENTITE_CONTROLEE" -eq 1 ] && for qui in '%an <%ae>' '%cn <%ce>'; do
            q=$(git -C "$RACINE" log -1 --format="$qui")
            [ "$q" = "$IDENTITE" ] || signaler "dernier commit : identité « $q » (attendu « $IDENTITE »)"
        done ;;
    historique)
        RACINE=$(git rev-parse --show-toplevel 2>/dev/null) || { echo "pas un dépôt git" >&2; exit 2; }
        echo "fouille de l'historique de $REF ($(git -C "$RACINE" rev-list --count "$REF") commit(s))"
        fouiller_historique "$RACINE" "$REF" ;;
    fichiers)
        [ ${#FICHIERS[@]} -gt 0 ] || usage
        echo "fouille de ${#FICHIERS[@]} fichier(s) produit(s)"
        fouiller_fichiers "${FICHIERS[@]}" ;;
esac

if [ "$MANQUEMENTS" -gt 0 ]; then
    echo "ÉCHEC : $MANQUEMENTS manquement(s) à la façade."
    exit 1
fi
case "$MODE:$IDENTITE_CONTROLEE" in
    fichiers:*) echo "verifie-facade : OK — aucune trace (témoin vu)." ;;
    *:1)        echo "verifie-facade : OK — aucune trace, identité conforme (témoin vu)." ;;
    *)          echo "verifie-facade : OK — aucune trace (identité non contrôlée), témoin vu." ;;
esac
