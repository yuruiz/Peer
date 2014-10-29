#ifndef QUEUE_H
#define QUEUE_H
#include "request.h"
#include "conn.h"


typedef struct queueNode_t{
    Packet* pkt;
    int seq;
    char* hash;
    struct queueNode_t* prev;
    struct queueNode_t* next;
}queueNode;

typedef struct queue_t{
    queueNode* head;
    queueNode* tail;
    int peerID;
    struct queue_t *prev;
    struct queue_t *next;
}queue;

void clearqueue(queue *q);
void enDataQueue(Packet* pkt, int peerID);
void AckQueueProcess(Packet *packet, int peerID);
void flashDataQueue(int peerID, conn_peer *connNode, struct sockaddr_in* from);

#endif