
#ifndef _PELTAR_FIXED_POINT_H_
#define _PELTAR_FIXED_POINT_H_

#include <stdint.h>

#define FIX_SHIFT 14
#define FIX_MULTIPLE (1 << FIX_SHIFT)
#define FIX_MASK (FIX_MULTIPLE - 1)

typedef uint32_t peltar_fixed;

#endif

