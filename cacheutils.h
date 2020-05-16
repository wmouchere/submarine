#ifndef CACHEUTILS_H
#define CACHEUTILS_H

#include <sys/types.h>
#include <sys/stat.h>
#include <sys/mman.h>
#include <unistd.h>
#include <stdint.h>
#include <stdlib.h>
#include <fcntl.h>
#include <sched.h>

#ifndef HIDEMINMAX
#define MAX(X,Y) (((X) > (Y)) ? (X) : (Y))
#define MIN(X,Y) (((X) < (Y)) ? (X) : (Y))
#endif

typedef struct map_handle_s {
  int fd;
  size_t range;
  void* mapping;
} map_handle_t;

uint64_t rdtsc_nofence();

uint64_t rdtsc();

void maccess(void* p);

void flush(void* p);

void prefetch(void* p);

void longnop();

void* map_file(const char* filename, map_handle_t** handle);

void unmap_file(map_handle_t* handle);

#endif

