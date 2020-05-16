#include <stdio.h>
#include "cacheutils.h"
#include "communication.h"

int main(){
    map_handle_t* handle;
    map_file("chaton2.jpg", &handle);
    
    uint count = 0;
    //synchro();
    //size_t reset_time = rdtsc();
    while(1) {
        if(!(rdtsc() & (size_t)1 << NEWRYTHM)) {
            printf("count: %d\n", count);
            count = 0;
            while(!(rdtsc() & (size_t)1 << NEWRYTHM));
            //reset_time = rdtsc();
        }
        count += flushAndReload(handle->mapping);
        for (int i = 0; i < 3; ++i)
            sched_yield();
    }

    unmap_file(handle);
    return 0;
}