#include <stdlib.h>
#include <stdint.h>
#include <string.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <unistd.h>
#include "peer.h"
#include "chunklist.h"
#include "request.h"
#include "spiffy.h"


/*Right now Packet only have the default header, the length now is 16*/
Packet *buildDefaultPacket() {
    Packet *pkt = calloc(sizeof(Packet), 1);

    uint8_t *serial = pkt->serial;
    *((uint16_t *) serial) = htons(15441);
    *(serial + VERSION_OFFSET) = 1;
    *((uint16_t *) (serial + HDLEN_OFFSET)) = htons(16);
    *((uint16_t *) (serial + PKLEN_OFFET)) = htons(16);

    return pkt;

}

void setPakcetType(Packet *pkt, char *type) {
    uint8_t *serial = pkt->serial;
    uint8_t typeNum = 255;

    if (strcmp(type, "WHOHAS") == 0) {
        typeNum = 0;
    }
    else if (strcmp(type, "IHAVE") == 0) {
        typeNum = 1;
    }
    else if (strcmp(type, "GET") == 0) {
        typeNum = 2;
    }
    else if (strcmp(type, "DATA") == 0) {
        typeNum = 3;
    }
    else if (strcmp(type, "ACK") == 0) {
        typeNum = 4;
    }
    else if (strcmp(type, "DENIED") == 0) {
        typeNum = 5;
    }

    *(serial + TYPE_OFFSET) = typeNum;

}

uint8_t getPacketType(Packet *pkt){
    uint8_t *ptr = (pkt->serial);
    return *((uint8_t *)(ptr + TYPE_OFFSET));
}

void incPacketSize(Packet *pkt, uint8_t size) {
    uint8_t *serial = pkt->serial;
    uint16_t CurSize = ntohs(*((uint16_t *) (serial + PKLEN_OFFET)));
    CurSize += size;

    *((uint16_t *) (serial + PKLEN_OFFET)) = htons(CurSize);
    return;
}

uint16_t getPacketSize(Packet *pkt) {
    uint8_t *serial = (pkt->serial);
    return ntohs(*((uint16_t *) (serial + PKLEN_OFFET)));
}

uint8_t getHashCount(Packet *pkt){
    uint8_t *serial = pkt->serial;

    return *(serial + HASHNUM_OFFSET);
}

void insertHash(Packet *pkt, uint8_t *hash) {
    uint8_t *serial = pkt->serial;

    /*Get the count of hash currently in payload*/
    uint8_t curHashCount = *(serial + HASHNUM_OFFSET);

    /*Copy the hash to the payload*/
    memcpy(serial + HASH_OFFSET + curHashCount * SHA1_HASH_LENGTH, hash, SHA1_HASH_LENGTH);

    *(serial + HASHNUM_OFFSET) = curHashCount + 1;

    incPacketSize(pkt, SHA1_HASH_LENGTH);


}

void WhoHasRequest(chunklist *cklist, bt_config_t *config) {

    int packetNum = (cklist->chunkNum + MAX_HASH_NUM - 1) / MAX_HASH_NUM;
    int hashIndex = 0;
    int i;

    for (i = 0; i < packetNum; ++i) {

        int hashCount, j;
        Packet *pkt = buildDefaultPacket();

        setPakcetType(pkt, "WHOHAS");

        /*Include the hash padding to the size*/
        incPacketSize(pkt, 4);
        if (i < packetNum - 1) {
            hashCount = MAX_HASH_NUM;
        } else {
            hashCount = cklist->chunkNum % MAX_HASH_NUM;
        }

        for (j = 0; j < hashCount; ++j) {
            insertHash(pkt, cklist->list[hashIndex].hash);
            hashIndex++;
        }

        bt_peer_t *p;

        for (p = config->peers; p != NULL; p = p->next) {
            if (p->id != config->identity) {
                if (spiffy_sendto(getSock(), pkt->serial, getPacketSize(pkt), 0, (struct sockaddr *) &(p->addr), sizeof(p->addr)) > 0) {
                    printf("Send whohas request to %d success. %d\n", p->id, getPacketSize(pkt));
                } else {
                    fprintf(stderr, "send packet failed\n");
                }
            }
        }

        free(pkt);
        return;
    }
}

// send back IHAVE message.
void IHaveRequest(char **haschunk_list, int size, struct sockaddr_in* from) {

    int num_chunks;

    Packet *pkt = buildDefaultPacket();
    setPakcetType(pkt, "IHAVE");
    incPacketSize(pkt, 4);
    printf("Response payload has: \n");
    for (num_chunks = 0; num_chunks < size; num_chunks++) {
        uint8_t buf[SHA1_HASH_LENGTH];
      //  memset(buf, 0, SHA1_HASH_LENGTH);
        if (haschunk_list[num_chunks] != NULL) {
            hex2binary(haschunk_list[num_chunks], SHA1_HASH_LENGTH * 2, buf);
            insertHash(pkt, buf);
            printf("%s\n", haschunk_list[num_chunks]);
            continue;
        }
        insertHash(pkt, buf);
    }

    if (spiffy_sendto(getSock(), pkt->serial, getPacketSize(pkt), 0, (struct sockaddr *)from, sizeof(*from)) > 0) {
        printf("Send IHAVE request success. %d\n", getPacketSize(pkt));
    } else {
        fprintf(stderr, "send packet failed\n");
    }

    free(pkt);
}