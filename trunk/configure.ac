AC_PREREQ(2.59)

m4_define(ario_version,0.4)

PACKAGE=Ario
VERSION=0.4

AC_INIT(ario,ario_version)
AM_CONFIG_HEADER(config.h)
AM_INIT_AUTOMAKE($PACKAGE,$VERSION)

AC_PROG_CC
AC_PROG_INSTALL
AC_STDC_HEADERS
AM_PROG_LIBTOOL
AC_PROG_INTLTOOL([0.35.0])

PKG_CHECK_MODULES(DEPS, [gtk+-2.0 >= 2.10
                         glib-2.0 >= 2.4
                         gobject-2.0 >= 2.4
                         libglade-2.0
                         gmodule-2.0
                         gthread-2.0
                         libcurl
                         avahi-client
                         avahi-glib])

AC_SUBST(DEPS_CFLAGS)
AC_SUBST(DEPS_LIBS)

dnl i18n
GETTEXT_PACKAGE=Ario
AC_SUBST(GETTEXT_PACKAGE)
AC_DEFINE_UNQUOTED(GETTEXT_PACKAGE,"$GETTEXT_PACKAGE", [Gettext package])
AM_GLIB_GNU_GETTEXT

dnl Gconf
AC_PATH_PROG(GCONFTOOL, gconftool-2)
AM_GCONF_SOURCE_2

dnl Debugging information
AC_ARG_ENABLE(debug,
              AC_HELP_STRING([--enable-debug],
                             [Add runtime debugging information (slower)]))
if test x"$enable_debug" = xyes; then
   AC_DEFINE(DEBUG, 1, [Add runtime debugging information (slower)])
fi

dnl GCONF
AC_ARG_ENABLE(gconf,
              AC_HELP_STRING([--disable-gconf],
                             [Disable support of GConf]))
if test x"$enable_gconf" != xno; then
   AC_DEFINE(ENABLE_GCONF, 1, [Add support of GConf])
   PKG_CHECK_MODULES(GCONF, [gconf-2.0])
   AC_SUBST(GCONF_CFLAGS)
   AC_SUBST(GCONF_LIBS)
fi
AM_CONDITIONAL(USE_GCONF, test x"$enable_gconf" != xno)

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

PLUGINDIR='${libdir}/ario/plugins'
AC_SUBST(PLUGINDIR)

AC_OUTPUT([
Makefile
src/Makefile
data/Makefile
plugins/Makefile
plugins/filesystem/Makefile
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
if test x${enable_gconf} != xno; then
        echo "GConf support is:                         ENABLED  (--disable-gconf)"
else
        echo "GConf support is:                         DISABLED"
fi
if test x${enable_search} != xno; then
        echo "Search view support is:                   ENABLED  (--disable-search)"
else
        echo "Search view support is:                   DISABLED"
fi
if test x${enable_eggtrayicon} != xno; then
        echo "Eggtrayicon support is:                   ENABLED  (--disable-eggtrayicon)"
else
        echo "Eggtrayicon support is:                   DISABLED"
fi
if test x${enable_playlists} != xno; then
        echo "Playlists support is:                     ENABLED  (--disable-playlists)"
else
        echo "Playlists support is:                     DISABLED"
fi
