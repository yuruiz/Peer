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
#include "debug.h"
#include "spiffy.h"
#include "bt_parse.h"
#include "input_buffer.h"
#include "chunklist.h"
#include "peer.h"
#include "conn.h"
#include "queue.h"
#include "request.h"

#define WHOHAS_TIME_OUT 2
/*Record the time the program start*/
static struct timeval startTime;

/*check who has request has been answered or not*/
int whohasAnswered = 1;
struct timeval whohasSendTime;

/*record user commands*/
static job *userjob = NULL;

/*the program would quit if set to 0*/
static int quit = 1;

/*record the socket variable*/
static int _sock;
/*global value point to the config varial*/
static bt_config_t *_config;

/*the chunks this client current own*/
static chunklist haschunklist;

/*Record the Connection Count*/
static int numConn = 0;

static int Timeout = 0;

void checkWhohas(job* user_job, bt_config_t* config) {
    if (whohasAnswered == 0) {
        struct timeval curT;

        gettimeofday(&curT, NULL);

        if (curT.tv_sec - whohasSendTime.tv_sec > WHOHAS_TIME_OUT) {
            printf("whohas timeout!!\n");
            WhoHasRequest(&user_job->chunk_list, config);
        }
    }
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

            whohasAnswered = 1;

            chunk_list = retrieve_chunk_list(&incomingPacket);

            buildDownNode(nodeInMap, chunk_list, getHashCount(&incomingPacket), &incomingPacket.src);

            GetRequest(nodeInMap, &incomingPacket.src, userjob);

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
//            printf("receive packet %d from peer %d\n", getPacketSeq(&incomingPacket), nodeInMap);
            downNode = getDownNode(nodeInMap);
            if (downNode == NULL) {
                break;
            }

            gettimeofday(&downNode->pktArrive, NULL);

            int nextExpected = downNode->nextExpected;
            int pktSeq = getPacketSeq(&incomingPacket);
            if (pktSeq != nextExpected) {
                ACKrequest(&incomingPacket.src, nextExpected - 1);
                Packet *newPkt = malloc(sizeof(Packet));

                memcpy(newPkt, &incomingPacket, sizeof(Packet));

                if (pktSeq > nextExpected) {
                    enUnCfPktQueue(newPkt, nodeInMap);
                }
            }
            else {
                processData(&incomingPacket, downNode->downJob, nodeInMap);

                queue *UnCFQueue = findUnCfPktQueue(nodeInMap);

                downNode->nextExpected = getPacketSeq(&incomingPacket) + 1;

                if (UnCFQueue != NULL) {

                    while (peekQueue(UnCFQueue) > 0 && peekQueue(UnCFQueue) <= downNode->nextExpected) {

                        queueNode *nextNode = dequeue(UnCFQueue);

                        if (nextNode == NULL) {
                            break;
                        }

                        if (nextNode->seq == downNode->nextExpected) {
                            processData(nextNode->pkt, downNode->downJob, nodeInMap);
                            downNode->nextExpected++;
                        }

                        free(nextNode->pkt);
                        free(nextNode);
                    }
                }

                ACKrequest(&incomingPacket.src, downNode->nextExpected - 1);

                linkNode *curhead = downNode->hashhead;

                if (curhead == NULL) {
                    printf("receive data error, cannot find corresponding hash\n");
                    break;
                }

                if (downNode->receivedSize == BT_CHUNK_SIZE) {
                    FILE *outfile;
                    outfile = fopen(userjob->outputf, "r+b");

                    setChunkDone(curhead->chunkHash, userjob);

                    if (userjob->chunk_list.unfetchedNum > 0) {
                        userjob->chunk_list.unfetchedNum--;
                    }

                    // look for position to insert a data chunk
                    long int offset = BT_CHUNK_SIZE * downNode->downJob;
                    fseek(outfile, offset, SEEK_SET);
                    fwrite(downNode->buffer, sizeof(char), BT_CHUNK_SIZE, outfile);
                    fclose(outfile);

                    if (curhead->next != NULL) {
                        printf("Got %s\n", curhead->chunkHash);
                        linkNode *temp = curhead;
                        downNode->hashhead = curhead->next;
                        downNode->receivedSize = 0;
                        memset(downNode->buffer, 0, sizeof(char) * BT_CHUNK_SIZE);
                        free(temp);
                        GetRequest(nodeInMap, &incomingPacket.src, userjob);
                    }
                    else if (curhead->next == NULL) {
                        free(downNode->buffer);
                        removeDownNode(downNode);
                        decreseConn();
                        free(curhead);
                        if (list_empty(userjob) == EXIT_SUCCESS && getDownNodeHead() == NULL) {
                            whohasAnswered = 1;
                            free(userjob);
                            userjob = NULL;
                            printf("job is done\n");
                        } else if(getDownNodeHead() == NULL) {
                            WhoHasRequest(&userjob->chunk_list, config);
                        }
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

            gettimeofday(&upNode->pktArrive, NULL);
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
void process_get(job* usercommnd) {
    usercommnd->chunk_list.type = GET;
    printf("Processing GET request %s, %s\n", usercommnd->chunkf, usercommnd->outputf);

    if ((usercommnd->chunk_list.chunkfptr = fopen(usercommnd->chunkf, "r")) == NULL) {
        fprintf(stderr, "Open file %s failed\n", usercommnd->chunkf);
        return;
    }

    if ((usercommnd->chunk_list.ouptputfptr = fopen(usercommnd->outputf, "w")) == NULL) {
        fprintf(stderr, "Open file %s failed\n", usercommnd->outputf);
        return;
    }

    buildChunkList(&(usercommnd->chunk_list));

    fclose(usercommnd->chunk_list.chunkfptr);

    if (usercommnd->chunk_list.chunkNum == 0) {
        printf("There is no chunk in the file\n");
        return;
    }

    printf("Chunk file parsed successfully, the parse result is as below\n");

    int i;
    for (i = 0; i < usercommnd->chunk_list.chunkNum; i++) {
        char buf[2 * SHA1_HASH_LENGTH + 1];
        bzero(buf, 2 * SHA1_HASH_LENGTH + 1);
        chunkline *line = &(usercommnd->chunk_list.list[i]);
        binary2hex(line->hash, SHA1_HASH_LENGTH, buf);
        printf("%d: %s\n", line->seq, buf);
    }

    WhoHasRequest(&usercommnd->chunk_list, _config);
    return;
}

void handle_user_input(char *line, void *cbdata) {

    char chunkf[BT_FILENAME_LEN];
    char outputf[BT_FILENAME_LEN];

    memset(chunkf, 0, BT_FILENAME_LEN * sizeof(char));
    memset(outputf, 0, BT_FILENAME_LEN * sizeof(char));



    if (sscanf(line, "GET %120s %120s", chunkf, outputf) == 2) {
        userjob = malloc(sizeof(job));
        memset(userjob, 0, sizeof(job));

        memcpy(userjob->chunkf, chunkf, BT_FILENAME_LEN * sizeof(char));
        memcpy(userjob->outputf, outputf, BT_FILENAME_LEN * sizeof(char));

        process_get(userjob);
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
        checkWhohas(userjob, config);
        checkTimoutPeer(userjob, config);
    }
}


int getSock() {
    return _sock;
}


void setTimeout(int timeout) {
    Timeout = timeout;
}

int getTimeout() {
    return Timeout;
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
void processData(Packet *incomingPacket, int downJob, int peerID) {
    conn_peer *node = getDownNode(peerID);
    int PayloadLen = getPacketSize(incomingPacket) - DATA_OFFSET;

    memcpy(node->buffer + node->receivedSize, incomingPacket->serial + DATA_OFFSET, PayloadLen);
    node->receivedSize += PayloadLen;
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

struct timeval* getStartTime() {
    return &startTime;
}

int main(int argc, char **argv) {
    bt_config_t config;

    gettimeofday(&startTime, NULL);

    bt_init(&config, argc, argv);

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

