SUBDIRS=32x32

# Use $(DESTDIR) so DESTDIR installs (packaging) don't touch the live cache.
# Leading '-' : a failure to refresh the cache must not break install/uninstall.
UPDATE_ICON_CACHE=gtk-update-icon-cache -f -t $(DESTDIR)$(datadir)/icons/hicolor

install-data-hook:
	-$(UPDATE_ICON_CACHE)
uninstall-hook:
	-$(UPDATE_ICON_CACHE)
