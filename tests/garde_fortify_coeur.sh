#!/bin/bash
# SPDX-License-Identifier: GPL-2.0-or-later
# tests/garde_fortify_coeur.sh — le CŒUR est-il réellement fortifié ?
#
# POURQUOI CE BANC EXISTE
#
# garde_durcissement.sh mesure un binaire entier. Sur qt6, sdl3 et ncurses, le
# backend est optimisé : il apporte ses propres appels __*_chk, et le binaire
# paraît fortifié même quand le cœur ne l'est pas. C'est ce qui s'est produit :
# libsermocore compilait sans aucun -O, _FORTIFY_SOURCE=3 n'y faisait rien, et
# aucun banc ne le voyait. Celui-ci regarde la bibliothèque du cœur elle-même.
#
# Usage : garde_fortify_coeur.sh <libsermocore.a>
# Codes : 0 = cœur fortifié · 1 = cœur non fortifié (ou témoin raté) · 2 = usage

set -u
LIB="${1:-}"
[[ -f "$LIB" ]] || { echo "usage: $0 <libsermocore.a>" >&2; exit 2; }
command -v nm >/dev/null || { echo "outil manquant : nm" >&2; exit 2; }

# Nombre d'objets de l'archive qui appellent au moins une fonction __*_chk,
# __stack_chk_fail exclu (il vient du protecteur de pile, pas de FORTIFY).
objets_fortifies() {
    LC_ALL=C nm -A "$1" 2>/dev/null \
        | grep -E ' U __[a-z0-9_]+_chk$' | grep -v '__stack_chk' \
        | cut -d: -f2 | sort -u | wc -l
}

# Témoin : la mesure doit voir la différence entre une archive compilée avec et
# sans optimisation. Sans compilateur, on le dit au lieu de conclure.
if command -v cc >/dev/null && command -v ar >/dev/null; then
    T=$(mktemp -d); trap 'rm -rf "$T"' EXIT
    # Un snprintf dans un tampon de taille connue ne suffit pas : le compilateur
    # prouve qu'il ne déborde pas et retire le contrôle (constaté avec gcc 16).
    # fprintf et une copie de longueur inconnue restent contrôlés.
    printf '#include <stdio.h>\n#include <string.h>\nvoid f(const char *s, size_t n){ char b[64]; memcpy(b, s, n); fprintf(stderr, "%%.*s\\n", (int)n, b); }\n' > "$T/t.c"
    cc -c -O2 -D_FORTIFY_SOURCE=3 -fstack-protector-all -o "$T/fort.o" "$T/t.c" 2>/dev/null && ar rcs "$T/fort.a" "$T/fort.o"
    cc -c -O0 -D_FORTIFY_SOURCE=3 -fstack-protector-all -o "$T/nu.o" "$T/t.c" 2>/dev/null && ar rcs "$T/nu.a" "$T/nu.o"
    if [[ "$(objets_fortifies "$T/fort.a")" -lt 1 || "$(objets_fortifies "$T/nu.a")" -ne 0 ]]; then
        echo "ÉCHEC DU TÉMOIN : la mesure ne distingue pas une archive fortifiée d'une archive compilée sans -O" >&2
        exit 1
    fi
else
    echo "(témoin non joué : pas de cc/ar)" >&2
fi

total=$(ar t "$LIB" | wc -l)
fort=$(objets_fortifies "$LIB")
if [[ "$fort" -lt 1 ]]; then
    echo "ÉCHEC : aucun des $total objets de $(basename "$LIB") n'appelle une fonction fortifiée (__*_chk)."
    echo "Le cœur a probablement été compilé sans optimisation : _FORTIFY_SOURCE n'agit qu'avec -O1 ou plus."
    exit 1
fi
echo "OK : $fort objet(s) sur $total appellent des fonctions fortifiées dans $(basename "$LIB")."
