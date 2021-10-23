udevrulesdir = $(udevdir)/rules.d
udevrules_DATA = $(top_srcdir)/data/60-resetmsmice.rules 

AM_CPPFLAGS = -g -Wall

if DISABLE_GUI
bin_PROGRAMS = resetmsmice
else
bin_PROGRAMS = resetmsmice resetmsmice-gui
endif

sbin_SCRIPTS = $(top_srcdir)/linux/resetmsmice-enable-boot

resetmsmice_SOURCES = src/main.c src/parse.c src/pthread-misc.c src/resetmsmice.c include/hid-userland.h include/basictypes.h include/pthread-misc.h include/resetmsmice.h
resetmsmice_CFLAGS = -I$(top_srcdir)/include $(USB_DEPS_CFLAGS)
resetmsmice_LDADD = $(USB_DEPS_LIBS) $(LIBPTHREAD)
resetmsmicedir = $(pkgdatadir)

desktopdir = $(datadir)/applications

if DISABLE_GUI
desktop_DATA = $(top_srcdir)/data/resetmsmice-cmd.desktop
else
desktop_DATA = $(top_srcdir)/data/resetmsmice.desktop $(top_srcdir)/data/resetmsmice-cmd.desktop
endif

iconsdir = $(datadir)/icons/hicolor/scalable/apps
icons_DATA = $(top_srcdir)/data/resetmsmice.svg

if DISABLE_GUI
# if disable, do not add anything!
else
resetmsmice_gui_SOURCES = src/gtk3/resetWindow.cc src/gtk3/execSave.cc
resetmsmice_gui_CPPFLAGS = -I$(top_srcdir)/include/gtk3 $(GTK_DEPS_CFLAGS) $(GLIB_DEPS_CFLAGS)
resetmsmice_gui_LDADD = $(GTK_DEPS_LIBS) $(GLIB_DEPS_LIBS)
endif

install-data-hook:
	gtk-update-icon-cache -f -t -q $(datadir)/icons/hicolor || :
	update-desktop-database $(datadir)/applications || :
	udevadm trigger --attr-match=idVendor=045e || :

install-exec-hook: 
	groupadd -f ms-usb || :
	chgrp ms-usb $(DESTDIR)$(bindir)/resetmsmice || :
	chmod 2755 $(DESTDIR)$(bindir)/resetmsmice || :
	@$(top_srcdir)/linux/resetmsmice-enable-boot --enable || :

uninstall-hook:
	@$(top_srcidr)/linux/resetmsmice-enable-boot --disable

