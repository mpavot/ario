desktopdir 	 = $(datadir)/applications
desktop_in_files = ario.desktop.in
desktop_DATA 	 = $(desktop_in_files:.desktop.in=.desktop)
@INTLTOOL_DESKTOP_RULE@

schemasdir	 = $(GCONF_SCHEMA_FILE_DIR)
schemas_in_files = ario.schemas.in
schemas_DATA 	 = $(schemas_in_files:.schemas.in=.schemas)
@INTLTOOL_SCHEMAS_RULE@

artdir = $(pkgdatadir)/ario/art
art_DATA = ario.png		\
	   play.png		\
	   shuffle.png		\
	   volume-medium.png    \
	   volume-zero.png	\
	   repeat.png		\
	   volume-max.png	\
	   volume-min.png

gladedir = $(pkgdatadir)/ario/glade
glade_DATA = connection-prefs.glade	\
	     cover-prefs.glade		\
	     interface-prefs.glade	\
	     server-prefs.glade		\
	     cover-progress.glade	\
	     cover-select.glade

appicon_DATA = ario.png
appicondir = $(datadir)/icons/hicolor/48x48/apps

uidir = $(pkgdatadir)/ario/ui
ui_DATA = ario-ui.xml

alldatadir = $(pkgdatadir)/ario/data
alldata_DATA = radios.xml.default

if GCONF_SCHEMAS_INSTALL
install-data-local:
	        if test -z "$(DESTDIR)" ; then \
	                for p in $(schemas_DATA) ; do \
	                        GCONF_CONFIG_SOURCE=$(GCONF_SCHEMA_CONFIG_SOURCE) $(GCONFTOOL) --makefile-install-rule $(top_builddir)/data/$$p ; \
	                done \
	        fi
	        killall -HUP gconfd-2;
else
install-data-local:
endif
EXTRA_DIST = 				\
	$(desktop_in_files)		\
	$(schemas_in_files)		\
	$(art_DATA)	\
	$(appicon_DATA)	\
	$(glade_DATA)	\
	$(alldata_DATA)	\
	$(ui_DATA)