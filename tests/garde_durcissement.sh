#!/bin/bash
# SPDX-License-Identifier: GPL-2.0-or-later
# tests/garde_durcissement.sh — haplo-dialog — 2026 — GPL-2.0-or-later
#
# SECURITY.md promet un tableau de durcissement. Ce banc le mesure sur le
# BINAIRE PRODUIT, pas sur les drapeaux annonces : un flag pose dans un
# Makefile ne prouve rien s'il ne survit pas a l'edition de liens. C'est
# exactement ce qui s'etait passe pour CET — -fcf-protection=full etait bien
# la, l'instrumentation ENDBR aussi, et le binaire n'avait pourtant AUCUNE
# propriete IBT/SHSTK, faute de -Wl,-z,ibt -Wl,-z,shstk.
BIN="${1:?usage: garde_durcissement.sh /chemin/vers/binaire}"
[[ -x "$BIN" ]] || { echo "binaire introuvable : $BIN" >&2; exit 2; }
command -v readelf >/dev/null || { echo "outil manquant : readelf" >&2; exit 2; }

echecs=()

# FORTIFY laisse des appels __*_chk (__snprintf_chk, __memcpy_chk…). ATTENTION :
# __stack_chk_fail, qui vient du protecteur de pile, a la meme forme. Une premiere
# version de ce banc le comptait : elle repondait OK sur des binaires dont AUCUN
# objet n'etait fortifie (cœur compile sans -O). On l'exclut nommement.
fortify_present() {
    LC_ALL=C readelf -sW "$1" | grep -oE '__[a-z0-9_]+_chk' | grep -vqE '^__stack_chk' && echo oui
}

# Temoin : le banc doit savoir distinguer un binaire fortifie d'un binaire qui ne
# l'est pas mais porte __stack_chk_fail. Sans compilateur, on le dit.
if command -v cc >/dev/null; then
    T=$(mktemp -d); trap 'rm -rf "$T"' EXIT
    # Un snprintf dans un tampon de taille connue ne suffit pas : le compilateur
    # prouve qu'il ne deborde pas et retire le controle. fprintf et une copie de
    # longueur inconnue restent controles.
    printf '#include <stdio.h>\n#include <string.h>\nstatic void cp(char *d, const char *s, size_t n){ memcpy(d, s, n); }\nint main(int c, char **v){ char b[64]; (void)c; cp(b, v[0], strlen(v[0]) + 1); fprintf(stderr, "%%s\\n", b); return 0; }\n' > "$T/t.c"
    cc -O2 -D_FORTIFY_SOURCE=3 -fstack-protector-all -o "$T/fort" "$T/t.c" 2>/dev/null
    cc -O0 -U_FORTIFY_SOURCE -fstack-protector-all -o "$T/nu" "$T/t.c" 2>/dev/null
    if [[ "$(fortify_present "$T/fort")" != oui || -n "$(fortify_present "$T/nu")" ]]; then
        echo "ECHEC DU TEMOIN : la mesure FORTIFY ne distingue pas un binaire fortifie d'un binaire nu" >&2
        exit 1
    fi
else
    echo "(temoin FORTIFY non joue : pas de compilateur cc)" >&2
fi
ok=0
verifie() { if [[ "$2" == oui ]]; then ok=$((ok+1)); else echecs+=("$1"); fi; }

verifie "PIE"             "$(LC_ALL=C readelf -hW "$BIN" | grep -q 'Type:[[:space:]]*DYN' && echo oui)"
verifie "RELRO"           "$(LC_ALL=C readelf -lW "$BIN" | grep -q 'GNU_RELRO' && echo oui)"
verifie "BIND_NOW"        "$(LC_ALL=C readelf -dW "$BIN" | grep -qE 'BIND_NOW|FLAGS.*NOW' && echo oui)"
verifie "pile non executable" "$(LC_ALL=C readelf -lW "$BIN" | grep -A1 'GNU_STACK' | grep -q 'RWE' || echo oui)"
verifie "CET IBT"         "$(LC_ALL=C readelf -nW "$BIN" | grep -q 'IBT' && echo oui)"
verifie "CET SHSTK"       "$(LC_ALL=C readelf -nW "$BIN" | grep -q 'SHSTK' && echo oui)"
verifie "stack protector" "$(LC_ALL=C readelf -sW "$BIN" | grep -q '__stack_chk_fail' && echo oui)"
verifie "FORTIFY"         "$(fortify_present "$BIN")"

if (( ${#echecs[@]} > 0 )); then
    echo "ECHEC : ${#echecs[@]} garantie(s) de durcissement absente(s) du binaire :"
    printf '    %s\n' "${echecs[@]}"
    exit 1
fi
echo "OK : $ok garanties de durcissement mesurees sur le binaire"
