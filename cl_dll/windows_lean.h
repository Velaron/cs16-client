// this usually must be included first!

#ifndef HL_WINDOWS_LEAN
#define HL_WINDOWS_LEAN

#if _WIN32
#define WIN32_LEAN_AND_MEAN		// Exclude rarely-used stuff from Windows headers
#define WIN32_EXTRA_LEAN
#define NOMINMAX
#define HSPRITE WINDOWS_HSPRITE
#include <windows.h>
#undef HSPRITE
#endif

#endif
