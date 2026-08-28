#
# We do this because this way we can use multiple parsers in a single binary
# program.
#
AM_LFLAGS=-P$$(basename $@ lexer.c) -olex.yy.c
AM_YFLAGS=-d -p $$(basename $@ parser.c)
BUILT_SOURCES = \
	gtkdialog_parser.h

# GTK4 port: binary renamed gtk4sermo for coexistence with gtk3 gtk4sermo
bin_PROGRAMS = gtk4sermo
gtk4sermo_SOURCES = \
	gtk4-compat.c gtk4-compat.h \
	gtkdialog.c gtkdialog.h \
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
	gtk4-events.c gtk4-events.h \
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
	widget_calendar.c widget_calendar.h \
	widget_filechooser.c widget_filechooser.h \
	widget_image.c widget_image.h \
	widget_infobar.c widget_infobar.h \
	widget_linkbutton.c widget_linkbutton.h \
	widget_password.c widget_password.h \
	widget_pulse.c widget_pulse.h \
	widget_searchentry.c widget_searchentry.h \
	widget_spinner.c widget_spinner.h \
	widget_aspectframe.c widget_aspectframe.h \
	widget_vscale.c widget_vscale.h \
	widget_vseparator.c widget_vseparator.h \
	widget_togglebutton.c widget_togglebutton.h \
	widget_levelbar.c widget_levelbar.h \
	widget_revealer.c widget_revealer.h \
	widget_stack.c widget_stack.h \
	widget_flowbox.c widget_flowbox.h \
	widget_overlay.c widget_overlay.h \
	widget_drawingarea.c widget_drawingarea.h

AM_CFLAGS = \
	$(GTK_CFLAGS) \
	$(GTHREAD_CFLAGS) \
	$(VTE_CFLAGS) -DHAVE_VTE=$(HAVE_VTE) \
	-include $(top_srcdir)/src/gtk4-compat.h \
	-Wall -Wextra -Wno-unused-parameter \
	-Wno-deprecated-declarations \
	-Wno-unused-variable \
	-Wshadow \
	-Wstrict-prototypes \
	-Wno-missing-prototypes \
	-Wnull-dereference \
	-Wimplicit-fallthrough=3 \
	-Wformat=2 -Wformat-overflow=2 -Werror=format-security \
	-D_FORTIFY_SOURCE=3 \
	-D_POSIX_C_SOURCE=200809L \
	-fstack-protector-strong \
	-fstack-clash-protection \
	-fcf-protection=full \
	-fPIE

LIBS = \
	$(GTK_LIBS) \
	$(GTHREAD_LIBS) \
	$(VTE_LIBS) \
	@LIBS@

AM_LDFLAGS = -pie -Wl,-z,relro -Wl,-z,now -Wl,-z,noexecstack \
	-Wl,-z,ibt -Wl,-z,shstk

# Man page — installed to $(mandir)/man1/
# Pages engendrees par configure depuis les .in : la version suit AC_INIT
# toute seule. Avant le 2026-08-27 elles portaient « 1.0.0 » en dur dans
# leur ligne .TH, et le pied de page de man mentait a chaque montee de version.
man_MANS = gtk4sermo.1 gtk4sermo-xml.5
EXTRA_DIST = gtk4sermo.1.in gtk4sermo-xml.5.in
CLEANFILES = gtk4sermo.1 gtk4sermo-xml.5
