bin_PROGRAMS = ario

INCLUDES = -Wcomment -Wformat -Wnonnull -Wimplicit-int -Wimplicit \
	   -Wmain -Wmissing-braces -Wparentheses -Wsequence-point \
	   -Wreturn-type -Wswitch -Wtrigraphs -Wunused-function \
	   -Wunused-label -Wunused-value \
	   -Wchar-subscripts -Wmissing-declarations -Wmissing-prototypes \
	   -Wnested-externs -Wpointer-arith \
	   -Wcast-align -Wall

ario_SOURCES = \
	ario-debug.h\
	ario-main.c\
	ario-mpd.c\
	ario-mpd.h\
	ario-profiles.c\
	ario-profiles.h\
	ario-util.c\
	ario-util.h\
	covers/ario-cover-amazon.c\
	covers/ario-cover-amazon.h\
	covers/ario-cover.c\
	covers/ario-cover.h\
	covers/ario-cover-handler.c\
	covers/ario-cover-handler.h\
	covers/ario-cover-lastfm.c\
	covers/ario-cover-lastfm.h\
	covers/ario-cover-local.c\
	covers/ario-cover-local.h\
	covers/ario-cover-manager.c\
	covers/ario-cover-manager.h\
	covers/ario-cover-provider.c\
	covers/ario-cover-provider.h\
        lib/ario-conf.c\
	lib/ario-conf.h\
	lib/bacon-message-connection.c\
	lib/bacon-message-connection.h\
	lib/eggtrayicon.c\
	lib/eggtrayicon.h\
	lib/libmpdclient.c\
	lib/libmpdclient.h\
	lib/rb-glade-helpers.c\
	lib/rb-glade-helpers.h\
	lib/libsexy/sexy-tooltip.c\
	lib/libsexy/sexy-tooltip.h\
        lyrics/ario-lyrics-leoslyrics.c\
        lyrics/ario-lyrics-leoslyrics.h\
        lyrics/ario-lyrics-lyricwiki.c\
        lyrics/ario-lyrics-lyricwiki.h\
        lyrics/ario-lyrics-manager.c\
        lyrics/ario-lyrics-manager.h\
        lyrics/ario-lyrics-provider.c\
        lyrics/ario-lyrics-provider.h\
        lyrics/ario-lyrics.c\
        lyrics/ario-lyrics.h\
	plugins/ario-module.c\
	plugins/ario-module.h\
	plugins/ario-plugin.c\
	plugins/ario-plugin.h\
	plugins/ario-plugin-info.c\
	plugins/ario-plugin-info.h\
	plugins/ario-plugin-info-priv.h\
	plugins/ario-plugin-manager.c\
	plugins/ario-plugin-manager.h\
	plugins/ario-plugins-engine.c\
	plugins/ario-plugins-engine.h\
	preferences/ario-connection-preferences.c\
	preferences/ario-connection-preferences.h\
	preferences/ario-cover-preferences.c\
	preferences/ario-cover-preferences.h\
	preferences/ario-interface-preferences.c\
	preferences/ario-interface-preferences.h\
	preferences/ario-lyrics-preferences.c\
	preferences/ario-lyrics-preferences.h\
	preferences/ario-proxy-preferences.c\
	preferences/ario-proxy-preferences.h\
	preferences/ario-server-preferences.c\
	preferences/ario-server-preferences.h\
        preferences/ario-stats-preferences.c\
        preferences/ario-stats-preferences.h\
	preferences/ario-trayicon-preferences.c\
	preferences/ario-trayicon-preferences.h\
	preferences/ario-preferences.h\
	sources/ario-browser.c\
	sources/ario-browser.h\
	sources/ario-search.c\
	sources/ario-search.h\
	sources/ario-source.c\
	sources/ario-source.h\
	sources/ario-source-manager.c\
	sources/ario-source-manager.h\
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
	shell/ario-shell-similarartists.c\
	shell/ario-shell-similarartists.h\
	widgets/ario-firstlaunch.c\
	widgets/ario-firstlaunch.h\
	widgets/ario-header.c\
	widgets/ario-header.h\
	widgets/ario-lyrics-editor.c\
	widgets/ario-lyrics-editor.h\
	widgets/ario-playlist.c\
	widgets/ario-playlist.h\
        widgets/ario-songlist.c\
	widgets/ario-songlist.h\
	widgets/ario-status-bar.c\
	widgets/ario-status-bar.h\
	widgets/ario-volume.c\
	widgets/ario-volume.h

if USE_EGGTRAYICON
        ario_SOURCES +=  widgets/ario-tray-icon.c\
	                 widgets/ario-tray-icon.h
else
        ario_SOURCES +=  widgets/ario-status-icon.c\
	                 widgets/ario-status-icon.h
endif

if USE_AVAHI
        ario_SOURCES += ario-avahi.c\
	                ario-avahi.h
endif

ario_LDADD = \
        $(DEPS_LIBS)\
        $(AVAHI_LIBS)

AM_CPPFLAGS = \
                $(DEPS_CFLAGS) \
                $(AVAHI_CFLAGS) \
                -DLOCALE_DIR=\""$(pkgdatadir)/locale"\"\
                -DPIXMAP_PATH=\""$(pkgdatadir)/ario/art/"\"\
                -DDATA_PATH=\""$(pkgdatadir)/ario/data/"\"\
                -DUI_PATH=\""$(pkgdatadir)/ario/ui/"\"\
        	-DARIO_PLUGIN_DIR=\"$(PLUGINDIR)\"\
                -DGLADE_PATH=\""$(pkgdatadir)/ario/glade/"\"

