# tests/Makefile.am — Suite de tests gtk3dialog 1.0.0
#
# Dépendances :
#   libcheck-dev  (pkg-config name: check)
#   libglib2.0-dev
#   libgtk-3-dev  (pour test_stringman uniquement)
#
# Utilisation :
#   make check         — compile et exécute tous les tests
#   make check-shell   — exécute uniquement run_tests.sh
#
# Les tests unitaires C sont basés sur la bibliothèque Check et produisent
# une sortie TAP compatible avec automake.
#
# haplo-dialog — 2026 — GPL-2.0-or-later

# ---------------------------------------------------------------------------
# Flags communs
# ---------------------------------------------------------------------------
AM_CFLAGS  = -Wall -Wextra -g $(GLIB_CFLAGS)
AM_LDFLAGS =

# ---------------------------------------------------------------------------
# Test 1 : test_safe_exec (ne dépend que de GLib — pas de GTK)
# ---------------------------------------------------------------------------
check_PROGRAMS = test_safe_exec

test_safe_exec_SOURCES  = test_safe_exec.c ../src/safe_exec.c
test_safe_exec_CFLAGS   = $(AM_CFLAGS) $(CHECK_CFLAGS) -I$(top_srcdir)/src
test_safe_exec_LDADD    = $(CHECK_LIBS) $(GLIB_LIBS)

# ---------------------------------------------------------------------------
# Test 2 : test_stringman (dépend de GTK3 + tout le codebase)
# Compilé conditionnellement si CHECK et GTK sont disponibles.
# Désactivé par défaut pour éviter une dépendance de compilation lourde.
# Pour activer : ./configure --enable-unit-tests
# ---------------------------------------------------------------------------
if ENABLE_UNIT_TESTS
check_PROGRAMS       += test_stringman
test_stringman_SOURCES = test_stringman.c ../src/stringman.c
test_stringman_CFLAGS  = $(AM_CFLAGS) $(CHECK_CFLAGS) $(GTK_CFLAGS) \
                          -I$(top_srcdir)/src
test_stringman_LDADD   = $(CHECK_LIBS) $(GTK_LIBS) $(GLIB_LIBS)
endif

# ---------------------------------------------------------------------------
# Tests shell (run_tests.sh) — intégration make check
# ---------------------------------------------------------------------------
TESTS          = $(check_PROGRAMS) check-shell.sh
TEST_EXTENSIONS = .sh
SH_LOG_COMPILER = bash

# Génère check-shell.sh comme wrapper autour de run_tests.sh
check-shell.sh: run_tests.sh
	echo '#!/bin/bash' > $@
	echo 'exec "$(srcdir)/run_tests.sh"' >> $@
	chmod +x $@

CLEANFILES = check-shell.sh

# ---------------------------------------------------------------------------
# Distribution
# ---------------------------------------------------------------------------
EXTRA_DIST = \
	run_tests.sh \
	test_safe_exec.c \
	test_stringman.c \
	README.tests
