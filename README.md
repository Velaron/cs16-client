# CS16Client
## Counter-Strike 1.6 client.dll rewrite project.

* Based on HLSDK 2.3. 
* Works without VGUI.
* Runs on your phone.
* Helpful for modders.

[Latest stable release](https://github.com/SDLash3D/cs16-client/releases/latest)

## How to run

**Install**:

1) Install the APK from [Google Play](https://play.google.com/store/apps/details?id=in.celest.xash3d.cs16client).

2) Install the latest [Xash3D FWGS](https://play.google.com/store/apps/details?id=in.celest.xash3d.hl).

3) Copy `cstrike` and `valve` folders from your **Steam CS1.6** installation to `xash` folder on SDCard. 

4) Run CS16Client and enjoy!

### How to build

* Install Android NDK and SDK.
* Run script named "build" in android/ directory. 
NOTE: Don't forget to init git submodules!

Other platforms than Android is **not** officially supported. Issues for platforms other than Android will not be accepted.

## Contributing

There is many things must be done. For example:

* Translations!
* Better and user-friendly Java Launcher design.
* Fixing crashes.
* CZero support.

#### CS1.6 incompability
1) Explain what's wrong with cs16-client.

2) Attach a screenshot from Steam version of CS1.6. 

#### Crashes or bugs
1) Explain what's wrong with cs16-client

2) Attach a screenshot with cs16-client. Attach an engine.log. (if engine.log isn't written, rerun engine with `-log` parameter)

If you are experiencing bug on Android, attach an ADB log. 

### Code guide

For some reason, originally client is more "C with classes" than "C++". Someday it will be refactored, but now I recommend to use "C++" if it looks more convenient. 

For example, if you need to work with vectors, use `Vector` class, instead of `float[3]`. They are data-compatible, but `Vector` is more convenient for C++ code.

For code-style guide: Use `.clang-format`, Luke!
