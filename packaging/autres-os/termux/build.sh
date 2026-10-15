# Maintainer: s.cage <devel@haplo-dialog.fr>
# SPDX-License-Identifier: GPL-2.0-or-later
#
# sermo pour Termux (Android) : le port ncurses seul, au format termux-packages
# (packages/sermo/build.sh). La construction est celle de
# packaging/autres-os/construire-os.sh, comme pour les autres recettes.
#
# Ce que Termux change :
#   - le shell du repli « -c » est celui de Termux, pas /bin/sh (absent
#     d'Android) : SERMO_SHELL_DEFAUT=$TERMUX_PREFIX/bin/sh ;
#   - bionic n'a pas fopencookie : la limite des <input> passe par funopen,
#     choisi par CMake après essai ;
#   - ARM64 : protection du flot de contrôle par BTI/PAC, choisie de même.
#
# ⚠️ Rejoué SUR l'appareil (construction dans Termux, Android 15, arm64) ; la
# construction croisée de la ferme termux-packages n'a pas été rejouée.
TERMUX_PKG_HOMEPAGE=https://gitlab.com/haplo-dialog/sermo
TERMUX_PKG_DESCRIPTION="Dialog boxes for shell scripts, described in XML (ncurses port)"
TERMUX_PKG_LICENSE="GPL-2.0"
TERMUX_PKG_MAINTAINER="s.cage <devel@haplo-dialog.fr>"
TERMUX_PKG_VERSION=3.0.0
TERMUX_PKG_SRCURL=https://gitlab.com/haplo-dialog/sermo/-/archive/v${TERMUX_PKG_VERSION}/sermo-v${TERMUX_PKG_VERSION}.tar.gz
# La somme se pose à la publication de l'étiquette.
TERMUX_PKG_SHA256=SKIP_CHECKSUM
TERMUX_PKG_DEPENDS="ncurses"
TERMUX_PKG_BUILD_DEPENDS="flex, bison"
TERMUX_PKG_BUILD_IN_SRC=true

termux_step_configure() {
	# construire-os.sh configure lui-même le cœur puis le port.
	:
}

termux_step_make() {
	CMAKE_EXTRA="-DSERMO_SHELL_DEFAUT=$TERMUX_PREFIX/bin/sh" \
		sh packaging/autres-os/construire-os.sh construire . "$TERMUX_PKG_SRCDIR/construit" ncurses
}

termux_step_make_install() {
	sh packaging/autres-os/construire-os.sh installer . "$TERMUX_PKG_SRCDIR/construit" ncurses / "$TERMUX_PREFIX"
	install -Dm644 COPYING "$TERMUX_PREFIX/share/doc/$TERMUX_PKG_NAME/copyright"
	# La commande « sermo » : sous Termux, un seul port, donc un simple lien.
	ln -sf ncursessermo "$TERMUX_PREFIX/bin/sermo"
}
