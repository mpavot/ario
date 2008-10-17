AUTOMAKE_OPTIONS = foreign 1.6

if ENABLE_PYTHON
        SUBDIRS = bindings
else
        SUBDIRS =
endif
SUBDIRS += data\
           plugins\
           po\
           src

EXTRA_DIST = 			\
	intltool-extract.in 	\
	intltool-merge.in 	\
	intltool-update.in

DISTCLEANFILES = 		\
	intltool-extract 	\
	intltool-merge 		\
	intltool-update
