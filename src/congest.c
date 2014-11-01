#include "congest.h"

void expandWin(conn_peer *node) {

    switch (node->congestCtl) {
        case SLOW_START:
            node->windowSize++;

            if (node->windowSize >= node->ssthreshold) {
                node->congestCtl = CONGEST_AVOID;
            }
            break;
        case CONGEST_AVOID:
            node->roundInc++;

            if (node->roundInc >= node->windowSize) {
                node->windowSize++;
                node->roundInc = 0;
            }
            break;
        default:
            printf("Expand windows error! Unknown congestion control mode!\n");
            break;
    }
}


void shrinkWin(conn_peer *node){
    node->congestCtl = SLOW_START;

    printf("Now Shringking window size from %d to %d\n", node->windowSize, node->windowSize/2);
    if (node->windowSize > 2) {
        node->ssthreshold = node->windowSize/2;
    }
    printf("The Slow Start Threshold is now %d\n", node->ssthreshold);

    node->windowSize = 1;

}