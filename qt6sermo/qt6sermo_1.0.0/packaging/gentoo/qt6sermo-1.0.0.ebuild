EAPI=8
inherit cmake

DESCRIPTION="Qt6 dialog window creator for shell scripts"
HOMEPAGE="https://haplo-dialog.fr"
SRC_URI="https://haplo-dialog.fr/pub/qt6sermo/${P}.tar.gz"

LICENSE="GPL-2.0-or-later"
SLOT="0"
KEYWORDS="~amd64 ~arm64 ~x86"
IUSE=""

BDEPEND="
	sys-devel/flex
	sys-devel/bison
	dev-build/cmake
	dev-qt/qttools:6
"

DEPEND="
	dev-qt/qtbase:6
"

RDEPEND="
	dev-qt/qtbase:6
"

src_prepare() {
	cmake_src_prepare
}

src_configure() {
	local mycmakeargs=(
		-DCMAKE_BUILD_TYPE=Release
	)
	cmake_src_configure
}

src_compile() {
	cmake_src_compile
}

src_install() {
	cmake_src_install
	dodoc README NEWS AUTHORS
	if [ -d examples ]; then
		insinto /usr/share/doc/${PN}/examples
		doins -r examples/*
	fi
}

pkg_postinst() {
	elog "Binary installed: /usr/bin/qt6sermo"
	elog "Documentation: /usr/share/doc/${PN}/"
	
}
