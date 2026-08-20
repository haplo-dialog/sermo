EAPI=8
inherit autotools

DESCRIPTION="GTK3 dialog window creator for shell scripts"
HOMEPAGE="https://haplo-dialog.fr"
# Le nom du paquet est haplo-dialog ; l'archive amont garde le nom du port.
MY_P="gtk3dialog-${PV}"
SRC_URI="https://haplo-dialog.fr/pub/gtk3dialog/${MY_P}.tar.gz"
S="${WORKDIR}/${MY_P}"

LICENSE="GPL-2.0-or-later"
SLOT="0"
KEYWORDS="~amd64 ~arm64 ~x86"
IUSE=""

BDEPEND="
	sys-devel/flex
	sys-devel/bison
	sys-devel/autoconf
	sys-devel/automake
"

DEPEND="
	x11-libs/gtk+:3
	x11-libs/vte:2.91
	gui-libs/gtk-layer-shell
"

RDEPEND="
	!!x11-misc/gtk3dialog
	x11-libs/gtk+:3
	x11-libs/vte:2.91
	gui-libs/gtk-layer-shell
"

src_prepare() {
	eautoreconf
	default
}

src_configure() {
	econf 
}

src_compile() {
	emake
}

src_install() {
	emake DESTDIR="${D}" install
	dodoc README NEWS AUTHORS
	if [ -d examples ]; then
		insinto /usr/share/doc/${PN}/examples
		doins -r examples/*
	fi
}

pkg_postinst() {
	elog "Binary installed: /usr/bin/gtk3dialog"
	elog "Documentation: /usr/share/doc/${PN}/"
	elog "Compatibility symlink: /usr/bin/gtkdialog → /usr/bin/gtk3dialog"
}
