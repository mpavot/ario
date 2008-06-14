AC_PREREQ(2.59)

m4_define(ario_version,1.1)

PACKAGE=Ario

AC_INIT(ario,ario_version)
AM_CONFIG_HEADER(config.h)
AM_INIT_AUTOMAKE($PACKAGE,ario_version)

AC_PROG_CC
AC_PROG_INSTALL
AC_STDC_HEADERS
AM_PROG_LIBTOOL
AC_PROG_INTLTOOL([0.35.0])

PKG_CHECK_MODULES(DEPS, [gtk+-2.0 >= 2.12
                         glib-2.0 >= 2.4
                         gobject-2.0 >= 2.4
                         libglade-2.0
                         gmodule-2.0
                         gthread-2.0
                         libcurl
                         gnutls])

AC_SUBST(DEPS_CFLAGS)
AC_SUBST(DEPS_LIBS)

dnl i18n
GETTEXT_PACKAGE=Ario
AC_SUBST(GETTEXT_PACKAGE)
AC_DEFINE_UNQUOTED(GETTEXT_PACKAGE,"$GETTEXT_PACKAGE", [Gettext package])
AM_GLIB_GNU_GETTEXT

dnl Debugging information
AC_ARG_ENABLE(debug,
              AC_HELP_STRING([--enable-debug],
                             [Add runtime debugging information (slower)]))
if test x"$enable_debug" = xyes; then
   AC_DEFINE(DEBUG, 1, [Add runtime debugging information (slower)])
fi

dnl Search
AC_ARG_ENABLE(search,
              AC_HELP_STRING([--disable-search],
                             [Disable support of search view]))
if test x"$enable_search" != xno; then
   AC_DEFINE(ENABLE_SEARCH, 1, [Add support of search view])
   AC_DEFINE(MULTIPLE_VIEW, 1, [Radio or search or playlists])
fi

dnl Stored Playlists
AC_ARG_ENABLE(playlists,
              AC_HELP_STRING([--disable-playlists],
                             [Disable support of playlists]))
if test x"$enable_playlists" != xno; then
   AC_DEFINE(ENABLE_STOREDPLAYLISTS, 1, [Add support of playlists])
   AC_DEFINE(MULTIPLE_VIEW, 1, [Radio or search or playlists])
fi

dnl Eggtrayicon
AC_ARG_ENABLE(eggtrayicon,
              AC_HELP_STRING([--disable-eggtrayicon],
                             [Disable support of eggtrayicon]))
if test x"$enable_eggtrayicon" != xno; then
   AC_DEFINE(ENABLE_EGGTRAYICON, 1, [Add support of eggtrayicon])
fi
AM_CONDITIONAL(USE_EGGTRAYICON, test x"$enable_eggtrayicon" != xno)

dnl DBUS
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

dnl AUDIOSCROBBLER
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

dnl Avahi
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

PLUGINDIR='${libdir}/ario/plugins'
AC_SUBST(PLUGINDIR)

AC_OUTPUT([
Makefile
src/Makefile
data/Makefile
plugins/Makefile
plugins/audioscrobbler/Makefile
plugins/filesystem/Makefile
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
