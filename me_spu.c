#include <stdio.h>
#include <spu_mfcio.h>
#include <spu_intrinsics.h>
#include <simdmath.h>
#include "cell_util.h"
#include "c63.h"
#include "spu.h"

float sad_block(vector float *block1, vector float *block2) {
    sum.vec = (vector float) {0.0f, 0.0f, 0.0f, 0.0f};
    int i,j;
    for(i = 0; i < 8; i++) {
        for(j = 0; j < 2; j++) {
            vector float tmp;
            tmp = spu_sub(block1[i * 2 + j], block2[i * 2 + j]);
            tmp = fabsf4(tmp);
            sum.vec = spu_add(tmp, sum.vec);
        }
    }

    //TODO: Horizontal add
    return sum.part[0] + sum.part[1] + sum.part[2] + sum.part[3];
}

void calc_me(workitem_t work) {
   
    int top,left,right,bottom;
    
    left = work.block_x * 8 - 16;
    right = work.block_x * 8 + 16;
    top = work.block_y * 8 - 16;
    bottom = work.block_y * 8 + 16;
    
    if(left < 0) {
        left = 0;
    }
    if(top < 0) {
        top = 0;
    }
    if(right + 8 > work.mb_cols * 8) {
        left += work.mb_cols * 8 - right - 8;
    }
    if(bottom + 8 > work.mb_rows * 8) {
        top += work.mb_rows * 8 - bottom - 8;
    }
   load_reference(work, left, top, right, bottom);
   load_orig(work);
   
   int i, j, k, l, best_x, best_y;
   float best_sad = 3000000;
   for(i = 0; i < 32; i++) {
        for(j = 0; j < 32; j++) {
            vector float ref_vec[2 * 8];
            for(k = 0; k < 8; k++) {
                for(l = 0; l < 2; l++) {
                    vector float tmp = vector_load_unaligned(&reference[(i + k) * 40 + j + l * 4]);
                    ref_vec[k * 2 + l] = tmp;       
                }
            }
            float sad = sad_block(original, ref_vec);
            if(sad < best_sad) {
                best_sad = sad;
                mb.mv_x = j + left - work.block_x * 8;
                mb.mv_y = i + top - work.block_y * 8;
                mb.use_mv = 1;
                best_x = j;
                best_y = i;
            }
        }
    }
    for(i = 0; i < 8; i++) {
        for(j = 0; j < 2; j++) {
            predicted[i * 2 + j] = vector_load_unaligned(&reference[(best_y + i) * 40 + best_x + j * 4]);
        }
    }
}



