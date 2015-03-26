#ifndef UTILS_H_
#define UTILS_H_

#include <stdlib.h>
#include "UFS.h"

// Test if another block(s) are needed for @spaceNumber octets in @currentSize
inline bool_t moreBlockNeeded(const UINT16 currentSize, const UINT16 spaceNeeded);

// Test if the block @blockNum (relative to @freeBlockBitmap) is Free
inline bool_t isBlockFree(const bool_t freeBlockBitmap[N_BLOCK_ON_DISK], const UINT16 blockNum);

// Test if the entry in @currentINodeEntry folder named @entryName exist.
bool_t	entryExist(const iNodeEntry *currentINodeEntry, const char *entryName);

// Get the i-Node @iNodeNum informations on @entry
int getINodeEntry(const UINT16 iNodeNum, iNodeEntry *entry);


int directoryNewEntryPreconditions(const char *pFilename, 
					  char basename[FILENAME_SIZE], 
					  char dirname[MAX_DIR_PATH_SIZE],
					  iNodeEntry *parentDirectoryEntry);

// Reserve a Block on disk (call WriteBlock)
int reserveBlock(iNodeEntry *reservedINodeEntry);

// Clear a Block on disk (call WriteBlock)
int clearBlock(const ino blockToClear, iNodeEntry *INodeEntry);

// Reserve an iNode on disk (call WriteBlock)
int reserveINode(iNodeEntry *reservedINode);

// Clear an iNode on disk (call WriteBlock)
int clearINode(const ino iNodeToClear);

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
