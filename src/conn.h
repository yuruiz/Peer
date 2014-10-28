#ifndef CONN_H
#define CONN_H

typedef struct connpeer_t {
    int peerID;
    int windowSize;
    int connected;
    struct connpeer_t* prev;
    struct connpeer_t* next;
} conn_peer;

conn_peer* getUpNode(int nodeID);
void insertNewupNode(conn_peer *newNode);
#endif