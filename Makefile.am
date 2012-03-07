AM_CPPFLAGS = -g
sbin_PROGRAMS = resetmsmice
sbin_SCRIPTS = resetmsmice-enable-boot.sh

resetmsmice_SOURCES = main.c
resetmsmice_LDADD = $(USB_DEPS_LIBS)

install-exec-hook: 
	@resetmsmice-enable-boot.sh

uninstall-hook:
	@./resetmsmice-enable-boot.sh --disable

