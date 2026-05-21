#pragma once
#ifndef STRL_H
#define STRL_H

#include <stddef.h>

#ifdef __cplusplus
extern "C" {
#endif

#ifndef HAVE_STRLCPY
size_t strlcpy( char *dst, const char *src, size_t dsize );
#endif

#ifndef HAVE_STRLCAT
size_t strlcat( char *dst, const char *src, size_t dsize );
#endif

#ifdef __cplusplus
}
#endif

#endif // STRL_H
