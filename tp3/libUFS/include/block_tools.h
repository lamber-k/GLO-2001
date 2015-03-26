#ifndef BLOCK_TOOLS_H_
#define BLOCK_TOOLS_H_
#include "UFS.h"
// add block to entry
int addBlock(iNodeEntry *fileEntry);
// remove block to entry
int freeBlock(iNodeEntry *fileEntry, int number);
#endif