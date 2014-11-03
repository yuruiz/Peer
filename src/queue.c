#include <stddef.h>
#include <stdlib.h>
#include <stdio.h>
#include <sys/time.h>
#include "queue.h"
#include "spiffy.h"
#include "congest.h"
#include "chunk.h"
#include "chunklist.h"

/*Data Queues Link list*/
static queue *DataQueueList_head = NULL;
static queue *DataQueueList_tail = NULL;

/*Ack Queue Link List*/
static queue *AckQueueList_head = NULL;
static queue *AckQueueList_tail = NULL;

/*Packets Received Out of Order Linked List*/
static queue *UnCfPktQueueList_head = NULL;
static queue *UnCfPktQueueList_tail = NULL;


queue *findUnCfPktQueue(int peerID) {
    if (UnCfPktQueueList_head == NULL) {
        return NULL;
    }

    queue *cur = UnCfPktQueueList_head;

    while (cur != NULL) {
        if (cur->peerID == peerID) {
            return cur;
        }
        cur = cur->next;
    }

    return NULL;
}

queue *findDataQueue(int peerID) {
    if (DataQueueList_head == NULL) {
        return NULL;
    }

    queue *cur = DataQueueList_head;

    while (cur != NULL) {
        if (cur->peerID == peerID) {
            return cur;
        }

        cur = cur->next;
    }

    return NULL;
}

queue *getAckQueueHead() {
    return AckQueueList_head;
}

queue *findAckQueue(int peerID) {
    if (AckQueueList_head == NULL) {
        return NULL;
    }

    queue *cur = AckQueueList_head;

    while (cur != NULL) {
        if (cur->peerID == peerID) {
            return cur;
        }

        cur = cur->next;
    }

    return NULL;
}

void insertUnCfPktQueue(queue *newQueue) {
    if (UnCfPktQueueList_head == NULL) {
        UnCfPktQueueList_head = newQueue;
        UnCfPktQueueList_tail = newQueue;
        return;
    }

    UnCfPktQueueList_tail->next = newQueue;
    newQueue->prev = UnCfPktQueueList_tail;
    UnCfPktQueueList_tail = newQueue;

    return;
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
    else {
        DataQueueList_head = Queue->next;
    }

    if (Queue->next != NULL) {
        Queue->next->prev = Queue->prev;
    }
    else {
        DataQueueList_tail = Queue->prev;
    }

    if (Queue->head != NULL) {

    }

    clearqueue(Queue);
    free(Queue);
}

int peekQueue(queue *q) {
    if (q->head == NULL) {
        return -1;
    }

    return q->head->seq;
}

void removeAckQueue(queue *Queue) {

    if (Queue->prev != NULL) {
        Queue->prev->next = Queue->next;
    }
    else {
        AckQueueList_head = Queue->next;
    }

    if (Queue->next != NULL) {
        Queue->next->prev = Queue->prev;
    }
    else {
        AckQueueList_tail = Queue->prev;
    }

    if (Queue->head != NULL) {

    }

    clearqueue(Queue);
    free(Queue);
}

