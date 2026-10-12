#!/bin/bash
# SPDX-License-Identifier: GPL-2.0-or-later
# catalogue-metiers.sh — verse les dialogues des scripts métier et des boîtes à
# outils dans le catalogue de sermoia :
#   examples/metiers/<NN-métier>/<N-sujet>.sh
#       → sermoia-mcp/examples/metier-<métier>/<N-sujet>-<dialogue>.xml
#   examples/boites-a-outils/<boîte>/<NN-outil>.sh (et lanceur.sh)
#       → sermoia-mcp/examples/boite-<boîte>/<NN-outil>-<dialogue>.xml
# Chaque XML commence par un commentaire qui nomme le script d'origine, pour que
# l'assistant puisse renvoyer l'utilisateur au script complet.
#
# Les scripts sont LUS, jamais lancés : chaque fonction dialogue_* est appelée
# dans un shell à part (la garde de fin empêche main de partir).
#
# Usage : sermoia-mcp/outils/catalogue-metiers.sh [--verifier]
#   --verifier : n'écrit rien ; code 1 si le catalogue n'est plus à jour.

set -u
ICI=$(CDPATH= cd -- "$(dirname -- "$0")/.." && pwd)    # .../sermoia-mcp
RACINE=$(CDPATH= cd -- "$ICI/.." && pwd)
SOURCE="$RACINE/examples/metiers"
BOITES="$RACINE/examples/boites-a-outils"
CIBLE="$ICI/examples"
VERIFIER=0; [ "${1:-}" = --verifier ] && VERIFIER=1

ETAGE="$RACINE/_build/catalogue-metiers"; mkdir -p "$ETAGE"
ETAGE=$(mktemp -d "$ETAGE/passe-XXXXXX")

for s in "$SOURCE"/[0-9][0-9]-*/[0-9]-*.sh "$BOITES"/*/lanceur.sh "$BOITES"/*/[0-9][0-9]-*.sh; do
	[ -f "$s" ] || continue
	case "$s" in
	"$BOITES"/*) d="$ETAGE/boite-$(basename "$(dirname "$s")")" ;;
	*)           metier=$(basename "$(dirname "$s")"); d="$ETAGE/metier-${metier#[0-9][0-9]-}" ;;
	esac
	sujet=$(basename "$s" .sh)
	mkdir -p "$d"
	rel=${s#"$RACINE"/}
	# Environnement neutre : les défauts en « $HOME/… » ne doivent pas porter
	# le dossier de celui qui génère (catalogue public, et identique partout).
	env -i HOME=/home/utilisateur USER=utilisateur LOGNAME=utilisateur PATH=/usr/bin:/bin \
		LANG=C.UTF-8 TZ=UTC \
	bash -c 'source "$1" >/dev/null 2>&1
		for f in $(declare -F | awk "{print \$3}" | grep "^dialogue_"); do
			{ printf "<!-- Script : %s (dialogue %s) -->\n" "$2" "${f#dialogue_}"
			  "$f"; } > "$3/$4-${f#dialogue_}.xml" 2>/dev/null
		done' _ "$s" "$rel" "$d" "$sujet"
done

ecarts=$(for d in "$ETAGE"/metier-* "$ETAGE"/boite-* "$CIBLE"/metier-* "$CIBLE"/boite-*; do
	[ -e "$d" ] || continue
	n=$(basename "$d")
	diff -rq "$ETAGE/$n" "$CIBLE/$n" >/dev/null 2>&1 || echo "$n"
done | sort -u)

if [ "$VERIFIER" = 1 ]; then
	if [ -n "$ecarts" ]; then
		echo "catalogue métier PÉRIMÉ (relancer sermoia-mcp/outils/catalogue-metiers.sh) : $(echo $ecarts)"
		exit 1
	fi
	echo "catalogue métier : à jour ($(ls -d "$ETAGE"/metier-* "$ETAGE"/boite-* 2>/dev/null | wc -l) sujets)"
	exit 0
fi

# Écriture : chaque sujet métier remplacé en bloc (dossier neuf, puis bascule).
for d in "$ETAGE"/metier-* "$ETAGE"/boite-*; do
	n=$(basename "$d")
	if [ -d "$CIBLE/$n" ]; then
		mv "$CIBLE/$n" "$ETAGE/ancien-$n"
	fi
	cp -r "$d" "$CIBLE/$n"
done
echo "catalogue métier écrit : $(ls -d "$CIBLE"/metier-* "$CIBLE"/boite-* | wc -l) sujets, $(ls "$CIBLE"/metier-*/*.xml "$CIBLE"/boite-*/*.xml | wc -l) dialogues"
