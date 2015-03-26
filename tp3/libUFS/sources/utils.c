#include <string.h>
#include <stdio.h>
#include <assert.h>
#include "utils.h"
#include "disque.h"
#include "path.h"

inline bool_t isBlockFree(const bool_t freeBlockBitmap[N_BLOCK_ON_DISK], const UINT16 blockNum) {
#if !defined(NDEBUG)
  assert(blockNum > 0 && blockNum < N_BLOCK_ON_DISK);
#endif
  return (freeBlockBitmap[blockNum] == BLOCK_FREE);
}

inline bool_t isINodeFree(const bool_t freeINodeBitmap[N_INODE_ON_DISK], const UINT16 inodeNum) {
#if !defined(NDEBUG)
  assert(inodeNum > 0 && inodeNum < N_INODE_ON_DISK);
#endif
  return (freeINodeBitmap[inodeNum] == INODE_FREE);
}

inline bool_t moreBlockNeeded(const UINT16 currentSize, const UINT16 spaceNeeded) {
  return (currentSize == 0 || (currentSize % BLOCK_SIZE) + spaceNeeded > BLOCK_SIZE);
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

int		reserveBlock(iNodeEntry *reservedBlock) {
  bool_t	iNodeBitmap[BLOCK_SIZE];
  UINT16	currentBlock = 1;

  if (ReadBlock(FREE_INODE_BITMAP, (char *)(iNodeBitmap)) == -1) {
#if !defined(NDEBUG)
    fprintf(stderr, "Function: %s: ReadBlock(%d) Failure\n", __PRETTY_FUNCTION__, FREE_INODE_BITMAP);
#endif
    return (-1);
  }
  while (currentBlock < N_INODE_ON_DISK) {
    if (isBlockFree(iNodeBitmap, currentBlock)) {
      iNodeBitmap[currentBlock] = INODE_NOT_FREE;
      if (WriteBlock(FREE_INODE_BITMAP, (const char *)(iNodeBitmap)) == -1) {
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
  int		iNodeBlockPos;

  if ((iNodeBlockPos = iNodeFindBlockPos(iNodeEntry, blockToClear)) == -1) {
#if !defined(NDEBUG)
    fprintf(stderr, "Function: %s: iNodeFindBlockPos() Failure\n", __PRETTY_FUNCTION__);
#endif
    return (-1);
  }
  if (ReadBlock(FREE_INODE_BITMAP, (char *)(blockBitmap)) == -1) {
#if !defined(NDEBUG)
    fprintf(stderr, "Function: %s: ReadBlock(%d) Failure\n", __PRETTY_FUNCTION__, FREE_INODE_BITMAP);
#endif
    return (-1);
  }

#if !defined(NDEBUG)
  assert(isBlockFree(blockBitmap, blockToClear) == FALSE);
#endif
  blockBitmap[blockToClear] = INODE_FREE;
  if (iNodeDeleteBlock(iNodeEntry, blockToClear) == -1) {
#if !defined(NDEBUG)
    fprintf(stderr, "Function: %s: iNodeDeleteBlock(%d) Failure\n", __PRETTY_FUNCTION__, blockToClear);
#endif
    return (-1);
  }
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


int	directoryNewEntryPreconditions(const char *pFilename, 
				       char basename[FILENAME_SIZE], 
				       char dirname[MAX_DIR_PATH_SIZE],
				       iNodeEntry *parentDirectoryEntry) {
  if (GetFilenameFromPath(pFilename, basename) == 0) {
#if !defined(NDEBUG)
    fprintf(stderr, "Function: %s: GetFilenameFromPath(%s) Failure\n", __PRETTY_FUNCTION__, pFilename);
#endif
    return (-1);
  }
  if (GetDirFromPath(pFilename, dirname) == 0) {
#if !defined(NDEBUG)
    fprintf(stderr, "Function: %s: GetDirFromPath(%s) Failure\n", __PRETTY_FUNCTION__, pFilename);
#endif
    return (-1);
  }
  if (resolvePath(dirname, parentDirectoryEntry) == -1) {
#if !defined(NDEBUG)
    fprintf(stderr, "Function: %s: resolvePath(%s) Failure\n", __PRETTY_FUNCTION__, dirname);
#endif
    return (-1);
  }
  if (parentDirectoryEntry->iNodeStat.st_size == MAX_ENTRIES_SIZE) {
#if !defined(NDEBUG)
    fprintf(stderr, "Function: %s: maximum entries reached for %s\n", __PRETTY_FUNCTION__, dirname);
#endif
    return (-1);
  }
  if (entryExist(parentDirectoryEntry, basename) == TRUE) {
#if !defined(NDEBUG)
    fprintf(stderr, "Function: %s: %s already exist on directory %s\n", __PRETTY_FUNCTION__, basename, dirname);
#endif
    return (-2);
  }
  return (0);
}

int	directoryAddEntrySelectBlock(iNodeEntry *parentDirectory,
				     ino *newEntryParentBlockIndex,
				     DirEntry dirEntryBlock[NUM_DIR_ENTRY_PER_BLOCK]) {
  if (moreBlockNeeded(parentDirectory->iNodeStat.st_size, sizeof(DirEntry)) == TRUE) {
    if (reserveBlock(parentDirectory) == -1) {
#if !defined(NDEBUG)
      fprintf(stderr, "Function: %s: reserveblock() failure\n", __PRETTY_FUNCTION__);
#endif
      return (-1);
    }
  }
  *newEntryParentBlockIndex = parentDirectory->Block[parentDirectory->iNodeStat.st_blocks - 1];
  parentDirectory->iNodeStat.st_size += sizeof(DirEntry);

  if (ReadBlock(*newEntryParentBlockIndex, (char *)(dirEntryBlock)) == -1) {
#if !defined(NDEBUG)
    fprintf(stderr, "Function: %s: ReadBlock(%d) Failure\n", __PRETTY_FUNCTION__, *newEntryParentBlockIndex);
#endif
    return (-1);
  }
  if (saveINodeEntry(parentDirectory) == -1) {
#if !defined(NDEBUG)
    fprintf(stderr, "Function: %s: saveINodeEntry(%d) failure\n", __PRETTY_FUNCTION__, 
	    parentDirectory->iNodeStat.st_ino);
#endif
    return (-1);
  }
  return (0);
}

int	directoryAddEntry(iNodeEntry *parentDirectory, 
			  iNodeEntry *newEntry,
			  const char *newEntryName) {
  ino		newEntryParentBlockIndex;
  DirEntry	dirEntryBlock[NUM_DIR_ENTRY_PER_BLOCK];
  UINT16	entryBlockIndex;

  memset(dirEntryBlock, 0, sizeof(dirEntryBlock));
  if (directoryAddEntrySelectBlock(parentDirectory, &newEntryParentBlockIndex, dirEntryBlock)) {
#if !defined(NDEBUG)
    fprintf(stderr, "Function: %s: directoryAddEntrySelectBlock(%d) Failure\n", __PRETTY_FUNCTION__, 
	    parentDirectory->iNodeStat.st_ino);
#endif
    return (-1);
  }

  entryBlockIndex = (numberOfDirEntry(parentDirectory->iNodeStat.st_size) - 1) % NUM_DIR_ENTRY_PER_BLOCK;
  strcpy(dirEntryBlock[entryBlockIndex].Filename, newEntryName);
  dirEntryBlock[entryBlockIndex].iNode = newEntry->iNodeStat.st_ino;
  newEntry->iNodeStat.st_nlink++;

  if (WriteBlock(newEntryParentBlockIndex, (const char *)(dirEntryBlock)) == -1) {
#if !defined(NDEBUG)
    fprintf(stderr, "Function: %s: WriteBlock(%d) Failure\n", __PRETTY_FUNCTION__, newEntryParentBlockIndex);
#endif
    return (-1);
  }
  if (saveINodeEntry(newEntry) == -1) {
#if !defined(NDEBUG)
    fprintf(stderr, "Function: %s: saveINodeEntry(%d) failure\n", __PRETTY_FUNCTION__, 
	    newEntry->iNodeStat.st_ino);
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

