#include <stdlib.h>
#include <string.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <unistd.h>
#include "peer.h"
#include "chunklist.h"
#include "request.h"
#include "spiffy.h"
#include "chunk.h"
#include "queue.h"

extern char **request_queue;
//extern short jobs[BT_MAX_PEERS];
//extern int nextExpected[BT_MAX_PEERS];

/*Right now Packet only have the default header, the length now is 16*/
Packet *buildDefaultPacket() {
    Packet *pkt = calloc(sizeof(Packet), 1);

    pkt->timestamp.tv_usec = 0;
    pkt->timestamp.tv_sec = 0;
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

uint8_t getPacketType(Packet *pkt) {
    uint8_t *ptr = (pkt->serial);
    return *(ptr + TYPE_OFFSET);
}

void incPacketSize(Packet *pkt, uint16_t size) {
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

uint8_t getHashCount(Packet *pkt) {
    uint8_t *serial = pkt->serial;

    return *(serial + HASHNUM_OFFSET);
}

uint8_t *getDataHash(Packet *pkt) {
    return pkt->serial + HASHNUM_OFFSET;
}



void setPacketSeq(Packet *pkt, int seq){
    uint8_t *serial = pkt->serial;

    *((uint32_t *)(serial + SEQNUM_OFFSET)) = htonl(seq);
}


void setPacketAck(Packet *pkt, int ack){
    uint8_t *serial = pkt->serial;

    *((uint32_t *)(serial + ACKNUM_OFFSET)) = htonl(ack);
}

uint32_t getPacketACK(Packet *pkt){
    uint8_t *serial = pkt->serial;

    return ntohl(*((uint32_t *)(serial + ACKNUM_OFFSET)));
}


uint32_t getPacketSeq(Packet *pkt){
    uint8_t *serial = pkt->serial;

    return ntohl(*((uint32_t *)(serial + SEQNUM_OFFSET)));
}

void insertHash(Packet *pkt, uint8_t *hash) {
    uint8_t *serial = pkt->serial;

    /*Get the count of hash currently in payload*/
    uint8_t curHashCount = *(serial + HASHNUM_OFFSET);

    /*Copy the hash to the payload*/
    memcpy(serial + HASH_OFFSET + curHashCount * SHA1_HASH_LENGTH, \
        hash, SHA1_HASH_LENGTH);

    *(serial + HASHNUM_OFFSET) = curHashCount + 1;

    incPacketSize(pkt, SHA1_HASH_LENGTH);
}


void insertGetHash(Packet *pkt, uint8_t *hash){
    uint8_t *serial = pkt->serial;

    memcpy(serial + HASHNUM_OFFSET, hash, SHA1_HASH_LENGTH);

    incPacketSize(pkt, SHA1_HASH_LENGTH);
}

int getHashIndex(uint8_t *hash, chunklist *hasChunklist) {
    int i;

    char buf[SHA1_HASH_LENGTH * 2 + 1];

    memset(buf, 0, SHA1_HASH_LENGTH * 2 + 1);

    binary2hex(hash, SHA1_HASH_LENGTH, buf);

    printf("%s\n", buf);
    for (i = 0; i < hasChunklist->chunkNum; ++i) {
        chunkline *hashline = &(hasChunklist->list[i]);
        if (strncmp((char*)hash, (char*)(hashline->hash), SHA1_HASH_LENGTH) == 0) {
            //todo fetch state
            return hasChunklist->list[i].seq;
        }
    }

    return -1;
}


Packet* buildDataPacket(int seq, int chunkID, int size, bt_config_t* config, struct sockaddr_in *dest){
    Packet* newPacket = buildDefaultPacket();
    setPakcetType(newPacket, "DATA");
    incPacketSize(newPacket, size);
    setPacketSeq(newPacket, seq);

    long fileoffset = chunkID * BT_CHUNK_SIZE + (seq - 1) * PACKET_DATA_SIZE;

    char* masterfile = config->master_data_file;

    FILE *mastfptr;

    if ((mastfptr = fopen(masterfile, "r")) == NULL) {
        fprintf(stderr, "Open file %s failed\n", masterfile);
        return NULL;
    }

    rewind(mastfptr);
    fseek(mastfptr, fileoffset, SEEK_SET);

    if(fread(newPacket->serial + DATA_OFFSET, sizeof(uint8_t), size, mastfptr) != size) {
        fprintf(stderr, "Reading master chunk file error\n");
        fclose(mastfptr);
        return NULL;
    }

//    printf("new packet size %d\n", getPacketSize(newPacket));

    fclose(mastfptr);

    memcpy(&newPacket->src, dest, sizeof(newPacket->src));

    return newPacket;

}

// build and send out whohas packet. 
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
            uint8_t buf[SHA1_HASH_LENGTH];
            char buf1[2 * SHA1_HASH_LENGTH + 1];
            bzero(buf1, 2* SHA1_HASH_LENGTH+1);
            strncpy((char*)buf, cklist->list[hashIndex].hash, SHA1_HASH_LENGTH);
            binary2hex(cklist->list[hashIndex].hash, SHA1_HASH_LENGTH, buf1);
            printf("send: %s\n", buf1);
            hashIndex++;
        }

        bt_peer_t *p;

        for (p = config->peers; p != NULL; p = p->next) {
            if (p->id != config->identity) {
                if (spiffy_sendto(getSock(), pkt->serial, getPacketSize(pkt), \
                    0, (struct sockaddr *) &(p->addr), sizeof(p->addr)) > 0) {
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
void IHaveRequest(char **haschunk_list, int size, struct sockaddr_in *from) {

    int num_chunks;

    Packet *pkt = buildDefaultPacket();
    setPakcetType(pkt, "IHAVE");
    incPacketSize(pkt, 4);
    printf("Response payload has: \n");
    for (num_chunks = 0; num_chunks < size; num_chunks++) {
        uint8_t buf[SHA1_HASH_LENGTH];
        if (haschunk_list[num_chunks] != NULL) {
            hex2binary(haschunk_list[num_chunks], SHA1_HASH_LENGTH * 2, buf);
            insertHash(pkt, buf);
            printf("%s\n", haschunk_list[num_chunks]);
            continue;
        }
        insertHash(pkt, buf);
    }

    if (spiffy_sendto(getSock(), pkt->serial, getPacketSize(pkt), 0, (struct sockaddr *) from, sizeof(*from)) > 0) {
        printf("Send IHAVE request success. %d\n", getPacketSize(pkt));
    } else {
        fprintf(stderr, "send packet failed\n");
    }
    free(pkt);
}

void DataRequest(bt_config_t *config, Packet *request, chunklist *haschunklist, int peerID) {
    uint8_t *data_hash = getDataHash(request);
    int hashIndex;

    if ((hashIndex = getHashIndex(data_hash, haschunklist)) < 0) {
        printf("Do not have the data chunk to respond to the DATA requtest\n");
        return;
    }

    int numPacket = (BT_CHUNK_SIZE + PACKET_DATA_SIZE - 1) / PACKET_DATA_SIZE;

    int i;

    for (i = 0; i < numPacket; ++i) {
        Packet *newPacket;
        if (i < numPacket - 1) {
            newPacket = buildDataPacket(i + 1, hashIndex, PACKET_DATA_SIZE, config, &request->src);
        }
        else{
            newPacket = buildDataPacket(i + 1, hashIndex, BT_CHUNK_SIZE % PACKET_DATA_SIZE, config, &request->src);
        }

        if (newPacket == NULL) {
            return;
        }

        enDataQueue(newPacket, peerID);
    }


    return;
}
// send GET message, allow concurrent download.
void GetRequest(int nodeID, struct sockaddr_in* from)
{
    Packet *p;
    int index;

    conn_peer *node = getDownNode(nodeID);

    if (node == NULL) {
        printf("GetRequest Error, Cannot get the DownNode\n");
        return;
    }
    linkNode *hashNode = node->hashhead;

    if(hashNode == NULL) {
        printf("GetRequest Error, Cannot get HashNode");
        return;
    }
    // check if the target chunkHash has been transferred
    while ((index = list_contains(hashNode->chunkHash)) < 0) {
        printf("inside1\n");
        if (hashNode->next == NULL) {
            if (list_empty() == EXIT_SUCCESS) {
                free(request_queue);
                node->numDataMisses = -1;
                printf("task is done\n");
            }
            return;
        }
        printf("inside2\n");

        linkNode *temp = hashNode;
        hashNode = hashNode->next;
        node->hashhead = hashNode;
        free(temp);
    }

    if (hashNode->chunkHash == NULL )
    {
        printf("sending chunk equals zero\n");
        return;
    }

    p = buildDefaultPacket();
    setPakcetType(p, "GET");

    uint8_t hashbuf[SHA1_HASH_LENGTH];
    hex2binary(hashNode->chunkHash, SHA1_HASH_LENGTH * 2, hashbuf);

    insertGetHash(p, hashbuf);
    if (spiffy_sendto(getSock(), p->serial, getPacketSize(p), 0, (struct sockaddr *) from, sizeof(*from)) > 0) {
        printf("Send GET request success. %d\n", getPacketSize(p));
    } else {
        fprintf(stderr, "send GET failed\n");
    }

    // jobs[nodeID] = getHashIndex(hashNode->chunkHash, haschunklist);
    node->downJob = index;
//
//    printf("Requesting chunk ID: %d from %d\n", jobs[nodeID], nodeID);

    node->nextExpected = 1;
 //   nextExpected[nodeID] = 1;
//    GETTTL[nodeInMap] = 0;
    free(p);

    // chunk is transferring, remove it so that
    // other peers won't transfer this again
//    list_remove(request_queue[]);
    free(request_queue[index]);
    request_queue[index] = NULL;
}


void ACKrequest(struct sockaddr_in *from, int seq){

    Packet* pkt = buildDefaultPacket();
    setPakcetType(pkt, "ACK");
    incPacketSize(pkt, 16);
    setPacketAck(pkt, seq);


    if (spiffy_sendto(getSock(), pkt->serial, getPacketSize(pkt), 0, (struct sockaddr *) from, sizeof(*from)) > 0) {
//        printf("Send ACK request success. %d\n", getPacketSeq(pkt));
    } else {
        fprintf(stderr, "send ACK failed\n");
    }

    free(pkt);
}