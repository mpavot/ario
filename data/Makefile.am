SUBDIRS = icons

desktopdir 	 = $(datadir)/applications
desktop_in_files = ario.desktop.in
desktop_DATA 	 = $(desktop_in_files:.desktop.in=.desktop)
@INTLTOOL_DESKTOP_RULE@

artdir = $(pkgdatadir)/art
art_DATA = ario.png		\
	   ario-play.png	\
	   ario-pause.png	\
	   logo.png		\
	   play.png		\
	   shuffle.png		\
	   repeat.png

uidir = $(pkgdatadir)/ui
ui_DATA = ario-ui.xml \
	     ui/browser-prefs.ui	\
	     ui/connection-autodetect.ui	\
	     ui/connection-prefs.ui	\
	     ui/connection-assistant.ui	\
	     ui/connection-widget.ui	\
	     ui/cover-prefs.ui		\
	     ui/cover-progress.ui       \
	     ui/cover-select.ui	        \
	     ui/lyrics-select.ui        \
	     ui/lyrics-prefs.ui         \
	     ui/others-prefs.ui		\
	     ui/playlist-prefs.ui	\
	     ui/server-prefs.ui		\
	     ui/similar-artists.ui      \
	     ui/song-infos.ui	        \
	     ui/stats-prefs.ui 		\
	     ui/trayicon-prefs.ui



alldatadir = $(pkgdatadir)/data
alldata_DATA = profiles.xml.default 

# Rule to update the icon cache after installing the icon
gtk_update_icon_cache = gtk-update-icon-cache -f -t $(datadir)/icons/hicolor

install-data-hook: update-icon-cache
uninstall-hook: update-icon-cache
update-icon-cache:
	@-if test -z "$(DESTDIR)"; then \
		echo "Updating Gtk icon cache."; \
		$(gtk_update_icon_cache); \
	else \
		echo "*** Icon cache not updated.  After (un)install, run this:"; \
		echo "***   $(gtk_update_icon_cache)"; \
	fi

EXTRA_DIST = 				\
	$(desktop_in_files)		\
	$(art_DATA)	\
	$(ui_DATA)	\
	$(alldata_DATA)	\
	$(ui_DATA)

CLEANFILES = $(desktop_DATA)
