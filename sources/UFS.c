#include <assert.h>
#include <stdlib.h>
#include <stdio.h>
#include <string.h>
#include "UFS.h"
#include "disque.h"

// Quelques fonctions qui pourraient vous être utiles
int NumberofDirEntry(int Size) {
  return Size/sizeof(DirEntry);
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
  int index;
	
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
  return (freeBlockBitmap[blockNum] == TRUE);
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

int bd_create(const char *pFilename) {
  return -1;
}

int bd_read(const char *pFilename, char *buffer, int offset, int numbytes) {
  return -1;
}

int bd_write(const char *pFilename, const char *buffer, int offset, int numbytes) { 
  return -1;
}

int bd_mkdir(const char *pDirName) {
  return -1;
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
  return -1;
}

