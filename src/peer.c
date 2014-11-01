/*
 * peer.c
 *
 * Authors: Ed Bardsley <ebardsle+441@andrew.cmu.edu>,
 *          Dave Andersen
 * Class: 15-441 (Spring 2005)
 *
 * Skeleton for 15-441 Project 2.
 *
 */

#include <sys/types.h>
#include <sys/time.h>
#include <arpa/inet.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <curses.h>
#include "debug.h"
#include "spiffy.h"
#include "bt_parse.h"
#include "input_buffer.h"
#include "chunklist.h"
#include "peer.h"
#include "conn.h"
#include "queue.h"

/* sliding window control */
//int nextExpected[BT_MAX_PEERS];

/* reliability control */
//int numGetMisses[BT_MAX_PEERS];
//int numDataMisses[BT_MAX_PEERS];
int numMismatches; //3 dup will invoke retransmission

/* time out */
struct timeval startTime;
//int TTL[BT_MAX_PEERS][CHUNK_SIZE]; // DATA timeout
//int GETTTL[BT_MAX_PEERS]; // GET timeout

/*record the chunk id currently receiving */
//short jobs[BT_MAX_PEERS];

/*record all the chunks not finish downloading yet*/
char **request_queue;

/*record the output file*/
static char outf[128];

/*the program would quit if set to 0*/
int quit = 1;

/*record the socket variable*/
static int _sock;
/*global value point to the config varial*/
static bt_config_t *_config;

/*the chunks this client current own*/
static chunklist haschunklist;


/*Record the Connection Count*/
static int numConn = 0;

// initialize global parameters.
void peer_init() {
//    int i, j;
    struct timeval tvBegin;
    startTime.tv_sec = tvBegin.tv_sec;
    startTime.tv_usec = tvBegin.tv_usec;
    gettimeofday(&tvBegin, NULL);
//    for (i = 0; i < BT_MAX_PEERS; i++) {
//        for (j = 0; j < CHUNK_SIZE; j++)
//            TTL[i][j] = -1;
//        jobs[i] = -1;
//        numGetMisses[i] = 0;
//        numDataMisses[i] = -1;
//    }
    numMismatches = 0;
}

