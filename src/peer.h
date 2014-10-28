#ifndef _PEER_H_
#define _PEER_H_

#include "bt_parse.h"
#include "request.h"

typedef struct ln
{
    char chunkHash[2 * SHA1_HASH_LENGTH + 1];
    struct ln *next;
} linkNode;

short nodeInMap; // the node the packet received from
linkNode *peers[BT_MAX_PEERS]; // keep info of all the available peers
chunklist requestList;
short jobs[BT_MAX_PEERS]; // current job running on each peer.
int windowSize[BT_MAX_PEERS];

void free_chunks(char **chunks, int size);
int getSock();

#endif /* _PEER_H_ */
