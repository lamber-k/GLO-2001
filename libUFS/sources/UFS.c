#include <assert.h>
#include <stdlib.h>
#include <stdio.h>
#include <string.h>
#include "UFS.h"
#include "disque.h"

// Test if another block(s) are needed for @spaceNumber octets in @currentSize
static inline BOOL moreBlockNeeded(const UINT16 currentSize, const UINT16 spaceNeeded);

// Test if the block @blockNum (relative to @freeBlockBitmap) is Free
static inline BOOL isBlockFree(const BOOL freeBlockBitmap[N_BLOCK_ON_DISK], const UINT16 blockNum);

// Test if the entry in @currentINodeEntry folder named @entryName exist.
static BOOL	entryExist(const iNodeEntry *currentINodeEntry, const char *entryName);

// Get the i-Node @iNodeNum informations on @entry
static int getINodeEntry(const UINT16 iNodeNum, iNodeEntry *entry);

// Find the Directory Entry on @currentINodeEntry who his name is entryName. Return info on @dirEntryFound
static int findDirEntryByName(const iNodeEntry *currentINodeEntry, const char *entryName, DirEntry *dirEntryFound);

// Resolve the current section: searching @pathSection entry on @currentINodeEntry. Return info on @currentDirEntry
static int resolveSection(const char *pathSection, iNodeEntry *currentINodeEntry, DirEntry *currentDirEntry);

// Resolve the entire path @path. Return info about it on @entryFound
static int resolvePath(char *path, iNodeEntry *entryFound);

static int directoryNewEntryPreconditions(const char *pFilename, 
					  char basename[FILENAME_SIZE], 
					  char dirname[MAX_DIR_PATH_SIZE],
					  iNodeEntry *parentDirectoryEntry);

// Reserve a Block on disk (call WriteBlock)
static int reserveBlock(iNodeEntry *reservedINode);

// Clear a Block on disk (call WriteBlock)
static int clearBlock(const ino iNodeToClear);

// Save @toSave entry (call WriteBlock)
static int saveINodeEntry(const iNodeEntry *toSave);

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

/* Cette fonction va extraire le repertoire d'une chemin d'acces complet, et le copier
   dans pDir.  Par exemple, si le chemin fourni pPath="/doc/tmp/a.txt", cette fonction va
   copier dans pDir le string "/doc/tmp" . Si le chemin fourni est pPath="/a.txt", la fonction
   va retourner pDir="/". Si le string fourni est pPath="/", cette fonction va retourner pDir="/".
   Cette fonction est calquée sur dirname(), que je ne conseille pas d'utiliser car elle fait appel
   à des variables statiques/modifie le string entrant. Voir plus bas pour un exemple d'utilisation. */
int GetDirFromPath(const char *pPath, char *pDir) {
  strcpy(pDir,pPath);
  int len = strlen(pDir); // length, EXCLUDING null
	
  // On va a reculons, de la fin au debut
  while (pDir[len]!='/') {
    len--;
    if (len <0) {
      // Il n'y avait pas de slash dans le pathname
      return 0;
    }
  }
  if (len==0) { 
    // Le fichier se trouve dans le root!
    pDir[0] = '/';
    pDir[1] = 0;
  }
  else {
    // On remplace le slash par une fin de chaine de caractere
    pDir[len] = '\0';
  }
  return 1;
}

/* Cette fonction va extraire le nom de fichier d'une chemin d'acces complet.
   Par exemple, si le chemin fourni pPath="/doc/tmp/a.txt", cette fonction va
   copier dans pFilename le string "a.txt" . La fonction retourne 1 si elle
   a trouvée le nom de fichier avec succes, et 0 autrement. Voir plus bas pour
   un exemple d'utilisation. */   
