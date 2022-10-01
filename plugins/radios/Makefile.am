plugindir = $(PLUGINDIR)
plugindatadir = $(PLUGIN_DATA_DIR)
plugin_LTLIBRARIES = libradios.la

libradios_la_SOURCES = \
	ario-radio.c \
	ario-radio.h \
	ario-radios-plugin.c \
        ario-radios-plugin.h
	
libradios_la_LIBADD = $(DEPS_LIBS) -L$(top_srcdir)/src/.libs
libradios_la_LDFLAGS = $(PLUGIN_LIBTOOL_FLAGS) -no-undefined

uixmldir = $(plugindatadir)
uixml_DATA = ario-radios-menu.ui

alldatadir = $(plugindatadir)
alldata_DATA = radios.xml.default

INCLUDES = 						\
	-DLOCALE_DIR=\""$(prefix)/$(DATADIRNAME)/locale"\"	\
	$(DEPS_CFLAGS)   				\
	-I$(top_srcdir) 				\
	-I$(top_srcdir)/src      			\
	-I$(top_srcdir)/src/lib                        	\
	-I$(top_srcdir)/src/plugins                   	\
	-I$(top_srcdir)/src/sources                   	\
	-I$(top_srcdir)/src/shell                       \
	$(WARNINGS)

if WINDOWS
libradios_la_LIBADD += -lario-0
INCLUDES += -mwindows
else
if MACOS 
libradios_la_LIBADD += -lario
else
plugin_in_files = radios.ario-plugin.desktop.in
%.ario-plugin: %.ario-plugin.desktop.in $(INTLTOOL_MERGE) $(wildcard $(top_srcdir)/po/*po) ; $(INTLTOOL_MERGE) $(top_srcdir)/po $< $@ -d -u -c $(top_builddir)/po/.intltool-merge-cache
endif
endif

plugin_DATA = $(plugin_in_files:.ario-plugin.desktop.in=.ario-plugin)

EXTRA_DIST = $(plugin_in_files)\
             $(uixml_DATA)\
             $(alldata_DATA)

CLEANFILES = $(plugin_DATA)
DISTCLEANFILES = $(plugin_DATA)
