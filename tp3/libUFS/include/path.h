#ifndef	PATH_H
# define PATH_H

# include "UFS.h"

// Find the Directory Entry on @currentINodeEntry who his name is entryName. Return info on @dirEntryFound
int findDirEntryByName(const iNodeEntry *currentINodeEntry, const char *entryName, DirEntry *dirEntryFound);

// Resolve the current section: searching @pathSection entry on @currentINodeEntry. Return info on @currentDirEntry
int resolveSection(const char *pathSection, iNodeEntry *currentINodeEntry, DirEntry *currentDirEntry);

// Resolve the entire path @path. Return info about it on @entryFound
int resolvePath(const char *path, iNodeEntry *entryFound);

int splitFilenameAndPath(const char *path, char basename[FILENAME_SIZE], char dirname[MAX_DIR_PATH_SIZE]);

int GetFilenameFromPath(const char *pPath, char *pFilename);

int GetDirFromPath(const char *pPath, char *pDir);

#endif // !PATH_H
