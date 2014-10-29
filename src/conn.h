#ifndef CONN_H
#define CONN_H

#include "request.h"
#include "peer.h"

typedef struct connpeer_t {
    int peerID;
    int windowSize;
    int connected;
    linkNode* hashhead;
    int down_chunkID;
    struct connpeer_t* prev;
    struct connpeer_t* next;
} conn_peer;

conn_peer* getUpNode(int nodeID);
conn_peer* getDownNode(int nodeID);
void removeDownNode(conn_peer *Node);
conn_peer* buildDownNode(int nodeID, char **chunk_list, int size);
void insertNewupNode(conn_peer *newNode);
#endif