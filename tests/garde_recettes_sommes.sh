#!/bin/sh
# SPDX-License-Identifier: GPL-2.0-or-later
# tests/garde_recettes_sommes.sh — haplo-dialog — 2026
#
# Les recettes qui téléchargent l'archive de l'étiquette vérifient ce qu'elles
# téléchargent. Jusqu'à la 3.3.0, Arch (sha256sums=('SKIP')) et Termux
# (SKIP_CHECKSUM) construisaient n'importe quelle archive servie à cette
# adresse ; seule la formule Homebrew portait une somme.
#
# Vérifie, sans réseau :
#   - Homebrew, Arch et Termux portent une somme SHA-256 (64 chiffres hexa),
#     pas « SKIP » ;
#   - elles annoncent la même version et, tirant la même archive, la même somme.
#   - Alpine porte une somme SHA-512 (128 chiffres hexa) pour la même version ;
#   - le distinfo d'OpenBSD porte la même SHA-256, en base64, et une taille.
# FreeBSD n'est pas jugé : son port tire l'archive par l'API de GitLab
# (USE_GITLAB), une autre archive, dont la somme se pose par « make makesum ».
# Ne vérifie PAS que la somme est celle de l'archive en ligne (il faudrait la
# télécharger) : c'est le rôle de chaque outil de construction.
#
# Usage : garde_recettes_sommes.sh [racine]
# Codes : 0 = tient · 1 = défaut · 2 = recette introuvable
set -u
RACINE=$(CDPATH= cd -- "${1:-$(dirname -- "$0")/..}" && pwd) || exit 2
D="$RACINE/packaging/autres-os"
for f in homebrew/sermo.rb arch/PKGBUILD termux/build.sh; do
    [ -f "$D/$f" ] || { echo "recette introuvable : $f" >&2; exit 2; }
done
v_brew=$(sed -n 's|^ *url ".*/archive/v\([0-9.]*\)/.*|\1|p' "$D/homebrew/sermo.rb")
s_brew=$(sed -n 's|^ *sha256 "\(.*\)"|\1|p' "$D/homebrew/sermo.rb")
v_arch=$(sed -n 's|^pkgver=||p' "$D/arch/PKGBUILD")
s_arch=$(sed -n "s|^sha256sums=('\(.*\)')|\1|p" "$D/arch/PKGBUILD")
v_termux=$(sed -n 's|^TERMUX_PKG_VERSION=||p' "$D/termux/build.sh")
s_termux=$(sed -n 's|^TERMUX_PKG_SHA256=||p' "$D/termux/build.sh")
rc=0
for r in brew arch termux; do
    eval "v=\$v_$r; s=\$s_$r"
    case "$s" in
        *[!0-9a-f]*|'') echo "ECHEC : $r : somme « $s » — 64 chiffres hexadécimaux attendus"; rc=1 ;;
        *) [ ${#s} -eq 64 ] || { echo "ECHEC : $r : somme de ${#s} caractères (64 attendus)"; rc=1; } ;;
    esac
    [ -n "$v" ] || { echo "ECHEC : $r : version illisible"; rc=1; }
done
[ "$v_arch" = "$v_brew" ] && [ "$v_termux" = "$v_brew" ] \
    || { echo "ECHEC : versions différentes (Homebrew $v_brew, Arch $v_arch, Termux $v_termux)"; rc=1; }
[ "$s_arch" = "$s_brew" ] && [ "$s_termux" = "$s_brew" ] \
    || { echo "ECHEC : même archive, sommes différentes (Homebrew, Arch, Termux)"; rc=1; }
v_alpine=$(sed -n 's|^pkgver=||p' "$D/alpine/APKBUILD")
s_alpine=$(sed -n '/^sha512sums="/,/^"/s|^\([0-9a-f]\{1,\}\)  sermo-.*|\1|p' "$D/alpine/APKBUILD")
[ ${#s_alpine} -eq 128 ] || { echo "ECHEC : alpine : somme SHA-512 absente ou mal formée (${#s_alpine} caractères)"; rc=1; }
[ "$v_alpine" = "$v_brew" ] || { echo "ECHEC : alpine : version $v_alpine, Homebrew $v_brew"; rc=1; }
grep -q "^$s_alpine  sermo-$v_alpine\.tar\.gz\$" "$D/alpine/APKBUILD" \
    || { echo "ECHEC : alpine : la somme ne nomme pas sermo-$v_alpine.tar.gz"; rc=1; }
O="$D/openbsd/devel/sermo"
v_obsd=$(sed -n 's|^V=[[:space:]]*||p' "$O/Makefile")
[ "$v_obsd" = "$v_brew" ] || { echo "ECHEC : openbsd : version $v_obsd, Homebrew $v_brew"; rc=1; }
if [ -f "$O/distinfo" ] && command -v python3 >/dev/null 2>&1; then
    attendu=$(python3 -I -c 'import sys,base64; print(base64.b64encode(bytes.fromhex(sys.argv[1])).decode())' "$s_brew" 2>/dev/null)
    grep -qxF "SHA256 (sermo-v$v_obsd.tar.gz) = $attendu" "$O/distinfo" \
        || { echo "ECHEC : openbsd : distinfo ne porte pas la SHA-256 de l'archive (base64)"; rc=1; }
    grep -q "^SIZE (sermo-v$v_obsd.tar.gz) = [1-9][0-9]*\$" "$O/distinfo" \
        || { echo "ECHEC : openbsd : distinfo sans taille"; rc=1; }
else
    echo "ECHEC : openbsd : distinfo absent (ou python3 manquant pour le lire)"; rc=1
fi
[ $rc -eq 0 ] && echo "OK : Homebrew, Arch, Termux, Alpine et OpenBSD : version $v_brew, sommes posées et d'accord"
exit $rc
