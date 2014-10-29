#include <stddef.h>
#include <stdlib.h>
#include <stdio.h>
#include "queue.h"
#include "spiffy.h"
#include "peer.h"


static queue* DataQueueList_head = NULL;
static queue* DataQueueList_tail = NULL;

static queue* AckQueueList_head = NULL;
static queue* AckQueueList_tail = NULL;

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


queue* findAckQueue(int peerID) {
    if (AckQueueList_head == NULL) {
        return NULL;
    }

    queue* cur = AckQueueList_head;

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

void insertAckQueue(queue *newQueue) {
    if (AckQueueList_head == NULL) {
        AckQueueList_head = newQueue;
        AckQueueList_tail = newQueue;
        return;
    }

    AckQueueList_tail->next = newQueue;
    newQueue->prev = AckQueueList_tail;
    AckQueueList_tail = newQueue;

    return;
}

void removeDataQueue(queue *Queue) {

    if (Queue->prev != NULL) {
        Queue->prev->next = Queue->next;
    }
    else{
        DataQueueList_head = Queue->next;
    }

    if (Queue->next != NULL) {
        Queue->next->prev = Queue->prev;
    }
    else{
        DataQueueList_tail = Queue->prev;
    }

    if (Queue->head != NULL) {

    }

    clearqueue(Queue);
    free(Queue);
}


void removeAckQueue(queue *Queue) {

    if (Queue->prev != NULL) {
        Queue->prev->next = Queue->next;
    }
    else{
        AckQueueList_head = Queue->next;
    }

    if (Queue->next != NULL) {
        Queue->next->prev = Queue->prev;
    }
    else{
        AckQueueList_tail = Queue->prev;
    }

    if (Queue->head != NULL) {

    }

    clearqueue(Queue);
    free(Queue);
}

void clearqueue(queue *q) {

    if (q->head == NULL) {
        return;
    }

    queueNode* cur = q->head;

    while (cur != NULL) {
        queueNode* temp = cur;
        cur = cur->next;
        free(temp);
    }
}


int removeQueueNode(queue *q, int seq){
    if (q->head == NULL) {
        return -1;
    }

    queueNode *cur = q->head;

    while (cur != NULL) {
        if (cur->seq == seq) {
            if (cur->prev != NULL) {
                cur->prev->next = cur->next;
            }
            else{
                q->head = cur->next;
            }

            if (cur->next != NULL) {
                cur->next->prev = cur->prev;
            }
            else{
                q->tail = cur->prev;
            }

            return 0;
        }

        cur = cur->next;
    }

    return -1;
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

queueNode* getQueueNodebySeq(queue* q, int seq) {
    if (q->head == NULL) {
        return NULL;
    }

    queueNode *cur = q->head;

    while (cur != NULL) {
        if (cur->seq == seq) {
            return cur;
        }

        cur = cur->next;
    }

    return NULL;
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

void enAckQueue(Packet* pkt, int peerID){

    queue*AckQueue = findAckQueue(peerID);

    if (AckQueue == NULL) {
        AckQueue = malloc(sizeof(queue));
        AckQueue->peerID = peerID;
        AckQueue->prev = NULL;
        AckQueue->next = NULL;
        AckQueue->head = NULL;
        AckQueue->tail = NULL;

        insertAckQueue(AckQueue);
    }

    queueNode *newNode = malloc(sizeof(queueNode));

    newNode->next = NULL;
    newNode->prev = NULL;
    newNode->pkt = pkt;
    newNode->seq = getPacketSeq(pkt);

    enqueue(AckQueue, newNode);
    return;
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
    newNode->seq = getPacketSeq(pkt);

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
        queueNode *node = dequeue(DataQueue);

        if (node == NULL) {
            removeDataQueue(DataQueue);
            return;
        }

        Packet* pkt = node->pkt;
        if(spiffy_sendto(getSock(), pkt->serial, getPacketSize(pkt), 0, (struct sockaddr *)from, sizeof(*from)) > 0){
            printf("Send Data request success. %d\n", getPacketSeq(pkt));
            connNode->windowSize--;
            enAckQueue(pkt, peerID);
            free(node);
        }else{
            fprintf(stderr, "send Data packet failed\n");
            enqueue(DataQueue, node);
        }
    }
}

void AckQueueProcess(Packet *packet, int peerID){
    queue *AckQueue = findAckQueue(peerID);

    if (AckQueue == NULL) {
        printf("Cannot find ACK Queue!\n");
        return;
    }

    if (removeQueueNode(AckQueue, getPacketACK(packet)) < 0) {
        printf("Cannot find the AckNode!\n");
        return;
    }

    return;
}