#ifndef _PEER_H_
#define _PEER_H_

#define WHOHAS 0
#define IHAVE 1
#define BT_CHUNK_HASH_SIZE 20

#include "bt_parse.h"

typedef struct ln
{
    char chunkHash[BT_CHUNK_HASH_SIZE + 1];
    struct ln *next;
} linkNode;

void free_chunks(char **chunks, int size);
int getSock();

#endif /* _PEER_H_ */
