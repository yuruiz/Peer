#include <stddef.h>
#include <stdlib.h>
#include <string.h>
#include "peer.h"
#include "conn.h"
#include "request.h"


static conn_peer *uploadlist_head = NULL;
static conn_peer *uploadlist_tail = NULL;

static conn_peer *downloadlist_head = NULL;
static conn_peer *downloadlist_tail = NULL;

conn_peer* buildUpNode(int nodeID){
    conn_peer* upNode;
    upNode = malloc(sizeof(conn_peer));
    upNode->peerID = nodeID;
    upNode->connected = 1;
    upNode->next = NULL;
    upNode->windowSize = WIN_SIZE;

    insertNewupNode(upNode);

    return upNode;
}

conn_peer* getUpNode(int nodeID) {
    if (uploadlist_head == NULL) {
        return buildUpNode(nodeID);
    }

    conn_peer* cur = uploadlist_head;

    while (cur != NULL) {
        if (cur->peerID == nodeID) {
            return cur;
        }

        cur = cur->next;
    }

    return buildUpNode(nodeID);
}

void insertNewupNode(conn_peer *newNode) {
    if (uploadlist_head == NULL) {
        uploadlist_head = newNode;
        uploadlist_tail = newNode;
        return;
    }

    uploadlist_tail->next = newNode;
    newNode->prev = uploadlist_tail;
    uploadlist_tail = newNode;

    return;
}

void insertDownNode(conn_peer *newNode){
    if (downloadlist_head == NULL) {
        downloadlist_head = newNode;
        downloadlist_tail = newNode;
        return;
    }

    downloadlist_tail->next = newNode;
    newNode->prev = downloadlist_tail;
    downloadlist_tail = newNode;

    return;
}

conn_peer* buildDownNode(int nodeID, char **chunk_list, int size){
    conn_peer *DownNode = (conn_peer *) malloc(sizeof(conn_peer));
    linkNode *curNode = (linkNode *) malloc(sizeof(linkNode));
    DownNode->peerID = nodeID;
    DownNode->windowSize = 0;
    DownNode->prev = NULL;
    DownNode->next = NULL;
    DownNode->hashhead = curNode;

    int i;
    for (i = 0; i < size; i++) {
        strncpy(curNode->chunkHash, chunk_list[i], 2 * SHA1_HASH_LENGTH + 1);
        if (i + 1 < size) {
            curNode->next = (linkNode *) malloc(sizeof(linkNode));
            curNode = curNode->next;
        }
    }

    insertDownNode(DownNode);

    return DownNode;
}
