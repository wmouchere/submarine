#include <stdio.h>
#include <pthread.h>
#include <semaphore.h>
#include <math.h>
#include "cacheutils.h"
#include "communication.h"

typedef struct {
    void* ack_channel;
    packet_t* buffer;
    sem_t* semempty;
    sem_t* semfull;
} reading_op_t;

void* reader(void* vop) {
    reading_op_t* op = (reading_op_t*) vop;
    void* ack_channel = op->ack_channel;
    packet_t* packet = op->buffer;
    sem_t* semempty = op->semempty;
    sem_t* semfull = op->semfull;

    FILE* file = NULL;

    uint8_t session = 0;
    uint8_t* syns;
    uchar* data;
    uint32_t len;
    uint nb_syns;
    uint nb_packets;

    uint cp = 0;
    while(1) {
        sem_wait(semempty);
        if(packet[cp].crc == calculate_crc(packet[cp].len, packet[cp].sqn, packet[cp].data)) {
            sendPacket(ack_channel, 0, packet[cp].sqn, NULL);
            //print_packet(&packet[cp]);
            if(!session && packet[cp].sqn == 0) {
                printf("Session started.\n");
                session = 1;
                nb_syns = 0;
                memcpy(&len, packet[cp].data, 4);
                nb_packets = ceil((len+4)/(double)PACKET_LENGTH);
                syns = malloc(sizeof(uint8_t)*nb_packets);
                memset(syns, 0, sizeof(uint8_t)*nb_packets); // Initialize table to 0 as it might reuse "dirty" data
                syns[0] = 1;
                data = malloc(sizeof(uchar)*len);
                memcpy(data,&(packet[cp].data[4]),packet[cp].len-4);
                nb_syns++;
            } else if(session && !syns[packet[cp].sqn]) {
                syns[packet[cp].sqn] = 1;
                memcpy(&data[PACKET_LENGTH-4+PACKET_LENGTH*(packet[cp].sqn-1)], packet[cp].data, packet[cp].len);
                nb_syns++;
            }
            if(session && nb_syns == nb_packets) {
                session = 0;
                printf("Session ended. Data result: ");
                printTab(data, len);
                file = fopen("out/submarine_small_output.png","w");
                if(file == (FILE*) NULL) {
                    fprintf(stderr, "Impossible d'ouvrir le fichier\n");
                    exit(2);
                }
                fwrite(data, sizeof(uchar), len, file);
                fclose(file);
                free(syns);
                free(data);
            }
            //printf("sqn:%d session:%d len:%d nb_syns:%d nb_packets:%d\n", packet[cp].sqn, session, len, nb_syns, nb_packets);
        }
        sem_post(semfull);
        cp = (cp+1)%10;
    }
    return NULL;
}

int main(){
    map_handle_t* handle;
    map_file("chaton2.jpg", &handle);

    sem_t semempty;
    sem_init(&semempty, 0, 0);
    sem_t semfull;
    sem_init(&semfull, 0, 10);

    uchar lastByte = 0x00;
    uint lastByteCount = 0;

    packet_t packet;
    packet.data = malloc(sizeof(uchar)*255);
    packet.valid = 0;

    packet_t* transfer;
    transfer = malloc(sizeof(packet_t)*10);
    for(int i = 0; i < 10; ++i)
        transfer[i].data = malloc(sizeof(uchar)*255);

    reading_op_t op;
    op.ack_channel = handle->mapping + 4096;
    op.buffer = transfer;
    op.semempty = &semempty;
    op.semfull = &semfull;
    pthread_t tid;
    pthread_create(&tid, NULL, reader, &op);
    
    uint cp = 0;
    uint count = 0;
    while(1) {
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
                        sem_wait(&semfull);
                        copyPackets(&packet, &transfer[cp]);
                        sem_post(&semempty);
                        cp=(cp+1)%10;
                    }
                }
            }
            if(lastByte == 0xAB && !packet.valid) {
                packet.valid = 1;
                packet.count = 0;
            }
            while(!(rdtsc() & (size_t)1 << NEWRYTHM));
        }
        count += flushAndReload(handle->mapping);
        for (int i = 0; i < 3; ++i)
            sched_yield();
    }

    pthread_join(tid, NULL);
    sem_destroy(&semempty);
    sem_destroy(&semfull);
    unmap_file(handle);
    return 0;
}
