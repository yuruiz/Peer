#include <stddef.h>
#include "conn.h"


conn_peer *uploadlist_head = NULL;
conn_peer *uploadlist_tail = NULL;


conn_peer* getUpNode(int nodeID) {
    if (uploadlist_head == NULL) {
        return NULL;
    }

    conn_peer* cur = uploadlist_head;

    while (cur != NULL) {
        if (cur->peerID == nodeID) {
            return cur;
        }

        cur = cur->next;
    }

    return NULL;
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