#!/bin/sh

mkdir -p pak/
mkdir -p assets/
cp -vr extras_src/CS16client_extrass/* pak/
cp -vr extras_src/CS16Client_vgui_buy_classic/touch/* pak/touch/
cp -vr extras_src/CS16client_vgui_radio/touch/* pak/touch/
cp -vr extras_src/CS16Client_vgui_team_classic/touch/* pak/touch/
python2 makepak.py pak/ assets/extras.pak

ndk-build NDK_TOOLCHAIN_VERSION=4.8 NDK_DEBUG=0 V=0 -j2
ant debug
#jarsigner -verbose -sigalg SHA1withRSA -digestalg SHA1 -keystore ../../myks.keystore bin/cs16-client-release-unsigned.apk xashdroid -tsa https://timestamp.geotrust.com/tsa
#/home/a1ba/.android/android-sdk-linux/build-tools/22.0.1/zipalign 4 bin/cs16-client-release-unsigned.apk bin/cs16-client.apk
#adb install -r -f bin/cs16-client-debug.apk
mv bin/cs16-client-debug.apk cs16-client-debug-`date +"%d-%m-%y-%H-%M"`.apk
