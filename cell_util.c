#include <stdlib.h>
#include <stdio.h>
#include <pthread.h>
#include <libspe2.h>
#include "c63.h"
#include "cell_util.h"

#define NUM_SPU 6
extern spe_program_handle_t c63_spu;
int speids[NUM_SPU];
int status[NUM_SPU];
spe_context_ptr_t context[6];
pthread_t spu_threads[NUM_SPU];
workitem_t* work __attribute__((aligned(16)));

void setup_work(int mb_cols, int mb_rows, float *orig, float *ref,struct macroblock *mb, int cc, int offset) {

    int i,j;
    //setup work;
    int length = mb_cols * mb_rows;
    for(i = 0; i < mb_rows; i++) {
        for(j = 0; j < mb_cols; j++) {
            workitem_t *current = &work[offset + i * mb_cols + j];
            current->block_x = j;
            current->block_y = i;
            current->content_orig = orig;
            current->content_ref = ref;
            current->mb = mb;
        }
    }
}

void calc_me(int mb_cols, int mb_rows, int cc) {
    int length = mb_cols * mb_rows;
    uint32_t i,count;
    int high_capacity = 0;
    int high_spe;
    for(count = 0; count < length; count++) {
        high_spe = -1;
        while(1) {
            for(i = 0; i < NUM_SPU; i++) {
                int capacity = spe_in_mbox_status(context[i]);
                if(capacity > high_capacity) {
                    high_spe = i;
                    high_capacity = capacity;
                }
            }
            if(high_spe != -1)
                break;
        }
        fprintf(stderr, "Writing msg to: %d capacity %d \n", context[high_spe], high_capacity);
        int num_written = spe_in_mbox_write(context[high_spe], &count, 1, SPE_MBOX_ANY_BLOCKING);
        if(num_written == 0) {
            printf("Could not write message!\n");
        }
    }
}


void cell_me(struct c63_common *cm) 
{
    setup_work(cm->mb_cols, cm->mb_rows, cm->curframe->orig->Y, cm->refframe->recons->Y, cm->curframe->mbs[0], 0, 0);
    setup_work(cm->mb_cols/2, cm->mb_rows/2, cm->curframe->orig->U, cm->refframe->recons->U, cm->curframe->mbs[1], 1, cm->mb_rows * cm->mb_cols / 4);
    setup_work(cm->mb_cols/2, cm->mb_rows/2, cm->curframe->orig->V, cm->refframe->recons->V, cm->curframe->mbs[2], 2, cm->mb_rows * cm->mb_cols / 4);

    calc_me(cm->mb_cols, cm->mb_rows, 0);
    calc_me(cm->mb_cols/2, cm->mb_rows/2, 1);
    calc_me(cm->mb_cols/2, cm->mb_rows/2, 2);
}
typedef struct p_args {
    spe_context_ptr_t context;
} p_args_t;
void *pthread_helper(void *data) {
	int retval;
    unsigned int entry_point = SPE_DEFAULT_ENTRY; /* Required for continuing 
      execution, SPE_DEFAULT_ENTRY is the standard starting offset. */
    p_args_t *args = (p_args_t*)data;
    fprintf(stderr, "args->context = %x\n", args->context);
	/* Load the embedded code into this context */
    spe_program_load(args->context, &c63_spu);

	/* Run the SPE program until completion */
	retval = spe_context_run(args->context, &entry_point, 0, NULL, &work, NULL);

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
        fprintf(stderr, "context[%d] = %x\n", i, context[i]);
    }
    for(i = 0; i < NUM_SPU; i++) {
    	//Create Contexts
        p_args_t args;
        args.context = context[i];
        /* Create Thread */
    	retvals[i] = pthread_create( &spu_threads[i], /* Thread object */
	    	                     NULL, /* Thread attributes */
		                         pthread_helper, /* Thread function */
		                         &args /* Thread argument */ );
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
