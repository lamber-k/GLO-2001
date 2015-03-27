#include	<stdio.h>
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
