AC_INIT([resetmsmice], 1.1.1, [paulrichards321@gmail.com])
AM_INIT_AUTOMAKE
AC_CONFIG_HEADER([conf.h])
PKG_CHECK_MODULES(USB_DEPS, libusb-1.0 >= 1.0)
PKG_CHECK_MODULES(GTK_DEPS, gtk+-2.0 >= 2.0)
AC_CHECK_LIB(pthread, pthread_create, [LIBPTHREAD="-lpthread"], [LIBPTHREAD=""])
AC_PROG_CC
AC_PROG_CXX
AC_SUBST(LIBRT)
AC_SUBST(LIBPTHREAD)

AC_CHECK_HEADERS([stdio.h stdarg.h string.h stdlib.h iconv.h locale.h sys/types.h sys/stat.h sys/ioctl.h sys/utsname.h syslog.h fcntl.h wchar.h pthread.h libusb-1.0/libusb.h errno.h unistd.h getopt.h ctype.h])
AC_CHECK_HEADERS([gtk/gtk.h sys/stat.h])
AC_HEADER_STDBOOL

AC_SUBST(udevdir)
PKG_PROG_PKG_CONFIG
# check for udevdir
AC_ARG_WITH([udevdir],
    [AS_HELP_STRING([--with-udevdir=DIR],
                    [your udev directory])],
    [udevdir="$withval"],
    [PKG_CHECK_MODULES([UDEV_DEPS],[udev],
     [udevdir=`$PKG_CONFIG --variable=udevdir udev`],
     [udevdir="/etc/udev/rules.d"])]
)

AC_CONFIG_FILES([Makefile])
AC_OUTPUT
