AC_PREREQ([2.69])
# Process this file with autoconf to produce a configure script.
# Port GTK4 — haplo-dialog — 2026
AC_INIT([gtk4sermo], [1.1.4], [devel@haplo-dialog.fr], [gtk4sermo])
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
AM_PROG_LEX([noyywrap])
AC_PROG_YACC
AC_SUBST(YACC)

# Checks for libraries.
# GTK4 — requires 4.6.0 minimum (stable, widely packaged on Debian Testing)
pkg_modules="gtk4 >= 4.6.0"
PKG_CHECK_MODULES(GTK, [$pkg_modules])

# X11 : nécessaire au placement de fenêtre (XMoveWindow), que GTK 4 a retiré
# côté client. FACULTATIF — sans lui le placement avertit au lieu d'agir, et
# rien d'autre ne change. Aucune dépendance nouvelle en pratique :
# libgtk-4-dev dépend déjà de libx11-dev, et libX11.so.6 est déjà liée au
# binaire par GTK.
PKG_CHECK_MODULES([X11], [x11],
  [AC_DEFINE([HAVE_X11], [1], [Placement de fenêtre disponible via XMoveWindow])],
  [AC_SUBST(X11_CFLAGS, "") AC_SUBST(X11_LIBS, "")])
AC_SUBST(GTK_CFLAGS)
AC_SUBST(GTK_LIBS)

# gthread is included in GLib/GTK4 — kept for compatibility
PKG_CHECK_MODULES(GTHREAD, [gthread-2.0], [], [AC_SUBST(GTHREAD_CFLAGS, "") AC_SUBST(GTHREAD_LIBS, "")])
AC_SUBST(GTHREAD_CFLAGS)
AC_SUBST(GTHREAD_LIBS)

# libglade is removed — GtkBuilder is built into GTK4.
# glade_support.c uses GtkBuilder directly.
# AC_DEFINE puts HAVE_GLADE_LIB into config.h so #if HAVE_GLADE_LIB works in C.
# (AC_SUBST alone only affects Makefiles, not C preprocessor.)
AC_DEFINE([HAVE_GLADE_LIB], [1], [GtkBuilder-based glade support available])
AC_SUBST(GLADE_LIB_CFLAGS, "")
AC_SUBST(GLADE_LIB_LIBS, "")

# VTE terminal emulator — vte-2.91-gtk4 for the GTK4 build.
# Falls back gracefully if not installed (HAVE_VTE=0).
vte_lib="vte-2.91-gtk4"
PKG_CHECK_MODULES([VTE], \
	[$vte_lib], \
	AC_SUBST(HAVE_VTE, 1),\
	AC_SUBST(HAVE_VTE, 0))
AC_SUBST(VTE_CFLAGS)
AC_SUBST(VTE_LIBS)

# Checks for header files.
AC_CHECK_HEADERS([locale.h])

# Checks for typedefs, structures, and compiler characteristics.

# Checks for library functions.
AC_CHECK_FUNCS([setlocale])

#AC_CONFIG_FILES([])

# ---------------------------------------------------------------------------
# Optional: --enable-debug
# ---------------------------------------------------------------------------
AC_ARG_ENABLE([debug],
  AS_HELP_STRING([--enable-debug],
    [Enable verbose debug output and -g -O0 (default: no)]),
  [enable_debug=$enableval],
  [enable_debug=no])

if test "x$enable_debug" = "xyes"; then
    DEBUG_CFLAGS="-g -O0 -DDEBUG -DDEBUG_TRANSITS -DDEBUG_CONTENT"
else
    DEBUG_CFLAGS="-O2"
fi
AC_SUBST([DEBUG_CFLAGS])


# ---------------------------------------------------------------------------
# Hardening flags — vérifiés à la compilation
# ---------------------------------------------------------------------------
# Source unique du durcissement. Ces deux variables sont consommees par
# src/Makefile.am (@HARDENING_CFLAGS@ / @HARDENING_LDFLAGS@) : ne pas remettre
# les drapeaux en dur la-bas, ils y avaient diverge de ceux-ci.
# -U_FORTIFY_SOURCE desarme le -D_FORTIFY_SOURCE=2 pose par les drapeaux Debian
# dans CPPFLAGS ; sans lui, chaque fichier compile emet un avertissement.
AC_SUBST([HARDENING_CFLAGS],
  ["-U_FORTIFY_SOURCE -D_FORTIFY_SOURCE=3 -D_POSIX_C_SOURCE=200809L -fstack-protector-strong -fstack-clash-protection -fcf-protection=full -fPIE"])
AC_SUBST([HARDENING_LDFLAGS],
  ["-pie -Wl,-z,relro -Wl,-z,now -Wl,-z,noexecstack -Wl,-z,ibt -Wl,-z,shstk"])

AC_OUTPUT(
	Makefile
	gtkdialog.spec
	src/Makefile
	src/gtk4sermo.1
	src/gtk4sermo-xml.5
	doc/Makefile
	data/Makefile
	data/icons/Makefile
	data/icons/hicolor/Makefile
	data/icons/hicolor/32x32/Makefile
	data/icons/hicolor/32x32/apps/Makefile
)
