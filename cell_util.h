#include "c63.h"

typedef union {
    unsigned long long ull;
    unsigned int ui[2];
    void *p;
} addr64;

#define spu_mfcdma32(ls, l, sz, tag, cmd) { \
    printf("spu_mfcdma32(%p, %x, %d, %d, %d) -- Line: %d\n", ls, l, sz, tag, cmd, __LINE__); \
    spu_mfcdma32(ls, l, sz, tag, cmd); \
}

#define spu_mfcdma64(ls, h, l, sz, tag, cmd) { \
    printf("spu_mfcdma64(%p, %x, %x, %d, %d, %d) -- Line: %d\n", ls, h, l, sz, tag, cmd, __LINE__); \
    spu_mfcdma64(ls, h, l, sz, tag, cmd); \
} 
void cell_me(struct c63_common *cm);
void init_cell(struct c63_common *cm);
