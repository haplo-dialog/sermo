EAPI=8
inherit autotools

DESCRIPTION="GTK3 dialog window creator for shell scripts"
HOMEPAGE="https://haplo-dialog.fr"
SRC_URI="https://gitlab.com/haplo-dialog/sermo/-/archive/v1.0.0/sermo-v1.0.0.tar.gz -> sermo-v${PV}.tar.gz"
# L\'archive du tag contient les DEUX ports ; on descend dans celui-ci.
S="${WORKDIR}/sermo-v${PV}/gtk3sermo/gtk3sermo_${PV}"

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
"

RDEPEND="
	x11-libs/gtk+:3
	x11-libs/vte:2.91
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
	elog "Binary installed: /usr/bin/gtk3sermo"
	elog "Documentation: /usr/share/doc/${PN}/"
	elog "Compatibility symlink: /usr/bin/gtkdialog → /usr/bin/gtk3sermo"
}
