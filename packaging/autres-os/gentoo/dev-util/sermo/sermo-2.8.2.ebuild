# SPDX-License-Identifier: GPL-2.0-or-later
# Copyright (C) 2026 S. Cage
# haplo-dialog <devel@haplo-dialog.fr>

# Écrit sans Gentoo sous la main : la suite des gestes de construction est
# rejouée (packaging/autres-os/README.md), pas les atomes de dépendance.
# Pas de mécanisme « alternatives » ici : chaque backend livre /usr/bin/<port>sermo.

EAPI=8

DESCRIPTION="Dialog boxes for shell scripts, described in XML (gtkdialog successor)"
HOMEPAGE="https://gitlab.com/haplo-dialog/sermo"
SRC_URI="https://gitlab.com/haplo-dialog/sermo/-/archive/v${PV}/sermo-v${PV}.tar.gz -> ${P}.tar.gz"
S="${WORKDIR}/sermo-v${PV}"

LICENSE="GPL-2+ MIT lvgl? ( BSD BSD-2 ZLIB Apache-2.0 MPL-2.0 OFL-1.1 )"
SLOT="0"
KEYWORDS="~amd64 ~arm64"
IUSE="gtk3 gtk4 qt6 fltk efl sdl lvgl ncurses html doc"
REQUIRED_USE="|| ( gtk3 gtk4 qt6 fltk efl sdl lvgl ncurses html )"

DEPEND="
	gtk3? (
		x11-libs/gtk+:3
		x11-libs/vte:2.91
		gui-libs/gtk-layer-shell
	)
	gtk4? (
		gui-libs/gtk:4
		gui-libs/vte:2.91-gtk4
	)
	qt6? ( dev-qt/qtbase:6[gui,widgets] )
	fltk? ( x11-libs/fltk:1 )
	efl? ( dev-libs/efl )
	sdl? ( media-libs/libsdl3 )
	lvgl? (
		x11-libs/libX11
		media-libs/libpng
		dev-libs/wayland
		x11-libs/libxkbcommon
	)
	ncurses? ( sys-libs/ncurses:= )
"
RDEPEND="${DEPEND}"
BDEPEND="
	dev-build/cmake
	app-alternatives/lex
	app-alternatives/yacc
	virtual/pkgconfig
	doc? ( sys-apps/texinfo )
	lvgl? ( dev-libs/wayland-protocols )
"

# Les ports voulus, dans l'ordre des USE.
_sermo_ports() {
	use gtk3    && echo gtk3
	use gtk4    && echo gtk4
	use qt6     && echo qt6
	use fltk    && echo fltk1
	use efl     && echo efl1
	use sdl     && echo sdl3
	use lvgl    && echo lvgl
	use ncurses && echo ncurses
	use html    && echo html
}

src_compile() {
	sh packaging/autres-os/construire-os.sh construire . "${WORKDIR}/construit" $(_sermo_ports) || die
	if use doc; then
		sh packaging/autres-os/construire-os.sh manuel . "${WORKDIR}/construit" "${T}/doc" /usr || die
	fi
}

src_install() {
	local p
	for p in $(_sermo_ports); do
		sh packaging/autres-os/construire-os.sh installer . "${WORKDIR}/construit" "${p}" "${D}" /usr || die
	done
	if use doc; then
		doinfo "${T}/doc/usr/share/info/sermo-doc.info"
	fi
}
