plugindir = $(PLUGINDIR)
plugin_LTLIBRARIES = libradios.la

libradios_la_SOURCES = \
	ario-radio.c \
	ario-radio.h \
	ario-radios-plugin.c \
        ario-radios-plugin.h
	
libradios_la_LDFLAGS = $(PLUGIN_LIBTOOL_FLAGS)

uixmldir = $(plugindir)
uixml_DATA = radios-ui.xml

alldatadir = $(plugindir)
alldata_DATA = radios.xml.default

INCLUDES = 						\
	-DLOCALE_DIR=\""$(prefix)/$(DATADIRNAME)/locale"\"	\
        -DUI_PATH=\""$(uixmldir)/"\"                    \
        -DDATA_PATH=\""$(alldatadir)/"\"                \
	$(DEPS_CFLAGS)   				\
	-I$(top_srcdir) 				\
	-I$(top_srcdir)/src      			\
	-I$(top_srcdir)/src/lib                        	\
	-I$(top_srcdir)/src/plugins                   	\
	-I$(top_srcdir)/src/sources                   	\
	-I$(top_srcdir)/src/shell

plugin_in_files = radios.ario-plugin.desktop.in

%.ario-plugin: %.ario-plugin.desktop.in $(INTLTOOL_MERGE) $(wildcard $(top_srcdir)/po/*po) ; $(INTLTOOL_MERGE) $(top_srcdir)/po $< $@ -d -u -c $(top_builddir)/po/.intltool-merge-cache

plugin_DATA = $(plugin_in_files:.ario-plugin.desktop.in=.ario-plugin)

EXTRA_DIST = $(plugin_in_files)\
             $(uixml_DATA)\
             $(all_DATA)

CLEANFILES = $(plugin_DATA)
DISTCLEANFILES = $(plugin_DATA)
