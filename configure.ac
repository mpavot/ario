AC_PREREQ([2.71])

m4_define(ario_version,1.6.1)

AC_INIT([Ario],[ario_version],[marc.pavot@gmail.com],[ario])
AM_CONFIG_HEADER(config.h)
AM_INIT_AUTOMAKE([subdir-objects])
m4_ifdef([AM_SILENT_RULES],[AM_SILENT_RULES([yes])])

AC_PROG_CC
AC_PROG_INSTALL

LT_INIT

dnl ================================================================
dnl i18n
dnl ================================================================
IT_PROG_INTLTOOL(0.35.5)
GETTEXT_PACKAGE=ario
AC_SUBST(GETTEXT_PACKAGE)
AC_DEFINE_UNQUOTED(GETTEXT_PACKAGE,"$GETTEXT_PACKAGE", [Gettext package])
AM_GLIB_GNU_GETTEXT

dnl ================================================================
dnl Host
dnl ================================================================
AC_ARG_ENABLE(mswin,
              AS_HELP_STRING([--enable-mswin],[Compile for MS Windows]))
if test x"$enable_mswin" = xyes; then
   AC_DEFINE(WINDOWS, 1, [Compile for MS Windows)])
fi
AM_CONDITIONAL(WINDOWS, test x"$enable_mswin" = xyes)

dnl ================================================================
dnl Dependencies
dnl ================================================================

PKG_CHECK_MODULES(DEPS, [gtk+-3.0
                         glib-2.0 >= 2.4
                         gobject-2.0 >= 2.4
                         gmodule-2.0
                         gthread-2.0
                         gio-2.0
                         libxml-2.0
                         libcurl])

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
-Wmissing-parameter-type \
-Wold-style-declaration -Woverride-init \
-Wsign-compare -Wtype-limits -Wuninitialized \
-Wold-style-definition"

for option in $WARN_CFLAGS; do
        SAVE_CFLAGS="$CFLAGS"
        CFLAGS="$CFLAGS $option"
        AC_MSG_CHECKING([whether gcc understands $option])
        AC_COMPILE_IFELSE([AC_LANG_PROGRAM([[]], [[]])],[has_option=yes],[has_option=no])
        if test x$has_option = xyes; then
                WARNINGS="$WARNINGS $option"
        fi
        AC_MSG_RESULT($has_option)
        CFLAGS="$SAVE_CFLAGS"
        unset has_option
        unset SAVE_CFLAGS
done
unset option

dnl ================================================================
dnl Debugging information
dnl ================================================================
AC_ARG_ENABLE(debug,
              AS_HELP_STRING([--enable-debug],[Add runtime debugging information (slower)]))
if test x"$enable_debug" = xyes; then
   AC_DEFINE(DEBUG, 1, [Add runtime debugging information (slower)])
fi

dnl ================================================================
dnl XMMS2 support
dnl ================================================================
AC_ARG_ENABLE(xmms2,
              AS_HELP_STRING([--enable-xmms2],[Add support of XMMS2 (VERY EXPERIMENTAL)]))
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
dnl libmpdclient2
dnl ================================================================
AC_ARG_ENABLE(libmpdclient2,
              AS_HELP_STRING([--disable-libmpdclient2],[Disable use libmpdclient2 (embedded version of libmpdclient zill be used)]))
if test x"$enable_libmpdclient2" != xno; then
   AC_DEFINE(ENABLE_LIBMPDCLIENT2, 1, [Use libmpdclient2])
   PKG_CHECK_MODULES(LIBMPDCLIENT2, [libmpdclient >= 2.0])
   AC_SUBST(LIBMPDCLIENT2_CFLAGS)
   AC_SUBST(LIBMPDCLIENT2_LIBS)
fi
AM_CONDITIONAL(USE_LIBMPDCLIENT2, test x${enable_libmpdclient2} != xno)

# Silence warning: ar: 'u' modifier ignored since 'D' is the default
AC_SUBST(AR_FLAGS, [cr])

