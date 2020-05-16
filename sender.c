#include <stdio.h>
#include <string.h>
#include <math.h>
#include <pthread.h>
#include <sys/time.h>
#include "cacheutils.h"
#include "communication.h"

typedef struct {
    void* ack_channel;
    uint32_t nb_packets;
    uint8_t* acks;
    uint32_t* acked;
    pthread_mutex_t* mutex_acks;
    pthread_mutex_t* mutex_acked;
} receiver_ack_op_t;

uint32_t fileSize(FILE* file) {
    fseek(file, 0L, SEEK_END);
    uint32_t sz = ftell(file);
    rewind(file);
    return sz;
}

uchar compare_byte(uchar data1, uchar data2) {
    uchar result = 0;
    for(int i = 7; i >= 0; --i) {
        result += ((data1 >> i) ^ (data2 >> i)) & 0xFE;
    }
    return result;
}

uint compare(uchar* data1, uchar* data2, uint32_t len) {
    uint result = 0;
    for(int i = 0; i < len; ++i) {
        result += compare_byte(data1[i], data2[i]);
    }
    return result;
}

void* receiver_ack(void* vop) {
    receiver_ack_op_t* op = (receiver_ack_op_t*) vop;
    
    uint8_t stop = 0;
    
    uchar lastByte = 0x00;
    uint lastByteCount = 0;

    packet_t packet;
    packet.data = malloc(sizeof(uchar)*255);
    packet.valid = 0;

    uint count = 0;

    printf("Ack listening started\n");
    while(!stop) {
        if(!(rdtsc() & (size_t)1 << NEWRYTHM)) {
            lastByte  = lastByte << 1;
            if(count > COUNT_CAP) {
                lastByte |= 0x01;
            }
            count = 0;
            if(packet.valid) {
                packet.count++;
                if(packet.count == 8) {
                    packet.len = lastByte;
                } else if(packet.count == 16) {
                    packet.sqn = (uint32_t)lastByte << 24;
                } else if(packet.count == 24) {
                    packet.sqn += (uint32_t)lastByte << 16;
                } else if(packet.count == 32) {
                    packet.sqn += (uint32_t)lastByte << 8;
                } else if(packet.count == 40) {
                    packet.sqn += lastByte;
                } else if(packet.count % 8 == 0) {
                    if(packet.count/8 - 6 < packet.len) {
                        packet.data[packet.count/8 - 6] = lastByte;
                    } else if(packet.count/8 - 6 == packet.len) {
                        packet.crc = (uint16_t)lastByte << 8;
                    } else if(packet.count/8 - 6 == packet.len + 1) {
                        packet.crc += lastByte;
                        packet.valid = 0;
                        print_packet(&packet);
                        if(calculate_crc(packet.len, packet.sqn, packet.data) == packet.crc) {
                            pthread_mutex_lock(op->mutex_acks);
                            if(!op->acks[packet.sqn]) {
                                op->acks[packet.sqn] = 1;
                                pthread_mutex_lock(op->mutex_acked);
                                *(op->acked) += 1;
                                if(*(op->acked) == op->nb_packets)
                                    stop = 1;
                                pthread_mutex_unlock(op->mutex_acked);
                            }
                            pthread_mutex_unlock(op->mutex_acks);
                        }
                            
                    }
                }
            }
            if(lastByte == 0xAB && !packet.valid) {
                packet.valid = 1;
                packet.count = 0;
            }
            while(!(rdtsc() & (size_t)1 << NEWRYTHM));
        }
        count += flushAndReload(op->ack_channel);
        for (int i = 0; i < 3; ++i)
            sched_yield();
    }
    printf("Ack listening ended\n");
    return NULL;
}

