#include <stdio.h>
#include <spu_mfcio.h>
#include <spu_intrinsics.h>
#include "cell_util.h"
#include "c63.h"
addr64 global_work;
workitem_t local_work;

int main(unsigned long long speid, addr64 argp, addr64 envp) {
    global_work = envp;
    fprintf(stderr, "Starting SPE \n");
    while(1) {
        int tag = 1, tag_mask = 1 << tag;
        int offset = spu_read_in_mbox() * sizeof(workitem_t);
        addr64 work_addr;
        work_addr.ull = global_work.ull + offset;
	    spu_mfcdma64(&local_work, work_addr.ui[0], work_addr.ui[1], 1, tag, MFC_GET_CMD);
        mfc_write_tag_mask(tag_mask);
        mfc_read_tag_status_all();
    }
	fprintf(stderr, "Stopping SPE \n");
    return 0;

}
