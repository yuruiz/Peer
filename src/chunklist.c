#include "chunklist.h"
#include "request.h"
#include "input_buffer.h"

#define LINESIZE 8096

void buildChunkList(chunklist *cklist) {

    char linebuf[LINESIZE];
    int chunkCount = 0;

    if (cklist->chunkfptr == NULL) {
        fprintf(stderr, "chunkfptr is null!\n");
        return;
    }

    while (!feof(cklist->chunkfptr)) {
        char hashbuf[LINESIZE];
        int hashindex;

        memset(hashbuf, 0, LINESIZE);
        if (fgets(linebuf, LINESIZE, cklist->chunkfptr) == NULL) {
            break;
        }

        if (sscanf(linebuf, "%d %s", &hashindex, hashbuf) != 2) {
            fprintf(stderr, "parse hash file error %d\n", chunkCount);
            exit(EXIT_FAILURE);
        }

        cklist->list[chunkCount].seq = hashindex;
        hex2binary(hashbuf, 2 * SHA1_HASH_LENGTH, \
            cklist->list[chunkCount].hash);
        chunkCount++;
    }

    cklist->chunkNum = chunkCount;

    return;

}


// parse chunk hash into a chunk_list from incoming whohas packet.
char **retrieve_chunk_list(Packet *incomingPacket) {
    char **chunk_list;
    int num_chunks, head;

    chunk_list = (char **) malloc(getHashCount(incomingPacket) * \
        sizeof(char *));
    memset(chunk_list, 0, getHashCount(incomingPacket) * sizeof(char *));

    for (num_chunks = 0; num_chunks < getHashCount(incomingPacket); \
        num_chunks++) {
        chunk_list[num_chunks] = (char *) malloc(SHA1_HASH_LENGTH * 2 + 1);
        head = num_chunks * SHA1_HASH_LENGTH;
        uint8_t buf[SHA1_HASH_LENGTH];

        strncpy((char*)buf, (const char*)incomingPacket->serial + \
            HASH_OFFSET + head, SHA1_HASH_LENGTH);

        binary2hex(buf, SHA1_HASH_LENGTH, chunk_list[num_chunks]);
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
        for (num_chunks = 0; num_chunks < getHashCount(p); num_chunks++){
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