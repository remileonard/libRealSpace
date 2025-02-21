#pragma once
#ifndef BLOCK_WIDTH
    #define BLOCK_WIDTH_ORIG 20000
    #define BLOCK_WIDTH 20000
    #define BLOCK_PER_MAP_SIDE 18
    #define BLOCK_PER_MAP_SIDE_DIV_2 (BLOCK_PER_MAP_SIDE/2)
    #define BLOCKS_PER_MAP (BLOCK_PER_MAP_SIDE*BLOCK_PER_MAP_SIDE)
    #define BLOCK_COORD_SCALE (BLOCK_WIDTH/BLOCK_WIDTH_ORIG)*1.0f
    #define HEIGH_MAP_SCALE 1.0f
#endif