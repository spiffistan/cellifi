#include <stdio.h>
#include <spu_mfcio.h>
#include <spu_intrinsics.h>
#include <simdmath.h>
#include "cell_util.h"
#include "c63.h"
#include "spu.h"
#include "spu_tables.h"
//Function for loading unaligned vectors.
//Ref list 4-11 at http://www.kernel.org/pub/linux/kernel/people/geoff/cell/ps3-linux-docs/ps3-linux-docs-08.06.09/CellProgrammingTutorial/AdvancedCellProgramming.html
vector float vector_load_unaligned(void* ptr)
{
     vector float qw0, qw1, qw;
     int shift;

     qw0 = *((vector float*)(((unsigned int)ptr) & ~0xf));
     qw1 =  *((vector float*)(((unsigned int)(ptr+16)) & ~0xf));
     shift = (unsigned int) ((unsigned int)ptr) & 0xf;
     qw = spu_or(spu_slqwbyte(qw0, shift), spu_rlmaskqwbyte(qw1, shift-16));

     return (qw);
}

void load_orig(workitem_t local_work) {
    int tag = 1, tag_mask = 1 << tag;
    int i;
    
    addr64 addr = local_work.content_orig;
    addr.ull += (local_work.block_y * 8 * local_work.stride + local_work.block_x * 8) * sizeof(float);
    for(i = 0; i < 8; i++) {
       spu_mfcdma64(&original[i*2], addr.ui[0], addr.ui[1],  2 * sizeof(vector float), tag, MFC_GET_CMD);
       addr.ull += local_work.stride * sizeof(float);
    }
    mfc_write_tag_mask(tag_mask);
    mfc_read_tag_status_all();
}
workcomplete_t BOOL_TRUE;
int16_t res[64];
void store_residual(workitem_t local_work) {
    int i,j;
    for(i = 0; i < 8; i++) {
        for(j = 0; j < 8; j++) {
            res[i*8+j] = spu_extract(residual[(i*2)+(j/4)], j & 3);
        }
    }
    
    int tag = 2, tag_mask = 1 << tag;
    addr64 addr = local_work.residual; 
    addr.ull += (local_work.block_y * 8 * local_work.stride + local_work.block_x * 64) * sizeof(int16_t);
    
    spu_mfcdma64(&res, addr.ui[0], addr.ui[1], 64 * sizeof(int16_t), tag, MFC_PUT_CMD);
    
    mfc_write_tag_mask(tag_mask);
    mfc_read_tag_status_all();
    BOOL_TRUE.completed = 1;
    addr = local_work.work_complete;
    addr.ull += (local_work.block_y * local_work.mb_cols + local_work.block_x) * sizeof(workcomplete_t);
    spu_mfcdma64(&BOOL_TRUE, addr.ui[0], addr.ui[1], sizeof(workcomplete_t), tag, MFC_PUT_CMD);
}

void store_mb(workitem_t local_work) {
     int tag = 2, tag_mask = 1 << tag;
     addr64 addr = local_work.mb;
     addr.ull += (local_work.block_y * local_work.mb_cols + local_work.block_x) * sizeof(struct macroblock);
     spu_mfcdma64(&mb, addr.ui[0], addr.ui[1], sizeof(struct macroblock), tag, MFC_PUT_CMD);

}

void load_reference(workitem_t local_work, int left, int top, int right, int bottom) {
    int i;
    int tag = 1, tag_mask = 1 << tag;
    
    addr64 start_addr = local_work.content_ref;
    start_addr.ull += (top * local_work.stride + left) * sizeof(float);
    for(i = 0; i < 40; i++) {
        spu_mfcdma64(&reference[i * 40], start_addr.ui[0], start_addr.ui[1],40 * sizeof(float), tag, MFC_GET_CMD);
        start_addr.ull += local_work.stride * sizeof(float);       
    }

    mfc_write_tag_mask(tag_mask);
    mfc_read_tag_status_all();
}

void init_quanttables() {
    int cc, i;
    for(cc = 0; cc < 3; cc++) {
        for(i = 0; i < 64; i++) {
            q_table[cc][i] = q_table[cc][i] / (25 / 10.0);
        }
    }
}

int main(unsigned long long speid, addr64 argp, addr64 envp) {
    global_work = envp;
    init_quanttables();
    uint32_t tag_id = 1;
    workitem_t local_work;
    while(1) {
        int offset = spu_read_in_mbox() * sizeof(workitem_t);
        addr64 work_addr;
        work_addr.ull = global_work.ull + offset;
        
	    spu_mfcdma64(&local_work, work_addr.ui[0], work_addr.ui[1], sizeof(workitem_t), tag_id, MFC_GET_CMD);
        mfc_write_tag_mask(1 << tag_id);
        mfc_read_tag_status_all();

        calc_me(local_work);
        store_mb(local_work);
        spu_dct_quant_block_8x8(local_work.cc);
        store_residual(local_work); 
        //spu_dequant_idct_block_8x8(local_work.cc);
    }
    return 0;
}
