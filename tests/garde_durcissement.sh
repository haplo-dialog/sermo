#!/bin/bash
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
ok=0
verifie() { if [[ "$2" == oui ]]; then ok=$((ok+1)); else echecs+=("$1"); fi; }

verifie "PIE"             "$(LC_ALL=C readelf -hW "$BIN" | grep -q 'Type:[[:space:]]*DYN' && echo oui)"
verifie "RELRO"           "$(LC_ALL=C readelf -lW "$BIN" | grep -q 'GNU_RELRO' && echo oui)"
verifie "BIND_NOW"        "$(LC_ALL=C readelf -dW "$BIN" | grep -qE 'BIND_NOW|FLAGS.*NOW' && echo oui)"
verifie "pile non executable" "$(LC_ALL=C readelf -lW "$BIN" | grep -A1 'GNU_STACK' | grep -q 'RWE' || echo oui)"
verifie "CET IBT"         "$(LC_ALL=C readelf -nW "$BIN" | grep -q 'IBT' && echo oui)"
verifie "CET SHSTK"       "$(LC_ALL=C readelf -nW "$BIN" | grep -q 'SHSTK' && echo oui)"
verifie "stack protector" "$(LC_ALL=C readelf -sW "$BIN" | grep -q '__stack_chk_fail' && echo oui)"
verifie "FORTIFY"         "$(LC_ALL=C readelf -sW "$BIN" | grep -qE '__[a-z]+_chk' && echo oui)"

if (( ${#echecs[@]} > 0 )); then
    echo "ECHEC : ${#echecs[@]} garantie(s) de durcissement absente(s) du binaire :"
    printf '    %s\n' "${echecs[@]}"
    exit 1
fi
echo "OK : $ok garanties de durcissement mesurees sur le binaire"
