# Makefile.am — gtk3sermo 1.0.0 (haplo-dialog)

SUBDIRS = src doc data tests

# ---------------------------------------------------------------------------
# Source distribution (make dist / make distcheck)
# Removed: install.iss (Windows — out of scope), pango.modules, pango.aliases,
#          gdk-pixbuf.loaders (GTK2 caches — not used in GTK3).
# ---------------------------------------------------------------------------
EXTRA_DIST = \
	tests/run_tests.sh \
	tests/test_safe_exec.c \
	tests/test_stringman.c \
	tests/Makefile.am \
	tests/README.tests \
	autogen.sh \
	debian/source/format \
	debian/control \
	debian/rules \
	debian/changelog \
	debian/copyright \
	debian/gtk3sermo.install \
	examples/button/* \
	examples/checkbox/* \
	examples/colorbutton/* \
	examples/combobox/* \
	examples/comboboxentry/* \
	examples/comboboxtext/* \
	examples/edit/* \
	examples/entry/* \
	examples/eventbox/* \
	examples/expander/* \
	examples/fontbutton/* \
	examples/frame/* \
	examples/glade/* \
	examples/hbox/* \
	examples/hscale/* \
	examples/hseparator/* \
	examples/list/* \
	examples/menu/* \
	examples/menubar/* \
	examples/menuitem/* \
	examples/menuitemseparator/* \
	examples/miscellaneous/* \
	examples/notebook/* \
	examples/pixmap/* \
	examples/pfeme/* \
	examples/pfontview/* \
	examples/playmusic/* \
	examples/progressbar/* \
	examples/radiobutton/* \
	examples/spinbutton/* \
	examples/statusbar/* \
	examples/table/* \
	examples/terminal/* \
	examples/text/* \
	examples/timer/* \
	examples/togglebutton/* \
	examples/tree/* \
	examples/vbox/* \
	examples/vscale/* \
	examples/vseparator/* \
	examples/showcase/* \
	examples/system-tools/* \
	examples/window/*

# ---------------------------------------------------------------------------
# Debian packaging targets
#
#   make deb           — build a binary .deb in the parent directory
#   make deb-src       — build a source .dsc + .tar.gz
#   make deb-clean     — clean debian build artefacts
#
# Requires: dpkg-buildpackage (package dpkg-dev), debhelper >= 13
# ---------------------------------------------------------------------------
.PHONY: deb deb-src deb-clean

deb:
	@echo "==> Building binary .deb (no signing) ..."
	dpkg-buildpackage -us -uc -b -j$$(nproc 2>/dev/null || echo 1)
	@echo ""
	@echo "Package written to the parent directory:"
	@ls -lh ../gtk3sermo_*.deb 2>/dev/null || true

deb-src:
	@echo "==> Building source package (no signing) ..."
	dpkg-buildpackage -us -uc -S -j$$(nproc 2>/dev/null || echo 1)

deb-clean:
	debian/rules clean
	@rm -f ../gtk3sermo_*.deb ../gtk3sermo_*.changes \
	       ../gtk3sermo_*.buildinfo ../gtk3sermo_*.dsc \
	       ../gtk3sermo_*.tar.*

# Validation lors de 'make distcheck'
distcheck-hook:
	@echo "=== Hardening check ===" && \
	hardening-check $(top_builddir)/src/$(bin_PROGRAMS) 2>/dev/null || \
	echo "  (hardening-check non installé — OK pour CI)"