void removeUnCfPktQueue(queue *Queue) {

    if (Queue->prev != NULL) {
        Queue->prev->next = Queue->next;
    }
    else {
        UnCfPktQueueList_head = Queue->next;
    }

    if (Queue->next != NULL) {
        Queue->next->prev = Queue->prev;
    }
    else {
        UnCfPktQueueList_tail = Queue->prev;
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

    queueNode *cur = q->head;

    while (cur != NULL) {
        queueNode *temp = cur;
        cur = cur->next;
        free(temp);
    }
}


int removeQueueNode(queue *q, int seq) {
    if (q->head == NULL) {
        return -1;
    }

    queueNode *cur = q->head;

    while (cur != NULL) {
        if (cur->seq == seq) {
            if (cur->prev != NULL) {
                cur->prev->next = cur->next;
            }
            else {
                q->head = cur->next;
            }

            if (cur->next != NULL) {
                cur->next->prev = cur->prev;
            }
            else {
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

queueNode *getQueueNodebySeq(queue *q, int seq) {
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

queueNode *dequeue(queue *q) {
    if (q->head == NULL) {
        return NULL;
    }

    queueNode *retNode = q->head;
    q->head = q->head->next;

    if (q->head != NULL) {
        q->head->prev = NULL;
    } else {
        q->tail = NULL;
    }
    retNode->next = NULL;
    return retNode;
}

int mergeQueue(queue *head, queue *end) {
    if (head == NULL || end == NULL || head->head == NULL) {
        return -1;
    }

    if (end->head == NULL) {
        end->head = head->head;
        end->tail = head->tail;
    }
    else {
        head->tail->next = end->head;
        end->head = head->head;
    }

    head->head = NULL;
    head->tail = NULL;

    return 0;
}

void enAckQueue(Packet *pkt, int peerID) {

    queue *AckQueue = findAckQueue(peerID);

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

void enDataQueue(Packet *pkt, int peerID) {

    queue *DataQueue = findDataQueue(peerID);

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

void enUnCfPktQueue(Packet *pkt, int peerID) {

    queue *PktQueue = findUnCfPktQueue(peerID);

    if (PktQueue == NULL) {
        PktQueue = malloc(sizeof(queue));
        PktQueue->peerID = peerID;
        PktQueue->prev = NULL;
        PktQueue->next = NULL;
        PktQueue->head = NULL;
        PktQueue->tail = NULL;

        insertUnCfPktQueue(PktQueue);
    }

    queueNode *newNode = malloc(sizeof(queueNode));

    newNode->next = NULL;
    newNode->prev = NULL;
    newNode->pkt = pkt;
    newNode->seq = getPacketSeq(pkt);

    enqueue(PktQueue, newNode);
    return;
}

void flushDataQueue(int peerID, conn_peer *connNode, struct sockaddr_in *from) {
    queue *DataQueue = findDataQueue(peerID);

    if (DataQueue == NULL) {
        return;
    }

    while (connNode->lastAck + connNode->windowSize > connNode->lastSend) {
        queueNode *node = dequeue(DataQueue);

        while (node != NULL && connNode->lastAck >= node->seq) {
            free(node->pkt);
            free(node);

            node = dequeue(DataQueue);
        }

        if (node == NULL) {
            return;
        }

        Packet *pkt = node->pkt;
        if (spiffy_sendto(getSock(), pkt->serial, getPacketSize(pkt), 0, (struct sockaddr *) from, sizeof(*from)) > 0) {
            printf("Send Data request success. seq %d size: %d\n", getPacketSeq(pkt), getPacketSize(pkt));
            gettimeofday(&pkt->timestamp, NULL);
            connNode->lastSend = getPacketSeq(pkt);
            enAckQueue(pkt, peerID);
            free(node);
        } else {
            fprintf(stderr, "send Data packet failed\n");
            enqueue(DataQueue, node);
        }
    }
}

void AckQueueProcess(Packet *packet, int peerID) {
    queue *AckQueue = findAckQueue(peerID);
    conn_peer *upNode = getUpNode(peerID);
    int ack = getPacketACK(packet);

    if (AckQueue == NULL) {
        printf("Cannot find ACK Queue!\n");
        return;
    }

    if (upNode == NULL) {
        printf("Ack Queue Process error! Cannot get the upNode\n");
        return;
    }

    printf("receive ack %d, oldest ACK %d\n", ack, upNode->lastAck);

    gettimeofday(&(upNode->pktArrive), NULL);

    if (ack > upNode->lastAck) {
        upNode->ackdup = 0;
        upNode->lastAck = ack;
        int oldestSeq = peekQueue(AckQueue);

        while (oldestSeq > 0 && ack >= oldestSeq) {
            queueNode *temp = dequeue(AckQueue);

            if (ack == getPacketSeq(temp->pkt)) {
                struct timeval curT;
                gettimeofday(&curT, NULL);

                int prevTimout = getTimeout();
                int curTimeout = ((int) (curT.tv_sec - temp->pkt->timestamp.tv_sec) / 2 + prevTimout / 2);
                setTimeout(curTimeout);
            }
            free(temp->pkt);
            free(temp);
            oldestSeq = peekQueue(AckQueue);
            expandWin(upNode);
        }

        if (ack == CHUNK_SIZE) {
            printf("Data transmisstion finished\n");
            removeDataQueue(findDataQueue(peerID));
            removeAckQueue(findAckQueue(peerID));

            /*Reinitalize the Upnode*/
            upNode->ackdup = 0;
            upNode->hashhead = NULL;
            upNode->lastAck = 0;
            upNode->lastSend = 0;
            decreseConn();

        }
    } else {
        if (ack == upNode->lastAck) {
            if (upNode->congestCtl == FAST_RETRANSMIT) {
                printf("Already in FAST_RETRANSMIT MODE\n");
                return;
            }
            upNode->ackdup++;
            printf("duplicate ack received\n");

            if (upNode->ackdup >= MAX_DUP_NUM) {
                printf("%d duplicate Ack %d received\n", MAX_DUP_NUM, ack);

                struct timeval curT;
                gettimeofday(&curT, NULL);

                queue *DataQueue = findDataQueue(peerID);

                if (DataQueue == NULL) {
                    printf("Merge Error! Cannot get Dataqueue for peer %d\n", peerID);
                    return;
                }

                mergeQueue(AckQueue, DataQueue);
                upNode->lastSend = peekQueue(AckQueue);
                shrinkWin(upNode);
            }

        }
    }

    return;
}

void checkTimoutPeer(job* userjob, bt_config_t* config){
    conn_peer* curDownNode = getDownNodeHead();

    if (curDownNode == NULL) {
        return;
    }

    while (curDownNode != NULL) {
        struct timeval curT;

        gettimeofday(&curT, NULL);

        if (curT.tv_sec - curDownNode->pktArrive.tv_sec > PEER_CRASH_TIMEOUT) {
            printf("peer %d seems crashed\n", curDownNode->peerID);
            resetChunk(curDownNode->hashhead->chunkHash, userjob);
            WhoHasRequest(&userjob->chunk_list, config);

            free(curDownNode->buffer);
            removeDownNode(curDownNode);
        }

        curDownNode = curDownNode->next;
    }

}

void flushTimeoutAck() {
    queue *cur = getAckQueueHead();

    while (cur != NULL) {
        if (cur->head == NULL) {
            cur = cur->next;
            continue;
        }

        struct timeval curT;
        Packet *pkt = cur->head->pkt;

        gettimeofday(&curT, NULL);

        int curTime = getTimeout();

        if (curTime == 0) {
            curTime = TIME_OUT;
        }

        if (curT.tv_sec - pkt->timestamp.tv_sec > curTime) {
//            printf("Packet %d time out\n", getPacketSeq(pkt));
            conn_peer *upnode = getUpNode(cur->peerID);


            shrinkWin(upnode);
            if (mergeQueue(cur, findDataQueue(cur->peerID)) < 0) {
                printf("merge failed\n");
            }

            upnode->lastSend = peekQueue(cur);

            if (upnode->ackdup < MAX_DUP_NUM) {
                flushDataQueue(cur->peerID, upnode, &pkt->src);
            }

        }
        cur = cur->next;

    }
}