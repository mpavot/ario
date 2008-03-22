plugindir = $(PLUGINDIR)
plugin_LTLIBRARIES = libaudioscrobbler.la

libaudioscrobbler_la_SOURCES = \
	ario-audioscrobbler-plugin.c \
	ario-audioscrobbler.c \
        ario-audioscrobbler.h

libaudioscrobbler_la_LDFLAGS = $(PLUGIN_LIBTOOL_FLAGS)

libaudioscrobbler_la_LIBADD =  $(SOUP_LIBS)

schemasdir	 = $(GCONF_SCHEMA_FILE_DIR)
schemas_in_files = ario-audioscrobbler.schemas.in
schemas_DATA 	 = $(schemas_in_files:.schemas.in=.schemas)
@INTLTOOL_SCHEMAS_RULE@

if GCONF_SCHEMAS_INSTALL
install-data-local:
	        if test -z "$(DESTDIR)" ; then \
	                for p in $(schemas_DATA) ; do \
	                        GCONF_CONFIG_SOURCE=$(GCONF_SCHEMA_CONFIG_SOURCE) $(GCONFTOOL) --makefile-install-rule $(top_builddir)/plugins/audioscrobbler/$$p ; \
	                done \
	        fi
else
install-data-local:
endif

INCLUDES = 						\
	-DLOCALE_DIR=\""$(prefix)/$(DATADIRNAME)/locale"\"	\
        -DPLUGINDIR=\""$(plugindir)/"\"\
	$(DEPS_CFLAGS)   				\
	$(SOUP_CFLAGS)   				\
	$(GCONF_CFLAGS)   				\
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

gladedir = $(plugindir)
glade_DATA = audioscrobbler-prefs.glade

plugin_in_files = audioscrobbler.ario-plugin.desktop.in

%.ario-plugin: %.ario-plugin.desktop.in $(INTLTOOL_MERGE) $(wildcard $(top_srcdir)/po/*po) ; $(INTLTOOL_MERGE) $(top_srcdir)/po $< $@ -d -u -c $(top_builddir)/po/.intltool-merge-cache

BUILT_SOURCES =	\
	$(plugin_in_files:.ario-plugin.desktop.in=.ario-plugin)

plugin_DATA =	\
	$(BUILT_SOURCES)						\
	$(plugin_in_files:.ario-plugin.desktop.in=.ario-plugin)	\
	audioscrobbler.png

EXTRA_DIST = $(plugin_in_files)         \
             $(plugin_DATA)             \
	     $(glade_DATA)	        \
             $(schemas_in_files)        \
             $(uixml_DATA)

CLEANFILES = $(BUILT_SOURCES) \
            $(schemas_DATA)
DISTCLEANFILES = $(BUILT_SOURCES) \
                 $(schemas_DATA)
