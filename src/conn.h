#ifndef CONN_H
#define CONN_H

#include "request.h"
#include "peer.h"

#define MAX_DUP_NUM 3
#define INIT_THRESH 128

typedef enum _congMode{
    SLOW_START, CONGEST_AVOID
}mode;

typedef struct connpeer_t {
    int peerID;
    int ackdup;
    int connected;
    int nextExpected;
    int downJob;
    int numGetMisses;
    int numDataMisses;
    int TTL[CHUNK_SIZE];
    linkNode* hashhead;
    int down_chunkID;

    /*Congestion Control*/
    mode congestCtl;
    int windowSize;
    int lastSend;
    int lastAck;
    int ssthreshold;
    int roundInc;

    struct connpeer_t* prev;
    struct connpeer_t* next;
} conn_peer;

conn_peer* getUpNode(int nodeID);
conn_peer* getDownNode(int nodeID);
void removeDownNode(conn_peer *Node);
conn_peer* buildDownNode(int nodeID, char **chunk_list, int size);
void insertNewupNode(conn_peer *newNode);
#endif