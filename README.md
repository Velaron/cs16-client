# CS16Client
## Counter-Strike 1.6 client.dll rewrite project.

* Based on HLSDK 2.3. 
* Works without VGUI.
* Runs on your phone.
* Helpful for modders.

[Latest stable release](https://github.com/SDLash3D/cs16-client/releases/latest)

## How to run

**Install**:

1) Install the APK from [GitHub Releases](https://github.com/SDLash3D/cs16-client/releases/latest). 
* If you have **multi-core** device, choose APK with `omp` postfix. 
* If you have **single-core** device or you have **problems** with OMP version, choose APK with `noomp` postfix.

2) Install the latest [Xash3D Android](https://github.com/SDLash3D/xash3d-android-project/releases/latest).

3) Copy `cstrike` and `valve` folders from your **Steam CS1.6** installation to `xash` folder on SDCard.

4) Run CS16Client and enjoy!

## Contributing

There is many things must be done. For example:

* Translations!
* Better and user-friendly Java Launcher design.
* Better weapon prediction system, animation fixes.
* Fixing crashes.
* Ports to another platforms.
* CZero support.

### Issues

**Issues temporarily disabled due to spam. Sorry about that.**

#### CS1.6 incompability
1) Explain what's wrong with cs16-client.

2) Attach a screenshot from Steam version of CS1.6. Please, please, don't send issues about modificated servers!

#### Crashes or bugs
1) Explain what's wrong with cs16-client

2) Attach a screenshot with cs16-client. Attach an engine.log. (if engine.log isn't written, rerun engine with `-log` parameter)

If you are experiencing bug on Android, attach an ADB log. 

### Code guide

For some reason, originally client is more "C with classes" than "C++". Someday it will be refactored, but now I recommend to use "C++" if it looks more convenient. 

For example, if you need to work with vectors, use `Vector` class, instead of `float[3]`. They are data-compatible, but `Vector` is more convenient for C++ code.

For code-style guide: Use `.clang-format`, Luke!
