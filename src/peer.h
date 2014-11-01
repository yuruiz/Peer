#ifndef _PEER_H_
#define _PEER_H_

#include "bt_parse.h"
#include "request.h"

#define HASH_LENGTH 20
#define MAX_CONN 1024
#define MAX_PEER_NUM 1024
#define PACK_SIZE 1500
#define WIN_SIZE 1
#define TIME_OUT 4

typedef struct ln
{
    char chunkHash[2 * SHA1_HASH_LENGTH + 1];
    struct ln *next;
} linkNode;

typedef struct command
{
	char **request_queue;
	int command_num;
	struct command *next;
} command_list;


//short nodeInMap; // the node the packet received from
//linkNode *peers[BT_MAX_PEERS]; // keep info of all the available peers
//short jobs[BT_MAX_PEERS]; // current job running on each peer.
//int windowSize[BT_MAX_PEERS];

void free_chunks(char **chunks, int size);
int getSock();
void processData(Packet *incomingPacket, int downJob);
void peer_init();
void increaseConn();
void decreseConn();
int getTimeout();
void setTimeout(int timeout);
#endif /* _PEER_H_ */