void process_inbound_udp(int sock, bt_config_t *config) {
    socklen_t fromlen;
    Packet incomingPacket;
    bt_peer_t *curPeer;
    char **chunk_list;
    char **haschunk_list;
    int nodeInMap = 0;
    conn_peer *upNode;
    conn_peer *downNode;

    fromlen = sizeof(incomingPacket.src);
    memset(incomingPacket.serial, 0, PACK_SIZE);
    spiffy_recvfrom(sock, (void *) &(incomingPacket.serial), PACK_SIZE, 0, \
        (struct sockaddr *) &incomingPacket.src, &fromlen);

    /*check node*/
    for (curPeer = config->peers; curPeer != NULL; curPeer = curPeer->next)
        if (strcmp(inet_ntoa(curPeer->addr.sin_addr), inet_ntoa(incomingPacket.src.sin_addr))
                == 0 && ntohs(curPeer->addr.sin_port) == ntohs(incomingPacket.src.sin_port))
            nodeInMap = curPeer->id;

    switch (getPacketType(&incomingPacket)) {
        case 0:
            // receive WHOHAS request
            dprintf(STDOUT_FILENO, "WHOHAS received\n");
            chunk_list = retrieve_chunk_list(&incomingPacket);
            haschunk_list = has_chunks(config, &incomingPacket, chunk_list);

            if (haschunk_list[0] != NULL) {
                IHaveRequest(haschunk_list, getHashCount(&incomingPacket), &incomingPacket.src);
            }

            free_chunks(chunk_list, getHashCount(&incomingPacket));
            free_chunks(haschunk_list, getHashCount(&incomingPacket));
            break;
        case 1:
            /*receive IHAVE request*/
            dprintf(STDOUT_FILENO, "IHAVE received\n");

            chunk_list = retrieve_chunk_list(&incomingPacket);
            printf("retrieve: %s\n", chunk_list[0]);

            buildDownNode(nodeInMap, chunk_list, getHashCount(&incomingPacket));

            GetRequest(nodeInMap, &incomingPacket.src);

            free_chunks(chunk_list, getHashCount(&incomingPacket));

            break;
        case 2:
            /*receive GET request*/
            dprintf(STDOUT_FILENO, "GET received\n");
            if (numConn >= config->max_conn) {
                dprintf(STDOUT_FILENO, "too much connection\n");
                break;
            }

            if ((upNode = getUpNode(nodeInMap)) == NULL) {
                break;
            }

            upNode->connected = 1;

            increaseConn();

            Packet *pkt = malloc(sizeof(Packet));
            memcpy(pkt, &incomingPacket, sizeof(Packet));

            /*Build data packet and add packet to corresponding data packet queue*/
            DataRequest(config, pkt, &haschunklist, nodeInMap);

            /*flush data packet queue*/
            flushDataQueue(nodeInMap, upNode, &incomingPacket.src);
            break;
        case 3:
            /*receive DATA request*/
            printf("receive data:%d \r", getPacketSeq(&incomingPacket));
            downNode = getDownNode(nodeInMap);
            int nextExpected = downNode->nextExpected;
            if (getPacketSeq(&incomingPacket) != nextExpected && numMismatches < 3) {
                ACKrequest(&incomingPacket.src, nextExpected - 1);
                numMismatches++;
                downNode->numDataMisses = 0;
            }
            else if (getPacketSeq(&incomingPacket) == nextExpected) {
                downNode->numDataMisses = 0;
                numMismatches = 0;
                processData(&incomingPacket, downNode->downJob);
                downNode->nextExpected = getPacketSeq(&incomingPacket) + 1;
                linkNode *curhead = downNode->hashhead;

                if (curhead == NULL) {
                    printf("receive data error, cannot find corresponding hash\n");
                    break;
                }

                if (getPacketSeq(&incomingPacket) == (BT_CHUNK_SIZE / DATA_SIZE) && curhead->next != NULL) {
                    printf("Got %s\n", curhead->chunkHash);
                    linkNode *temp = curhead;
                    downNode->hashhead = curhead->next;
                    free(temp);
                    GetRequest(nodeInMap, &incomingPacket.src);
                }
                else if (getPacketSeq(&incomingPacket) == (BT_CHUNK_SIZE / DATA_SIZE) && curhead->next == NULL) {
                    removeDownNode(downNode);
                    downNode->numDataMisses = -1;
//                free_chunks(request_queue, MAX_CHUNK_NUM);
                    if (list_empty() == EXIT_SUCCESS) {
                        free(request_queue);
                        printf("JOB is done\n");
                    }
                }
            }
            break;
        case 4:
            /*receive ACK request*/

            if ((upNode = getUpNode(nodeInMap)) == NULL) {
                printf("Received unknow ack\n");
                break;
            }

            AckQueueProcess(&incomingPacket, nodeInMap);

            flushDataQueue(nodeInMap, upNode, &incomingPacket.src);
            break;
        case 5:
            break;
        default:
            dprintf(STDOUT_FILENO, "Receive a packet of unknow type!!\n");
            break;
    }

}

// send out whohas request
char **process_get(char *chunkfile, char *outputfile) {
    chunklist requestList;
    requestList.type = GET;
    char **chunk_list;
    printf("Processing GET request %s, %s\n", chunkfile, outputfile);

    if ((requestList.chunkfptr = fopen(chunkfile, "r")) == NULL) {
        fprintf(stderr, "Open file %s failed\n", chunkfile);
        return NULL;
    }

    if ((requestList.ouptputfptr = fopen(outputfile, "w")) == NULL) {
        fprintf(stderr, "Open file %s failed\n", outputfile);
        return NULL;
    }

    chunk_list = buildChunkList(&requestList);

    fclose(requestList.chunkfptr);

    if (requestList.chunkNum == 0) {
        printf("There is no chunk in the file\n");
        return NULL;
    }

    printf("Chunk file parsed successfully, the parse result is as below\n");

    int i;
    for (i = 0; i < requestList.chunkNum; i++) {
        char buf[2* SHA1_HASH_LENGTH+1];
        bzero(buf, 2* SHA1_HASH_LENGTH+1);
        chunkline *line = &(requestList.list[i]);
        binary2hex(line->hash, SHA1_HASH_LENGTH, buf);
        printf("%d: %s\n", line->seq, buf);
    }

    WhoHasRequest(&requestList, _config);
    return chunk_list;
}

