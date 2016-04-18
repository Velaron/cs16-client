#!/bin/sh

# Clean
rm -rf obj/ libs/armeabi-v7a libs/armeabi libs/x86 bin/ gen/ assets/ pak/

# Regenerate PAK file
mkdir -p pak/
mkdir -p assets/
cp -vr ../3rdparty/extras/* pak/
python2 makepak.py pak/ assets/extras.pak

# Build OpenMP version
ndk-build NDK_TOOLCHAIN_VERSION=4.8 -j5 _CS16CLIENT_ENABLE_OPENMP=1
ant release
jarsigner -verbose -sigalg SHA1withRSA -digestalg SHA1 -keystore ../../myks.keystore bin/cs16-client-release-unsigned.apk xashdroid -tsa https://timestamp.geotrust.com/tsa
/home/a1ba/.android/android-sdk-linux/build-tools/22.0.1/zipalign 4 bin/cs16-client-release-unsigned.apk bin/cs16-client.apk
mv bin/cs16-client.apk cs16-client-release-omp.apk

# Clean out
rm -rf obj/ libs/armeabi-v7a lib/armeabi libs/x86 bin/

# Build NoOpenMP version
ndk-build NDK_TOOLCHAIN_VERSION=4.8 -j5 _CS16CLIENT_ENABLE_OPENMP=0
ant release
jarsigner -verbose -sigalg SHA1withRSA -digestalg SHA1 -keystore ../../myks.keystore bin/cs16-client-release-unsigned.apk xashdroid -tsa https://timestamp.geotrust.com/tsa
/home/a1ba/.android/android-sdk-linux/build-tools/22.0.1/zipalign 4 bin/cs16-client-release-unsigned.apk bin/cs16-client.apk
mv bin/cs16-client.apk cs16-client-release-noomp.apk
