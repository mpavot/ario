# sample python plugin
plugindir = $(PLUGINDIR)
plugindatadir = $(PLUGIN_DATA_DIR)
plugin_PYTHON = sample-python.py

plugin_in_files = sample-python.ario-plugin.desktop.in
%.ario-plugin: %.ario-plugin.desktop.in $(INTLTOOL_MERGE) $(wildcard $(top_srcdir)/po/*po) ; $(INTLTOOL_MERGE) $(top_srcdir)/po $< $@ -d -u -c $(top_builddir)/po/.intltool-merge-cache

plugin_DATA = $(plugin_in_files:.ario-plugin.desktop.in=.ario-plugin)

EXTRA_DIST = $(plugin_in_files) sample-python.py

CLEANFILES = $(plugin_DATA)
DISTCLEANFILES = $(plugin_DATA)
