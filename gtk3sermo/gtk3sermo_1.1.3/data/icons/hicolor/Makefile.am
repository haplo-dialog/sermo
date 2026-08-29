SUBDIRS=32x32

# Use $(DESTDIR) so DESTDIR installs (packaging) don't touch the live cache.
# The -q flag silences non-fatal errors (missing theme cache is OK during pkg).
UPDATE_ICON_CACHE=gtk-update-icon-cache -f -t $(DESTDIR)$(datadir)/icons/hicolor

install-data-hook:
	-$(UPDATE_ICON_CACHE)
uninstall-hook:
	-$(UPDATE_ICON_CACHE)
