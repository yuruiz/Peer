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
#include <arpa/inet.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include "debug.h"
#include "spiffy.h"
#include "bt_parse.h"
#include "input_buffer.h"
#include "chunklist.h"
#include "chunk.h"
#include "request.h"
#include "peer.h"
#include "conn.h"
#include "queue.h"

short nodeInMap; // the node the packet received from
//linkNode *peers[BT_MAX_PEERS]; // keep info of all the available peers
chunklist requestList;

static bt_config_t *_config;
static int _sock;
static chunklist haschunklist;

void peer_run(bt_config_t *config);

int main(int argc, char **argv) {
    bt_config_t config;

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
    peer_run(&config);
    return 0;
}

void process_inbound_udp(int sock, bt_config_t *config) {
    socklen_t fromlen;
    Packet incomingPacket;
    bt_peer_t *curPeer;
    char **chunk_list;
    char **haschunk_list;
    static int numConn = 0;

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
        conn_peer* upNode;
        case 0:
            // receive WHOHAS request
            dprintf(STDOUT_FILENO, "WHOHAS received\n");
            chunk_list = retrieve_chunk_list(&incomingPacket);
            haschunk_list = has_chunks(config, &incomingPacket, chunk_list);
            if (haschunk_list[0] != NULL) {
                IHaveRequest(haschunk_list, getHashCount(&incomingPacket), \
                    &incomingPacket.src);
            }
            free_chunks(chunk_list, getHashCount(&incomingPacket));
            free_chunks(haschunk_list, getHashCount(&incomingPacket));
            break;
        case 1:
            /*receive IHAVE request*/
            dprintf(STDOUT_FILENO, "IHAVE received\n");
            chunk_list = retrieve_chunk_list(&incomingPacket);
            buildDownNode(nodeInMap, chunk_list, getHashCount(&incomingPacket));
            GetRequest(sock, &incomingPacket.src);
            free_chunks(chunk_list, getHashCount(&incomingPacket));
            break;
        case 2:
            /*receive GET request*/
            dprintf(STDOUT_FILENO, "GET received\n");
            if (numConn >= MAX_CONN) {
                dprintf(STDOUT_FILENO, "too much connection\n");
                break;
            }

            numConn++;

            if ((upNode = getUpNode(nodeInMap)) == NULL) {

                break;
            }

            Packet *pkt = malloc(sizeof(Packet));
            memcpy(pkt, &incomingPacket, sizeof(Packet));

            /*Build data packet and add packet to corresponding data packet queue*/
            DataRequest(config, pkt, &haschunklist, nodeInMap);

            /*flush data packet queue*/
            flashDataQueue(nodeInMap, upNode);

            break;
        case 3:
            /*receive DATA request*/
            dprintf(STDOUT_FILENO, "DATA received\n");
            break;
        case 4:
            /*receive ACK request*/

            if ((upNode = getUpNode(nodeInMap)) == NULL) {
                printf("Received unknow ack\n");
                break;
            }

            if(upNode->windowSize < 8){
                upNode->windowSize++;
            }

            flashDataQueue(nodeInMap, upNode);
            break;
        case 5:
            break;
        default:
            dprintf(STDOUT_FILENO, "Receive a packet of unknow type!!\n");
            break;
    }

}

// send out whohas request
void process_get(char *chunkfile, char *outputfile) {
    requestList.type = GET;
    printf("Processing GET request %s, %s\n", chunkfile, outputfile);

    if ((requestList.chunkfptr = fopen(chunkfile, "r")) == NULL) {
        fprintf(stderr, "Open file %s failed\n", chunkfile);
        return;
    }

    if ((requestList.ouptputfptr = fopen(outputfile, "w")) == NULL) {
        fprintf(stderr, "Open file %s failed\n", outputfile);
        return;
    }

    buildChunkList(&requestList);

    fclose(requestList.chunkfptr);

    if (requestList.chunkNum == 0) {
        printf("There is no chunk in the file\n");
        return;
    }

    printf("Chunk file parsed successfully, the parse result is as below\n");

    int i;
    for (i = 0; i < requestList.chunkNum; i++) {
        char buf[50];
        bzero(buf, 50);
        chunkline *line = &(requestList.list[i]);
        binary2hex(line->hash, 20, buf);
        printf("%d: %s\n", line->seq, buf);
    }

    WhoHasRequest(&requestList, _config);
}

void handle_user_input(char *line, void *cbdata) {
    char chunkf[128], outf[128];

    bzero(chunkf, sizeof(chunkf));
    bzero(outf, sizeof(outf));

    if (sscanf(line, "GET %120s %120s", chunkf, outf)) {
        if (strlen(outf) > 0) {
            process_get(chunkf, outf);
        }
    }
}


void peer_run(bt_config_t *config) {
    int sock;
    struct sockaddr_in myaddr;
    fd_set readfds;
    struct user_iobuf *userbuf;

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

    while (1) {
        int nfds;
        FD_SET(STDIN_FILENO, &readfds);
        FD_SET(sock, &readfds);

        nfds = select(sock + 1, &readfds, NULL, NULL, NULL);

        if (nfds > 0) {
            if (FD_ISSET(sock, &readfds)) {
                process_inbound_udp(sock, config);
            }

            if (FD_ISSET(STDIN_FILENO, &readfds)) {
                process_user_input(STDIN_FILENO, userbuf, handle_user_input,
                        "Currently unused");
            }
        }
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
