#include <assert.h>
#include <stdlib.h>
#include <stdio.h>
#include <string.h>
#include "UFS.h"
#include "disque.h"
#include "utils.h"
#include "io_operation.h"
#include "path.h"
#include "inode_tools.h"
#include "links.h"

/* ----------------------------------------------------------------------------------------
   à vous de jouer, maintenant!
   ---------------------------------------------------------------------------------------- */

int	bd_countfreeblocks(void)
{
  bool_t	freeBlockBitmap[N_BLOCK_ON_DISK];
  int	i = 0;
  int	freeBlockNumber = 0;

  if (ReadBlock(FREE_BLOCK_BITMAP, (char *)(freeBlockBitmap)) == -1) {
#if !defined(NDEBUG)
    fprintf(stderr, "Function: %s: ReadBlock Failure\n", __PRETTY_FUNCTION__);
#endif
    return (-1);
  }
  while (i < N_BLOCK_ON_DISK) {
    if (isBlockFree(freeBlockBitmap, i))
      freeBlockNumber++;
    ++i;
  }
  return freeBlockNumber;
}

int		bd_stat(const char *pFilename, gstat *pStat)
{
  iNodeEntry	entryFound;

  if (resolvePath(pFilename, &entryFound) == -1) {
#if !defined(NDEBUG)
    fprintf(stderr, "Function: %s: resolvePath(%s) Failure\n", __PRETTY_FUNCTION__, pFilename);
#endif
    return (-1);
  }
  memcpy(pStat, &entryFound.iNodeStat, sizeof(entryFound.iNodeStat));
  return (0);
}

int		bd_create(const char *pFilename)
{
  char		basename[FILENAME_SIZE];
  char		dirname[MAX_DIR_PATH_SIZE];
  iNodeEntry	parentDirectoryEntry;
  iNodeEntry	newFileINodeEntry;
  int		preconditionStatus;

  if ((preconditionStatus = directoryNewEntryPreconditions(pFilename, basename, dirname,
							   &parentDirectoryEntry)) != 0) {
#if !defined(NDEBUG)
    fprintf(stderr, "Function: %s: directoryNewEntryPreconditon(%s) failure\n", __PRETTY_FUNCTION__, pFilename);
#endif
    return (preconditionStatus);
  }
  newFileINodeEntry.iNodeStat.st_mode = (G_IFREG | G_IRWXU | G_IRWXG);
  newFileINodeEntry.iNodeStat.st_nlink = 0;
  newFileINodeEntry.iNodeStat.st_size = 0;
  newFileINodeEntry.iNodeStat.st_blocks = 0;
  if (reserveINode(&newFileINodeEntry) == -1) {
#if !defined(NDEBUG)
    fprintf(stderr, "Function: %s: reserveINode() failure\n", __PRETTY_FUNCTION__);
#endif
    return (-1);
  }
  if (saveINodeEntry(&newFileINodeEntry) == -1) {
#if !defined(NDEBUG)
    fprintf(stderr, "Function: %s: saveINodeEntry(%d) failure\n", __PRETTY_FUNCTION__,
	    newFileINodeEntry.iNodeStat.st_ino);
#endif
    return (-1);
  }
  if (directoryAddEntry(&parentDirectoryEntry, &newFileINodeEntry, basename) == -1) {
#if !defined(NDEBUG)
    fprintf(stderr, "Function: %s: directoryAddEntry(%d, %d) failure\n", __PRETTY_FUNCTION__,
	    parentDirectoryEntry.iNodeStat.st_ino, newFileINodeEntry.iNodeStat.st_ino);
#endif
    return (-1);
  }
  return (0);
}

int bd_read(const char *pFilename, char *buffer, int offset, int numbytes)
{
  if (pFilename == NULL || buffer == NULL || offset < 0 || numbytes < 0) {
    return -1;
  }
  return _bd_read(pFilename, buffer, offset, numbytes);
}

int bd_write(const char *pFilename, const char *buffer, int offset, int numbytes)
{
  if (pFilename == NULL || buffer == NULL || offset < 0 || numbytes < 0) {
    return -1;
  }
  return _bd_write(pFilename, buffer, offset, numbytes);
}

int bd_truncate(const char *pFilename, int offset)
{
  if (pFilename == NULL || offset < 0) {
    return -1;
  }
  return _bd_truncate(pFilename, offset);
}


