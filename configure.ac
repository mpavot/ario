AC_PREREQ(2.59)

m4_define(ario_version,1.3)

AC_INIT([Ario], [ario_version], [marc.pavot@gmail.com], [ario])
AM_CONFIG_HEADER(config.h)
AM_INIT_AUTOMAKE

AC_PROG_CC
AC_PROG_INSTALL
AC_STDC_HEADERS
AM_PROG_LIBTOOL

dnl ================================================================
dnl i18n
dnl ================================================================
IT_PROG_INTLTOOL(0.35.5)
GETTEXT_PACKAGE=ario
AC_SUBST(GETTEXT_PACKAGE)
AC_DEFINE_UNQUOTED(GETTEXT_PACKAGE,"$GETTEXT_PACKAGE", [Gettext package])
AM_GLIB_GNU_GETTEXT

dnl ================================================================
dnl Dependencies
dnl ================================================================
AM_PATH_LIBGCRYPT([1.4.0], [have_libgcrypt="yes"], [have_libgcrypt="no"])
if test "${have_libgcrypt}" != "yes"; then
	AC_MSG_ERROR([Missing dependency: libgcrypt])
fi

PKG_CHECK_MODULES(DEPS, [gtk+-2.0 >= 2.16
                         glib-2.0 >= 2.4
                         gobject-2.0 >= 2.4
                         gmodule-2.0
                         gthread-2.0
                         gio-2.0
                         libxml-2.0
                         libcurl
                         gnutls])

AC_SUBST(DEPS_CFLAGS)
AC_SUBST(DEPS_LIBS)

AC_SUBST(CFLAGS)
AC_SUBST(LDFLAGS)

AC_PATH_PROG(GLIB_MKENUMS, glib-mkenums)

dnl ================================================================
dnl Warnings
dnl ================================================================
WARN_CFLAGS="-Wall \ 
-Wall -Wimplicit -Wunused-label \
-Wmissing-declarations -Wmissing-prototypes \
-Wnested-externs -Wpointer-arith \
-Wcast-align -Wpointer-sign \
-Wclobbered -Wempty-body -Wignored-qualifiers \
-Wmissing-field-initializers -Wmissing-parameter-type \
-Wold-style-declaration -Woverride-init \
-Wsign-compare -Wtype-limits -Wuninitialized \
-Wbad-function-cast -Wold-style-definition"

for option in $WARN_CFLAGS; do
        SAVE_CFLAGS="$CFLAGS"
        CFLAGS="$CFLAGS $option"
        AC_MSG_CHECKING([whether gcc understands $option])
        AC_TRY_COMPILE([], [],
                       has_option=yes,
                       has_option=no,)
        if test x$has_option = xyes; then
                WARNINGS="$WARNINGS $option"
        fi
        AC_MSG_RESULT($has_option)
        CFLAGS="$SAVE_CFLAGS"
        unset has_option
        unset SAVE_CFLAGS
done
unset option
AC_SUBST(WARNINGS)

dnl ================================================================
dnl Debugging information
dnl ================================================================
AC_ARG_ENABLE(debug,
              AC_HELP_STRING([--enable-debug],
                             [Add runtime debugging information (slower)]))
if test x"$enable_debug" = xyes; then
   AC_DEFINE(DEBUG, 1, [Add runtime debugging information (slower)])
fi

dnl ================================================================
dnl XMMS2 support
dnl ================================================================
AC_ARG_ENABLE(xmms2,
              AC_HELP_STRING([--enable-xmms2],
                             [Add support of XMMS2 (VERY EXPERIMENTAL)]))
if test x"$enable_xmms2" = xyes; then
   AC_DEFINE(ENABLE_XMMS2, 1, [Add support of XMMS2 (VERY EXPERIMENTAL)])
   PKG_CHECK_MODULES(XMMS2,
                     xmms2-client
		     xmms2-client-glib)
   AC_SUBST(XMMS2_CFLAGS)
   AC_SUBST(XMMS2_LIBS)
fi
AM_CONDITIONAL(USE_XMMS2, test x${enable_xmms2} = xyes)

dnl ================================================================
dnl Python plugins
dnl ================================================================
AC_ARG_ENABLE(python,
              AC_HELP_STRING([--enable-python],
                             [Enable python plugins support]))
