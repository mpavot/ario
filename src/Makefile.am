BUILT_SOURCES = 
bin_PROGRAMS = ario

noinst_LTLIBRARIES = libario.la

INCLUDES = $(WARNINGS)

ario_SOURCES = ario-main.c
ario_LDADD = libario.la $(DEPS_LIBS)

libario_la_LIBADD = \
        $(DEPS_LIBS)\
        $(AVAHI_LIBS)\
        $(TAGLIB_LIBS)\
        $(XMMS2_LIBS)\
        $(LIBGCRYPT_LIBS)

libario_la_SOURCES = \
	ario-enum-types.c\
	ario-enum-types.h\
	ario-debug.h\
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
	lib/gtk-builder-helpers.c\
	lib/gtk-builder-helpers.h\
	lib/libmpdclient.c\
	lib/libmpdclient.h\
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
	notification/ario-notification-manager.c\
	notification/ario-notification-manager.h\
	notification/ario-notifier.c\
	notification/ario-notifier.h\
	notification/ario-notifier-tooltip.c\
	notification/ario-notifier-tooltip.h\
	playlist/ario-playlist-manager.c\
	playlist/ario-playlist-manager.h\
	playlist/ario-playlist-mode.c\
	playlist/ario-playlist-mode.h\
	playlist/ario-playlist-normal.c\
	playlist/ario-playlist-normal.h\
	playlist/ario-playlist-queue.c\
	playlist/ario-playlist-queue.h\
	playlist/ario-playlist-dynamic.c\
	playlist/ario-playlist-dynamic.h\
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
	preferences/ario-browser-preferences.c\
	preferences/ario-browser-preferences.h\
	preferences/ario-connection-preferences.c\
	preferences/ario-connection-preferences.h\
	preferences/ario-cover-preferences.c\
	preferences/ario-cover-preferences.h\
	preferences/ario-lyrics-preferences.c\
	preferences/ario-lyrics-preferences.h\
	preferences/ario-others-preferences.c\
	preferences/ario-others-preferences.h\
	preferences/ario-playlist-preferences.c\
	preferences/ario-playlist-preferences.h\
	preferences/ario-server-preferences.c\
	preferences/ario-server-preferences.h\
        preferences/ario-stats-preferences.c\
        preferences/ario-stats-preferences.h\
	preferences/ario-trayicon-preferences.c\
	preferences/ario-trayicon-preferences.h\
	preferences/ario-preferences.h\
	servers/ario-mpd.c\
	servers/ario-mpd.h\
	servers/ario-server.c\
	servers/ario-server.h\
	servers/ario-server-interface.c\
	servers/ario-server-interface.h\
	sources/ario-browser.c\
	sources/ario-browser.h\
	sources/ario-tree.c\
	sources/ario-tree.h\
	sources/ario-tree-albums.c\
	sources/ario-tree-albums.h\
	sources/ario-tree-songs.c\
	sources/ario-tree-songs.h\
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
	widgets/ario-connection-widget.c\
	widgets/ario-connection-widget.h\
	widgets/ario-dnd-tree.c\
	widgets/ario-dnd-tree.h\
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
	widgets/ario-tooltip.c\
	widgets/ario-tooltip.h\
	widgets/ario-tray-icon.c\
	widgets/ario-tray-icon.h\
	widgets/ario-volume.c\
	widgets/ario-volume.h

if USE_AVAHI
        libario_la_SOURCES += ario-avahi.c\
	                      ario-avahi.h
endif

if USE_XMMS2
        libario_la_SOURCES += servers/ario-xmms.c\
                	      servers/ario-xmms.h
endif

if ENABLE_PYTHON
        ario_SOURCES += plugins/ario-python-module.c\
                        plugins/ario-python-module.h\
                        plugins/ario-python-plugin.c\
                        plugins/ario-python-plugin.h

        INCLUDES += \
	        $(NO_STRICT_ALIASING_CFLAGS)	\
	        $(PYGTK_CFLAGS)			\
	        $(PYTHON_CFLAGS)		\
	        $(AM_CFLAGS)

        libario_la_LIBADD += \
	        $(top_builddir)/bindings/python/ario.la
endif

INST_H_FILES = \
        plugins/ario-plugin.h \
        servers/ario-server.h \
        sources/ario-source-manager.h \
        sources/ario-source.h \
        shell/ario-shell.h

regenerate-enum-types:
	        (cd $(srcdir) && $(GLIB_MKENUMS) --template ario-enum-types.h.template $(INST_H_FILES)) > ario-enum-types.h
	        (cd $(srcdir) && $(GLIB_MKENUMS) --template ario-enum-types.c.template $(INST_H_FILES)) > ario-enum-types.c

AM_CPPFLAGS = \
                $(DEPS_CFLAGS) \
                $(AVAHI_CFLAGS) \
                $(TAGLIB_CFLAGS) \
		$(LIBGCRYPT_CFLAGS) \
                $(XMMS2_CFLAGS) \
                -DLOCALE_DIR=\""$(datadir)/locale"\"\
                -DPIXMAP_PATH=\""$(pkgdatadir)/art/"\"\
                -DDATA_PATH=\""$(pkgdatadir)/data/"\"\
                -DUI_PATH=\""$(pkgdatadir)/ui/"\"\
        	-DARIO_PLUGIN_DIR=\"$(PLUGINDIR)\"\
        	-DARIO_PLUGIN_DATA_DIR=\"$(PLUGIN_DATA_DIR)\"
if MPD_GLIB
AM_CPPFLAGS += -DMPD_GLIB
endif

AUTHORS.tab : $(top_srcdir)/AUTHORS
	sed -e 's/^/"/' -e 's/$$/",/' < $(top_srcdir)/AUTHORS > $@.tmp
	mv $@.tmp $@

BUILT_SOURCES += AUTHORS.tab

CLEANFILES = $(BUILT_SOURCES)
