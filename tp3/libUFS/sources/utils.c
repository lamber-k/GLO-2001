#include <string.h>
#include <stdio.h>
#include <assert.h>
#include "utils.h"
#include "disque.h"
#include "path.h"
#include "inode_tools.h"

inline bool_t isBlockFree(const bool_t freeBlockBitmap[N_BLOCK_ON_DISK], const UINT16 blockNum) {
#if !defined(NDEBUG)
  assert(blockNum >= 0 && blockNum < N_BLOCK_ON_DISK);
#endif
  return (freeBlockBitmap[blockNum] == BLOCK_FREE);
}

inline bool_t moreBlockNeeded(const UINT16 currentSize, const UINT16 spaceNeeded) {
  return (currentSize == 0 || (currentSize % BLOCK_SIZE) + spaceNeeded > BLOCK_SIZE);
}


int		reserveBlock(iNodeEntry *reservedBlock) {
  bool_t	blockBitmap[BLOCK_SIZE];
  UINT16	currentBlock = 1;

  if (ReadBlock(FREE_BLOCK_BITMAP, (char *)(blockBitmap)) == -1) {
#if !defined(NDEBUG)
    fprintf(stderr, "Function: %s: ReadBlock(%d) Failure\n", __PRETTY_FUNCTION__, FREE_BLOCK_BITMAP);
#endif
    return (-1);
  }
  while (currentBlock < N_BLOCK_ON_DISK) {
    if (isBlockFree(blockBitmap, currentBlock)) {
      blockBitmap[currentBlock] = BLOCK_NOT_FREE;
      if (WriteBlock(FREE_BLOCK_BITMAP, (const char *)(blockBitmap)) == -1) {
#if !defined(NDEBUG)
	fprintf(stderr, "Function: %s: WriteBlock Failure\n", __PRETTY_FUNCTION__);
#endif
	return (-1);
      }
      reservedBlock->Block[reservedBlock->iNodeStat.st_blocks] = currentBlock;
      reservedBlock->iNodeStat.st_blocks++;
      return (0);
    }
    ++currentBlock;
  }
  return (-1);
}

static int	iNodeFindBlockPos(const iNodeEntry *iNodeEntry, const ino numBlock) {
  UINT16	i = 0;

  while (i < N_BLOCK_PER_INODE && i < iNodeEntry->iNodeStat.st_blocks) {
    if (iNodeEntry->Block[i] == numBlock) {
      return (i);
    }
    ++i;
  }
  return (-1);
}

static int	iNodeDeleteBlock(iNodeEntry *iNodeEntry, const ino numBlock) {
  int		iNodeBlockPos;
  UINT16	i;

  if ((iNodeBlockPos = iNodeFindBlockPos(iNodeEntry, numBlock)) == -1) {
#if !defined(NDEBUG)
    fprintf(stderr, "Function: %s: iNodeFindBlockPos() Failure\n", __PRETTY_FUNCTION__);
#endif
    return (-1);
  }
  i = iNodeBlockPos;
  while (i < (iNodeEntry->iNodeStat.st_blocks - 1)) {
    iNodeEntry->Block[i] = iNodeEntry->Block[i + 1];
    ++i;
  }
  iNodeEntry->iNodeStat.st_blocks--;
  return (0);
 }

int		clearBlock(const ino blockToClear, iNodeEntry *iNodeEntry) {
  bool_t	blockBitmap[BLOCK_SIZE];
  int		blockBlockPos;

  if ((blockBlockPos = iNodeFindBlockPos(iNodeEntry, blockToClear)) == -1) {
#if !defined(NDEBUG)
    fprintf(stderr, "Function: %s: iNodeFindBlockPos() Failure\n", __PRETTY_FUNCTION__);
#endif
    return (-1);
  }
  if (ReadBlock(FREE_BLOCK_BITMAP, (char *)(blockBitmap)) == -1) {
#if !defined(NDEBUG)
    fprintf(stderr, "Function: %s: ReadBlock(%d) Failure\n", __PRETTY_FUNCTION__, FREE_BLOCK_BITMAP);
#endif
    return (-1);
  }

#if !defined(NDEBUG)
  assert(isBlockFree(blockBitmap, blockToClear) == FALSE);
#endif
  blockBitmap[blockToClear] = BLOCK_FREE;
  if (iNodeDeleteBlock(iNodeEntry, blockToClear) == -1) {
#if !defined(NDEBUG)
    fprintf(stderr, "Function: %s: iNodeDeleteBlock(%d) Failure\n", __PRETTY_FUNCTION__, blockToClear);
#endif
    return (-1);
  }
  if (WriteBlock(FREE_BLOCK_BITMAP, (const char *)(blockBitmap)) == -1) {
#if !defined(NDEBUG)
    fprintf(stderr, "Function: %s: WriteBlock Failure\n", __PRETTY_FUNCTION__);
#endif
    return (-1);
  }
  return (0);
}

// Quelques fonctions qui pourraient vous être utiles
UINT16 numberOfDirEntry(UINT16 size) {
  return (size / sizeof(DirEntry));
}

int min(int a, int b) {
  return a<b ? a : b;
}

int max(int a, int b) {
  return a>b ? a : b;
}


/* Cette fonction sert à afficher à l'écran le contenu d'une structure d'i-node */
void printiNode(iNodeEntry iNode) {
  printf("\t\t========= inode %d ===========\n",iNode.iNodeStat.st_ino);
  printf("\t\t  blocks:%d\n",iNode.iNodeStat.st_blocks);
  printf("\t\t  size:%d\n",iNode.iNodeStat.st_size);
  printf("\t\t  mode:0x%x\n",iNode.iNodeStat.st_mode);
  int index = 0;
  for (index =0; index < N_BLOCK_PER_INODE; index++) {
    printf("\t\t      Block[%d]=%d\n",index,iNode.Block[index]);
  }
}

