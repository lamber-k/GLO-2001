#include	<stdlib.h>
#include	<string.h>
#include	<stdio.h>
#include	<assert.h>
#include	"disque.h"
#include	"inode_tools.h"

inline bool_t isINodeFree(const bool_t freeINodeBitmap[N_INODE_ON_DISK], const UINT16 inodeNum) {
#if !defined(NDEBUG)
  assert(inodeNum > 0 && inodeNum < N_INODE_ON_DISK);
#endif
  return (freeINodeBitmap[inodeNum] == INODE_FREE);
}

int		reserveINode(iNodeEntry *reservedINode) {
  bool_t	iNodeBitmap[BLOCK_SIZE];
  UINT16	currentINode = 1;

  if (ReadBlock(FREE_INODE_BITMAP, (char *)(iNodeBitmap)) == -1) {
#if !defined(NDEBUG)
    fprintf(stderr, "Function: %s: ReadBlock(%d) Failure\n", __PRETTY_FUNCTION__, FREE_INODE_BITMAP);
#endif
    return (-1);
  }
  while (currentINode < N_INODE_ON_DISK) {
    if (isINodeFree(iNodeBitmap, currentINode)) {
      iNodeBitmap[currentINode] = INODE_NOT_FREE;
      if (WriteBlock(FREE_INODE_BITMAP, (const char *)(iNodeBitmap)) == -1) {
#if !defined(NDEBUG)
	fprintf(stderr, "Function: %s: WriteBlock Failure\n", __PRETTY_FUNCTION__);
#endif
	return (-1);
      }
      reservedINode->iNodeStat.st_ino = currentINode;
      return (0);
    }
    ++currentINode;
  }
  return (-1);
}

int		clearINode(const ino iNodeToClear) {
  bool_t	blockBitmap[BLOCK_SIZE];

  if (ReadBlock(FREE_INODE_BITMAP, (char *)(blockBitmap)) == -1) {
#if !defined(NDEBUG)
    fprintf(stderr, "Function: %s: ReadBlock(%d) Failure\n", __PRETTY_FUNCTION__, FREE_INODE_BITMAP);
#endif
    return (-1);
  }

#if !defined(NDEBUG)
  assert(isINodeFree(blockBitmap, iNodeToClear) == FALSE);
#endif
  blockBitmap[iNodeToClear] = INODE_FREE;
  if (WriteBlock(FREE_INODE_BITMAP, (const char *)(blockBitmap)) == -1) {
#if !defined(NDEBUG)
    fprintf(stderr, "Function: %s: WriteBlock Failure\n", __PRETTY_FUNCTION__);
#endif
    return (-1);
  }
  return (0);
}

int		saveINodeEntry(const iNodeEntry *toSave) {
  const ino	iNodeEntryBlockIndex = INODE_BLOCK(toSave->iNodeStat.st_ino);
  const UINT16	iNodeEntriesIndex = INODE_IDX_ON_BLOCK(toSave->iNodeStat.st_ino);
  iNodeEntry	iNodeEntriesBlock[NUM_INODE_PER_BLOCK];

  if (ReadBlock(iNodeEntryBlockIndex, (char *)(iNodeEntriesBlock)) == -1) {
#if !defined(NDEBUG)
    fprintf(stderr, "Function: %s: ReadBlock(%d) Failure\n", __PRETTY_FUNCTION__, iNodeEntryBlockIndex);
#endif
    return (-1);
  }

  memcpy(&iNodeEntriesBlock[iNodeEntriesIndex], toSave, sizeof(*toSave));

  if (WriteBlock(iNodeEntryBlockIndex, (const char *)(iNodeEntriesBlock)) == -1) {
#if !defined(NDEBUG)
    fprintf(stderr, "Function: %s: WriteBlock(%d) Failure\n", __PRETTY_FUNCTION__, iNodeEntryBlockIndex);
#endif
    return (-1);
  }
  return (0);
}


int getINodeEntry(const UINT16 iNodeNum, iNodeEntry *entry) {
#if !defined(NDEBUG)
  assert(iNodeNum > 0 && iNodeNum < N_INODE_ON_DISK);
#endif
  iNodeEntry	iNodesBlock[NUM_INODE_PER_BLOCK];

  if (ReadBlock(INODE_BLOCK(iNodeNum), (char *)(iNodesBlock)) == -1) {
#if !defined(NDEBUG)
    fprintf(stderr, "Function: %s: ReadBlock Failure\n", __PRETTY_FUNCTION__);
#endif
    return (-1);
  }

  memcpy(entry, iNodesBlock + INODE_IDX_ON_BLOCK(iNodeNum), sizeof(*entry));
  return (0);
}
