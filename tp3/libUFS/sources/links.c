#include	<stdio.h>
#include	<string.h>
#include	"disque.h"
#include	"path.h"
#include	"utils.h"
#include	"inode_tools.h"
#include	"links.h"

static int	isHardlinkValid(const char *pathSource, const char *pathDest, iNodeEntry *pathExistantINodeEntry) {
  iNodeEntry	pathNewHardlinkINodeEntry;

  if (resolvePath(pathSource, pathExistantINodeEntry) != 0) {
#if !defined(NDEBUG)
    fprintf(stderr, "Function: %s: resolvePath(%s) failure\n", __PRETTY_FUNCTION__, pathSource);
#endif
    return (-1);
  }
  if (G_ISDIR(pathExistantINodeEntry->iNodeStat.st_mode)) {
    return (-3);
  }
  if (resolvePath(pathDest, &pathNewHardlinkINodeEntry) == 0) {
    return (-2);
  }
  return (0);
}

int		addHardlink(const char *pathSource, const char *pathDest) {
  iNodeEntry	pathExistantINodeEntry;
  char		destBaseName[FILENAME_SIZE];
  char		destBaseDir[MAX_DIR_PATH_SIZE];
  iNodeEntry	destParentDirectoryEntry;
  int		preconditionStatus;

  if ((preconditionStatus = isHardlinkValid(pathSource, pathDest, &pathExistantINodeEntry))) {
    return (preconditionStatus);
  }
  if (splitFilenameAndPath(pathDest, destBaseName, destBaseDir) == -1) {
#if !defined(NDEBUG)
    fprintf(stderr, "Function: %s: splitFilenameAndPath(%s) failure\n", __PRETTY_FUNCTION__, pathDest);
#endif
    return (-1);
  }
  if (resolvePath(destBaseDir, &destParentDirectoryEntry) != 0) {
#if !defined(NDEBUG)
    fprintf(stderr, "Function: %s: resolvePath(%s) failure\n", __PRETTY_FUNCTION__, destBaseDir);
#endif
    return (-1);
  }
  if (directoryAddEntry(&destParentDirectoryEntry, &pathExistantINodeEntry, destBaseName) == -1) {
#if !defined(NDEBUG)
    fprintf(stderr, "Function: %s: directoryAddEntry(%d, %d, %s) failure\n", __PRETTY_FUNCTION__,
	    destParentDirectoryEntry.iNodeStat.st_ino, pathExistantINodeEntry.iNodeStat.st_ino, destBaseName);
#endif
    return (-1);
  }
  return (0);
}

static int	deleteHardlinkRemoveFile(const iNodeEntry *pathExistantINodeEntry) {
  int		i = 0;
  bool_t	blockBitmap[BLOCK_SIZE];

  if (ReadBlock(FREE_BLOCK_BITMAP, (char *)blockBitmap) == -1) {
#if !defined(NDEBUG)
    fprintf(stderr, "Function: %s: ReadBlock(%d) Failure\n", __PRETTY_FUNCTION__, FREE_BLOCK_BITMAP);
#endif
    return (-1);
  }

  while (i < pathExistantINodeEntry->iNodeStat.st_blocks) {
    blockBitmap[pathExistantINodeEntry->Block[i]] = BLOCK_FREE;
    ++i;
  }

  if (WriteBlock(FREE_BLOCK_BITMAP, (char *)blockBitmap) == -1) {
#if !defined(NDEBUG)
    fprintf(stderr, "Function: %s: ReadBlock(%d) Failure\n", __PRETTY_FUNCTION__, FREE_BLOCK_BITMAP);
#endif
    return (-1);
  }
  if (clearINode(pathExistantINodeEntry->iNodeStat.st_ino) != 0) {
#if !defined(NDEBUG)
    fprintf(stderr, "Function: %s: clearINode(%d) failure\n", __PRETTY_FUNCTION__, 
	    pathExistantINodeEntry->iNodeStat.st_ino);
#endif          
    return (-1);
  }
  return (0);
}

