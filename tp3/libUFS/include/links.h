#ifndef		LINKS_H
# define	LINKS_H

# include	"UFS.h"

int	addHardlink(const char *pathSrc, const char *pathDest);

int	deleteHardlink(const char *filename);

int	directoryNewEntryPreconditions(const char *pFilename, 
				       char basename[FILENAME_SIZE], 
				       char dirname[MAX_DIR_PATH_SIZE],
				       iNodeEntry *parentDirectoryEntry);


int	directoryAddEntry(iNodeEntry *parentDirectory, 
			  iNodeEntry *newEntry,
			  const char *newEntryName);

int	directoryDelEntry(iNodeEntry *parentDirectory,
			  const char *deletedEntryName);
int	changeParentDirectory(iNodeEntry *dirEntry, iNodeEntry *parentDestinationDir, iNodeEntry *parentSourceDir);

#endif // !LINKS_H
