#ifndef _PEER_H_
#define _PEER_H_

#include "bt_parse.h"

#define HASH_LENGTH 20
#define MAX_CONN 1024
#define MAX_PEER_NUM 1024
#define PACK_SIZE 1500
#define WIN_SIZE 8;

typedef struct ln
{
    char chunkHash[HASH_LENGTH + 1];
    struct ln *next;
} linkNode;

void free_chunks(char **chunks, int size);
int getSock();

#endif /* _PEER_H_ */
