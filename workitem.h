#include <stdint.h>
#include <inttypes.h>

typedef union {
    unsigned long long ull;
    unsigned int ui[2];
    void *p;
} addr64;

typedef struct workitem {
    uint8_t block_x;
    uint8_t block_y;
    uint8_t mb_cols;
    uint8_t mb_rows;
    addr64 content_orig;
    addr64 content_ref;
    addr64 mb;
    addr64 residual;
    addr64 work_complete;
    uint16_t stride;
    uint8_t cc;
    uint8_t pad[13];

} workitem_t __attribute__((aligned(16))); 
