plugindir = $(PLUGINDIR)
plugin_LTLIBRARIES = libwikipedia.la

libwikipedia_la_SOURCES = \
	ario-wikipedia-plugin.c \
        ario-wikipedia-plugin.h

libwikipedia_la_LDFLAGS = $(PLUGIN_LIBTOOL_FLAGS)

uixmldir = $(plugindir)
uixml_DATA = wikipedia-ui.xml

INCLUDES = 						\
	-DLOCALE_DIR=\""$(prefix)/$(DATADIRNAME)/locale"\"	\
	$(DEPS_CFLAGS)   				\
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

plugin_in_files = wikipedia.ario-plugin.desktop.in

%.ario-plugin: %.ario-plugin.desktop.in $(INTLTOOL_MERGE) $(wildcard $(top_srcdir)/po/*po) ; $(INTLTOOL_MERGE) $(top_srcdir)/po $< $@ -d -u -c $(top_builddir)/po/.intltool-merge-cache

BUILT_SOURCES =	\
	$(plugin_in_files:.ario-plugin.desktop.in=.ario-plugin)

plugin_DATA =	\
	$(BUILT_SOURCES)						\
	$(plugin_in_files:.ario-plugin.desktop.in=.ario-plugin)	\
	wikipedia.png

EXTRA_DIST = $(plugin_in_files)         \
             $(plugin_DATA)             \
             $(uixml_DATA)

CLEANFILES = $(BUILT_SOURCES)

DISTCLEANFILES = $(BUILT_SOURCES)
