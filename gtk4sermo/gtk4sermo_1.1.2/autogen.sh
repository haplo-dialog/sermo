#!/bin/sh
# autogen.sh — gtk4sermo 1.0.0 (haplo-dialog)
# Régénère le système de build autotools depuis configure.ac, puis lance
# configure (sauf si NOCONFIGURE est défini, p. ex. à l'empaquetage Debian).
# Dépendances : autoconf >= 2.69, automake >= 1.14, pkg-config, flex, bison.
set -e

srcdir=$(cd "$(dirname "$0")" && pwd)
cd "$srcdir"

test -f configure.ac || {
    echo "**Error**: configure.ac introuvable dans $srcdir" >&2
    exit 1
}

echo "==> autoreconf -fiv"
autoreconf -fiv

if [ -n "$NOCONFIGURE" ]; then
    echo "==> NOCONFIGURE défini : configure ignoré."
    exit 0
fi

echo "==> ./configure $*"
./configure "$@"
