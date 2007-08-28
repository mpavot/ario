AC_PREREQ(2.59)

m4_define(ario_version, 0.1)

PACKAGE=Ario
VERSION=0.1

AC_INIT(ario, ario_version)
AC_CONFIG_HEADER(config.h)
AM_INIT_AUTOMAKE($PACKAGE,$VERSION)

AC_PROG_CC
AC_PROG_INSTALL
AC_STDC_HEADERS

AC_PROG_INTLTOOL([0.35.0])

PKG_CHECK_MODULES(DEPS, [gtk+-2.0 >= 2.6 glib-2.0 >= 2.4 gobject-2.0 >= 2.4 libglade-2.0 gconf-2.0 gmodule-2.0 libcurl])

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

dnl Radios
AC_ARG_ENABLE(radios,
              AC_HELP_STRING([--enable-radios],
                             [Add support of internet radios]))
if test x"$enable_radios" = xyes; then
   AC_DEFINE(ENABLE_RADIOS, 1, [Add support of internet radios])
   AC_DEFINE(MULTIPLE_VIEW, 1, [Radio or search])
fi

dnl Search
AC_ARG_ENABLE(search,
              AC_HELP_STRING([--enable-search],
                             [Add support of search view]))
if test x"$enable_search" = xyes; then
   AC_DEFINE(ENABLE_SEARCH, 1, [Add support of search view])
   AC_DEFINE(MULTIPLE_VIEW, 1, [Radio or search])
fi

AC_OUTPUT([
Makefile
src/Makefile
data/Makefile
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
if test x${enable_radios} = xyes; then
        echo "Internet radios support is:               ENABLED"
else
        echo "Internet radios support is:               DISABLED (--enable-radios)"
fi
if test x${enable_search} = xyes; then
        echo "Search view support is:                   ENABLED"
else
        echo "Search view support is:                   DISABLED (--enable-search)"
fi