dnl ================================================================
dnl Search
dnl ================================================================
AC_ARG_ENABLE(search,
              AS_HELP_STRING([--disable-search],[Disable support of search view]))
if test x"$enable_search" != xno; then
   AC_DEFINE(ENABLE_SEARCH, 1, [Add support of search view])
fi

dnl ================================================================
dnl MPD idle mode
dnl ================================================================
AC_ARG_ENABLE(mpdidle,
              AS_HELP_STRING([--disable-mpdidle],[Disable MPD idle mode (MPD idle mode reduces CPU and network usage)]))
if test x"$enable_mpdidle" != xno; then
   AC_DEFINE(ENABLE_MPDIDLE, 1, [Use MPD idle mode (reduces CPU and network usage but still experimental)])
fi
AM_CONDITIONAL(MPD_GLIB, test x"$enable_mpdidle" != xno)

dnl ================================================================
dnl Stored Playlists
dnl ================================================================
AC_ARG_ENABLE(playlists,
              AS_HELP_STRING([--disable-playlists],[Disable support of playlists]))
if test x"$enable_playlists" != xno; then
   AC_DEFINE(ENABLE_STOREDPLAYLISTS, 1, [Add support of playlists])
fi

dnl ================================================================
dnl Avahi
dnl ================================================================
AC_ARG_ENABLE(avahi,
              AS_HELP_STRING([--disable-avahi],[Disable support of avahi]))
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
              AS_HELP_STRING([--disable-taglib],[Disable support of Taglib]))
if test x"$enable_taglib" != xno; then
   AC_DEFINE(ENABLE_TAGLIB, 1, [Add support of taglib])
   PKG_CHECK_MODULES(TAGLIB, [taglib_c])
   AC_SUBST(TAGLIB_CFLAGS)
   AC_SUBST(TAGLIB_LIBS)
fi

dnl ================================================================
dnl Deprecations
dnl ================================================================
AC_ARG_ENABLE(deprecations,
              AS_HELP_STRING([--enable-deprecations],[Warn about deprecated usages]))
if test x"$enable_deprecations" = xyes; then
        WARNINGS="$WARNINGS -DG_DISABLE_DEPRECATED \
-DGDK_DISABLE_DEPRECATED \
-DGTK_DISABLE_SINGLE_INCLUDES \
-DGDK_PIXBUF_DISABLE_DEPRECATED \
-DGTK_DISABLE_DEPRECATED \
-DGSEAL_ENABLE \
-DG_DISABLE_SINGLE_INCLUDES"
fi
AC_SUBST(WARNINGS)

PLUGINDIR='${libdir}/ario/plugins'
AC_SUBST(PLUGINDIR)

PLUGIN_DATA_DIR='${pkgdatadir}/plugins'
AC_SUBST(PLUGIN_DATA_DIR)

AC_CONFIG_FILES([
Makefile
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
plugins/filesystem/Makefile
plugins/information/Makefile
plugins/radios/Makefile
po/Makefile.in
data/ario.desktop.in
])
AC_OUTPUT
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
if test x${enable_playlists} != xno; then
        echo "Playlists support is:                     ENABLED  (--disable-playlists)"
else
        echo "Playlists support is:                     DISABLED"
fi
if test x${enable_mpdidle} != xno; then
        echo "MPD 'idle' mode support is:               ENABLED  (--disable-mpdidle)"
else
        echo "MPD 'idle' mode support is:               DISABLED"
fi
if test x${enable_taglib} != xno; then
        echo "Taglib support is:                        ENABLED  (--disable-taglib)"
else
        echo "Taglib support is:                        DISABLED"
fi
if test x${enable_libmpdclient2} != xno; then
        echo "libmpdclient2 support is:                 ENABLED  (--disable-libmpdclient2)"
else
        echo "libmpdclient2 support is:                 DISABLED"
fi
if test x${enable_mswin} = xyes; then
        echo "Compilation for MS Windows is:            ENABLED"
else
        echo "Compilation for MS Windows is:            DISABLED (--enable-mswin)"
fi
