bin_PROGRAMS = ario

INCLUDES = -Wcomment -Wformat -Wnonnull -Wimplicit-int -Wimplicit \
	   -Wmain -Wmissing-braces -Wparentheses -Wsequence-point \
	   -Wreturn-type -Wswitch -Wtrigraphs -Wunused-function \
	   -Wunused-label -Wunused-value \
	   -Wchar-subscripts -Wmissing-declarations -Wmissing-prototypes \
	   -Wnested-externs -Wpointer-arith \
	   -Wcast-align -Wall

ario_SOURCES = \
	ario-main.c\
	ario-shell.c\
	ario-shell.h\
	ario-avahi.c\
	ario-avahi.h\
	ario-firstlaunch.c\
	ario-firstlaunch.h\
	ario-browser.c\
	ario-browser.h\
	ario-radio.c\
	ario-radio.h\
	ario-source.c\
	ario-source.h\
	ario-search.c\
	ario-search.h\
	ario-storedplaylists.c\
	ario-storedplaylists.h\
	ario-mpd.c\
	ario-mpd.h\
	libmpdclient.c\
	libmpdclient.h\
	ario-header.c\
	ario-header.h\
	eggtrayicon.c\
	eggtrayicon.h\
	eel-gconf-extensions.c\
	eel-gconf-extensions.h\
	ario-tray-icon.c\
	ario-tray-icon.h\
	ario-playlist.c\
	ario-playlist.h\
	rb-glade-helpers.c\
	rb-glade-helpers.h\
	ario-util.c\
	ario-util.h\
	ario-preferences.c\
	ario-preferences.h\
	ario-cover.c\
	ario-cover.h\
	ario-volume.c\
	ario-volume.h\
	ario-shell-coverdownloader.c\
	ario-shell-coverdownloader.h\
	ario-shell-coverselect.c\
	ario-shell-coverselect.h\
	ario-status-bar.c\
	ario-status-bar.h

ario_LDADD = $(DEPS_LIBS)

AM_CPPFLAGS = $(DEPS_CFLAGS) \
                -DLOCALE_DIR=\""$(pkgdatadir)/locale"\"\
                -DPIXMAP_PATH=\""$(pkgdatadir)/ario/art/"\"\
                -DDATA_PATH=\""$(pkgdatadir)/ario/data/"\"\
                -DUI_PATH=\""$(pkgdatadir)/ario/ui/"\"\
                -DGLADE_PATH=\""$(pkgdatadir)/ario/glade/"\"

