# Process this file with autoconf to produce a configure script.
# Modernised for GTK3 — 2026 (haplo-dialog 1.1.0)
# Renamed gtk3d → gtk3sermo (patch 13, 2026-05-22)
AC_PREREQ([2.69])
AC_INIT([gtk3sermo], [1.1.1], [devel@haplo-dialog.fr], [gtk3sermo],
        [https://haplo-dialog.fr])
AM_INIT_AUTOMAKE([foreign subdir-objects -Wall -Werror])
AC_CONFIG_HEADERS([config.h])

# Build details
# BUILD_DETAILS ne doit PAS repeter le nom ni la version : gtkdialog.c les
# imprime deja par PACKAGE_NAME et PACKAGE_VERSION juste avant. Avant le
# 2026-08-27 la chaine etait ecrite en dur et donnait « gtk3sermo version
# 1.1.0 gtk3sermo 1.1.0 » cote GTK 3, et « ... 1.1.0 1.1.0-gtk4 » cote GTK 4
# -- soit deux numeros a lire, ce que la 1.1.0 disait justement avoir supprime.
AC_DEFINE_UNQUOTED(BUILD_DETAILS, ["sermo"], [Build details])

# Checks for programs.
AC_PROG_CC
# AC_PROG_CC_STDC obsolète depuis autoconf 2.70 — remplacé par AC_PROG_CC
# AC_HEADER_STDC  obsolète depuis autoconf 2.70 — fonctionnalité intégrée dans AC_PROG_CC
AC_PROG_INSTALL
AC_PROG_LN_S
PKG_PROG_PKG_CONFIG
AM_PROG_LEX([noyywrap])
AC_PROG_YACC
AC_SUBST(YACC)

# ---------------------------------------------------------------------------
# Optional: --enable-debug
#   Activates the DEBUG_TRANSITS / DEBUG_CONTENT / PIP_DEBUG macros and
#   compiles without optimisation so that gdb/valgrind are usable.
# ---------------------------------------------------------------------------
AC_ARG_ENABLE([debug],
  AS_HELP_STRING([--enable-debug],
    [Enable verbose debug output (DEBUG_TRANSITS, DEBUG_CONTENT, PIP_DEBUG) and -g -O0]),
  [enable_debug=$enableval],
  [enable_debug=no])

if test "x$enable_debug" = "xyes"; then
    DEBUG_CFLAGS="-g -O0 -DDEBUG -DDEBUG_TRANSITS -DDEBUG_CONTENT"
    AC_MSG_NOTICE([Debug mode enabled — binary will be unoptimised])
else
    DEBUG_CFLAGS=""
fi
AC_SUBST([DEBUG_CFLAGS])

# Checks for libraries.
# GTK3 — replaces gtk+-2.0 and embeds gthread
pkg_modules="gtk+-3.0 >= 3.22.0"
PKG_CHECK_MODULES(GTK, [$pkg_modules])
AC_SUBST(GTK_CFLAGS)
AC_SUBST(GTK_LIBS)

# gthread is included in GTK3 glib — kept for compatibility
PKG_CHECK_MODULES(GTHREAD, [gthread-2.0],
    [],
    [AC_SUBST(GTHREAD_CFLAGS, "") AC_SUBST(GTHREAD_LIBS, "")])
AC_SUBST(GTHREAD_CFLAGS)
AC_SUBST(GTHREAD_LIBS)

# libglade is removed — GtkBuilder is built into GTK3.
# glade_support.c now uses GtkBuilder directly.
# AC_DEFINE puts HAVE_GLADE_LIB into config.h so #if HAVE_GLADE_LIB works in C.
AC_DEFINE([HAVE_GLADE_LIB], [1], [GtkBuilder-based glade support available])
AC_SUBST(GLADE_LIB_CFLAGS, "")
AC_SUBST(GLADE_LIB_LIBS, "")

# ---------------------------------------------------------------------------
# VTE terminal emulator — require 2.91 (matches GTK3)
#   Use --without-vte to build without terminal widget support.
# ---------------------------------------------------------------------------
AC_ARG_WITH([vte],
  AS_HELP_STRING([--without-vte], [Disable VTE terminal widget support]),
  [with_vte=$withval],
  [with_vte=yes])

if test "x$with_vte" = "xyes"; then
    vte_lib="vte-2.91"
    PKG_CHECK_MODULES([VTE], [$vte_lib],
        [AC_SUBST(HAVE_VTE, 1)
         AC_MSG_NOTICE([VTE terminal widget: enabled])],
        [AC_SUBST(HAVE_VTE, 0)
         AC_MSG_WARN([VTE library not found — terminal widget disabled])])
else
    AC_SUBST(HAVE_VTE, 0)
    AC_SUBST(VTE_CFLAGS, "")
    AC_SUBST(VTE_LIBS, "")
    AC_MSG_NOTICE([VTE terminal widget: disabled by --without-vte])
fi
AC_SUBST(VTE_CFLAGS)
AC_SUBST(VTE_LIBS)

# ---------------------------------------------------------------------------
# gtk-layer-shell - Wayland wlr-layer-shell surfaces (layer=/edge=/dist=)
#   Use --without-layer-shell to build without it.  Without the library the
#   three attributes are simply ignored at runtime.
#   >= 0.9 additionally provides gtk_layer_is_supported(), which is the only
#   safe way to detect a Wayland compositor that lacks the protocol.
# ---------------------------------------------------------------------------
AC_ARG_WITH([layer-shell],
  AS_HELP_STRING([--without-layer-shell],
    [Disable Wayland layer-shell window anchoring]),
  [with_layer_shell=$withval],
  [with_layer_shell=yes])

AC_SUBST(HAVE_LAYER_SHELL_IS_SUPPORTED, 0)
if test "x$with_layer_shell" = "xyes"; then
    PKG_CHECK_MODULES([LAYER_SHELL], [gtk-layer-shell-0 >= 0.8.0],
        [AC_SUBST(HAVE_LAYER_SHELL, 1)
         AC_MSG_NOTICE([Wayland layer-shell: enabled])
         PKG_CHECK_EXISTS([gtk-layer-shell-0 >= 0.9.0],
            [AC_SUBST(HAVE_LAYER_SHELL_IS_SUPPORTED, 1)],
            [AC_MSG_WARN([gtk-layer-shell < 0.9, falling back to a Wayland-display probe])])],
        [AC_SUBST(HAVE_LAYER_SHELL, 0)
         AC_MSG_WARN([gtk-layer-shell not found, layer/edge/dist attributes disabled])])
else
    AC_SUBST(HAVE_LAYER_SHELL, 0)
    AC_SUBST(LAYER_SHELL_CFLAGS, "")
    AC_SUBST(LAYER_SHELL_LIBS, "")
    AC_MSG_NOTICE([Wayland layer-shell: disabled by --without-layer-shell])
fi
AC_SUBST(LAYER_SHELL_CFLAGS)
AC_SUBST(LAYER_SHELL_LIBS)

# ---------------------------------------------------------------------------
# Tests unitaires C — libcheck
#   --enable-unit-tests  Compile test_safe_exec + test_stringman
#   (test_safe_exec est toujours compilé si check est disponible)
# ---------------------------------------------------------------------------
AC_ARG_ENABLE([unit-tests],
  AS_HELP_STRING([--enable-unit-tests],
    [Compile les tests unitaires C (nécessite libcheck-dev + libgtk-3-dev)]),
  [enable_unit_tests=$enableval],
  [enable_unit_tests=no])

PKG_CHECK_MODULES([CHECK], [check >= 0.9.6],
    [have_check=yes
     AC_MSG_NOTICE([libcheck trouvée — tests unitaires disponibles (make check)])],
    [have_check=no
     AC_MSG_WARN([libcheck non trouvée — tests unitaires C désactivés (run_tests.sh reste disponible)])])
AC_SUBST(CHECK_CFLAGS)
AC_SUBST(CHECK_LIBS)

# GLib seul suffit pour test_safe_exec
PKG_CHECK_MODULES([GLIB], [glib-2.0 >= 2.56],
    [],
    [AC_MSG_ERROR([GLib 2.56+ requis])])
AC_SUBST(GLIB_CFLAGS)
AC_SUBST(GLIB_LIBS)

AM_CONDITIONAL([ENABLE_UNIT_TESTS],
    [test "x$enable_unit_tests" = "xyes" && test "x$have_check" = "xyes"])

# Checks for header files.
AC_CHECK_HEADERS([locale.h])

# Checks for library functions.
AC_CHECK_FUNCS([setlocale])

# ---------------------------------------------------------------------------
# --enable-gtkdialog-alias : pose /usr/bin/gtkdialog -> gtk3sermo a l'install.
# ETEINT par defaut : « make install » ne doit pas ecraser en silence un
# gtkdialog fourni par la distribution. Le paquet gtksermo est la voie propre.
# ---------------------------------------------------------------------------
AC_ARG_ENABLE([gtkdialog-alias],
  AS_HELP_STRING([--enable-gtkdialog-alias],
    [install the gtkdialog compatibility symlink (default: no)]),
  [enable_gtkdialog_alias=$enableval],
  [enable_gtkdialog_alias=no])
AM_CONDITIONAL([ALIAS_GTKDIALOG], [test "x$enable_gtkdialog_alias" = "xyes"])

AC_CONFIG_FILES([
    Makefile
    gtk3sermo.spec
    src/Makefile
    src/gtk3sermo.1
    src/haplo-dialog-xml.5
    doc/Makefile
    data/Makefile
    data/icons/Makefile
    data/icons/hicolor/Makefile
    data/icons/hicolor/32x32/Makefile
    data/icons/hicolor/32x32/apps/Makefile
    tests/Makefile
])

# ---------------------------------------------------------------------------
# Hardening flags — vérifiés à la compilation
# ---------------------------------------------------------------------------
AC_SUBST([HARDENING_CFLAGS],
  ["-fstack-protector-strong -fstack-clash-protection -fcf-protection=full -U_FORTIFY_SOURCE -D_FORTIFY_SOURCE=3 -D_POSIX_C_SOURCE=200809L"])
AC_SUBST([HARDENING_LDFLAGS],
  ["-pie -Wl,-z,relro -Wl,-z,now -Wl,-z,noexecstack"])

AC_OUTPUT
