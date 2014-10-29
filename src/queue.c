#include <stddef.h>
#include <stdlib.h>
#include <stdio.h>
#include <CoreFoundation/CoreFoundation.h>
#include "queue.h"
#include "spiffy.h"
#include "peer.h"


static queue* DataQueueList_head = NULL;
static queue* DataQueueList_tail = NULL;


queue* findDataQueue(int peerID) {
    if (DataQueueList_head == NULL) {
        return NULL;
    }

    queue* cur = DataQueueList_head;

    while (cur != NULL) {
        if (cur->peerID == peerID) {
            return cur;
        }

        cur = cur->next;
    }

    return NULL;
}


void insertDataQueue(queue *newQueue) {
    if (DataQueueList_head == NULL) {
        DataQueueList_head = newQueue;
        DataQueueList_tail = newQueue;
        return;
    }

    DataQueueList_tail->next = newQueue;
    newQueue->prev = DataQueueList_tail;
    DataQueueList_tail = newQueue;

    return;
}


void enqueue(queue *q, queueNode *node) {
    if (q->head == NULL) {
        q->head = node;
        q->tail = node;
        return;
    }

    q->tail->next = node;
    node->prev = q->tail;
    q->tail = node;

    return;
}

queueNode* dequeue(queue* q) {
    if (q->head == NULL) {
        return NULL;
    }

    queueNode* retNode = q->head;
    q->head = q->head->next;

    if (q->head != NULL) {
        q->head->prev = NULL;
    }
    retNode->next = NULL;
    return retNode;
}
void enDataQueue(Packet* pkt, int peerID) {

    queue* DataQueue = findDataQueue(peerID);

    if (DataQueue == NULL) {
        DataQueue = malloc(sizeof(queue));
        DataQueue->peerID = peerID;
        DataQueue->prev = NULL;
        DataQueue->next = NULL;
        DataQueue->head = NULL;
        DataQueue->tail = NULL;

        insertDataQueue(DataQueue);
    }

    queueNode *newNode = malloc(sizeof(queueNode));

    newNode->next = NULL;
    newNode->prev = NULL;
    newNode->pkt = pkt;

    enqueue(DataQueue, newNode);
    return;
}

void flashDataQueue(int peerID, conn_peer *connNode, struct sockaddr_in* from){
    queue *DataQueue = findDataQueue(peerID);

    if (DataQueue == NULL) {
        printf("Cannot get the corresponding dataqueue, flash data queue error\n");
        return;
    }

    while (connNode->windowSize > 0) {
        printf("here\n");
        queueNode *node = dequeue(DataQueue);

        if (node == NULL) {
            //todo free the dataQueue
            return;
        }

        Packet* pkt = node->pkt;
        if(spiffy_sendto(getSock(), pkt->serial, getPacketSize(pkt), 0, (struct sockaddr *)from, sizeof(*from)) > 0){
            printf("Send Data request success. %d\n", getPacketSeq(pkt));
            connNode->windowSize--;
            free(node);
            free(pkt);
            printf("finished\n");
        }else{
            fprintf(stderr, "send Data packet failed\n");
            enqueue(DataQueue, node);
        }
    }

    printf("end\n");
}