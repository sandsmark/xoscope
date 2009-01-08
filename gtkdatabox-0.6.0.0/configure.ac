dnl This configure.in file has been beveloped by looking at the tutorial of
dnl Raph Levian
dnl at http://www.levien.com/gnome/cvs-lib-howto.html
dnl
dnl Additional features where taken from gtk+-1.1.1/configure.in and allied 
dnl files.
dnl 
dnl Thank's to all the gimp-glib-gtk-gnome-etc.-people
dnl
dnl If you know automake and friends better then me:
dnl Please send any helpful comments to rbock at eudoxos de

dnl Initialize autoconf by any header file of the source
dnl

AC_INIT(gtkdatabox, 0.6.0.0)

dnl This configures autoheader to generate a config.h file
dnl
AM_CONFIG_HEADER(config.h)

AC_SUBST(LT_RELEASE, 0.6.0)
AC_SUBST(LT_CURRENT, 0)

AM_INIT_AUTOMAKE(gtkdatabox, 0.6.0.0)

AM_PROG_LIBTOOL

AC_PROG_CC
AC_PROG_CPP
AC_PROG_INSTALL

AM_PATH_GTK_2_0

AC_OUTPUT(Makefile              \
	gtk/Makefile            \
	examples/Makefile       \
        gtkdatabox.pc           \
	)


