udevrulesdir = $(udevdir)/rules.d
udevrules_DATA = 60-resetmsmice.rules 

AM_CPPFLAGS = -g -Wall
bin_PROGRAMS = resetmsmice resetmsmice-gui
sbin_SCRIPTS = resetmsmice-enable-boot

resetmsmice_SOURCES = main.c parse.c hid-userland.h
resetmsmice_LDADD = $(USB_DEPS_LIBS) $(LIBPTHREAD)
resetmsmicedir = $(pkgdatadir)
resetmsmice_DATA = resetmsmice.ui

desktopdir = $(datadir)/applications
desktop_DATA = resetmsmice.desktop

resetmsmice_gui_SOURCES = gui.cc
resetmsmice_gui_CPPFLAGS = $(GTK_DEPS_CFLAGS)
resetmsmice_gui_LDADD = $(GTK_DEPS_LIBS)

install-data-hook:
	update-desktop-database $(datadir)/applications || :
	udevadm trigger --attr-match=idVendor=045e || :

install-exec-hook: 
	groupadd -f ms-usb || :
	chgrp ms-usb $(DESTDIR)$(bindir)/resetmsmice || :
	chmod 2755 $(DESTDIR)$(bindir)/resetmsmice || :
	@./resetmsmice-enable-boot --enable || :

uninstall-hook:
	@./resetmsmice-enable-boot --disable

