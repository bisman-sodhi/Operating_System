#ifndef MEMORY_H
#define MEMORY_H

#include "simulator.h"

typedef struct Memory_Segments_struct{

    bool mem_seg[8];

} Mem_Segs;

extern Mem_Segs* mem_seg_ptr;

int find_available_mem_seg(Mem_Segs* memory_segment_ptr);

#endif