plugindir = $(PLUGINDIR)
plugindatadir = $(PLUGIN_DATA_DIR)
plugin_LTLIBRARIES = libmmkeys.la

libmmkeys_la_SOURCES = \
	ario-mmkeys-plugin.c \
        ario-mmkeys-plugin.h
	
libmmkeys_la_LDFLAGS = $(PLUGIN_LIBTOOL_FLAGS)

libmmkeys_la_LIBADD =  $(DBUS_LIBS)

INCLUDES = 						\
	-DLOCALE_DIR=\""$(prefix)/$(DATADIRNAME)/locale"\"	\
	$(DEPS_CFLAGS)   				\
	$(DBUS_CFLAGS)					\
	-I$(top_srcdir) 				\
	-I$(top_srcdir)/src      			\
	-I$(top_srcdir)/src/lib                        	\
	-I$(top_srcdir)/src/plugins                   	\
	-I$(top_srcdir)/src/shell                       \
        -Wcomment -Wformat -Wnonnull -Wimplicit-int -Wimplicit \
        -Wmain -Wmissing-braces -Wparentheses -Wsequence-point \
        -Wreturn-type -Wswitch -Wtrigraphs -Wunused-function \
        -Wunused-label -Wunused-value \
        -Wchar-subscripts -Wmissing-declarations -Wmissing-prototypes \
        -Wnested-externs -Wpointer-arith \
        -Wcast-align -Wall

plugin_in_files = mmkeys.ario-plugin.desktop.in

%.ario-plugin: %.ario-plugin.desktop.in $(INTLTOOL_MERGE) $(wildcard $(top_srcdir)/po/*po) ; $(INTLTOOL_MERGE) $(top_srcdir)/po $< $@ -d -u -c $(top_builddir)/po/.intltool-merge-cache

plugin_DATA = $(plugin_in_files:.ario-plugin.desktop.in=.ario-plugin)

EXTRA_DIST = $(plugin_in_files)

CLEANFILES = $(plugin_DATA)
DISTCLEANFILES = $(plugin_DATA)
