#!/bin/bash
# SPDX-License-Identifier: GPL-2.0-or-later
# verifie.sh — les scripts métier et les boîtes à outils tiennent-ils, et chaque
# port les rend-il comme l'étalon gtk3sermo ? Test de réussite des ports, à partir
# de dialogues réels.
#
# Pour chaque script de examples/metiers/*/ et de examples/boites-a-outils/*/ :
#   1. bash -n ; en-tête (Métier ou Boîte, Besoin, Enrobe, Ports, Terminal, Usage) ; garde
#      de fin « [ "${BASH_SOURCE[0]}" = "$0" ] && main » (le script se LIT sans
#      se lancer) ;
#   2. chaque fonction dialogue_* s'analyse sur chaque port (--print-ir, code 0) ;
#   3. chaque dialogue est JOUÉ sur chaque port : un minuteur invisible, glissé
#      dans le <vbox> racine, le ferme (exit:essai) ; les commandes du XML sont
#      bloquées (SERMO_ALLOWED_CMDS sur une commande qui n'existe pas). Les
#      variables rendues doivent être celles de gtk3sermo. htmlsermo ne joue pas
#      les minuteurs : il rend EXIT="abort", accepté ; ses variables comptent.
#
# Usage : examples/metiers/verifie.sh [--images DOSSIER] [--ports "gtk3 qt6 …"] [script…]
#         [--garder DOSSIER]   range la passe (dialogues extraits, valeurs de chaque
#                              port) dans DOSSIER ; sur un système sans GTK (macOS),
#                              avec --ports "ncurses", rien n'est comparé ici ;
#         [--rejouer DOSSIER]  reprend une passe gardée sans réextraire : les ports
#                              de --ports (gtk3) y jouent les MÊMES dialogues, et
#                              leurs valeurs sont comparées à celles déjà là.
#   Sans xvfb-run, les dialogues se jouent sans écran virtuel (ncurses, html).
#   Binaires : sermo-backend-<port>/_build/<port>sermo, sinon /usr/bin/<port>sermo.
#   --images : une image par dialogue (gtk3sermo), pour la relecture.
# Codes : 0 tout concorde · 1 un écart · 77 gtk3sermo absent, rien vérifié.

set -u
ICI=$(CDPATH= cd -- "$(dirname -- "$0")" && pwd)
RACINE=$(CDPATH= cd -- "$ICI/../.." && pwd)
PORTS="gtk3 gtk4 qt6 fltk1 efl1 sdl3 lvgl ncurses html"
IMAGES=""
SCRIPTS=()
while [ $# -gt 0 ]; do
	case "$1" in
	--images) IMAGES=$2; shift 2 ;;
	--ports)  PORTS=$2; shift 2 ;;
	--garder)  GARDER=$2; shift 2 ;;
	--rejouer) REJOUER=$2; shift 2 ;;
	*)        SCRIPTS+=("$1"); shift ;;
	esac
