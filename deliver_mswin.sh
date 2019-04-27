#!/bin/sh

#Core
mkdir -p msbuild
cp src/.libs/libario-0.dll msbuild
cp src/.libs/ario.exe msbuild
cp -r /mingw64/share/ario/* msbuild

#locale
mkdir -p msbuild/locale
for f in po/*.gmo
do
	lang=`echo $f | sed "s/po\///g" | sed "s/\.gmo//g"`
	mkdir -p msbuild/locale/$lang
	mkdir -p msbuild/locale/$lang/LC_MESSAGES
	cp $f msbuild/locale/$lang/Ario.mo
done

#plugins
cp plugins/filesystem/filesystem.ario-plugin.w32 msbuild/plugins/filesystem.ario-plugin
cp plugins/filesystem/.libs/libfilesystem-0.dll msbuild/plugins/libfilesystem.dll
cp plugins/information/information.ario-plugin.w32 msbuild/plugins/information.ario-plugin
cp plugins/information/.libs/libinformation-0.dll msbuild/plugins/libinformation.dll
cp plugins/radios/radios.ario-plugin.w32 msbuild/plugins/radios.ario-plugin
cp plugins/radios/.libs/libradios-0.dll msbuild/plugins/libradios.dll

#deps
ldd src/.libs/libario-0.dll | grep '\/mingw.*\.dll' -o | xargs -I{} cp "{}" msbuild

#icons
cp -R /mingw64/share/icons/* msbuild/art

#pixbuf loaders
mkdir -p msbuild/lib/gdk-pixbuf-2.0/2.10.0
cp -r /mingw64/lib/gdk-pixbuf-2.0/2.10.0/loaders* msbuild/lib/gdk-pixbuf-2.0/2.10.0