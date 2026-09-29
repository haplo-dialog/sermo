#!/bin/bash
# SPDX-License-Identifier: GPL-2.0-or-later
# packaging/bancs-sur-paquets.sh — rejoue TOUS les bancs de ci/bancs.sh sur les
# binaires LIVRÉS, extraits des paquets .deb, et non sur ceux de l'arbre.
#
# POURQUOI
#
# Le binaire d'un paquet n'est pas celui de la CI : dpkg-buildflags ajoute ses
# drapeaux, debhelper sépare les symboles de débogage, les chemins de
# construction sont réécrits. C'est lui qui part chez les utilisateurs : c'est
# lui qu'on mesure.
#
# CE QUE LE SCRIPT FAIT
#  1. clone le COMMIT donné (défaut : HEAD) dans un dossier neuf et construit
#     l'arbre avec ci/construire.sh — les bancs du cœur (FORTIFY des
#     bibliothèques) en ont besoin ;
#  2. extrait chaque sermo-backend-<port>_<version>_*.deb du dossier de paquets
#     et REMPLACE sermo-backend-<port>/_build/<port>sermo par le binaire livré
#     (sommes SHA-256 comparées après la copie) ;
#  3. vérifie que chaque binaire livré trouve ses symboles de débogage dans le
#     paquet -dbgsym du même port (même build-id) ;
#  4. joue ci/bancs.sh tel quel : tous les bancs par port et transverses
#     tournent donc sur les binaires des paquets.
# La version des paquets doit être celle de debian/changelog du commit.
#
# Usage : packaging/bancs-sur-paquets.sh <dossier-des-paquets> [commit]
# Variables :
#   SERMO_VERIF     dossier de travail (défaut : packaging/sortie/verif-paquets)
#   SERMO_TASKSET   cœurs autorisés, ex. « 0,1 » pour mesurer comme une CI à deux
#                   cœurs. ⚠️ Ne rien lancer d'autre sur ces cœurs pendant la
#                   mesure : les bancs graphiques attendent leurs fenêtres un
#                   temps borné (10 s pour les exemples, 12 s pour le clic).
# Codes : 0 = tout est vert · 1 = un banc ou un contrôle a échoué · 2 = usage/outil

set -uo pipefail
PAQUETS="${1:-}"
[ -d "$PAQUETS" ] || { echo "usage : $0 <dossier-des-paquets> [commit]" >&2; exit 2; }
PAQUETS=$(readlink -f "$PAQUETS")
RACINE=$(git rev-parse --show-toplevel 2>/dev/null) || { echo "pas un dépôt git" >&2; exit 2; }
COMMIT=$(git -C "$RACINE" rev-parse --verify "${2:-HEAD}^{commit}" 2>/dev/null) \
    || { echo "commit inconnu : ${2:-HEAD}" >&2; exit 2; }
for t in git dpkg-deb dpkg-parsechangelog readelf sha256sum; do
    command -v "$t" >/dev/null || { echo "outil manquant : $t" >&2; exit 2; }
done
TS=()
if [ -n "${SERMO_TASKSET:-}" ]; then
    command -v taskset >/dev/null || { echo "outil manquant : taskset" >&2; exit 2; }
    TS=(taskset -c "$SERMO_TASKSET")
fi
PORTS=(gtk3 gtk4 qt6 fltk1 efl1 sdl3 ncurses)
VERIF="${SERMO_VERIF:-$RACINE/packaging/sortie/verif-paquets}"
EXTRAIT=$(mktemp -d); trap 'rm -rf "$EXTRAIT"' EXIT

etape() { printf '\n── %s\n' "$*"; }
echec() { printf 'ÉCHEC : %s\n' "$*"; exit 1; }

etape "1. clone de ${COMMIT:0:7} et construction de l'arbre (journal : $VERIF/_journaux/)"
rm -rf "$VERIF"
git clone -q "$RACINE" "$VERIF" && git -C "$VERIF" checkout -q "$COMMIT" || echec "clone"
VERSION=$(dpkg-parsechangelog -l "$VERIF/debian/changelog" -S Version)
for p in "${PORTS[@]}"; do
    ls "$PAQUETS"/sermo-backend-"${p}"_"${VERSION}"_*.deb >/dev/null 2>&1 \
        || echec "paquet sermo-backend-$p $VERSION absent de $PAQUETS"
done
echo "  paquets $VERSION : $PAQUETS"
( cd "$VERIF" && "${TS[@]}" bash ci/construire.sh ) | sed 's/^/  /'
[ "${PIPESTATUS[0]}" -eq 0 ] || echec "construction de l'arbre"

etape "2-3. binaires des paquets à la place de ceux de l'arbre ; symboles de débogage"
controles=0
for p in "${PORTS[@]}"; do
    deb=$(ls "$PAQUETS"/sermo-backend-"${p}"_"${VERSION}"_*.deb)
    dbg=$(ls "$PAQUETS"/sermo-backend-"${p}"-dbgsym_"${VERSION}"_*.deb 2>/dev/null)
    dpkg-deb -x "$deb" "$EXTRAIT/$p" || echec "extraction de $deb"
    livre="$EXTRAIT/$p/usr/bin/${p}sermo"
    cible="$VERIF/sermo-backend-$p/_build/${p}sermo"
    [ -x "$livre" ] || echec "$deb ne contient pas usr/bin/${p}sermo"
    cp -f "$livre" "$cible" || echec "copie de ${p}sermo"
    s1=$(sha256sum < "$livre" | cut -d' ' -f1); s2=$(sha256sum < "$cible" | cut -d' ' -f1)
    [ "$s1" = "$s2" ] || { echo "  ✘ $p : la copie diffère du binaire livré"; controles=$((controles + 1)); continue; }
    # LC_ALL=C : en français, readelf écrit « ID construction » au lieu de
    # « Build ID » (vu le 2026-09-17 : sept binaires crus sans build-id).
    id=$(LC_ALL=C readelf -n "$livre" 2>/dev/null | awk '/Build ID/ {print $3}')
    if [ -z "$dbg" ]; then
        echo "  ✘ $p : paquet -dbgsym absent"; controles=$((controles + 1))
    elif [ -z "$id" ]; then
        echo "  ✘ $p : binaire sans build-id"; controles=$((controles + 1))
    # La liste est lue en entier AVANT la recherche : avec pipefail, « … | grep -q »
    # s'arrête au premier résultat, dpkg-deb meurt d'un relais brisé et la
    # condition devient fausse alors que le fichier est là (vu le 2026-09-17).
    elif grep -q "usr/lib/debug/.build-id/${id:0:2}/${id:2}.debug" <<< "$(dpkg-deb -c "$dbg")"; then
        echo "  ✔ $p : ${s1:0:16}… livré en place ; symboles de débogage trouvés (build-id ${id:0:12}…)"
    else
        echo "  ✘ $p : aucun fichier de symboles pour le build-id $id dans $(basename "$dbg")"
        controles=$((controles + 1))
    fi
done

etape "4. ci/bancs.sh sur les binaires livrés (journaux : $VERIF/_journaux/bancs/)"
( cd "$VERIF" && "${TS[@]}" bash ci/bancs.sh )
rc=$?

printf '\nPaquets %s, commit %s : ' "$VERSION" "${COMMIT:0:7}"
if [ "$rc" -eq 0 ] && [ "$controles" -eq 0 ]; then
    echo "tous les bancs sont verts sur les binaires livrés."
    exit 0
fi
echo "bancs rc=$rc, contrôles en échec : $controles."
exit 1
