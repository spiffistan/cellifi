#include <stdlib.h>
#include <stdio.h>
#include <pthread.h>
#include <libspe2.h>
#include "c63.h"
#include "cell_util.h"
#include "workitem.h"

#define NUM_SPU 6 
extern spe_program_handle_t c63_spu;
int speids[NUM_SPU];
int status[NUM_SPU];
spe_context_ptr_t context[NUM_SPU];
pthread_t spu_threads[NUM_SPU];
workitem_t* work __attribute__((aligned(16)));

void setup_work(int mb_cols, int mb_rows, float *orig, float *ref,struct macroblock *mb, int16_t *res, int cc, workcomplete_t *work_complete, int offset) {

    int i,j;
    //setup work;
    int length = mb_cols * mb_rows;
    for(i = 0; i < mb_rows; i++) {
        for(j = 0; j < mb_cols; j++) {
            work[offset + i * mb_cols + j].block_x = j;
            work[offset + i * mb_cols + j].block_y = i;
            work[offset + i * mb_cols + j].mb_cols = mb_cols;
            work[offset + i * mb_cols + j].mb_rows = mb_rows;
            work[offset + i * mb_cols + j].content_orig.ull = (uint64_t)orig;
            work[offset + i * mb_cols + j].content_ref.ull = (uint64_t)ref;
            work[offset + i * mb_cols + j].mb.ull = (uint64_t)mb;
            work[offset + i * mb_cols + j].work_complete.ull = (uint64_t)work_complete;
            work[offset + i * mb_cols + j].residual.ull = (uint64_t)res;
            work[offset + i * mb_cols + j].stride = mb_cols * 8;
            work[offset + i * mb_cols + j].cc = cc;
        }
    }
}

void calc_me(int mb_cols, int mb_rows) {
    int length = mb_cols * mb_rows;
    length += (mb_cols / 2) * (mb_rows / 2);
    length += (mb_cols / 2) * (mb_rows / 2);
    uint32_t i,count;
    int high_capacity;
    int high_spe;
    for(count = 0; count < length; count++) {
        high_spe = -1;
        high_capacity = 0;
        int capacity = 0;
        while(1) {
            for(i = 0; i < NUM_SPU; i++) {
                capacity = spe_in_mbox_status(context[i]);
                if(capacity > high_capacity) {
                    high_spe = i;
                    high_capacity = capacity;
                }
            }
            if(high_spe != -1)
                break;
        }
        int num_written = spe_in_mbox_write(context[high_spe], &count, 1, SPE_MBOX_ANY_BLOCKING);
        if(num_written == 0) {
            printf("Could not write message!\n");
        }
    }
}


void cell_me(struct c63_common *cm) 
{
    setup_work(cm->mb_cols, cm->mb_rows, cm->curframe->orig->Yfloat, cm->refframe->recons->Yfloat, cm->curframe->mbs[0], cm->curframe->residuals->Ydct, 0,cm->curframe->work_complete_Y, 0);
    setup_work(cm->mb_cols/2, cm->mb_rows/2, cm->curframe->orig->Ufloat, cm->refframe->recons->Ufloat, cm->curframe->mbs[1], cm->curframe->residuals->Udct,1, cm->curframe->work_complete_U, cm->mb_rows * cm->mb_cols);
    setup_work(cm->mb_cols/2, cm->mb_rows/2, cm->curframe->orig->Vfloat, cm->refframe->recons->Vfloat, cm->curframe->mbs[2], cm->curframe->residuals->Vdct,2, cm->curframe->work_complete_V, cm->mb_rows * cm->mb_cols + cm->mb_rows * cm->mb_cols / 4);

    calc_me(cm->mb_cols, cm->mb_rows);
}

void *pthread_helper(void *data) {
	int retval;
    unsigned int entry_point = SPE_DEFAULT_ENTRY; /* Required for continuing 
      execution, SPE_DEFAULT_ENTRY is the standard starting offset. */
    
    spe_context_ptr_t *context = (spe_context_ptr_t*)data;
	/* Load the embedded code into this context */
    spe_program_load(*context, &c63_spu);

	/* Run the SPE program until completion */
	retval = spe_context_run(*context, &entry_point, 0, NULL, work, NULL);

    fprintf(stderr, "SPE_EXITING (retval:%d)!\n", retval);
	pthread_exit(NULL);
}

void init_cell(struct c63_common *cm) {
	int i,j,cc;
    int retvals[NUM_SPU];
    //allocate work.
    work = memalign(16, (cm->mb_cols * cm->mb_rows * 3 / 2 ) * sizeof(workitem_t) );
    for(i = 0; i < NUM_SPU; i++) {
        context[i] = spe_context_create(0,NULL);
    }
    for(i = 0; i < NUM_SPU; i++) {
        /* Create Thread */
    	retvals[i] = pthread_create( &spu_threads[i], /* Thread object */
	    	                     NULL, /* Thread attributes */
		                         pthread_helper, /* Thread function */
		                         &context[i] /* Thread argument */ );
        if(retvals[i] != 0) {
            printf("Error: There is no SPU!");
        }
    }

}
    

void destroy_cell() {
    int i;
    for(i = 0; i < NUM_SPU; i++) {
        pthread_join(spu_threads[i], 0);
        wait(15);
        spe_context_destroy(context[i]);
    }
}