void handle_user_input(char *line, void *cbdata) {
    char chunkf[128];
    char **chunk_list;

    bzero(chunkf, sizeof(chunkf));
    bzero(outf, sizeof(outf));

    if (sscanf(line, "GET %120s %120s", chunkf, outf)) {
        if (strlen(outf) > 0) {
            if ((chunk_list = process_get(chunkf, outf)) == NULL) {
                perror("I/O error");
                exit(-1);
            }
            request_queue = chunk_list;
        }
    }
    else if (strcmp(line, "quit") == 0) {
        quit = 0;
    }
}


void peer_run(bt_config_t *config) {
    int sock;
    struct sockaddr_in myaddr;
    fd_set readfds;
    struct user_iobuf *userbuf;
    struct timeval timeout;

    if ((userbuf = create_userbuf()) == NULL) {
        perror("peer_run could not allocate userbuf");
        exit(-1);
    }

    if ((sock = socket(AF_INET, SOCK_DGRAM, IPPROTO_IP)) == -1) {
        perror("peer_run could not create socket");
        exit(-1);
    }
    _sock = sock;

    bzero(&myaddr, sizeof(myaddr));
    myaddr.sin_family = AF_INET;
    myaddr.sin_addr.s_addr = htonl(INADDR_ANY);
    myaddr.sin_port = htons(config->myport);

    if (bind(sock, (struct sockaddr *) &myaddr, sizeof(myaddr)) == -1) {
        perror("peer_run could not bind socket");
        exit(-1);
    }

    spiffy_init(config->identity, (struct sockaddr *) &myaddr, sizeof(myaddr));


    if ((haschunklist.chunkfptr = fopen(config->has_chunk_file, "r")) == NULL) {
        fprintf(stderr, "Open file %s failed\n", config->has_chunk_file);
        return;
    }

    buildChunkList(&haschunklist);
    fclose(haschunklist.chunkfptr);

    while (quit) {
        int nfds;
        FD_SET(STDIN_FILENO, &readfds);
        FD_SET(sock, &readfds);

        timeout.tv_sec = 1;
        timeout.tv_usec = 0;

        nfds = select(sock + 1, &readfds, NULL, NULL, &timeout);

        if (nfds > 0) {
            if (FD_ISSET(sock, &readfds)) {
                process_inbound_udp(sock, config);
            }

            if (FD_ISSET(STDIN_FILENO, &readfds)) {
                process_user_input(STDIN_FILENO, userbuf, handle_user_input, "Currently unused");
            }
        }

        flushTimeoutAck();
    }
}


int getSock() {
    return _sock;
}

// free chunk list.
void free_chunks(char **chunks, int size) {
    int i;
    for (i = 0; i < size; i++)
        if (chunks[i] != NULL)
            free(chunks[i]);
    free(chunks);
}

// put received data into outputfile
void processData(Packet *incomingPacket, int downJob) {
    FILE *outfile;
    outfile = fopen(outf, "r+b");

    // look for position to insert a data chunk
    long int offset = CHUNK_SIZE * DATA_SIZE * downJob
            + DATA_SIZE * (getPacketSeq(incomingPacket) - 1);
    fseek(outfile, offset, SEEK_SET);
    fwrite(incomingPacket->serial + DATA_OFFSET, sizeof(char), DATA_SIZE, outfile);
    fclose(outfile);

    ACKrequest(&incomingPacket->src, getPacketSeq(incomingPacket));
}

void decreseConn() {

    if (numConn > 0) {
        numConn--;
    }
}

void increaseConn() {
    if (numConn < _config->max_conn) {
        numConn++;
    }
}

int main(int argc, char **argv) {
    bt_config_t config;

    bt_init(&config, argc, argv);
    peer_init();

    DPRINTF(DEBUG_INIT, "peer.c main beginning\n");

#ifdef TESTING
  config.identity = 1; // your group number here
  strcpy(config.chunk_file, "chunkfile");
  strcpy(config.has_chunk_file, "haschunks");
#endif

    bt_parse_command_line(&config);

#ifdef DEBUG
  if (debug & DEBUG_INIT) {
    bt_dump_config(&config);
  }
#endif

    _config = &config;

    if (parseMasterDatafile(&config) < 0) {
        return EXIT_FAILURE;
    }

    peer_run(&config);
    return 0;
}

