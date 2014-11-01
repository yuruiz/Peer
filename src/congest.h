#ifndef CONGEST_H
#define CONGEST_H
#include "conn.h"

void shrinkWin(conn_peer *node);
void expandWin(conn_peer *node);
#endif