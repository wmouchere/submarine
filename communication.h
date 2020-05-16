#ifndef COMMUNICATION_H
#define COMMUNICATION_H

#include <stdio.h>
#include <string.h>
#include "cacheutils.h"

// #define MIN_CACHE_MISS_CYCLES (240) // William Laptop
#define MIN_CACHE_MISS_CYCLES (280) // William Desktop
#define PACKET_LENGTH (64)
#define COUNT_CAP (200)
#define NEWRYTHM (20)

typedef unsigned int uint;
typedef unsigned char uchar;

typedef struct {
    uchar* data;
    uint32_t sqn;
    uint16_t crc;
    uchar len;
    uint32_t count;
    uchar valid;
} packet_t;

void printTab(uchar* data, uint len);
void print_packet(packet_t* packet);

void copyPackets(packet_t* in, packet_t* out);

void crc_byte(uint16_t* count, uchar data);
uint16_t calculate_crc(uchar len, uint32_t sqn, uchar* data);

uint flushAndReload(void* addr);

void inline accessBit(void* addr, uchar bit) {
    if(bit) {
        maccess(addr);
    }
}
void sendByte(void* addr, uchar data);
void sendBit(void* addr, uchar data);
void sendPacket(void* addr, uchar len, uint32_t sqn, uchar* data);

#endif