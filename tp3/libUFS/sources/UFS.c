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
#include "block_tools.h"

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
  iNodeEntry	dirEntry;
  iNodeEntry	parentDirEntry;
  char		directoryName[FILENAME_SIZE];
  char		*pathParentDirectory;

  if (strcmp(pFilename, "/") == 0) {
#if !defined(NDEBUG)
    fprintf(stderr, "Function: %s: cannot remove / directory\n", __PRETTY_FUNCTION__);
#endif
    return -1;
  }
  if (GetFilenameFromPath(pFilename, directoryName) == 0) {
#if !defined(NDEBUG)
    fprintf(stderr, "Function: %s: GetFilenameFromPath(%s) failure\n", __PRETTY_FUNCTION__, pFilename);
#endif
    return -1;
  }

  if (resolvePath(pFilename, &dirEntry) != 0) {
#if !defined(NDEBUG)
    fprintf(stderr, "Function: %s: resolvePath(%s) failure\n", __PRETTY_FUNCTION__, pFilename);
#endif
    return -1;
  }
  pathParentDirectory = calloc(strlen(pFilename) + 1, sizeof(char));
  if (pathParentDirectory == NULL) {
#if !defined(NDEBUG)
    fprintf(stderr, "Function: %s: Bad allocation\n", __PRETTY_FUNCTION__);
#endif
    return -1;
  }
  if (GetDirFromPath(pFilename, pathParentDirectory) == 0) {
    free(pathParentDirectory);
#if !defined(NDEBUG)
    fprintf(stderr, "Function: %s: GetDirFromPath(%s) failure\n", __PRETTY_FUNCTION__, pFilename);
#endif
    return -1;
  }
  if (resolvePath(pathParentDirectory, &parentDirEntry) != 0) {
    free(pathParentDirectory);
#if !defined(NDEBUG)
    fprintf(stderr, "Function: %s: resolvePath(%s) failure\n", __PRETTY_FUNCTION__, pathParentDirectory);
#endif
    return -1;
  }
  if (!G_ISDIR(dirEntry.iNodeStat.st_mode)) {
    free(pathParentDirectory);
#if !defined(NDEBUG)
    fprintf(stderr, "Function: %s: %s is not a directory\n", __PRETTY_FUNCTION__, pFilename);
#endif
    return -2;
  }
  if (dirEntry.iNodeStat.st_size > (UINT16) sizeof(DirEntry) * 2) {
    free(pathParentDirectory);
#if !defined(NDEBUG)
    fprintf(stderr, "Function: %s: %s is not empty\n", __PRETTY_FUNCTION__, pFilename);
#endif
    return -3;
  }
  if (directoryDelEntry(&parentDirEntry, directoryName) != 0) {
    free(pathParentDirectory);
#if !defined(NDEBUG)
    fprintf(stderr, "Function: %s: directoryDelEntry(%s, %s) failure\n", __PRETTY_FUNCTION__, pathParentDirectory, directoryName);
#endif
    return -1;
  }
  parentDirEntry.iNodeStat.st_nlink--;
  if (saveINodeEntry(&parentDirEntry) == -1) {
#if !defined(NDEBUG)
    fprintf(stderr, "Function: %s: saveINodeEntry(%d) failure\n", __PRETTY_FUNCTION__,
	    parentDirEntry.iNodeStat.st_ino);
#endif
    return (-1);
  }
  if (freeBlock(&dirEntry, dirEntry.iNodeStat.st_blocks) != 0) {
#if !defined(NDEBUG)
    fprintf(stderr, "Function: %s: freeBlock(%s) failure\n", __PRETTY_FUNCTION__, directoryName);
    return -1;
#endif	
  }
  free(pathParentDirectory);
  return 0;
}

