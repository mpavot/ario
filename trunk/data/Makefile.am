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
	   volume-medium.png    \
	   volume-zero.png	\
	   repeat.png		\
	   volume-max.png	\
	   volume-min.png

gladedir = $(pkgdatadir)/glade
glade_DATA = glade/browser-prefs.glade	        \
             glade/connection-prefs.glade	\
             glade/connection-assistant.glade	\
             glade/connection-widget.glade	\
	     glade/cover-prefs.glade		\
	     glade/proxy-prefs.glade		\
	     glade/interface-prefs.glade	\
	     glade/trayicon-prefs.glade	        \
	     glade/server-prefs.glade		\
	     glade/cover-progress.glade         \
	     glade/lyrics-select.glade          \
	     glade/lyrics-prefs.glade           \
	     glade/cover-select.glade	        \
	     glade/song-infos.glade	        \
	     glade/similar-artists.glade        \
	     glade/stats-prefs.glade

appicon_DATA = ario.png
appicondir = $(datadir)/icons/hicolor/48x48/apps

uidir = $(pkgdatadir)/ui
ui_DATA = ario-ui.xml

alldatadir = $(pkgdatadir)/data
alldata_DATA = profiles.xml.default 

EXTRA_DIST = 				\
	$(desktop_in_files)		\
	$(art_DATA)	\
	$(appicon_DATA)	\
	$(glade_DATA)	\
	$(alldata_DATA)	\
	$(ui_DATA)

CLEANFILES = $(desktop_DATA)
