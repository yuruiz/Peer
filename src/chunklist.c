#include "chunklist.h"
#include "request.h"
#include "input_buffer.h"
#include "peer.h"

#define LINESIZE 8096

// check current chunk hash is requested.
int list_contains(char *chunkHash, job *userjob) {
    int i;
    for (i = 0; i < userjob->chunk_list.chunkNum; i++) {
        char buf[SHA1_HASH_LENGTH * 2 + 1];
        binary2hex(userjob->chunk_list.list[i].hash, SHA1_HASH_LENGTH, buf);

        if (strcmp(buf, chunkHash) == 0) {
            if (userjob->chunk_list.list[i].status == unfetched) {
                return i;
            }
            return -1;
        }
    }
    return -1;
}

void setChunkDone(char *chunkHash,job *userjob){
    int i;
    for (i = 0; i < userjob->chunk_list.chunkNum; i++) {
        char buf[SHA1_HASH_LENGTH * 2 + 1];
        binary2hex(userjob->chunk_list.list[i].hash, SHA1_HASH_LENGTH, buf);

        if (strcmp(buf, chunkHash) == 0) {
            userjob->chunk_list.list[i].status = fetched;
            return;
        }
    }
}


void resetChunk(char *chunkHash,job *userjob){
    int i;
    for (i = 0; i < userjob->chunk_list.chunkNum; i++) {
        char buf[SHA1_HASH_LENGTH * 2 + 1];
        binary2hex(userjob->chunk_list.list[i].hash, SHA1_HASH_LENGTH, buf);

        if (strcmp(buf, chunkHash) == 0) {
            userjob->chunk_list.list[i].status = unfetched;
            return;
        }
    }
}

int list_empty(job *userjob) {
    int i;
    for (i = 0; i < userjob->chunk_list.chunkNum; i++) {
        if (userjob->chunk_list.list[i].status == unfetched) {
            return EXIT_FAILURE;
        }
    }
    return EXIT_SUCCESS;
}

//void list_remove(char *chunkHash) {
//    int i;
//    for (i = 0; i < MAX_CHUNK_NUM; i++) {
//
//        if (request_queue[i] == NULL)
//            continue;
//        else if (strcmp(request_queue[i], chunkHash) == 0) {
//            free(request_queue[i]);
//            request_queue[i] = NULL;
//            return;
//        }
//    }
//}

void buildChunkList(chunklist *cklist) {

    char linebuf[LINESIZE];
    int chunkCount = 0;

    if (cklist->chunkfptr == NULL) {
        fprintf(stderr, "chunkfptr is null!\n");
        return;
    }

    while (!feof(cklist->chunkfptr)) {
        char hashbuf[LINESIZE];
        memset(hashbuf, 0, LINESIZE);
        int hashindex;

        if (fgets(linebuf, LINESIZE, cklist->chunkfptr) == NULL) {
            break;
        }

        if (sscanf(linebuf, "%d %s", &hashindex, hashbuf) != 2) {
            fprintf(stderr, "parse hash file error %d\n", chunkCount);
            exit(EXIT_FAILURE);
        }

        cklist->list[chunkCount].seq = hashindex;
        cklist->list[chunkCount].status = unfetched;
        hex2binary(hashbuf, 2 * SHA1_HASH_LENGTH, cklist->list[chunkCount].hash);
        chunkCount++;
    }

    int i;
    for (i = chunkCount; i < MAX_CHUNK_NUM; i++) {
        cklist->list[i].status = none;
    }
    cklist->chunkNum = chunkCount;
    cklist->unfetchedNum = chunkCount;

    return;

}


// parse chunk hash into a chunk_list from incoming whohas packet.
char **retrieve_chunk_list(Packet *incomingPacket) {
    char **chunk_list;
    int num_chunks, head;

    chunk_list = (char **) malloc(getHashCount(incomingPacket) * \
        sizeof(char *));
    memset(chunk_list, 0, getHashCount(incomingPacket) * sizeof(char *));

    for (num_chunks = 0; num_chunks < getHashCount(incomingPacket); num_chunks++) {
        chunk_list[num_chunks] = (char *) malloc(SHA1_HASH_LENGTH * 2 + 1);
        memset(chunk_list[num_chunks], 0, (SHA1_HASH_LENGTH * 2 + 1) * sizeof(char));
        head = num_chunks * SHA1_HASH_LENGTH;
        uint8_t buf[SHA1_HASH_LENGTH];

        strncpy((char *) buf, (const char *) incomingPacket->serial + HASH_OFFSET + head, SHA1_HASH_LENGTH);

        binary2hex(incomingPacket->serial + HASH_OFFSET + head, SHA1_HASH_LENGTH, chunk_list[num_chunks]);
        //   printf("retrieve: %s\n", chunk_list[num_chunks]);
    }

    return chunk_list;
}

// check if the requested chunks exits.
char **has_chunks(bt_config_t *config, Packet *p, char **chunk_list) {
    char **haschunk_list;
    FILE *fp;
    char buf[USERBUF_SIZE];
    char chunk[SHA1_HASH_LENGTH * 2 + 1];
    int num_chunks, haschunk_pos;

    haschunk_pos = 0;
    haschunk_list = (char **) malloc(getHashCount(p) * sizeof(char *));
    memset(haschunk_list, 0, getHashCount(p) * sizeof(char *));
    if ((fp = fopen(config->has_chunk_file, "r")) == NULL) {
        printf("Open has chunk file %s failed\n", config->has_chunk_file);
        exit(EXIT_FAILURE);
    }
    while (!feof(fp)) {
        fgets(buf, USERBUF_SIZE, fp);
        memset(chunk, 0, SHA1_HASH_LENGTH * 2 + 1);
        sscanf(buf, "%*d %s", chunk);
        for (num_chunks = 0; num_chunks < getHashCount(p); num_chunks++) {
            if (strncmp(chunk_list[num_chunks], chunk, SHA1_HASH_LENGTH * 2) == 0) {
                haschunk_list[haschunk_pos] = (char *) malloc(SHA1_HASH_LENGTH * 2 + 1);
                memset(haschunk_list[haschunk_pos], 0, SHA1_HASH_LENGTH * 2 + 1);
                strcpy(haschunk_list[haschunk_pos++], chunk);
            }
        }
        memset(buf, 0, USERBUF_SIZE);
    }
    fclose(fp);
    return haschunk_list;
}