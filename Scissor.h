#pragma once
#ifndef SCISSOR_H
#define SCISSOR_H

#include "BaseMenu.h"

namespace UI
{
void PushScissor( int x, int y, int w, int h );
inline void PushScissor( Point pt, Size sz ) { PushScissor( pt.x, pt.y, sz.w, sz.h ); }

void PopScissor();
}

#endif // SCISSOR_H
