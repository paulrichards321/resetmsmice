udevrulesdir = $(udevdir)/rules.d
udevrules_DATA = $(top_srcdir)/60-resetmsmice.rules 

AM_CPPFLAGS = -g -Wall
bin_PROGRAMS = resetmsmice resetmsmice-gui
sbin_SCRIPTS = $(top_srcdir)/resetmsmice-enable-boot

resetmsmice_SOURCES = $(top_srcdir)/main.c $(top_srcdir)/parse.c $(top_srcdir)/hid-userland.h
resetmsmice_LDADD = $(USB_DEPS_LIBS) $(LIBPTHREAD)
resetmsmicedir = $(pkgdatadir)
resetmsmice_DATA = $(top_srcdir)/resetmsmice.ui

desktopdir = $(datadir)/applications
desktop_DATA = $(top_srcdir)/resetmsmice.desktop

resetmsmice_gui_SOURCES = $(top_srcdir)/gui.cc
resetmsmice_gui_CPPFLAGS = $(GTK_DEPS_CFLAGS)
resetmsmice_gui_LDADD = $(GTK_DEPS_LIBS)

install-data-hook:
	update-desktop-database $(datadir)/applications || :
	udevadm trigger --attr-match=idVendor=045e || :

install-exec-hook: 
	groupadd -f ms-usb || :
	chgrp ms-usb $(DESTDIR)$(bindir)/resetmsmice || :
	chmod 2755 $(DESTDIR)$(bindir)/resetmsmice || :
	@$(top_srcdir)/resetmsmice-enable-boot --enable || :

uninstall-hook:
	@$(top_srcdir)/resetmsmice-enable-boot --disable

