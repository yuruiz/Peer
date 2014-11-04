#include <string.h>
#include "congest.h"
#include "peer.h"

#define CONGEST_OUTPUT_FILE "problem2-peer.txt"

static int lastlostACk = 0;

void expandWin(conn_peer *node) {

    FILE *output = fopen(CONGEST_OUTPUT_FILE, "a");

    struct timeval curT;
    char linebuf[200];
    memset(linebuf, 0, 200 * sizeof(char));

    struct timeval *startT = getStartTime();

    gettimeofday(&curT, NULL);

    int diff = (int) (1000 * (curT.tv_sec - startT->tv_sec) + curT.tv_usec - startT->tv_usec);

    switch (node->congestCtl) {
        case SLOW_START:
            node->windowSize++;

            sprintf(linebuf, "%d\t%d\t%d\n", node->peerID, diff, node->windowSize);
            fwrite(linebuf, sizeof(char), strlen(linebuf), output);

            if (node->windowSize >= node->ssthreshold) {
                node->congestCtl = CONGEST_AVOID;
            }
            break;
        case CONGEST_AVOID:
            node->roundInc++;

            if (node->roundInc >= node->windowSize) {
                node->windowSize++;
                node->roundInc = 0;
                sprintf(linebuf, "%d\t%d\t%d\n", node->peerID, diff, node->windowSize);
                fwrite(linebuf, sizeof(char), strlen(linebuf), output);
            }
            break;
        case FAST_RETRANSMIT:
            node->windowSize++;
            if (node->lastAck > lastlostACk) {
                node->congestCtl = SLOW_START;
            }
            break;
        default:
            printf("Expand windows error! Unknown congestion control mode!\n");
            break;
    }


    fclose(output);

}


void shrinkWin(conn_peer *node) {
    node->congestCtl = FAST_RETRANSMIT;

    lastlostACk = node->lastAck;
    FILE *output = fopen(CONGEST_OUTPUT_FILE, "a");

    struct timeval curT;
    char linebuf[200];
    memset(linebuf, 0, 200 * sizeof(char));

    gettimeofday(&curT, NULL);

    printf("Now Shringking window size from %d to 1\n", node->windowSize);
    if (node->windowSize > 4) {
        node->ssthreshold = node->windowSize / 2;
    }
    else {
        node->ssthreshold = 2;
    }
    printf("The Slow Start Threshold is now %d\n", node->ssthreshold);

    node->windowSize = 1;

    struct timeval *startT = getStartTime();
    int diff = (int) (1000 * (curT.tv_sec - startT->tv_sec) + curT.tv_usec - startT->tv_usec);
    sprintf(linebuf, "%d\t%d\t%d\n", node->peerID, diff, node->windowSize);
    fwrite(linebuf, sizeof(char), strlen(linebuf), output);
    fclose(output);

}