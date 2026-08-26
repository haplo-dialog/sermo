EAPI=8
inherit autotools

DESCRIPTION="GTK4 dialog window creator for shell scripts"
HOMEPAGE="https://haplo-dialog.fr"
SRC_URI="https://gitlab.com/haplo-dialog/sermo/-/archive/v1.0.0/sermo-v1.0.0.tar.gz -> sermo-v${PV}.tar.gz"
# L\'archive du tag contient les DEUX ports ; on descend dans celui-ci.
S="${WORKDIR}/sermo-v${PV}/gtk4sermo/gtk4sermo_${PV}"

LICENSE="GPL-2.0-or-later"
SLOT="0"
KEYWORDS="~amd64 ~arm64 ~x86"
IUSE=""

BDEPEND="
	sys-devel/flex
	sys-devel/bison
	sys-devel/autoconf
	sys-devel/automake
	sys-apps/texinfo
"

DEPEND="
	gui-libs/gtk:4
	x11-libs/vte:2.91-gtk4
"

RDEPEND="
	gui-libs/gtk:4
	x11-libs/vte:2.91-gtk4
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
	elog "Binary installed: /usr/bin/gtk4sermo"
	elog "Documentation: /usr/share/doc/${PN}/"
	
}
