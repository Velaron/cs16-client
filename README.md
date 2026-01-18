# CS16Client-OpenBSD

https://github.com/Velaron/cs16-client
"ported" to OpenBSD.

# Building & Installing

building is the same as the Linux build instructions of the https://github.com/Velaron/cs16-client
but here they are:
cmake -S . -B build
cmake --build build --config Release
cmake --install build --prefix <path-to-your-installation>

# Status

- Tested on OpenBSD version 7.8
- Using AMD64 architecture
- Many multiplayer servers do not work due to steam requirements
- Takes a long time to download resources from some servers, and some simply do not work because of steam auth
- Training mode works (needs more testing)
- Bots not tested yet
- No known issues yet

# TODO
- test bots
- maybe try to get added to ports tree

# Credits
original project: https://github.com/Velaron/cs16-client
