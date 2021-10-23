AC_INIT([resetmsmice], 1.2.0, [paulrichards321@gmail.com])
AC_ARG_ENABLE([gui],
  [AS_HELP_STRING([--disable-gui], [Disable building of gui program])],
  [  
     case "${enableval}" in

     no|false)
       gui=no
       ;;

     yes|true)
       gui=yes
       ;;

     *)
       AC_MSG_ERROR([Invalid value ${disableval} for --disable-gui])
       ;;

     esac
  ],
  [gui=yes])
  
AM_INIT_AUTOMAKE([subdir-objects])
AC_CONFIG_HEADER([conf.h])
AC_MSG_CHECKING([pkg-config m4 macros])
if test m4_ifdef([PKG_CHECK_MODULES], [yes], [no]) == yes; then
    AC_MSG_RESULT([yes]);
else
    AC_MSG_RESULT([no]);
    AC_MSG_ERROR([
pkg-config is required.
See pkg-config.freedesktop.org])
fi
AM_CONDITIONAL([DISABLE_GUI], [test "${gui}" = "no"])
AM_COND_IF([DISABLE_GUI],
  [PKG_CHECK_MODULES([USB_DEPS], [libusb-1.0 >= 1.0])],
  [PKG_CHECK_MODULES([USB_DEPS], [libusb-1.0 >= 1.0])
   PKG_CHECK_MODULES([GTK_DEPS], [gtk+-3.0 >= 3.0])
   PKG_CHECK_MODULES([GLIB_DEPS], [glib-2.0 >= 2.0])])

AC_CHECK_LIB(pthread, pthread_create, [LIBPTHREAD="-lpthread"], [LIBPTHREAD=""])
AM_PROG_CC_C_O
#AC_PROG_CC
AC_PROG_CXX
AC_SUBST(LIBRT)
AC_SUBST(LIBPTHREAD)

AC_CHECK_HEADERS([stdio.h stdarg.h string.h stdlib.h iconv.h locale.h sys/types.h sys/stat.h sys/ioctl.h sys/utsname.h syslog.h fcntl.h wchar.h pthread.h libusb-1.0/libusb.h errno.h unistd.h getopt.h ctype.h])
AC_CHECK_HEADERS([sys/stat.h])
AC_HEADER_STDBOOL

INCLUDES="$INCLUDES -I\$(top_srcdir)"

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
