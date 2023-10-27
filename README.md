# CS16Client [![Build Status](https://github.com/Velaron/cs16-client/actions/workflows/build.yml/badge.svg)](https://github.com/Velaron/cs16-client/actions) <img align="right" width="128" height="128" src="https://github.com/Velaron/cs16-client/raw/main/app/src/main/ic_launcher-playstore.png" alt="CS16Client" />
Counter-Strike reverse-engineered.

## Donate
[![Buy Me A Coffee](https://www.buymeacoffee.com/assets/img/custom_images/orange_img.png)](https://www.buymeacoffee.com/velaron)

[Support](https://www.buymeacoffee.com/velaron) me on Buy Me A Coffee, if you like my work and would like to support further development goals, like a working server implementation and reverse-engineering other great mods.

## Download
You can download a build at the [Releases](https://github.com/Velaron/cs16-client/releases/tag/continuous) section, or use these links:
* [Android](https://github.com/Velaron/cs16-client/releases/download/continuous/cs16-client.apk)
* [Linux](https://github.com/Velaron/cs16-client/releases/download/continuous/cs16-client_linux_i386.tar.gz) (not recommended, just use the Steam version)
* [Windows](https://github.com/Velaron/cs16-client/releases/download/continuous/cs16-client_win32_x86.zip) (not recommended, same as above)

## Installation
To run CS16Client you need the latest developer build of Xash3D FWGS, which you can get [here](https://github.com/FWGS/xash3d-fwgs/releases/tag/continuous) (unless on [Android](https://github.com/Velaron/xash3d-fwgs/releases/tag/continuous-android)).
You have to own the [game on Steam](https://store.steampowered.com/app/10/CounterStrike//) and copy `valve` and `cstrike` folders into your Xash3D FWGS directory.
After that, just install the APK and run.

## Building
Clone the source code:
```
git clone https://github.com/Velaron/cs16-client --recursive
```
### Windows
```
cmake -A Win32 -S . -B build
cmake --build build --config Release
```
### Linux
```
cmake -S . -B build
cmake --build build --config Release
```
### Android
```
./gradlew assembleRelease
```