int GetFilenameFromPath(const char *pPath, char *pFilename) {
  // Pour extraire le nom de fichier d'un path complet
  char *pStrippedFilename = strrchr(pPath,'/');
  if (pStrippedFilename!=NULL) {
    ++pStrippedFilename; // On avance pour passer le slash
    if ((*pStrippedFilename) != '\0') {
      // On copie le nom de fichier trouve
      strcpy(pFilename, pStrippedFilename);
      return 1;
    }
  }
  return 0;
}

/* Un exemple d'utilisation des deux fonctions ci-dessus :
   int bd_create(const char *pFilename) {
   char StringDir[256];
   char StringFilename[256];
   if (GetDirFromPath(pFilename, StringDir)==0) return 0;
   GetFilenameFromPath(pFilename, StringFilename);
   ...
*/


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


/* ----------------------------------------------------------------------------------------
   à vous de jouer, maintenant!
   ---------------------------------------------------------------------------------------- */

static inline BOOL isBlockFree(const BOOL freeBlockBitmap[N_BLOCK_ON_DISK], const UINT16 blockNum) {
#if !defined(NDEBUG)
  assert(blockNum > 0 && blockNum < N_BLOCK_ON_DISK);
#endif
  return (freeBlockBitmap[blockNum] == BLOCK_FREE);
}

static inline BOOL isINodeFree(const BOOL freeINodeBitmap[N_INODE_ON_DISK], const UINT16 blockNum) {
#if !defined(NDEBUG)
  assert(blockNum > 0 && blockNum < N_INODE_ON_DISK);
#endif
  return (freeINodeBitmap[blockNum] == INODE_FREE);
}

static inline BOOL moreBlockNeeded(const UINT16 currentSize, const UINT16 spaceNeeded) {
  return ((currentSize % BLOCK_SIZE) + spaceNeeded > BLOCK_SIZE);
}

static int	reserveBlock(iNodeEntry *reservedBlock) {
  BOOL		iNodeBitmap[BLOCK_SIZE];
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
      if (reservedBlock->iNodeStat.st_blocks == 0)
	reservedBlock->iNodeStat.st_ino = currentBlock;
      reservedBlock->Block[reservedBlock->iNodeStat.st_blocks] = currentBlock;
      reservedBlock->iNodeStat.st_blocks++;
      return (0);
    }
    ++currentBlock;
  }
  return (-1);
}

static int clearBlock(const ino blockToClear) {
  BOOL		blockBitmap[BLOCK_SIZE];

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
  if (WriteBlock(FREE_INODE_BITMAP, (const char *)(blockBitmap)) == -1) {
#if !defined(NDEBUG)
    fprintf(stderr, "Function: %s: WriteBlock Failure\n", __PRETTY_FUNCTION__);
#endif
    return (-1);
  }
  return (0);
}

