#ifndef CHUNKLIST_H
#define CHUNKLIST_H

#include <stddef.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include "request.h"
#include "chunk.h"


void buildChunkList(chunklist* cklist);
char **retrieve_chunk_list(Packet *incomingPacket);
char **has_chunks(bt_config_t *config, Packet *p, char **chunk_list);
#endif