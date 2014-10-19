#ifndef _PEER_H_
#define _PEER_H_

#include "bt_parse.h"

#define HASH_LENGTH 20

typedef struct ln
{
    char chunkHash[HASH_LENGTH + 1];
    struct ln *next;
} linkNode;

void free_chunks(char **chunks, int size);
int getSock();

#endif /* _PEER_H_ */