static int	saveINodeEntry(const iNodeEntry *toSave) {
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


static int getINodeEntry(const UINT16 iNodeNum, iNodeEntry *entry) {
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

static int	findDirEntryByName(const iNodeEntry *currentINodeEntry, const char *entryName, DirEntry *dirEntryFound) {
  const UINT16	numDirectoryEntries = currentINodeEntry->iNodeStat.st_size / sizeof(DirEntry);
  DirEntry	blockEntries[NUM_DIR_ENTRY_PER_BLOCK];
  int		i = 0;
  
  if (!G_ISDIR(currentINodeEntry->iNodeStat.st_mode)) {
#if !defined(NDEBUG)
    fprintf(stderr, "Function: %s: %d iNode is not a directory\n", __PRETTY_FUNCTION__, 
	    currentINodeEntry->iNodeStat.st_ino);
#endif
    return (-1);
  }
  while (i < numDirectoryEntries) {
    if (i % NUM_DIR_ENTRY_PER_BLOCK == 0) {

      const UINT16	currentBlock = i / NUM_DIR_ENTRY_PER_BLOCK;

      if (ReadBlock(currentINodeEntry->Block[currentBlock], (char *)(blockEntries)) == -1) {
#if !defined(NDEBUG)
	fprintf(stderr, "Function: %s: ReadBlock(%d) Failure\n", __PRETTY_FUNCTION__, currentBlock);
#endif
	return (-1);
      }
    }

    const DirEntry	*currentEntry = blockEntries + (i % NUM_DIR_ENTRY_PER_BLOCK);

    if (strcmp(currentEntry->Filename, entryName) == 0) {
      memcpy(dirEntryFound, currentEntry, sizeof(*dirEntryFound));
      return (0);
    }
    ++i;
  }
  return (-1);
}

static BOOL	entryExist(const iNodeEntry *currentINodeEntry, const char *entryName) {
  DirEntry	entryFound;

  return (findDirEntryByName(currentINodeEntry, entryName, &entryFound) == 0 ? TRUE : FALSE);
}

static int	resolveSection(const char *pathSection, iNodeEntry *currentINodeEntry, DirEntry *currentDirEntry) {
  if (pathSection != NULL) {
    if (findDirEntryByName(currentINodeEntry, pathSection, currentDirEntry) == -1) {
#if !defined(NDEBUG)
      fprintf(stderr, "Function: %s: findDirEntry(%d, %s) Failure. Entry don't exist\n", __PRETTY_FUNCTION__, 
	      currentINodeEntry->iNodeStat.st_ino, pathSection);
#endif
      return (-1);
    }

    if (getINodeEntry(currentDirEntry->iNode, currentINodeEntry) == -1) {
#if !defined(NDEBUG)
      fprintf(stderr, "Function: %s: getINodeEntry(%d) Failure\n", __PRETTY_FUNCTION__, currentDirEntry->iNode);
#endif
      return (-1);
    }
  }
  return (0);
}

static int	resolvePath(char *path, iNodeEntry *entryFound) {
  iNodeEntry	currentINodeEntry;
  DirEntry	currentDirEntry;
  char		*saveptrPFilename;
  char		*pathSection;

  if (getINodeEntry(ROOT_INODE, &currentINodeEntry) == -1) {
#if !defined(NDEBUG)
    fprintf(stderr, "Function: %s: getINodeEntry(%d) Failure\n", __PRETTY_FUNCTION__, ROOT_INODE);
#endif
    return (-1);
  }

  pathSection = strtok_r(path, PATH_DELIMITER, &saveptrPFilename);
  do {
    printf("part: %s\n", pathSection);
    if (resolveSection(pathSection, &currentINodeEntry, &currentDirEntry) == -1) {
#if !defined(NDEBUG)
      fprintf(stderr, "Function: %s: resolveSection(%d, %s) Failure. Entry don't exist\n", __PRETTY_FUNCTION__, ROOT_INODE, pathSection);
#endif
      return (-1);    
    }
  }
  while ((pathSection = strtok_r(NULL, PATH_DELIMITER, &saveptrPFilename)) != NULL);

  if (currentINodeEntry.iNodeStat.st_ino == ROOT_INODE) {
    memcpy(entryFound, &currentINodeEntry, sizeof(*entryFound));
  }
  else if (getINodeEntry(currentDirEntry.iNode, entryFound) == -1) {
#if !defined(NDEBUG)
    fprintf(stderr, "Function: %s: getINodeEntry(%d) Failure\n", __PRETTY_FUNCTION__, ROOT_INODE);
#endif
    return (-1);
  }
  return (0);
}

static int	directoryNewEntryPreconditions(const char *pFilename, 
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

static int	directoryAddEntrySelectBlock(iNodeEntry *parentDirectory,
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

static int	directoryAddEntry(iNodeEntry *parentDirectory, 
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

int	bd_countfreeblocks(void) {
  BOOL	freeBlockBitmap[N_BLOCK_ON_DISK];
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
  }
  return freeBlockNumber;
}

int		bd_stat(const char *pFilename, gstat *pStat) {
  iNodeEntry	entryFound;
  char		*path = strdup(pFilename);

  if (resolvePath(path, &entryFound) == -1) {
#if !defined(NDEBUG)
    fprintf(stderr, "Function: %s: resolvePath(%s) Failure\n", __PRETTY_FUNCTION__, pFilename);
#endif
    free(path);
    return (-1);
  }
  memcpy(pStat, &entryFound.iNodeStat, sizeof(entryFound.iNodeStat));
  free(path);
  return (0);
}

int		bd_create(const char *pFilename) {
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
  if (reserveBlock(&newFileINodeEntry) == -1) {
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
    clearBlock(newFileINodeEntry.iNodeStat.st_ino);
    return (-1);
  }
  if (directoryAddEntry(&parentDirectoryEntry, &newFileINodeEntry, basename) == -1)
  {
#if !defined(NDEBUG)
    fprintf(stderr, "Function: %s: directoryAddEntry(%d, %d) failure\n", __PRETTY_FUNCTION__, 
	    parentDirectoryEntry.iNodeStat.st_ino, newFileINodeEntry.iNodeStat.st_ino);
#endif
    clearBlock(newFileINodeEntry.iNodeStat.st_ino);
    return (-1);
  }
  return (0);
}

int bd_read(const char *pFilename, char *buffer, int offset, int numbytes) {
  return -1;
}

int bd_write(const char *pFilename, const char *buffer, int offset, int numbytes) { 
  return -1;
}

int		bd_mkdir(const char *pDirName) {
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
  if (reserveBlock(&newDirINodeEntry) == -1) {
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
    clearBlock(newDirINodeEntry.iNodeStat.st_ino);
    return (-1);
  }
  if (directoryAddEntry(&parentDirectoryEntry, &newDirINodeEntry, basename) == -1)
  {
#if !defined(NDEBUG)
    fprintf(stderr, "Function: %s: directoryAddEntry(%d, %d) failure\n", __PRETTY_FUNCTION__, 
	    parentDirectoryEntry.iNodeStat.st_ino, newDirINodeEntry.iNodeStat.st_ino);
#endif
    clearBlock(newDirINodeEntry.iNodeStat.st_ino);
    return (-1);
  }
  if (directoryAddEntry(&newDirINodeEntry, &newDirINodeEntry, ".") == -1)
  {
#if !defined(NDEBUG)
    fprintf(stderr, "Function: %s: directoryAddEntry(%d, %d) failure\n", __PRETTY_FUNCTION__, 
	    newDirINodeEntry.iNodeStat.st_ino, newDirINodeEntry.iNodeStat.st_ino);
#endif
    clearBlock(newDirINodeEntry.iNodeStat.st_ino);
    return (-1);
  }
  if (directoryAddEntry(&newDirINodeEntry, &parentDirectoryEntry, "..") == -1)
  {
#if !defined(NDEBUG)
    fprintf(stderr, "Function: %s: directoryAddEntry(%d, %d) failure\n", __PRETTY_FUNCTION__, 
	    newDirINodeEntry.iNodeStat.st_ino, parentDirectoryEntry.iNodeStat.st_ino);
#endif
    clearBlock(newDirINodeEntry.iNodeStat.st_ino);
    return (-1);
  }
  return (0);
}
	
int bd_hardlink(const char *pPathExistant, const char *pPathNouveauLien) {
  return -1;
}

int bd_unlink(const char *pFilename) {
  return -1;
}

int bd_rmdir(const char *pFilename) {
  return -1;
}

int bd_rename(const char *pFilename, const char *pDestFilename) {
  return -1;
}

int bd_readdir(const char *pDirLocation, DirEntry **ppListeFichiers) {
  iNodeEntry	directoryINodeEntry;
  UINT16	numEntries = 0;
  UINT16	currentBlock = 0;
  char		*path = strdup(pDirLocation);

  if (resolvePath(path, &directoryINodeEntry) == -1) {
#if !defined(NDEBUG)
    fprintf(stderr, "Function: %s: resolvePath(%s) failure\n", __PRETTY_FUNCTION__, pDirLocation);
#endif
    free(path);
    return (-1);
  }
  free(path);
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

