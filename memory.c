#include "memory.h"

int find_available_mem_seg(Mem_Segs* memory_segment_ptr) {
    bool seg_avail = FALSE;
    int seg_num = 0;
    for(seg_num; seg_num < 8; seg_num++) {
        if (memory_segment_ptr->mem_seg[seg_num] == FALSE) {
            seg_avail = TRUE;
            memory_segment_ptr->mem_seg[seg_num] = TRUE;
            break;
        }
    }
    if (seg_avail) {
        return seg_num;
    }
    else {
        return -1;
    }
}