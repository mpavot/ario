@echo off

rmdir /s /q bin\data
rmdir /s /q bin\po

mkdir bin\data
mkdir bin\data\ui
copy data\*.png bin\data
copy data\profiles.xml.default bin\data
copy data\ario-ui.xml bin\data\ui

copy data\ui\*.ui bin\data\ui\

mkdir bin\plugins
mkdir bin\plugins\icons
copy plugins\filesystem\filesystem-ui.xml bin\plugins
copy plugins\filesystem\filesystem.ario-plugin.w32 bin\plugins\filesystem.ario-plugin

copy plugins\wikipedia\wikipedia-ui.xml bin\plugins
copy plugins\wikipedia\wikipedia.png bin\plugins\icons
copy plugins\wikipedia\wikipedia.ario-plugin.w32 bin\plugins\wikipedia.ario-plugin

copy plugins\radios\radios-ui.xml bin\plugins
copy plugins\radios\radios.xml.default bin\plugins
copy plugins\radios\radios.ario-plugin.w32 bin\plugins\radios.ario-plugin

copy plugins\information\information.ui bin\plugins
copy plugins\information\information.ario-plugin.w32 bin\plugins\information.ario-plugin

copy plugins\audioscrobbler\audioscrobbler-prefs.ui bin\plugins
copy plugins\audioscrobbler\audioscrobbler.ario-plugin.w32 bin\plugins\audioscrobbler.ario-plugin
copy plugins\audioscrobbler\audioscrobbler.png bin\plugins\icons

mkdir bin\po

for %%a in (po\*.gmo) do (
	mkdir bin\po\%%~na
	mkdir bin\po\%%~na\LC_MESSAGES
	copy %%a bin\po\%%~na\LC_MESSAGES\Ario.mo
)

copy ..\deps\libgcrypt-1.2.2\bin\libgcrypt-11.dll bin
copy ..\deps\libgcrypt-1.2.2\bin\libgpg-error-0.dll bin
copy ..\deps\libsoup-2.4-1\bin\libsoup-2.4-1.dll bin
copy ..\deps\curl-7.19.2\bin\libcurl.dll bin
copy ..\deps\curl-7.19.2\bin\libeay32.dll bin
copy ..\deps\curl-7.19.2\bin\libidn-11.dll bin
copy ..\deps\curl-7.19.2\bin\libssl32.dll bin
copy ..\deps\gettext-0.14.4\bin\libintl3.dll bin
copy ..\bin-deps\libiconv-1.9.2-1-bin\bin\libiconv2.dll bin
