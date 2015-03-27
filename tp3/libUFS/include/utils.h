#ifndef UTILS_H_
#define UTILS_H_

#include <stdlib.h>
#include "UFS.h"

// Test if another block(s) are needed for @spaceNumber octets in @currentSize
inline bool_t moreBlockNeeded(const UINT16 currentSize, const UINT16 spaceNeeded);

// Test if the block @blockNum (relative to @freeBlockBitmap) is Free
inline bool_t isBlockFree(const bool_t freeBlockBitmap[N_BLOCK_ON_DISK], const UINT16 blockNum);

// Test if the entry in @currentINodeEntry folder named @entryName exist.
bool_t	entryExist(const iNodeEntry *currentINodeEntry, const char *entryName);

// Reserve a Block on disk (call WriteBlock)
int reserveBlock(iNodeEntry *reservedINodeEntry);

// Clear a Block on disk (call WriteBlock)
int clearBlock(const ino blockToClear, iNodeEntry *INodeEntry);

/* Function base */
UINT16 numberOfDirEntry(UINT16 size);

/* Cette fonction sert à afficher à l'écran le contenu d'une structure d'i-node */
void printiNode(iNodeEntry iNode);

#endif
