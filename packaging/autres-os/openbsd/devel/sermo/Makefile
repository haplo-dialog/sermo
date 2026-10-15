# Maintainer: s.cage <devel@haplo-dialog.fr>
# SPDX-License-Identifier: GPL-2.0-or-later
#
# Port OpenBSD de sermo : le port ncurses seul (curses du système de base).
# La construction est celle de packaging/autres-os/construire-os.sh.

COMMENT=	dialog boxes for shell scripts, described in XML (ncurses)

V=		3.0.0
DISTNAME=	sermo-v${V}
PKGNAME=	sermo-${V}
CATEGORIES=	devel

HOMEPAGE=	https://gitlab.com/haplo-dialog/sermo
MAINTAINER=	s.cage <devel@haplo-dialog.fr>

# GPLv2+
PERMIT_PACKAGE=	Yes

WANTLIB=	c curses pthread

SITES=		https://gitlab.com/haplo-dialog/sermo/-/archive/v${V}/

BUILD_DEPENDS=	devel/cmake/core \
		devel/bison

NO_TEST=	Yes

# MAKEFLAGS effacé : cmake --build peut lancer GNU make, qui lirait les
# options du make d'OpenBSD comme des cibles et ne construirait rien.
do-build:
	cd ${WRKSRC} && ${SETENV} -u MAKEFLAGS sh packaging/autres-os/construire-os.sh \
		construire . ${WRKDIR}/construit ncurses

# OpenBSD range les pages de manuel sous ${PREFIX}/man, pas share/man.
do-install:
	cd ${WRKSRC} && sh packaging/autres-os/construire-os.sh \
		installer . ${WRKDIR}/construit ncurses ${DESTDIR} ${PREFIX}
	mv ${DESTDIR}${PREFIX}/share/man ${DESTDIR}${PREFIX}/man
	rmdir ${DESTDIR}${PREFIX}/share
	ln -sf ncursessermo ${DESTDIR}${PREFIX}/bin/sermo

.include <bsd.port.mk>
