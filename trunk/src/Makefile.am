bin_PROGRAMS = ario

INCLUDES = -Wcomment -Wformat -Wnonnull -Wimplicit-int -Wimplicit \
	   -Wmain -Wmissing-braces -Wparentheses -Wsequence-point \
	   -Wreturn-type -Wswitch -Wtrigraphs -Wunused-function \
	   -Wunused-label -Wunused-value \
	   -Wchar-subscripts -Wmissing-declarations -Wmissing-prototypes \
	   -Wnested-externs -Wpointer-arith \
	   -Wcast-align -Wall

ario_SOURCES = \
	ario-avahi.c\
	ario-avahi.h\
	ario-cover.c\
	ario-cover.h\
	ario-cover-handler.c\
	ario-cover-handler.h\
	ario-debug.h\
	ario-lyrics.c\
	ario-lyrics.h\
	ario-lyrics-editor.c\
	ario-lyrics-editor.h\
	ario-main.c\
	ario-mpd.c\
	ario-mpd.h\
	ario-profiles.c\
	ario-profiles.h\
	ario-util.c\
	ario-util.h\
	lib/eel-gconf-extensions.c\
	lib/eel-gconf-extensions.h\
	lib/eggtrayicon.c\
	lib/eggtrayicon.h\
	lib/libmpdclient.c\
	lib/libmpdclient.h\
	lib/rb-glade-helpers.c\
	lib/rb-glade-helpers.h\
	lib/libsexy/sexy-tooltip.c\
	lib/libsexy/sexy-tooltip.h\
	preferences/ario-connection-preferences.c\
	preferences/ario-connection-preferences.h\
	preferences/ario-cover-preferences.c\
	preferences/ario-cover-preferences.h\
	preferences/ario-interface-preferences.c\
	preferences/ario-interface-preferences.h\
	preferences/ario-server-preferences.c\
	preferences/ario-server-preferences.h\
        preferences/ario-stats-preferences.c\
        preferences/ario-stats-preferences.h\
	preferences/ario-preferences.h\
	sources/ario-browser.c\
	sources/ario-browser.h\
	sources/ario-radio.c\
	sources/ario-radio.h\
	sources/ario-search.c\
	sources/ario-search.h\
        sources/ario-songlist.c\
	sources/ario-songlist.h\
	sources/ario-source.c\
	sources/ario-source.h\
	sources/ario-storedplaylists.c\
	sources/ario-storedplaylists.h\
	shell/ario-shell.c\
	shell/ario-shell.h\
	shell/ario-shell-coverdownloader.c\
	shell/ario-shell-coverdownloader.h\
	shell/ario-shell-coverselect.c\
	shell/ario-shell-coverselect.h\
	shell/ario-shell-lyrics.c\
	shell/ario-shell-lyrics.h\
	shell/ario-shell-lyricsselect.c\
	shell/ario-shell-lyricsselect.h\
	shell/ario-shell-preferences.c\
	shell/ario-shell-preferences.h\
	shell/ario-shell-songinfos.c\
	shell/ario-shell-songinfos.h\
	widgets/ario-firstlaunch.c\
	widgets/ario-firstlaunch.h\
	widgets/ario-header.c\
	widgets/ario-header.h\
	widgets/ario-playlist.c\
	widgets/ario-playlist.h\
	widgets/ario-status-bar.c\
	widgets/ario-status-bar.h\
	widgets/ario-tray-icon.c\
	widgets/ario-tray-icon.h\
	widgets/ario-volume.c\
	widgets/ario-volume.h


ario_LDADD = $(DEPS_LIBS)

AM_CPPFLAGS = $(DEPS_CFLAGS) \
                -DLOCALE_DIR=\""$(pkgdatadir)/locale"\"\
                -DPIXMAP_PATH=\""$(pkgdatadir)/ario/art/"\"\
                -DDATA_PATH=\""$(pkgdatadir)/ario/data/"\"\
                -DUI_PATH=\""$(pkgdatadir)/ario/ui/"\"\
                -DGLADE_PATH=\""$(pkgdatadir)/ario/glade/"\"

