plugindir = $(PLUGINDIR)
plugindatadir = $(PLUGIN_DATA_DIR)
plugin_LTLIBRARIES = liblibnotify.la

liblibnotify_la_SOURCES = \
	ario-libnotify-plugin.c \
        ario-libnotify-plugin.h \
        ario-notifier-libnotify.c \
        ario-notifier-libnotify.h

liblibnotify_la_LDFLAGS = $(PLUGIN_LIBTOOL_FLAGS)

liblibnotify_la_LIBADD =  $(NOTIFY_LIBS) 

INCLUDES = 						\
	-DLOCALE_DIR=\""$(prefix)/$(DATADIRNAME)/locale"\"	\
	$(DEPS_CFLAGS)   				\
	$(NOTIFY_CFLAGS)					\
	-I$(top_srcdir) 				\
	-I$(top_srcdir)/src      			\
	-I$(top_srcdir)/src/lib                        	\
	-I$(top_srcdir)/src/plugins                   	\
	-I$(top_srcdir)/src/shell                       \
	-Wall -Wimplicit -Wunused-label \
	-Wmissing-declarations -Wmissing-prototypes \
	-Wnested-externs -Wpointer-arith \
	-Wcast-align -Wpointer-sign \
	-Wclobbered -Wempty-body -Wignored-qualifiers \
	-Wmissing-field-initializers -Wmissing-parameter-type \
	-Wold-style-declaration -Woverride-init \
	-Wsign-compare -Wtype-limits -Wuninitialized \
	-Wbad-function-cast -Wold-style-definition

plugin_in_files = libnotify.ario-plugin.desktop.in

%.ario-plugin: %.ario-plugin.desktop.in $(INTLTOOL_MERGE) $(wildcard $(top_srcdir)/po/*po) ; $(INTLTOOL_MERGE) $(top_srcdir)/po $< $@ -d -u -c $(top_builddir)/po/.intltool-merge-cache

plugin_DATA = $(plugin_in_files:.ario-plugin.desktop.in=.ario-plugin)

EXTRA_DIST = $(plugin_in_files)

CLEANFILES = $(plugin_DATA)
DISTCLEANFILES = $(plugin_DATA)
