
#ifndef _PELTAR_COLOURS_H_
#define _PELTAR_COLOURS_H_

/* Comment/uncomment to set red/blue channel swapping */
//#define _RED_BLUE_SWAP

#ifndef _RED_BLUE_SWAP
#define DESERT_LIGHT_1 0xffffd3
#define DESERT_LIGHT_2 0xf0ae7c
#define DESERT_DARK_1  0xdcbb95
#define DESERT_DARK_2  0x9f6a48
#define SEA_COLOUR ((0x9 << 8) + 0x30)
#else
#define DESERT_LIGHT_1 0xd3ffff
#define DESERT_LIGHT_2 0x7caef0
#define DESERT_DARK_1  0x95bbdc
#define DESERT_DARK_2  0x486a9f
#define SEA_COLOUR ((0x9 << 8) + (0x30 << 16))
#endif

static inline uint32_t colours_swap_rb(uint32_t c)
{
	return ((c & 0xff) << 16) | (c & 0xff00) | ((c & 0xff0000) >> 16);
}

#endif

