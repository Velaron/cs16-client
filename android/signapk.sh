#!/bin/bash
# Sample usage is as follows;
# ./signapk myapp.apk debug.keystore android androiddebugkey
# 
# param1, APK file: Calculator_debug.apk
# param2, keystore location: ~/.android/debug.keystore
# param3, key storepass: android
# param4, key alias: androiddebugkey

USER_HOME=$(eval echo ~${SUDO_USER})

APK=$1
KEYSTORE="${2}"
ALIAS="${3}"

# get the filename
APK_BASENAME=$(basename $APK)
SIGNED_APK="signed_"$APK_BASENAME

#debug
echo param1 $APK
echo param2 $KEYSTORE
echo param3 $ALIAS

# delete META-INF folder
zip -d $APK META-INF/\*

# sign APK
jarsigner -verbose -sigalg SHA1withRSA -digestalg SHA1 -keystore $KEYSTORE $APK $ALIAS
#verify
jarsigner -verify $APK

#zipalign
rm $SIGNED_APK
$HOME/.android/build-tools/27.0.2/zipalign -v 4 $APK $SIGNED_APK 
