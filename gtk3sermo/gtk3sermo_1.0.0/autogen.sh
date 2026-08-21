#!/bin/sh
# autogen.sh — gtk3sermo 1.0.0 (haplo-dialog)
#
# Regenerates the autotools build system from scratch using autoreconf,
# then optionally runs configure.
#
# NOTE: autotools (AM_SANITY_CHECK) refuses to work in a directory whose
# absolute path contains spaces or shell metacharacters (& ; | < > ` $ …).
# If the source tree lives in such a path this script automatically:
#   1. creates a clean symlink  /tmp/gtk3sermo-src  →  <real srcdir>
#   2. runs configure from a dedicated build directory  /tmp/gtk3sermo-build/
#      (override with GTK3DIALOG_BUILDDIR=/your/path ./autogen.sh)
# The resulting Makefiles will reference the real sources through the symlink;
# 'make install' works normally.  'make distcheck' must be run from the build
# directory.
#
# Usage:
#   ./autogen.sh [configure options...]
#   NOCONFIGURE=1 ./autogen.sh           # regen only — skip configure
#   GTK3DIALOG_BUILDDIR=/opt/b ./autogen.sh  # custom build dir
#
# Dependencies: autoconf >= 2.69, automake >= 1.14, pkg-config, flex, bison

set -e

# Resolve the real absolute path of the source tree (follows symlinks).
srcdir=$(cd "$(dirname "$0")" && pwd)

echo "==> Running autoreconf -fiv ..."
(cd "$srcdir" && autoreconf -fiv)

if [ -n "$NOCONFIGURE" ]; then
    echo "==> Skipping configure (NOCONFIGURE is set)."
    exit 0
fi

# -----------------------------------------------------------------------
# Safety check: does the source path contain shell metacharacters?
# autotools' AM_SANITY_CHECK bails out on spaces, &, (, ), ; | < > ` $
# -----------------------------------------------------------------------
safe_srcdir="$srcdir"
case "$srcdir" in
    *\ *|*\&*|*\(*|*\)*|*\;*|*\|*|*\<*|*\>*|*\`*|*\$*)
        safe_srcdir="/tmp/gtk3sermo-src"
        rm -f "$safe_srcdir"
        ln -s "$srcdir" "$safe_srcdir"
        echo ""
        echo "NOTE: Source path contains shell metacharacters:"
        echo "        $srcdir"
        echo "      Created safe symlink for autotools:"
        echo "        $safe_srcdir -> $srcdir"
        ;;
esac

# -----------------------------------------------------------------------
# Choose build directory
# -----------------------------------------------------------------------
if [ "$safe_srcdir" != "$srcdir" ] || [ -n "$GTK3DIALOG_BUILDDIR" ]; then
    builddir="${GTK3DIALOG_BUILDDIR:-/tmp/gtk3sermo-build}"
    mkdir -p "$builddir"
    echo "==> Out-of-tree build directory: $builddir"
    echo "    (Run 'make' and 'make install' from that directory)"
    echo ""
    # If the source tree was previously configured in-tree, autoconf refuses to
    # run an out-of-tree configure ("source directory already configured").
    # We cannot use 'make distclean' because the stale Makefile may reference
    # a configure path that no longer exists (e.g. a CI/sandbox path).
    # Instead, remove the key artifacts directly.
    if [ -f "$srcdir/config.status" ] || \
       [ -f "$srcdir/src/gtk3sermo" ] || \
       [ -f "$srcdir/src/gtk3sermo.o" ]; then
        echo "==> Removing stale in-tree build artifacts ..."
        rm -f "$srcdir/config.status" "$srcdir/config.log" "$srcdir/config.h"
        find "$srcdir" -maxdepth 4 -name "Makefile" \
             ! -path "$srcdir/debian/*" -delete 2>/dev/null || true
        # Remove compiled objects and binary: if left in the source tree they
        # would be found by VPATH during an out-of-tree build and trick make
        # into believing the target is already up to date (skip recompile).
        find "$srcdir/src" -maxdepth 1 \
             \( -name "*.o" -o -name "*.lo" -o -name "*.a" \
                -o -name "gtk3sermo" -o -name "gtk3sermo.exe" \) \
             -delete 2>/dev/null || true
        echo "    Done."
    fi
else
    builddir="$srcdir"
fi

if [ -z "$*" ]; then
    echo "Note: running configure with no extra arguments."
    echo "Pass options to $(basename "$0") to forward them to configure."
    echo "  e.g.: $(basename "$0") --prefix=/usr --enable-debug"
    echo ""
fi

echo "==> Running configure $* ..."
cd "$builddir"
"$safe_srcdir/configure" "$@"

echo ""
if [ "$builddir" != "$srcdir" ]; then
    echo "Done. Build directory: $builddir"
    echo "Run: make -j\$(nproc)"
    echo "     make install    (or: sudo make install)"
    echo "     make check      (unit + shell tests)"
else
    echo "Done. Run 'make -j\$(nproc)' to build."
fi