int bd_rename(const char *pFilename, const char *pDestFilename)
{
  char		*pathSourceDirectory;
  char		*pathDestinationDirectory;
  char		fileNameDestination[FILENAME_SIZE];
  char		fileNameSource[FILENAME_SIZE];
  iNodeEntry	fileEntry;
  iNodeEntry	sourceDir;
  iNodeEntry	destinationDir;

  if (resolvePath(pFilename, &fileEntry) != 0) {
#if !defined(NDEBUG)
    fprintf(stderr, "Function: %s: resolvePath(%s) failure\n", __PRETTY_FUNCTION__, pFilename);
#endif
    return -1;
  }

  pathSourceDirectory = calloc(strlen(pFilename) + 1, sizeof(char));
  pathDestinationDirectory = calloc(strlen(pDestFilename) + 1, sizeof(char));
	
  if (pathDestinationDirectory == NULL || pathSourceDirectory == NULL) {
#if !defined(NDEBUG)
    fprintf(stderr, "Function: %s: Bad allocation\n", __PRETTY_FUNCTION__);
#endif
    return -1;
  }

  if (GetDirFromPath(pFilename, pathSourceDirectory) == 0) {
    free(pathDestinationDirectory);
    free(pathSourceDirectory);
#if !defined(NDEBUG)
    fprintf(stderr, "Function: %s: GetDirFromPath(%s) failure\n", __PRETTY_FUNCTION__, pFilename);
#endif
    return -1;
  }

  if (resolvePath(pathSourceDirectory, &sourceDir) != 0 ) {
    free(pathDestinationDirectory);
    free(pathSourceDirectory);
#if !defined(NDEBUG)
    fprintf(stderr, "Function: %s: resolvePath(%s) failure\n", __PRETTY_FUNCTION__, pathSourceDirectory);
#endif
    return -1;
  }

  if (GetDirFromPath(pDestFilename, pathDestinationDirectory) == 0) {
    free(pathDestinationDirectory);
    free(pathSourceDirectory);
#if !defined(NDEBUG)
    fprintf(stderr, "Function: %s: GetDirFromPath(%s) failure\n", __PRETTY_FUNCTION__, pDestFilename);
#endif
    return -1;
  }

  if (resolvePath(pathDestinationDirectory, &destinationDir) != 0 ) {
    free(pathDestinationDirectory);
    free(pathSourceDirectory);
#if !defined(NDEBUG)
    fprintf(stderr, "Function: %s: resolvePath(%s) failure\n", __PRETTY_FUNCTION__, pathDestinationDirectory);
#endif
    return -1;
  }

  if (GetFilenameFromPath(pDestFilename, fileNameDestination) == 0) {
    free(pathDestinationDirectory);
    free(pathSourceDirectory);
#if !defined(NDEBUG)
    fprintf(stderr, "Function: %s: GetFilenameFromPath(%s) failure\n", __PRETTY_FUNCTION__, pDestFilename);
#endif
    return -1;
  }

  if (GetFilenameFromPath(pFilename, fileNameSource) == 0) {
    free(pathDestinationDirectory);
    free(pathSourceDirectory);
#if !defined(NDEBUG)
    fprintf(stderr, "Function: %s: GetFilenameFromPath(%s) failure\n", __PRETTY_FUNCTION__, pFilename);
#endif
    return -1;
  }
  if (directoryAddEntry(&destinationDir, &fileEntry, fileNameDestination) != 0) {
    free(pathDestinationDirectory);
    free(pathSourceDirectory);
#if !defined(NDEBUG)
    fprintf(stderr, "Function: %s: directoryAddEntry(%s) failure\n", __PRETTY_FUNCTION__, fileNameDestination);
#endif
    return -1;
  }
  if (directoryDelEntry(&sourceDir, fileNameSource) != 0) {
    free(pathDestinationDirectory);
    free(pathSourceDirectory);
#if !defined(NDEBUG)
    fprintf(stderr, "Function: %s: directoryDelEntry(%s) failure\n", __PRETTY_FUNCTION__, fileNameSource);
#endif
    return -1;
  }
  if (G_ISDIR(fileEntry.iNodeStat.st_mode) == TRUE) {
    if (changeParentDirectory(&fileEntry, &destinationDir, &sourceDir) != 0) {
      free(pathDestinationDirectory);
      free(pathSourceDirectory);
#if !defined(NDEBUG)
      fprintf(stderr, "Function: %s: changeParentDirectory(%s) Failure\n", __PRETTY_FUNCTION__, fileNameDestination);
#endif
      return (-1);
    }
  }
  free(pathDestinationDirectory);
  free(pathSourceDirectory);
  return 0;
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
