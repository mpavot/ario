## Process this file with automake to produce Makefile.in

noinst_LTLIBRARIES = \
	ario.la

nodist_ario_la_SOURCES = \
	ario.c

ario_la_LDFLAGS = \
	-module -avoid-version

ario_la_LIBADD = \
	$(PYTHON_LIB_LOC)       \
	$(PYTHON_LIBS)		\
	$(PYTHON_EXTRA_LIBS)	\
	$(PYGTK_LIBS)

ario_la_CFLAGS = \
	-I$(top_srcdir) 				\
	-I$(top_srcdir)/src      			\
	-I$(top_srcdir)/src/lib                        	\
	-I$(top_srcdir)/src/plugins                   	\
	-I$(top_srcdir)/src/shell                       \
        $(DEPS_CFLAGS)                  \
	$(NO_STRICT_ALIASING_CFLAGS)	\
	$(PYGTK_CFLAGS)			\
	$(PYTHON_CFLAGS)		\
	$(AM_CFLAGS)

$(top_srcdir)/src/ario-enum-types.h:
	cd $(top_srcdir)/src && $(MAKE) ario-enum-types.h

ario.c: ario.defs ario.override $(top_builddir)/src/ario-enum-types.h
	( cd $(srcdir) && $(PYGTK_CODEGEN) \
		--register $(PYGTK_DEFSDIR)/gdk-types.defs \
		--register $(PYGTK_DEFSDIR)/gtk-types.defs \
		--override $*.override \
		--prefix py$* $(<F) ) > $@ 

BINDING_ARIO_HEADERS_SRCDIR_IN = \
	src/plugins/ario-plugin.h \
	src/servers/ario-server.h \
        src/shell/ario-shell.h

BINDING_HEADERS_BUILDDIR_IN = 

BINDING_ARIO_HEADERS_SRCDIR	:= $(addprefix $(top_srcdir)/,$(BINDING_ARIO_HEADERS_SRCDIR_IN))

BINDING_HEADERS_BUILDDIR	:= $(addprefix $(top_builddir)/,$(BINDING_HEADERS_BUILDDIR_IN))

regenerate-python-binding:
	$(PYGTK_H2DEF) $(sort $(BINDING_ARIO_HEADERS_SRCDIR) $(BINDING_HEADERS_BUILDDIR)) > ario.defs.new

BUILT_SOURCES = \
	ario.c

EXTRA_DIST = \
	ario.override		\
	ario.defs

CLEANFILES = $(BUILT_SOURCES)

dist-hook:
	cd $(distdir); rm -f $(BUILT_SOURCES)
