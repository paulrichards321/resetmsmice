AC_INIT([resetmsmice], 0.9, [paulrichards321@gmail.com])
AM_INIT_AUTOMAKE
AC_PROG_CC
AC_LANG(C)
AC_CONFIG_HEADER([conf.h])
PKG_CHECK_MODULES(USB_DEPS, libusb-1.0 >= 1.0)
AC_CHECK_HEADERS([stdio.h stdarg.h string.h stdlib.h iconv.h locale.h sys/types.h syslog.h libusb-1.0/libusb.h errno.h unistd.h getopt.h ctype.h])
AC_HEADER_STDBOOL
AC_CONFIG_FILES([Makefile])
AC_OUTPUT

