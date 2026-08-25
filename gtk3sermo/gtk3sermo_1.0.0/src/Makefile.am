#
# src/Makefile.am — gtk3sermo 1.0.0 (haplo-dialog)
# Renamed from gtkdialog → gtk3sermo (patch 13, 2026-05-22)
#
# We use per-target variables (gtk3sermo_CFLAGS, gtk3sermo_LDADD,
# gtk3sermo_LDFLAGS) rather than AM_* globals so that future additions
# of helper libraries or test binaries remain isolated.
#
# Multiple parsers in a single binary — AM_LFLAGS and AM_YFLAGS use
# the basename trick to give each generated file a unique prefix.
#
AM_LFLAGS = -P$$(basename $@ lexer.c) -olex.yy.c
AM_YFLAGS = -d -p $$(basename $@ parser.c)

BUILT_SOURCES = \
	gtkdialog_parser.h

bin_PROGRAMS = gtk3sermo

gtk3sermo_SOURCES = \
	gtkdialog.c gtk3d.h \
	safe_exec.c safe_exec.h \
	macros.h \
	printing.c printing.h \
	variables.c variables.h \
	gtkdialog_parser.y gtkdialog_lexer.l \
	stack.c stack.h \
	automaton.c automaton.h \
	attributes.c attributes.h \
	signals.c signals.h \
	stringman.c stringman.h \
	widgets.c widgets.h \
	actions.c actions.h \
	tag_attributes.c tag_attributes.h \
	glade_support.c glade_support.h \
	widget_button.c widget_button.h \
	widget_checkbox.c widget_checkbox.h \
	widget_colorbutton.c widget_colorbutton.h \
	widget_combobox.c widget_combobox.h \
	widget_comboboxtext.c widget_comboboxtext.h \
	widget_edit.c widget_edit.h \
	widget_entry.c widget_entry.h \
	widget_eventbox.c widget_eventbox.h \
	widget_expander.c widget_expander.h \
	widget_fontbutton.c widget_fontbutton.h \
	widget_frame.c widget_frame.h \
	widget_hbox.c widget_hbox.h \
	widget_hscale.c widget_hscale.h \
	widget_hseparator.c widget_hseparator.h \
	widget_list.c widget_list.h \
	widget_menubar.c widget_menubar.h \
	widget_menuitem.c widget_menuitem.h \
	widget_notebook.c widget_notebook.h \
	widget_pixmap.c widget_pixmap.h \
	widget_progressbar.c widget_progressbar.h \
	widget_radiobutton.c widget_radiobutton.h \
	widget_spinbutton.c widget_spinbutton.h \
	widget_statusbar.c widget_statusbar.h \
	widget_table.c widget_table.h \
	widget_terminal.c widget_terminal.h \
	widget_text.c widget_text.h \
	widget_timer.c widget_timer.h \
	widget_tree.c widget_tree.h \
	widget_vbox.c widget_vbox.h \
	widget_window.c widget_window.h \
	widget_switch.c widget_switch.h \
	widget_filechooser.c widget_filechooser.h \
	widget_calendar.c widget_calendar.h \
	widget_linkbutton.c widget_linkbutton.h \
	widget_searchentry.c widget_searchentry.h \
	widget_infobar.c widget_infobar.h \
	widget_vseparator.h \
	widget_vscale.h \
	widget_spinner.c widget_spinner.h \
	widget_levelbar.c widget_levelbar.h \
	widget_drawingarea.c widget_drawingarea.h \
	widget_image.c widget_image.h \
	widget_pulse.c widget_pulse.h \
	widget_password.c widget_password.h \
	widget_aspectframe.c widget_aspectframe.h

# Man page — installed to $(mandir)/man1/
# gtk3sermo.1 est le nom principal ; gtkdialog.1 est un alias de compatibilité
dist_man_MANS = gtk3sermo.1

# Lien symbolique de compatibilité ascendante : gtkdialog → gtk3sermo
install-exec-hook:
	cd $(DESTDIR)$(bindir) && \
	$(LN_S) -f gtk3sermo gtkdialog || true

install-data-hook:
	cd $(DESTDIR)$(mandir)/man1 && \
	$(LN_S) -f gtk3sermo.1 gtkdialog.1 || true

# Per-target compile flags.
# @DEBUG_CFLAGS@ is set by configure --enable-debug (empty by default).
# GLADE_LIB_CFLAGS is kept for symmetry even though it is empty
# (GtkBuilder is built into GTK3 — no external glade library needed).
gtk3sermo_CFLAGS = \
	$(GTK_CFLAGS) \
	$(GTHREAD_CFLAGS) \
	$(VTE_CFLAGS) -DHAVE_VTE=$(HAVE_VTE) \
	$(LAYER_SHELL_CFLAGS) -DHAVE_LAYER_SHELL=$(HAVE_LAYER_SHELL) \
	-DHAVE_LAYER_SHELL_IS_SUPPORTED=$(HAVE_LAYER_SHELL_IS_SUPPORTED) \
	$(GLADE_LIB_CFLAGS) \
	-Wall -Wextra -Wno-unused-parameter \
	-Wformat=2 -Wformat-overflow=2 -Werror=format-security \
	-Wno-deprecated-declarations \
	-Wno-unused-variable \
	-Wshadow \
	-Wstrict-prototypes \
	-Wno-missing-prototypes \
	-Wnull-dereference \
	-Wimplicit-fallthrough=3 \
	-D_FORTIFY_SOURCE=3 \
	-D_POSIX_C_SOURCE=200809L \
	-fstack-protector-strong \
	-fstack-clash-protection \
	-fcf-protection=full \
	-fPIE \
	@DEBUG_CFLAGS@

# Per-target link libraries.
gtk3sermo_LDADD = \
	$(GTK_LIBS) \
	$(GTHREAD_LIBS) \
	$(VTE_LIBS) \
	$(LAYER_SHELL_LIBS) \
	@LIBS@

# Per-target linker flags.
gtk3sermo_LDFLAGS = -pie -Wl,-z,relro -Wl,-z,now -Wl,-z,noexecstack \
	-Wl,-z,ibt -Wl,-z,shstk
