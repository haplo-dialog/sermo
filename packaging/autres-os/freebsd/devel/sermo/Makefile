# Maintainer: s.cage <devel@haplo-dialog.fr>
# SPDX-License-Identifier: GPL-2.0-or-later
#
# Port FreeBSD de sermo : le port ncurses seul (ncurses du système de base).
# La construction est celle de packaging/autres-os/construire-os.sh.

PORTNAME=	sermo
DISTVERSIONPREFIX=	v
DISTVERSION=	3.0.0
CATEGORIES=	devel

MAINTAINER=	devel@haplo-dialog.fr
COMMENT=	Dialog boxes for shell scripts, described in XML (ncurses port)
WWW=		https://gitlab.com/haplo-dialog/sermo

LICENSE=	GPLv2+
LICENSE_FILE=	${WRKSRC}/COPYING

BUILD_DEPENDS=	cmake:devel/cmake-core \
		bison:devel/bison

USES=		ncurses pkgconfig
USE_GITLAB=	yes
GL_ACCOUNT=	haplo-dialog

# MAKEFLAGS effacé : cmake --build peut lancer GNU make, qui lirait les options
# du make de FreeBSD comme des cibles et ne construirait rien (mesuré le
# 2026-10-12, FreeBSD 15.1 : « -m DOSSIER » pris pour une cible).
do-build:
	cd ${WRKSRC} && ${SETENV} -u MAKEFLAGS ${MAKE_ENV} sh packaging/autres-os/construire-os.sh \
		construire . ${WRKDIR}/construit ncurses

do-install:
	cd ${WRKSRC} && sh packaging/autres-os/construire-os.sh \
		installer . ${WRKDIR}/construit ncurses ${STAGEDIR} ${PREFIX}
	${LN} -sf ncursessermo ${STAGEDIR}${PREFIX}/bin/sermo

.include <bsd.port.mk>
