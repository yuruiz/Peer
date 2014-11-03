#ifndef REQUEST_H
#define REQUEST_H

#include <stdint.h>
#include <stdio.h>
#include <sys/time.h>
#include "bt_parse.h"

#define MAX_HASH_NUM ((1500 - 20) / 20)
#define PACKET_DATA_SIZE (1500 - 16)
#define MAGIC_OFFSET 0
#define VERSION_OFFSET 2
#define TYPE_OFFSET 3
#define HDLEN_OFFSET 4
#define PKLEN_OFFET 6
#define SEQNUM_OFFSET 8
#define ACKNUM_OFFSET 12
#define HASHNUM_OFFSET 16
#define HASHPAD_OFFSET 17
#define DATA_OFFSET 16
#define HASH_OFFSET 20
#define MAX_CHUNK_NUM 1000
#define SHA1_HASH_LENGTH 20
#define CHUNK_SIZE 512
#define DATA_SIZE 1024

typedef struct _packet{
    struct sockaddr_in src;
    struct timeval timestamp;
    uint8_t serial[1500];
} Packet;

typedef enum packType{
    GET, MASTER, HASH
}Type;

typedef enum _chunk_status{
    unfethced, fetching, fetched, none
}chunk_status;

typedef struct {
    int seq;
    chunk_status status;
    uint8_t hash[SHA1_HASH_LENGTH];
}chunkline;

typedef struct {
    int chunkNum;
    int unfetchedNum;
    Type type;
    FILE *chunkfptr;
    FILE *ouptputfptr;
    chunkline list[MAX_CHUNK_NUM];
}chunklist;

typedef struct _job
{
    chunklist chunk_list;
    char chunkf[BT_FILENAME_LEN];
    char outputf[BT_FILENAME_LEN];
} job;

void WhoHasRequest(chunklist *cklist, bt_config_t *config);
uint8_t getPacketType(Packet *pkt);
uint16_t getPacketSize(Packet *pkt);
uint8_t getHashCount(Packet *pkt);
uint32_t getPacketSeq(Packet *pkt);
uint32_t getPacketACK(Packet *pkt);
void insertHash(Packet *pkt, uint8_t *hash);
void IHaveRequest(char **haschunk_list, int size, struct sockaddr_in *from);
void DataRequest(bt_config_t *config, Packet *request, chunklist *haschunklist, int peerID);
void GetRequest(int nodeID, struct sockaddr_in* from, job* userjob);
void ACKrequest(struct sockaddr_in *from, int seq);


#endif