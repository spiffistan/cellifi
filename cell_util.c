#include <stdlib.h>
#include <stdio.h>
#include <pthread.h>
#include <libspe2.h>
#include "cell_util.h"

#define NUM_SPU 6
extern spe_program_handle_t c63_spu;

spe_context_ptr_t context;
int speids[NUM_SPU];
int status[NUM_SPU];

void *spe_test_function(void *data) {
	int retval;
	unsigned int entry_point = SPE_DEFAULT_ENTRY; /* Required for continuing 
      execution, SPE_DEFAULT_ENTRY is the standard starting offset. */
	spe_context_ptr_t my_context;

	/* Create the SPE Context */
	my_context = spe_context_create(SPE_EVENTS_ENABLE|SPE_MAP_PS, NULL);

	/* Load the embedded code into this context */
	spe_program_load(my_context, &c63_spu);

	/* Run the SPE program until completion */
	do {
		retval = spe_context_run(my_context, &entry_point, 0, NULL, NULL, NULL);
	} while (retval > 0); /* Run until exit or error */

	pthread_exit(NULL);
}

void init_cell() {
	pthread_t spu_threads[6];
	int retvals[6];
	int i;
    for(i = 0; i < NUM_SPU; i++) {
    	/* Create Thread */
    	retvals[i] = pthread_create( &spu_threads[i], /* Thread object */
	    	                     NULL, /* Thread attributes */
		                         spe_test_function, /* Thread function */
		                         NULL /* Thread argument */ );

    	/* Check for thread creation errors */
	    if(retvals[i]) {
	    	fprintf(stderr, "Error creating thread! Exit code is: %d\n", retvals[i]);
	    	exit(1);
    	}
    }
	/* Wait for Thread Completion */
    for(i = 0; i < NUM_SPU; i++) {
	    retvals[i] = pthread_join(spu_threads[i], NULL);
	    if(retvals[i]) {
		    fprintf(stderr, "Error joining thread! Exit code is: %d\n", retvals[i]);
		    exit(1);
	    }
    }
	/* Check for thread joining errors */
}



void destroy_cell() {
    spe_context_destroy(context);
}
