plugindir = $(PLUGINDIR)
plugindatadir = $(PLUGIN_DATA_DIR)
plugin_LTLIBRARIES = libinformation.la

libinformation_la_SOURCES = \
	ario-information.c \
	ario-information.h \
	ario-information-plugin.c \
        ario-information-plugin.h
	
libinformation_la_LIBADD = $(DEPS_LIBS) -L$(top_srcdir)/src/.libs
libinformation_la_LDFLAGS = $(PLUGIN_LIBTOOL_FLAGS) -no-undefined

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

uidir = $(plugindatadir)
ui_DATA = information.ui

if WINDOWS
libinformation_la_LIBADD += -lario-0
INCLUDES += -mwindows
else
if MACOS
libinformation_la_LIBADD += -lario
else
plugin_in_files = information.ario-plugin.desktop.in
%.ario-plugin: %.ario-plugin.desktop.in $(INTLTOOL_MERGE) $(wildcard $(top_srcdir)/po/*po) ; $(INTLTOOL_MERGE) $(top_srcdir)/po $< $@ -d -u -c $(top_builddir)/po/.intltool-merge-cache
endif
endif

plugin_DATA = $(plugin_in_files:.ario-plugin.desktop.in=.ario-plugin)

EXTRA_DIST = $(plugin_in_files)\
	     $(ui_DATA)

CLEANFILES = $(plugin_DATA)
DISTCLEANFILES = $(plugin_DATA)
