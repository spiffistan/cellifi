#include <spu_intrinsics.h>
#include "workitem.h"

addr64 global_work;
vector float original[2*8];
vector float predicted[2*8];
vector float residual[2*8];
vector float reconstructed[2*8];
float reference[40*40];
struct macroblock mb;
volatile union {
    vector float vec;
    float part[4];
} sum;


#ifdef DEBUG 
#define spu_mfcdma32(ls, l, sz, tag, cmd) { \
    printf("spu_mfcdma32(%p, %x, %d, %d, %d) File: %s, -- Line: %d\n", ls, l, sz, tag, cmd, __FILE__, __LINE__); \
    spu_mfcdma32(ls, l, sz, tag, cmd); \
}

#define spu_mfcdma64(ls, h, l, sz, tag, cmd) { \
    printf("spu_mfcdma64(%p, %x, %x, %d, %d, %d) File: %s -- Line: %d\n", ls, h, l, sz, tag, cmd, __FILE__ , __LINE__); \
    spu_mfcdma64(ls, h, l, sz, tag, cmd); \
}
#endif

void calc_me(workitem_t);
vector float vector_load_unaligned(void *);
void spu_dequant_idct_block_8x8(uint8_t ccl);
void spu_dct_quant_block_8x8(uint8_t cc);