int main(){
    map_handle_t* handle;
    map_file("chaton2.jpg", &handle);

    FILE* file = NULL; 
    file = fopen("img/submarine_small.png","r");
    if(file == (FILE*) NULL) {
        fprintf(stderr, "Impossible d'ouvrir le fichier\n");
        exit(2);
    }
    
    
    uint32_t len = fileSize(file);
    uchar* data = (uchar*) malloc(sizeof(uchar)*len);
    fread(data, sizeof(uchar)*len, 1, file);

    printTab(data, len);

    uint nbPackets = ceil((len+4)/(double)PACKET_LENGTH);

    uint8_t* acks = malloc(sizeof(uint8_t)*nbPackets);
    uint32_t acked = 0;

    receiver_ack_op_t op;
    pthread_mutexattr_t attr;
    pthread_mutexattr_init(&attr);
    pthread_mutex_t mutex_acks;
    pthread_mutex_t mutex_acked;
    pthread_mutex_init(&mutex_acks, &attr);
    pthread_mutex_init(&mutex_acked, &attr);
    pthread_mutexattr_destroy(&attr);
    op.ack_channel = handle->mapping + 4096;
    op.nb_packets = nbPackets;
    op.acks = acks;
    op.acked = &acked;
    op.mutex_acks = &mutex_acks;
    op.mutex_acked = &mutex_acked;
    pthread_t tid;
    pthread_create(&tid, NULL, receiver_ack, &op);

    printf("Sending %d bytes of data over %d packets\n", len, nbPackets);
    struct timeval start, end;
    gettimeofday(&start, NULL);
    pthread_mutex_lock(&mutex_acked);
    while(acked < nbPackets) {
        pthread_mutex_unlock(&mutex_acked);
        for(uint32_t i = 0; i < nbPackets; ++i) {
            pthread_mutex_lock(&mutex_acks);
            if(!acks[0]) { // Keep sending packet 0 until it is acked because the receiver needs it to allocate memory before all.
                i = 0;
            } else if(acks[i]) { // Skip if already acked
                pthread_mutex_unlock(&mutex_acks);
                continue;
            }
            pthread_mutex_unlock(&mutex_acks);
            if(i == 0) {
                uchar* temp = NULL;
                if(nbPackets>1) {
                    temp = malloc(sizeof(uchar)*PACKET_LENGTH);
                    memcpy(temp, &len, 4);
                    memcpy(&temp[4], data, PACKET_LENGTH - 4);
                    sendPacket(handle->mapping, PACKET_LENGTH, i, temp);
                } else {
                    temp = malloc(sizeof(uchar)*(len+4));
                    memcpy(temp, &len, 4);
                    memcpy(&temp[4], data, len);
                    sendPacket(handle->mapping, len+4, i, temp);
                }
                free(temp);
            } else if(i == nbPackets - 1) {
                sendPacket(handle->mapping, (len+4)%PACKET_LENGTH, i, &data[PACKET_LENGTH-4+PACKET_LENGTH*(i-1)]);
            } else {
                sendPacket(handle->mapping, PACKET_LENGTH, i, &data[PACKET_LENGTH-4+PACKET_LENGTH*(i-1)]);
            }
        }
        //lock before condition check
        pthread_mutex_lock(&mutex_acked);
    }
    gettimeofday(&end, NULL);
    float delta_time = (end.tv_sec - start.tv_sec) + (end.tv_usec - start.tv_usec) / (float)10000000;
    printf("Transmission ended in %lf seconds, speed: %lf bps\n", delta_time, len*8/delta_time);

    pthread_join(tid, NULL);
    pthread_mutex_destroy(&mutex_acked);
    pthread_mutex_destroy(&mutex_acks);

    FILE* output = NULL; 
    output = fopen("out/submarine_small_output.png","r");
    if(output == (FILE*) NULL) {
        fprintf(stderr, "Impossible d'ouvrir le fichier\n");
        exit(2);
    }
    uint32_t output_len = fileSize(output);
    uchar* output_data = (uchar*) malloc(sizeof(uchar)*output_len);
    fread(output_data, sizeof(uchar)*output_len, 1, output);

    printf("Difference between the two files : %d bits\n", compare(data, output_data, len));

    free(acks);
    free(data);
    free(output_data);
    fclose(output);
    fclose(file);
    unmap_file(handle);
    return 0;
}
