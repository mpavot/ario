plugindir = $(PLUGINDIR)
plugin_LTLIBRARIES = libwikipedia.la

libwikipedia_la_SOURCES = \
	ario-wikipedia-plugin.c \
        ario-wikipedia-plugin.h

libwikipedia_la_LDFLAGS = $(PLUGIN_LIBTOOL_FLAGS)

uixmldir = $(plugindir)
uixml_DATA = wikipedia-ui.xml

schemasdir	 = $(GCONF_SCHEMA_FILE_DIR)
schemas_in_files = ario-wikipedia.schemas.in
schemas_DATA 	 = $(schemas_in_files:.schemas.in=.schemas)
@INTLTOOL_SCHEMAS_RULE@

if GCONF_SCHEMAS_INSTALL
install-data-local:
	        if test -z "$(DESTDIR)" ; then \
	                for p in $(schemas_DATA) ; do \
	                        GCONF_CONFIG_SOURCE=$(GCONF_SCHEMA_CONFIG_SOURCE) $(GCONFTOOL) --makefile-install-rule $(top_builddir)/plugins/wikipedia/$$p ; \
	                done \
	        fi
else
install-data-local:
endif

INCLUDES = 						\
	-DLOCALE_DIR=\""$(prefix)/$(DATADIRNAME)/locale"\"	\
        -DUI_PATH=\""$(uixmldir)/"\"\
        -DPLUGINDIR=\""$(plugindir)/"\"\
	$(DEPS_CFLAGS)   				\
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
             $(schemas_in_files)        \
             $(uixml_DATA)

CLEANFILES = $(BUILT_SOURCES) \
            $(schemas_DATA)
DISTCLEANFILES = $(BUILT_SOURCES) \
                 $(schemas_DATA)
