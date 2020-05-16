#include <stdio.h>
#include "cacheutils.h"
#include "communication.h"

int main(){
    map_handle_t* handle;
    map_file("chaton2.jpg", &handle);

    sendByte(handle->mapping, 0xAA);
    sendByte(handle->mapping, 0xAA);
    sendByte(handle->mapping, 0xAA);
    sendByte(handle->mapping, 0xAA);
    sendByte(handle->mapping, 0xAA);
    sendByte(handle->mapping, 0xAB);

    unmap_file(handle);
    return 0;
}