if test x"$enable_python" = xyes; then
   AC_DEFINE(ENABLE_PYTHON, 1, [Enable python plugins support])
fi

if test "x$enable_python" = "xyes"; then
	AM_PATH_PYTHON([2.3],[],[no])
	if test "x$PYTHON" = "x:"; then
		enable_python=no
	fi
fi

if test "x$enable_python" = "xyes"; then
	PY_PREFIX=`$PYTHON -c 'import sys ; print sys.prefix'`
	PY_EXEC_PREFIX=`$PYTHON -c 'import sys ; print sys.exec_prefix'`
	PYTHON_LIBS="-lpython$PYTHON_VERSION"
	PYTHON_LIB_LOC="-L$PY_EXEC_PREFIX/lib/python$PYTHON_VERSION/config"
	PYTHON_CFLAGS="-I$PY_PREFIX/include/python$PYTHON_VERSION"
	PYTHON_MAKEFILE="$PY_EXEC_PREFIX/lib/python$PYTHON_VERSION/config/Makefile"
	PYTHON_LOCALMODLIBS=`sed -n -e 's/^LOCALMODLIBS=\(.*\)/\1/p' $PYTHON_MAKEFILE`
	PYTHON_BASEMODLIBS=`sed -n -e 's/^BASEMODLIBS=\(.*\)/\1/p' $PYTHON_MAKEFILE`
	PYTHON_OTHER_LIBS=`sed -n -e 's/^LIBS=\(.*\)/\1/p' $PYTHON_MAKEFILE`
	PYTHON_EXTRA_LIBS="$PYTHON_LOCALMODLIBS $PYTHON_BASEMODLIBS $PYTHON_OTHER_LIBS"
	AC_SUBST([PYTHON_LIBS])
	AC_SUBST([PYTHON_LIB_LOC])
	AC_SUBST([PYTHON_CFLAGS])
	AC_SUBST([PYTHON_EXTRA_LIBS])

	PKG_CHECK_MODULES([PYGTK], [pygobject-2.0 >= 2.11.5
                                    pygtk-2.0 >= 2.8.0])

	AC_SUBST([PYGTK_CFLAGS])
	AC_SUBST([PYGTK_LIBS])
	AC_MSG_CHECKING([for pygtk defs])
	PYGTK_DEFSDIR=`$PKG_CONFIG --variable=defsdir pygtk-2.0`
	AC_MSG_RESULT([$PYGTK_DEFSDIR])

	AC_MSG_CHECKING([for pygtk codegen])
	PYGTK_CODEGEN="$PYTHON `$PKG_CONFIG --variable=codegendir pygtk-2.0`/codegen.py"
	AC_MSG_RESULT([$PYGTK_CODEGEN])

	AC_MSG_CHECKING([for pygtk h2def])
	PYGTK_H2DEF="$PYTHON `$PKG_CONFIG --variable=codegendir pygtk-2.0`/h2def.py"
	AC_MSG_RESULT([$PYGTK_H2DEF])

	AC_SUBST([PYGTK_DEFSDIR])
	AC_SUBST([PYGTK_CODEGEN])
	AC_SUBST([PYGTK_H2DEF])
fi
AM_CONDITIONAL(ENABLE_PYTHON, test x"$enable_python" = xyes)

dnl ================================================================
dnl Search
dnl ================================================================
AC_ARG_ENABLE(search,
              AC_HELP_STRING([--disable-search],
                             [Disable support of search view]))
if test x"$enable_search" != xno; then
   AC_DEFINE(ENABLE_SEARCH, 1, [Add support of search view])
fi

dnl ================================================================
dnl MPD idle mode
dnl ================================================================
AC_ARG_ENABLE(mpdidle,
              AC_HELP_STRING([--enable-mpdidle],
                             [Use MPD idle mode (reduce CPU and network usage but still experimental)]))
if test x"$enable_mpdidle" = xyes; then
   AC_DEFINE(ENABLE_MPDIDLE, 1, [Use MPD idle mode (reduce CPU and network usage but still experimental)])
fi
AM_CONDITIONAL(MPD_GLIB, test x"$enable_mpdidle" = xyes)

