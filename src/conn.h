#ifndef CONN_H
#define CONN_H

#include <stddef.h>
#include "request.h"
#include "peer.h"

#define MAX_DUP_NUM 3
#define INIT_THRESH 64

typedef enum _congMode{
    SLOW_START, CONGEST_AVOID, FAST_RETRANSMIT
}mode;

typedef struct connpeer_t {
    int peerID;
    int connected;
    int nextExpected;
    int downJob;
    linkNode *hashhead;

    /*Up Node*/
    int sentSize;

    /*Down Node*/
    int receivedSize;
    char* buffer;

    /*Congestion Control*/
    mode congestCtl;
    int windowSize;
    int lastSend;
    int lastAck;
    int ssthreshold;
    int roundInc;
    int ackdup;
    struct timeval pktArrive;

    struct connpeer_t *prev;
    struct connpeer_t *next;
} conn_peer;

conn_peer* getUpNodeHead();
conn_peer* getDownNodeHead();
conn_peer* getUpNode(int nodeID);
conn_peer* getDownNode(int nodeID);
void removeDownNode(conn_peer *Node);
conn_peer* buildDownNode(int nodeID, char **chunk_list, int size);
void insertNewupNode(conn_peer *newNode);
void removeUpNode(conn_peer *Node);
#endif