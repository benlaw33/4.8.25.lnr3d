#pragma once

// Prevent x86 intrinsics from being included on ARM
#define SDL_DISABLE_IMMINTRIN_H 1
#define SDL_DISABLE_MMINTRIN_H 1
#define SDL_DISABLE_XMMINTRIN_H 1