dnl ================================================================
dnl Stored Playlists
dnl ================================================================
AC_ARG_ENABLE(playlists,
              AC_HELP_STRING([--disable-playlists],
                             [Disable support of playlists]))
if test x"$enable_playlists" != xno; then
   AC_DEFINE(ENABLE_STOREDPLAYLISTS, 1, [Add support of playlists])
fi

dnl ================================================================
dnl Eggtrayicon
dnl ================================================================
AC_ARG_ENABLE(eggtrayicon,
              AC_HELP_STRING([--disable-eggtrayicon],
                             [Disable support of eggtrayicon]))
if test x"$enable_eggtrayicon" != xno; then
   AC_DEFINE(ENABLE_EGGTRAYICON, 1, [Add support of eggtrayicon])
fi
AM_CONDITIONAL(USE_EGGTRAYICON, test x"$enable_eggtrayicon" != xno)

dnl ================================================================
dnl DBUS
dnl ================================================================
AC_ARG_ENABLE(dbus,
              AC_HELP_STRING([--disable-dbus],
                             [Disable support of dbus]))
if test x"$enable_dbus" != xno; then
   PKG_CHECK_MODULES(DBUS, [dbus-glib-1 >= 0.35])
   DBUS_CFLAGS="$DBUS_CFLAGS -DDBUS_API_SUBJECT_TO_CHANGE"
   DBUS_GLIB_BIN="`$PKG_CONFIG --variable=exec_prefix dbus-glib-1`/bin"
   AC_SUBST(DBUS_GLIB_BIN)
fi
AM_CONDITIONAL(USE_DBUS, test x"$enable_dbus" != xno)

dnl ================================================================
dnl Libnotify
dnl ================================================================
AC_ARG_ENABLE(notify,
              AC_HELP_STRING([--disable-notify],
                             [Disable support of libnotify]))
if test x"$enable_notify" != xno; then
   PKG_CHECK_MODULES(NOTIFY, libnotify)

fi
AM_CONDITIONAL(USE_NOTIFY, test x"$enable_notify" != xno)

dnl ================================================================
dnl AUDIOSCROBBLER
dnl ================================================================
AC_ARG_ENABLE(audioscrobbler,
              AC_HELP_STRING([--disable-audioscrobbler],
                             [Disable support of audioscrobbler]))
if test x"$enable_audioscrobbler" != xno; then
        PKG_CHECK_MODULES(SOUP,                            \
                          libsoup-2.4,
	 		  have_libsoup24=yes,
	 	          have_libsoup24=no)
	 if test x"$have_libsoup24" = "xno"; then
		PKG_CHECK_MODULES(SOUP,
				  libsoup-2.2)
	 fi
	 if test x"$have_libsoup24" = "xyes"; then
	 	AC_DEFINE(HAVE_LIBSOUP_2_4, 1, [Define if libsoup 2.4 support is enabled])
	 else
		AC_DEFINE(HAVE_LIBSOUP_2_2, 1, [Define if libsoup 2.2 support is enabled])
	 fi
fi
AM_CONDITIONAL(USE_AUDIOSCROBBLER, test x"$enable_audioscrobbler" != xno)

dnl ================================================================
dnl Avahi
dnl ================================================================
AC_ARG_ENABLE(avahi,
              AC_HELP_STRING([--disable-avahi],
                             [Disable support of avahi]))
if test x"$enable_avahi" != xno; then
   AC_DEFINE(ENABLE_AVAHI, 1, [Add support of avahi])
   PKG_CHECK_MODULES(AVAHI, [avahi-client
                             avahi-glib])
   AC_SUBST(AVAHI_CFLAGS)
   AC_SUBST(AVAHI_LIBS)
fi
AM_CONDITIONAL(USE_AVAHI, test x"$enable_avahi" != xno)

dnl ================================================================
dnl Taglib
dnl ================================================================
AC_ARG_ENABLE(taglib,
              AC_HELP_STRING([--disable-taglib],
                             [Disable support of Taglib]))
if test x"$enable_taglib" != xno; then
   AC_DEFINE(ENABLE_TAGLIB, 1, [Add support of taglib])
   PKG_CHECK_MODULES(TAGLIB, [taglib_c])
   AC_SUBST(TAGLIB_CFLAGS)
   AC_SUBST(TAGLIB_LIBS)
