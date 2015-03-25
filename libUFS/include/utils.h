#ifndef UTILS_H_
#define UTILS_H_

#include <stdlib.h>
#include "UFS.h"

// Test if another block(s) are needed for @spaceNumber octets in @currentSize
inline BOOL moreBlockNeeded(const UINT16 currentSize, const UINT16 spaceNeeded);

// Test if the block @blockNum (relative to @freeBlockBitmap) is Free
inline BOOL isBlockFree(const BOOL freeBlockBitmap[N_BLOCK_ON_DISK], const UINT16 blockNum);

// Test if the entry in @currentINodeEntry folder named @entryName exist.
BOOL	entryExist(const iNodeEntry *currentINodeEntry, const char *entryName);

// Get the i-Node @iNodeNum informations on @entry
int getINodeEntry(const UINT16 iNodeNum, iNodeEntry *entry);

// Find the Directory Entry on @currentINodeEntry who his name is entryName. Return info on @dirEntryFound
int findDirEntryByName(const iNodeEntry *currentINodeEntry, const char *entryName, DirEntry *dirEntryFound);

// Resolve the current section: searching @pathSection entry on @currentINodeEntry. Return info on @currentDirEntry
int resolveSection(const char *pathSection, iNodeEntry *currentINodeEntry, DirEntry *currentDirEntry);

// Resolve the entire path @path. Return info about it on @entryFound
int resolvePath(char *path, iNodeEntry *entryFound);

int directoryNewEntryPreconditions(const char *pFilename, 
					  char basename[FILENAME_SIZE], 
					  char dirname[MAX_DIR_PATH_SIZE],
					  iNodeEntry *parentDirectoryEntry);

// Reserve a Block on disk (call WriteBlock)
int reserveBlock(iNodeEntry *reservedINode);

// Clear a Block on disk (call WriteBlock)
int clearBlock(const ino iNodeToClear);

// Save @toSave entry (call WriteBlock)
int saveINodeEntry(const iNodeEntry *toSave);

int	directoryAddEntry(iNodeEntry *parentDirectory, 
				  iNodeEntry *newEntry,
				  const char *newEntryName);

/* Function base */
UINT16 numberOfDirEntry(UINT16 size);

/* Cette fonction va extraire le repertoire d'une chemin d'acces complet, et le copier
   dans pDir.  Par exemple, si le chemin fourni pPath="/doc/tmp/a.txt", cette fonction va
   copier dans pDir le string "/doc/tmp" . Si le chemin fourni est pPath="/a.txt", la fonction
   va retourner pDir="/". Si le string fourni est pPath="/", cette fonction va retourner pDir="/".
   Cette fonction est calquée sur dirname(), que je ne conseille pas d'utiliser car elle fait appel
   à des variables statiques/modifie le string entrant. Voir plus bas pour un exemple d'utilisation. */
int GetDirFromPath(const char *pPath, char *pDir);

/* Cette fonction va extraire le nom de fichier d'une chemin d'acces complet.
   Par exemple, si le chemin fourni pPath="/doc/tmp/a.txt", cette fonction va
   copier dans pFilename le string "a.txt" . La fonction retourne 1 si elle
   a trouvée le nom de fichier avec succes, et 0 autrement. Voir plus bas pour
   un exemple d'utilisation. */   
int GetFilenameFromPath(const char *pPath, char *pFilename);

/* Cette fonction sert à afficher à l'écran le contenu d'une structure d'i-node */
void printiNode(iNodeEntry iNode);

#endif