done
BOITES="$ICI/../boites-a-outils"
[ ${#SCRIPTS[@]} -gt 0 ] || mapfile -t SCRIPTS < <(ls "$ICI"/[0-9][0-9]-*/[0-9]-*.sh \
	"$BOITES"/*/lanceur.sh "$BOITES"/*/[0-9][0-9]-*.sh 2>/dev/null)

binaire() {
	local b="$RACINE/sermo-backend-$1/_build/$1sermo"
	[ -x "$b" ] && { echo "$b"; return; }
	b=/usr/bin/$1sermo; [ -x "$b" ] && echo "$b"
}
ETALON=$(binaire gtk3)
GARDER=${GARDER:-}; REJOUER=${REJOUER:-}
# Sans gtk3sermo, on peut encore jouer et garder la passe (--garder), pour la
# comparer ailleurs (--rejouer) ; sinon, rien n'est vérifiable.
if [ -z "$ETALON" ] && [ -z "$GARDER" ]; then
	echo "IGNORÉ : gtk3sermo introuvable, rien vérifié." >&2; exit 77
fi
if [ -n "$REJOUER" ] && [ ! -s "$REJOUER/liste" ]; then
	echo "IGNORÉ : passe à rejouer introuvable : $REJOUER" >&2; exit 77
fi

# Travail sous _build (ignoré par git), jamais sous /tmp : rien n'y est exécuté,
# mais le banc ne sème pas de fichiers ailleurs que dans l'arbre de construction.
# Installé (sermo-doc, sous /usr/share/doc), il n'y a pas d'arbre : le cache.
if [ -d "$RACINE/ci" ] && [ -w "$RACINE" ]; then
	T="$RACINE/_build/metiers"
else
	T="${XDG_CACHE_HOME:-$HOME/.cache}/sermo-metiers"
fi
mkdir -p "$T" || { echo "IGNORÉ : dossier de travail $T impossible à créer." >&2; exit 77; }
T=$(mktemp -d "$T/passe-XXXXXX")
[ -n "$IMAGES" ] && mkdir -p "$IMAGES"
ecarts=0
ko() { echo "ÉCART  $*"; ecarts=$((ecarts + 1)); }

# ── 1 et extraction des dialogues ─────────────────────────────────────────────
n_scripts=0; n_dialogues=0
: > "$T/liste"
if [ -n "$REJOUER" ]; then
	# Reprise : mêmes dialogues, chemins relatifs rendus absolus sous $T.
	cp -R "$REJOUER"/. "$T"/
	while IFS='|' read -r nom rel; do echo "$nom|$T/$rel"; done < "$REJOUER/liste" > "$T/liste"
	n_dialogues=$(wc -l < "$T/liste" | tr -d ' ')
	n_scripts=$(cut -d'|' -f1 "$T/liste" | sort -u | wc -l | tr -d ' ')
	SCRIPTS=()
fi
for s in "${SCRIPTS[@]}"; do
	n_scripts=$((n_scripts + 1))
	nom=$(basename "$(dirname "$s")")/$(basename "$s" .sh)
	bash -n "$s" || { ko "$nom : erreur de syntaxe bash"; continue; }
	grep -qE "^# (Métier|Boîte) *:" "$s" || ko "$nom : en-tête sans « Métier » ni « Boîte »"
	for champ in Besoin Enrobe Ports Terminal Usage; do
		grep -q "^# $champ *:" "$s" || ko "$nom : en-tête sans « $champ »"
	done
	grep -q '^\[ "${BASH_SOURCE\[0\]}" = "$0" \] && main "$@"$' "$s" || ko "$nom : garde de fin absente"
	[ -x "$s" ] || ko "$nom : pas exécutable"
	# sudo, pkexec ou eval en position de COMMANDE (début de ligne, après ; & | (
	# ou un mot-clé, ou en argument d'agir) — pas le mot dans un texte ou un nom
	# de groupe (« useradd -G sudo », « chevalet »).
	grep -qE '(^|[;&|(]|\b(then|do|else|agir|exec))[[:space:]]*(sudo|pkexec|eval)([[:space:]]|$)' \
		<(grep -v '^[[:space:]]*#' "$s") && ko "$nom : sudo, pkexec ou eval lancé par le script"
	# Les dialogues, lus sans lancer le script (fonctions seulement).
	d="$T/$(echo "$nom" | tr '/' '_')"; mkdir -p "$d"
	# La garde de fin vaut « faux » quand le script est lu : code 1, normal.
	bash -c 'source "$1" >/dev/null 2>&1
		declare -F main >/dev/null || exit 3
		for f in $(declare -F | awk "{print \$3}" | grep "^dialogue_"); do
			"$f" > "$2/${f#dialogue_}.xml" 2>/dev/null
		done' _ "$s" "$d" || { ko "$nom : le script ne se lit pas sans se lancer"; continue; }
	nd=0
	for x in "$d"/*.xml; do
		[ -s "$x" ] || { [ -e "$x" ] && ko "$nom : $(basename "$x" .xml) vide"; continue; }
		nd=$((nd + 1)); echo "$nom|$x" >> "$T/liste"
	done
	[ "$nd" -gt 0 ] || ko "$nom : aucune fonction dialogue_*"
	n_dialogues=$((n_dialogues + nd))
done

# ── 2 et 3, port par port, sous un seul écran virtuel ─────────────────────────
python3 -I - "$T/liste" <<'PY'
import sys
for l in open(sys.argv[1], encoding="utf-8"):
    nom, x = l.rstrip("\n").split("|", 1)
    s = open(x, encoding="utf-8").read()
    i = s.rfind("</vbox>")
    if i < 0:
        print(f"PAS-DE-VBOX {nom} {x}")
        continue
    t = '<timer visible="false" interval="1"><action>exit:essai</action></timer>\n'
    open(x[:-4] + ".essai", "w", encoding="utf-8").write(s[:i] + t + s[i:])
PY

# Jeu réel, sous un seul écran virtuel. La boucle est écrite dans un fichier :
# xvfb-run est un script sh, et dash retire les fonctions bash exportées.
cat > "$T/joue.sh" <<'JOUE'
T=$1; RACINE=$2; shift 2
for p in "$@"; do
	b="$RACINE/sermo-backend-$p/_build/${p}sermo"; [ -x "$b" ] || b=/usr/bin/${p}sermo
	[ -x "$b" ] || continue
	while IFS="|" read -r nom x; do
		e="${x%.xml}.essai"; [ -e "$e" ] || continue
		env SERMO_ALLOWED_CMDS=/sermo/banc-aucune-commande SERMO_HTML_OUT=/dev/null \
			QT_QPA_PLATFORM=xcb LC_ALL=fr_FR.UTF-8 HOME="$T" \
			timeout 30 "$b" --file="$e" </dev/null 2>/dev/null |
			grep -aE '^[A-Za-z_][A-Za-z0-9_]*="' | sort > "$x.$p.vals"
	done < "$T/liste"
done
JOUE
declare -A ok_port ko_port
for p in $PORTS; do
	b=$(binaire "$p")
	[ -n "$b" ] || { echo "port $p : binaire absent, sauté"; continue; }
	ok_port[$p]=0; ko_port[$p]=0
	while IFS='|' read -r nom x; do
		dn=$(basename "$x" .xml)
		if ! env -u DISPLAY -u WAYLAND_DISPLAY SERMO_HTML_OUT=/dev/null timeout 20 "$b" --print-ir --file="$x" >/dev/null 2>"$x.$p.ir"; then
			ko "$p $nom/$dn : l'analyseur refuse ($(grep -m1 -a 'Error' "$x.$p.ir" | cut -c1-120))"
			ko_port[$p]=$((ko_port[$p] + 1)); continue
		fi
		ok_port[$p]=$((ok_port[$p] + 1))
	done < "$T/liste"
done
# shellcheck disable=SC2086
if command -v xvfb-run >/dev/null 2>&1; then
	xvfb-run -a -s "-screen 0 1280x1024x24" bash "$T/joue.sh" "$T" "$RACINE" $PORTS 2>/dev/null
else
	bash "$T/joue.sh" "$T" "$RACINE" $PORTS 2>/dev/null   # sans écran : ncurses, html
fi

# Passe gardée : chemins relatifs, pour être rejouée ailleurs.
if [ -n "$GARDER" ]; then
	mkdir -p "$GARDER" && cp -R "$T"/. "$GARDER"/
	while IFS='|' read -r nom x; do echo "$nom|${x#"$T"/}"; done < "$T/liste" > "$GARDER/liste"
	echo "passe gardée : $GARDER ($(wc -l < "$GARDER/liste" | tr -d ' ') dialogues)"
fi
# Sans étalon ici : la comparaison se fera par --rejouer.
if [ -z "$ETALON" ] || ! echo " $PORTS " | grep -q ' gtk3 '; then
	if [ -z "$REJOUER" ]; then
		for p in $PORTS; do
			[ -n "${ok_port[$p]+x}" ] && echo "port $p : ${ok_port[$p]} dialogue(s) analysé(s), ${ko_port[$p]} refusé(s)"
		done
		echo "scripts : $n_scripts · dialogues : $n_dialogues · écarts : $ecarts (pas d'étalon ici)"
		[ "$ecarts" -eq 0 ] && { echo "verifie : joué, à comparer par --rejouer"; exit 0; }
		echo "verifie : $ecarts écart(s)"; exit 1
	fi
fi
# Reprise : les ports joués ailleurs (valeurs déjà là) se comparent aussi.
if [ -n "$REJOUER" ]; then
	for v in $(sed 's/.*|//' "$T/liste" | while read -r x; do ls "$x".*.vals 2>/dev/null; done \
	           | sed 's/.*\.\([a-z0-9]*\)\.vals$/\1/' | sort -u); do
		[ "$v" = gtk3 ] && continue
		echo " $PORTS " | grep -q " $v " && continue
		PORTS="$PORTS $v"; ok_port[$v]=0; ko_port[$v]=0
	done
fi

for p in $PORTS; do
	[ -n "${ok_port[$p]+x}" ] || continue
	[ "$p" = gtk3 ] && continue
	while IFS='|' read -r nom x; do
		dn=$(basename "$x" .xml); e="$x.gtk3.vals"; v="$x.$p.vals"
		[ -e "$v" ] || continue
		if ! grep -q '^EXIT="essai"$' "$e"; then
			continue   # l'étalon lui-même n'a pas fini : signalé plus bas
		fi
		# Tri en locale C des deux côtés : l'ordre de « sort » dépend de la
		# langue et du système (macOS et la glibc ne rangent pas pareil).
		attendu=$(grep -v '^EXIT=' "$e" | LC_ALL=C sort); obtenu=$(grep -v '^EXIT=' "$v" | LC_ALL=C sort)
		sortie=$(grep '^EXIT=' "$v")
		if [ "$p" != html ] && [ "$sortie" != 'EXIT="essai"' ]; then
			ko "$p $nom/$dn : le dialogue n'a pas fini par le minuteur (${sortie:-aucune sortie})"
		elif [ "$attendu" != "$obtenu" ]; then
			ko "$p $nom/$dn : valeurs différentes de gtk3sermo :"
			diff <(echo "$attendu") <(echo "$obtenu") | grep '^[<>]' | head -6 | sed 's/^/        /'
		fi
	done < "$T/liste"
done
while IFS='|' read -r nom x; do
	grep -q '^EXIT="essai"$' "$x.gtk3.vals" 2>/dev/null || ko "gtk3 $nom/$(basename "$x" .xml) : l'étalon n'a pas fini par le minuteur"
done < "$T/liste"

# ── Images, pour la relecture ─────────────────────────────────────────────────
if [ -n "$IMAGES" ]; then
	export T   # lu par le bash -c ci-dessous (oublié : 0 image produite)
	xvfb-run -a bash -c '
		while IFS="|" read -r nom x; do
			png="'"$IMAGES"'/$(echo "$nom" | tr "/" "_")_$(basename "$x" .xml).png"
			SERMO_ALLOWED_CMDS=/sermo/banc-aucune-commande timeout 20 "'"$ETALON"'" --render-png="$png" --file="$x" >/dev/null 2>&1
		done < "$T/liste"' 2>/dev/null
	echo "images : $(ls "$IMAGES"/*.png 2>/dev/null | wc -l) dans $IMAGES"
fi

for p in $PORTS; do
	[ -n "${ok_port[$p]+x}" ] && echo "port $p : ${ok_port[$p]} dialogue(s) analysé(s), ${ko_port[$p]} refusé(s)"
done
echo "scripts : $n_scripts · dialogues : $n_dialogues · écarts : $ecarts"
[ "$ecarts" -eq 0 ] && { echo "verifie : OK"; exit 0; }
echo "verifie : $ecarts écart(s)"; exit 1