fi

PLUGINDIR='${libdir}/ario/plugins'
AC_SUBST(PLUGINDIR)

PLUGIN_DATA_DIR='${pkgdatadir}/plugins'
AC_SUBST(PLUGIN_DATA_DIR)

AC_OUTPUT([
Makefile
bindings/Makefile
bindings/python/Makefile
src/Makefile
data/Makefile
data/icons/Makefile
data/icons/hicolor/Makefile
data/icons/hicolor/16x16/Makefile
data/icons/hicolor/16x16/apps/Makefile
data/icons/hicolor/22x22/Makefile
data/icons/hicolor/22x22/apps/Makefile
data/icons/hicolor/24x24/Makefile
data/icons/hicolor/24x24/apps/Makefile
data/icons/hicolor/32x32/Makefile
data/icons/hicolor/32x32/apps/Makefile
data/icons/hicolor/48x48/Makefile
data/icons/hicolor/48x48/apps/Makefile
data/icons/hicolor/64x64/Makefile
data/icons/hicolor/64x64/apps/Makefile
data/icons/hicolor/72x72/Makefile
data/icons/hicolor/72x72/apps/Makefile
data/icons/hicolor/96x96/Makefile
data/icons/hicolor/96x96/apps/Makefile
data/icons/hicolor/128x128/Makefile
data/icons/hicolor/128x128/apps/Makefile
data/icons/hicolor/256x256/Makefile
data/icons/hicolor/256x256/apps/Makefile
data/icons/hicolor/scalable/Makefile
data/icons/hicolor/scalable/apps/Makefile
plugins/Makefile
plugins/audioscrobbler/Makefile
plugins/filesystem/Makefile
plugins/information/Makefile
plugins/libnotify/Makefile
plugins/mmkeys/Makefile
plugins/radios/Makefile
plugins/wikipedia/Makefile
po/Makefile.in
data/ario.desktop.in
])
echo ""
echo ""
echo "-------------- Options --------------"
if test x${enable_debug} = xyes; then
        echo "Debug information is:                     ENABLED"
else
        echo "Debug information is:                     DISABLED (--enable-debug)"
fi
if test x${enable_xmms2} = xyes; then
        echo "XMMS2 support (VERY EXPERIMENTAL) is:     ENABLED"
else
        echo "XMMS2 support (VERY EXPERIMENTAL) is:     DISABLED (--enable-xmms2)"
fi
if test x${enable_python} = xyes; then
        echo "Python plugins support (Experimental) is: ENABLED"
else
        echo "Python plugins support (Experimental) is: DISABLED (--enable-python)"
fi
if test x${enable_search} != xno; then
        echo "Search view support is:                   ENABLED  (--disable-search)"
else
        echo "Search view support is:                   DISABLED"
fi
if test x${enable_avahi} != xno; then
        echo "Avahi support is:                         ENABLED  (--disable-avahi)"
else
        echo "Avahi support is:                         DISABLED"
fi
if test x${enable_dbus} != xno; then
        echo "DBUS support is:                          ENABLED  (--disable-dbus)"
else
        echo "DBUS support is:                          DISABLED"
fi
if test x${enable_notify} != xno; then
        echo "Libnotify support is:                     ENABLED  (--disable-notify)"
else
        echo "Libnotify support is:                     DISABLED"
fi
if test x${enable_playlists} != xno; then
        echo "Playlists support is:                     ENABLED  (--disable-playlists)"
else
        echo "Playlists support is:                     DISABLED"
fi
if test x${enable_audioscrobbler} != xno; then
        echo "Audioscrobbler support is:                ENABLED  (--disable-audioscrobbler)"
else
        echo "Audioscrobbler support is:                DISABLED"
fi
if test x${enable_mpdidle} = xyes; then
        echo "MPD 'idle' mode support (Experimental) is:ENABLED"
else
        echo "MPD 'idle' mode support (Experimental) is:DISABLED (--enable-mpdidle)"
fi
if test x${enable_taglib} != xno; then
        echo "Taglib support is:                        ENABLED  (--disable-taglib)"
else
        echo "Taglib support is:                        DISABLED"
fi
