plugindir = $(PLUGINDIR)
plugindatadir = $(PLUGIN_DATA_DIR)
plugin_LTLIBRARIES = libwikipedia.la

libwikipedia_la_SOURCES = \
	ario-wikipedia-plugin.c \
        ario-wikipedia-plugin.h

libwikipedia_la_LDFLAGS = $(PLUGIN_LIBTOOL_FLAGS)

uixmldir = $(plugindatadir)
uixml_DATA = wikipedia-ui.xml

INCLUDES = 						\
	-DLOCALE_DIR=\""$(prefix)/$(DATADIRNAME)/locale"\"	\
	$(DEPS_CFLAGS)   				\
	-I$(top_srcdir) 				\
	-I$(top_srcdir)/src      			\
	-I$(top_srcdir)/src/lib                        	\
	-I$(top_srcdir)/src/plugins                   	\
	-I$(top_srcdir)/src/shell                       \
	$(WARNINGS)

plugin_in_files = wikipedia.ario-plugin.desktop.in

%.ario-plugin: %.ario-plugin.desktop.in $(INTLTOOL_MERGE) $(wildcard $(top_srcdir)/po/*po) ; $(INTLTOOL_MERGE) $(top_srcdir)/po $< $@ -d -u -c $(top_builddir)/po/.intltool-merge-cache

plugin_DATA = $(plugin_in_files:.ario-plugin.desktop.in=.ario-plugin)

icondir = $(plugindatadir)/icons
icon_DATA = wikipedia.png

EXTRA_DIST = $(plugin_in_files)         \
             $(plugin_DATA)             \
             $(icon_DATA)               \
             $(uixml_DATA)

CLEANFILES = $(plugin_DATA)
DISTCLEANFILES = $(plugin_DATA)
