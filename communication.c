#include "communication.h"

void printTab(uchar* data, uint len) {
    for(int i = 0; i < len; ++i) {
        printf("0x%02X ", data[i]);
    }
    printf("\n");
}

void print_packet(packet_t* packet) {
    printf("PACKET SQN:%d LEN:%d CRC:%d DATA: ", packet->sqn, packet->len, packet->crc);
    printTab(packet->data, packet->len);
}

void copyPackets(packet_t* in, packet_t* out) {
    memcpy(out->data,in->data, 255);
    memcpy(&(out->sqn), &(in->sqn),7);
}

void crc_byte(uint16_t* count, uchar data) {
    for(int i = 0; i<8; i++) {
        if((data >> i) % 2 == 0)
            (*count)++;
    }
}

uint16_t calculate_crc(uchar len, uint32_t sqn, uchar* data) {
    uint16_t result = 0;
    crc_byte(&result, len);
    for(int i = 0; i < 4; ++i) {
        crc_byte(&result, (uchar) sqn >> (i*8));
    }
    for(int i = 0; i < len; ++i) {
        crc_byte(&result, data[i]);
    }
    return result;
}

uint flushAndReload(void* addr) {
    static size_t time = 0;
    static size_t delta = 0;
    time = rdtsc();
    maccess(addr);
    delta = rdtsc() - time;
    flush(addr);
    return delta < MIN_CACHE_MISS_CYCLES;
}

void sendBit(void* addr, uchar bit) {
    // static size_t time = 0;
    // time = rdtsc();
    while(!(rdtsc() & (size_t)1 << NEWRYTHM)) {
    }
    while(rdtsc() & (size_t)1 << NEWRYTHM) {
        accessBit(addr,bit);
    }
}

void sendByte(void* addr, uchar data) {
    for(int i = 7; i>=0; --i) {
        sendBit(addr, ((uchar)1 << i) & data);
    }
}

void sendPacket(void* addr, uchar len, uint32_t sqn, uchar* data) {
    uint16_t crc = calculate_crc(len, sqn, data);
    // Packet start 
    sendByte(addr, 0xAB);
    sendByte(addr, len);
    // Send Sequence Number
    for(int i = 3; i >= 0; --i) {
        sendByte(addr, (uchar) sqn >> (i*8));
    }
    // Send Data
    for(int i = 0; i < len; ++i) {
        sendByte(addr, data[i]);
    }
    // Send CRC
    sendByte(addr, (uchar) (crc >> 8));
    sendByte(addr, (uchar) crc);
    // Send trailing zeroes
    for(int i = 0; i < 50; ++i) {
        sendByte(addr, 0x00);
    }
}