int		deleteHardlink(const char *filename) {
  iNodeEntry	pathExistantINodeEntry;
  iNodeEntry	parentDirectoryEntry;
  char		basename[FILENAME_SIZE];
  char		basedir[MAX_DIR_PATH_SIZE];
  
  if (resolvePath(filename, &pathExistantINodeEntry) != 0) {
#if !defined(NDEBUG)
    fprintf(stderr, "Function: %s: resolvePath(%s) failure\n", __PRETTY_FUNCTION__, filename);
#endif
    return (-1);
  }
  if (!G_ISREG(pathExistantINodeEntry.iNodeStat.st_mode)) {
#if !defined(NDEBUG)
    fprintf(stderr, "Function: %s: %s is not a regular file\n", __PRETTY_FUNCTION__, filename);
#endif
    return (-2);
  }
  if (splitFilenameAndPath(filename, basename, basedir) == -1) {
#if !defined(NDEBUG)
    fprintf(stderr, "Function: %s: splitFilenameAndPath(%s) failure\n", __PRETTY_FUNCTION__, filename);
#endif
    return (-1);
  }
  if (resolvePath(basedir, &parentDirectoryEntry) != 0) {
#if !defined(NDEBUG)
    fprintf(stderr, "Function: %s: resolvePath(%s) failure\n", __PRETTY_FUNCTION__, basedir);
#endif
    return (-1);
  }
  if (directoryDelEntry(&parentDirectoryEntry, basename) == -1) {
#if !defined(NDEBUG)
    fprintf(stderr, "Function: %s: directoryDelEntry(%s) failure\n", __PRETTY_FUNCTION__, filename);
#endif    
    return (-1);
  }
  pathExistantINodeEntry.iNodeStat.st_nlink--;
  if (pathExistantINodeEntry.iNodeStat.st_nlink == 0) {
    return (deleteHardlinkRemoveFile(&pathExistantINodeEntry));
  }
  else if (saveINodeEntry(&pathExistantINodeEntry) != 0) {
#if !defined(NDEBUG)
    fprintf(stderr, "Function: %s: saveINodeEntry(%d) failure\n", __PRETTY_FUNCTION__, 
	    pathExistantINodeEntry.iNodeStat.st_ino);
#endif          
    return (-1);
  }
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

static int		compressionSave(iNodeEntry *directoryINodeEntry,
					DirEntry blockEntries[NUM_DIR_ENTRY_PER_BLOCK]) {
  directoryINodeEntry->iNodeStat.st_size -= sizeof(DirEntry);
  if ((directoryINodeEntry->iNodeStat.st_size % BLOCK_SIZE) == 0) {
    const ino	blockToClear = directoryINodeEntry->Block[directoryINodeEntry->iNodeStat.st_blocks - 1];

    if (clearBlock(blockToClear, directoryINodeEntry) == -1) {
#if !defined(NDEBUG)
      fprintf(stderr, "Function: %s: clearBlock(%d) Failure\n", __PRETTY_FUNCTION__, blockToClear);
#endif
      return (-1);
    }
    directoryINodeEntry->iNodeStat.st_blocks--;
  }
  else {
    const ino	blockToSave = directoryINodeEntry->Block[directoryINodeEntry->iNodeStat.st_blocks - 1];

    if (WriteBlock(blockToSave, (char *)(blockEntries)) == -1) {
#if !defined(NDEBUG)
      fprintf(stderr, "Function: %s: WriteBlock(%d) Failure\n", __PRETTY_FUNCTION__, blockToSave);
#endif
      return (-1);
    }
  }
  if (saveINodeEntry(directoryINodeEntry) == -1) {
#if !defined(NDEBUG)
    fprintf(stderr, "Function: %s: saveINodeEntry(%d) failure\n", __PRETTY_FUNCTION__, 
	    directoryINodeEntry->iNodeStat.st_ino);
#endif
    return (-1);
  }
  return (0);
}

static int		compressionDirEntries(iNodeEntry *directoryINodeEntry,
					      DirEntry blockEntries[NUM_DIR_ENTRY_PER_BLOCK],
					      const int beginEntryPosition) {
  const UINT16		numDirectoryEntries = numberOfDirEntry(directoryINodeEntry->iNodeStat.st_size) - 1;
  const DirEntry	*nextEntry;
  DirEntry		*currentEntry;
  int			i = beginEntryPosition;

  while (i < numDirectoryEntries) {
    if ((i + 1) % NUM_DIR_ENTRY_PER_BLOCK == 0) {

      const UINT16	currentBlock = (i + 1) / NUM_DIR_ENTRY_PER_BLOCK;
      DirEntry		newBlockEntries[NUM_DIR_ENTRY_PER_BLOCK];

      if (ReadBlock(directoryINodeEntry->Block[currentBlock], (char *)(newBlockEntries)) == -1) {
#if !defined(NDEBUG)
	fprintf(stderr, "Function: %s: ReadBlock(%d) Failure\n", __PRETTY_FUNCTION__, currentBlock);
#endif
	return (-1);
      }
      nextEntry = blockEntries + ((i + 1) % NUM_DIR_ENTRY_PER_BLOCK);
      currentEntry = blockEntries + (i % NUM_DIR_ENTRY_PER_BLOCK);
      memcpy(currentEntry, nextEntry, sizeof(DirEntry));
      if (WriteBlock(directoryINodeEntry->Block[currentBlock - 1], (char *)(blockEntries)) == -1) {
#if !defined(NDEBUG)
	fprintf(stderr, "Function: %s: WriteBlock(%d) Failure\n", __PRETTY_FUNCTION__, currentBlock - 1);
#endif
	return (-1);
      }
    }
    else {
      nextEntry = blockEntries + ((i + 1) % NUM_DIR_ENTRY_PER_BLOCK);
      currentEntry = blockEntries + (i % NUM_DIR_ENTRY_PER_BLOCK);
      memcpy(currentEntry, nextEntry, sizeof(DirEntry));
    }
    ++i;
  }
  return (compressionSave(directoryINodeEntry, blockEntries));
}

int	directoryDelEntry(iNodeEntry *parentDirectory,
			  const char *deletedEntryName) {
  const UINT16	numDirectoryEntries = numberOfDirEntry(parentDirectory->iNodeStat.st_size);
  DirEntry	blockEntries[NUM_DIR_ENTRY_PER_BLOCK];
  int		i = 0;
  
  while (i < numDirectoryEntries) {
    if (i % NUM_DIR_ENTRY_PER_BLOCK == 0) {

      const UINT16	currentBlock = i / NUM_DIR_ENTRY_PER_BLOCK;

      if (ReadBlock(parentDirectory->Block[currentBlock], (char *)(blockEntries)) == -1) {
#if !defined(NDEBUG)
	fprintf(stderr, "Function: %s: ReadBlock(%d) Failure\n", __PRETTY_FUNCTION__, currentBlock);
#endif
	return (-1);
      }
    }

    const DirEntry	*currentEntry = blockEntries + (i % NUM_DIR_ENTRY_PER_BLOCK);

    if (strcmp(currentEntry->Filename, deletedEntryName) == 0) {
      return (compressionDirEntries(parentDirectory, blockEntries, i));
    }
    ++i;
  }
  return (-1);
}