int		bd_mkdir(const char *pDirName)
{
  char		basename[FILENAME_SIZE];
  char		dirname[MAX_DIR_PATH_SIZE];
  iNodeEntry	parentDirectoryEntry;
  iNodeEntry	newDirINodeEntry;
  int		preconditionStatus;

  if ((preconditionStatus = directoryNewEntryPreconditions(pDirName, basename, dirname,
							   &parentDirectoryEntry)) != 0) {
#if !defined(NDEBUG)
    fprintf(stderr, "Function: %s: directoryNewEntryPreconditon(%s) failure\n", __PRETTY_FUNCTION__, pDirName);
#endif
    return (preconditionStatus);
  }
  newDirINodeEntry.iNodeStat.st_mode = (G_IFDIR | G_IRWXU | G_IRWXG);
  newDirINodeEntry.iNodeStat.st_nlink = 0;
  newDirINodeEntry.iNodeStat.st_size = 0;
  newDirINodeEntry.iNodeStat.st_blocks = 0;
  if (reserveINode(&newDirINodeEntry) == -1) {
#if !defined(NDEBUG)
    fprintf(stderr, "Function: %s: reserveINode() failure\n", __PRETTY_FUNCTION__);
#endif
    return (-1);
  }
  if (saveINodeEntry(&newDirINodeEntry) == -1) {
#if !defined(NDEBUG)
    fprintf(stderr, "Function: %s: saveINodeEntry(%d) failure\n", __PRETTY_FUNCTION__,
	    newDirINodeEntry.iNodeStat.st_ino);
#endif
    return (-1);
  }
  if (directoryAddEntry(&parentDirectoryEntry, &newDirINodeEntry, basename) == -1) {
#if !defined(NDEBUG)
    fprintf(stderr, "Function: %s: directoryAddEntry(%d, %d) failure\n", __PRETTY_FUNCTION__,
	    parentDirectoryEntry.iNodeStat.st_ino, newDirINodeEntry.iNodeStat.st_ino);
#endif
    return (-1);
  }
  if (directoryAddEntry(&newDirINodeEntry, &newDirINodeEntry, ".") == -1) {
#if !defined(NDEBUG)
    fprintf(stderr, "Function: %s: directoryAddEntry(%d, %d) failure\n", __PRETTY_FUNCTION__,
	    newDirINodeEntry.iNodeStat.st_ino, newDirINodeEntry.iNodeStat.st_ino);
#endif
    return (-1);
  }
  if (directoryAddEntry(&newDirINodeEntry, &parentDirectoryEntry, "..") == -1) {
#if !defined(NDEBUG)
    fprintf(stderr, "Function: %s: directoryAddEntry(%d, %d) failure\n", __PRETTY_FUNCTION__,
	    newDirINodeEntry.iNodeStat.st_ino, parentDirectoryEntry.iNodeStat.st_ino);
#endif
    return (-1);
  }
  return (0);
}

int		bd_hardlink(const char *pPathExistant, const char *pPathNewHardlink)
{
  return (addHardlink(pPathExistant, pPathNewHardlink));
}

int bd_unlink(const char *pFilename)
{
  return (deleteHardlink(pFilename));
}

int bd_rmdir(const char *pFilename)
{
  return -1;
}

int bd_rename(const char *pFilename, const char *pDestFilename)
{
  return -1;
}

int bd_readdir(const char *pDirLocation, DirEntry **ppListeFichiers)
{
  iNodeEntry	directoryINodeEntry;
  UINT16	numEntries = 0;
  UINT16	currentBlock = 0;

  if (resolvePath(pDirLocation, &directoryINodeEntry) != 0) {
#if !defined(NDEBUG)
    fprintf(stderr, "Function: %s: resolvePath(%s) failure\n", __PRETTY_FUNCTION__, pDirLocation);
#endif
    return (-1);
  }
  if (!G_ISDIR(directoryINodeEntry.iNodeStat.st_mode)) {
#if !defined(NDEBUG)
    fprintf(stderr, "Function: %s: %s is not a directory\n", __PRETTY_FUNCTION__, pDirLocation);
#endif
    return (-1);
  }
  numEntries = numberOfDirEntry(directoryINodeEntry.iNodeStat.st_size);
  *ppListeFichiers = calloc(numEntries, sizeof(**ppListeFichiers));
  while (currentBlock < directoryINodeEntry.iNodeStat.st_blocks) {
    const ino	currentBlockINode = directoryINodeEntry.Block[currentBlock];
    DirEntry	entriesBlock[NUM_DIR_ENTRY_PER_BLOCK];

    if (ReadBlock(currentBlockINode, (char *)entriesBlock) == -1) {
#if !defined(NDEBUG)
      fprintf(stderr, "Function: %s: ReadBlock(%d) Failure\n", __PRETTY_FUNCTION__, currentBlockINode);
#endif
      return (-1);
    }

    if (currentBlock == directoryINodeEntry.iNodeStat.st_blocks - 1)
      memcpy(&((*ppListeFichiers)[currentBlock * NUM_DIR_ENTRY_PER_BLOCK]), entriesBlock,
	     (numEntries % NUM_DIR_ENTRY_PER_BLOCK) * sizeof(DirEntry));
    else
      memcpy(&((*ppListeFichiers)[currentBlock * NUM_DIR_ENTRY_PER_BLOCK]), entriesBlock, BLOCK_SIZE);
    currentBlock++;
  }
  return (numEntries);
}
