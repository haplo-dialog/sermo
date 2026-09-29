#!/bin/bash
# SPDX-License-Identifier: GPL-2.0-or-later
# tests/garde_lecture_input.sh — haplo-dialog — 2026 — GPL-2.0-or-later
#
# 2.7.3 : toute lecture d'un <input> est plafonnée (libsermocore/include/
# sermo_input.h) — une commande par safe_popen(), un fichier par
# sermo_fopen_input(). Un lecteur qui ouvrirait un fichier par fopen(…, "r"),
# open(…, O_RDONLY) ou g_file_get_contents() échapperait à la limite : c'est
# ainsi que le <edit> de gtk3 et gtk4 lisait encore un fichier de 8 Go d'un seul
# tenant, et tombait (mesuré le 2026-09-24).
#
# Ce garde relève ces ouvertures dans le cœur et les ports, et refuse toute
# ouverture absente de la liste PERMIS ci-dessous : des lectures qui ne sont pas
# des <input>, chacune avec sa raison. Il est statique : une lecture ajoutée
# dans un widget qu'aucun banc n'ouvre sur /dev/zero serait vue quand même.
#
# Usage : garde_lecture_input.sh [racine du dépôt]
# Codes : 0 = propre · 1 = une lecture non plafonnée · 2 = racine illisible

set -u
export LC_ALL=C
RACINE=$(CDPATH= cd -- "${1:-$(dirname -- "$0")/..}" 2>/dev/null && pwd) \
    || { echo "racine illisible : ${1:-}" >&2; exit 2; }

# « fichier|ligne de code » → nombre d'occurrences permises.
declare -A PERMIS=(
    # Le script XML lui-même (--file) : il vient de l'auteur, ce n'est pas un <input>.
    ['libsermocore/src/gtkdialog.c|sourcefile = fopen(name, "r");']=2
    ['libsermocore/src-gtk4/gtkdialog.c|sourcefile = fopen(name, "r");']=2
    # La condition « if file() » : une ligne de 64 octets au plus (fgets borné).
    ['libsermocore/src/signals.c|if ((infile = fopen(argument, "r"))) {']=1
    ['libsermocore/src-gtk4/signals.c|if ((infile = fopen(argument, "r"))) {']=1
    # Les fichiers du thème d'icônes (index.theme, réglages GTK).
    ['libsermocore/src/sermo_icon_theme.c|f = fopen(path, "r");']=1
    ['libsermocore/src/sermo_icon_theme.c|if ((f = fopen(path, "r"))) {']=4
    # Le tube de safe_popen(), plafonné à la ligne suivante (sermo_input_wrap).
    ['libsermocore/src/safe_exec.c|stream = fdopen(stdout_fd, "r");']=1
    ['libsermocore/src-gtk4/safe_exec.c|stream = fdopen(stdout_fd, "r");']=1
    # L'ouvreur plafonné lui-même.
    ['libsermocore/src/sermo_input.c|return sermo_input_wrap(fopen(chemin, "r"), chemin);']=1
    # fltk1 : les réglages de thème du bureau (clair ou sombre).
    ['sermo-backend-fltk1/src/fltk-compat.cpp|FILE *f = fopen(path.c_str(), "r");']=1
)

MOTIF='\bfopen[[:space:]]*\(.*"r|\bfdopen[[:space:]]*\(.*"r|\bopen[[:space:]]*\(.*O_RDONLY|g_file_get_contents|g_file_load_contents|g_mapped_file_new|std::ifstream|QIODevice::ReadOnly'

# releve <racine> : « fichier|ligne » de chaque ouverture en lecture, commentaires ôtés.
releve() {
    local r=$1 f
    ( cd "$r" && find libsermocore/src libsermocore/src-gtk4 sermo-backend-*/src \
          -path '*/imgui' -prune -o -type f \( -name '*.c' -o -name '*.cpp' -o -name '*.cc' -o -name '*.h' \) -print 2>/dev/null ) \
    | sort | while read -r f; do
        sed -e 's#/\*.*\*/##g' -e 's#//.*$##' -e '/^[[:space:]]*\*/d' -e '/^[[:space:]]*\/\*/d' "$r/$f" \
            | grep -E "$MOTIF" | sed -e 's/^[[:space:]]*//' -e 's/[[:space:]]*$//' \
            | sed "s#^#$f|#"
    done
}

# Témoin : une lecture plantée dans un faux port doit être vue.
TEMOIN=$(mktemp -d); trap 'rm -rf "$TEMOIN"' EXIT
mkdir -p "$TEMOIN/libsermocore/src" "$TEMOIN/sermo-backend-faux/src"
printf 'static void lire(void)\n{\n\tFILE *fp = fopen(chemin, "r");  /* un <input file> */\n}\n' \
    > "$TEMOIN/sermo-backend-faux/src/widget_faux.c"
if ! releve "$TEMOIN" | grep -q '^sermo-backend-faux/src/widget_faux.c|FILE \*fp = fopen(chemin, "r");$'; then
    echo "ECHEC DU TEMOIN : une lecture fopen(…, \"r\") plantée n'est pas vue"; exit 1
fi

declare -A VU=()
lues=0
while IFS= read -r cle; do
    [ -n "$cle" ] || continue
    VU[$cle]=$(( ${VU[$cle]:-0} + 1 )); lues=$((lues + 1))
done < <(releve "$RACINE")
[ "$lues" -gt 0 ] || { echo "ECHEC : aucune ouverture relevée — le garde ne lit rien sous $RACINE"; exit 1; }

echecs=0
for cle in "${!VU[@]}"; do
    permis=${PERMIS[$cle]:-0}
    if [ "${VU[$cle]}" -gt "$permis" ]; then
        echo "ECHEC : lecture non plafonnée (${VU[$cle]} vue(s), $permis permise(s)) : ${cle%%|*}"
        echo "        ${cle#*|}"
        echecs=$((echecs + 1))
    fi
done
for cle in "${!PERMIS[@]}"; do
    [ -n "${VU[$cle]:-}" ] || echo "note : permis sans usage (le code a changé ?) : $cle"
done
if [ "$echecs" -gt 0 ]; then
    echo "Un <input file> se lit par sermo_fopen_input(), une commande par widget_opencommand()."
    exit 1
fi
echo "OK : $lues ouverture(s) en lecture, toutes hors <input> et nommées ; le témoin planté est vu"
