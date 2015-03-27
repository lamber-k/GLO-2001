#include	<stdio.h>
#include	<stdlib.h>
#include	<unistd.h>
#include	<string.h>
#include	"disque.h"
#include	"path.h"
#include	"inode_tools.h"

int	findDirEntryByName(const iNodeEntry *currentINodeEntry, const char *entryName, DirEntry *dirEntryFound) {
  const UINT16	numDirectoryEntries = currentINodeEntry->iNodeStat.st_size / sizeof(DirEntry);
  DirEntry	blockEntries[NUM_DIR_ENTRY_PER_BLOCK];
  int		i = 0;
  
  if (!G_ISDIR(currentINodeEntry->iNodeStat.st_mode)) {
#if !defined(NDEBUG)
    fprintf(stderr, "Function: %s: %d iNode is not a directory\n", __PRETTY_FUNCTION__, 
	    currentINodeEntry->iNodeStat.st_ino);
#endif
    return (-2);
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

bool_t	entryExist(const iNodeEntry *currentINodeEntry, const char *entryName) {
  DirEntry	entryFound;

  return (findDirEntryByName(currentINodeEntry, entryName, &entryFound) == 0 ? TRUE : FALSE);
}

int	resolveSection(const char *pathSection, iNodeEntry *currentINodeEntry, DirEntry *currentDirEntry) {
  int   findDirEntryByNameResult;

  if (pathSection != NULL) {
    if ((findDirEntryByNameResult = findDirEntryByName(currentINodeEntry, pathSection, currentDirEntry)) != 0) {
#if !defined(NDEBUG)
      fprintf(stderr, "Function: %s: findDirEntry(%d, %s): %d Failure. Entry don't exist\n", __PRETTY_FUNCTION__, 
	      currentINodeEntry->iNodeStat.st_ino, pathSection, findDirEntryByNameResult);
#endif
      return (findDirEntryByNameResult);
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

int		resolvePath(const char *path, iNodeEntry *entryFound) {
  iNodeEntry	currentINodeEntry;
  DirEntry	currentDirEntry;
  char		*saveptrPFilename;
  char		*pathSection;
  char		*pathDup = strdup(path);
  int		resolveSectionReturn;

  if (getINodeEntry(ROOT_INODE, &currentINodeEntry) == -1) {
#if !defined(NDEBUG)
    fprintf(stderr, "Function: %s: getINodeEntry(%d) Failure\n", __PRETTY_FUNCTION__, ROOT_INODE);
#endif
    free(pathDup);
    return (-1);
  }

  pathSection = strtok_r(pathDup, PATH_DELIMITER, &saveptrPFilename);
  do {
    printf("part: %s\n", pathSection);
    if ((resolveSectionReturn = resolveSection(pathSection, &currentINodeEntry, &currentDirEntry)) != 0) {
#if !defined(NDEBUG)
      fprintf(stderr, "Function: %s: resolveSection(%d, %s) Failure. Entry don't exist\n", __PRETTY_FUNCTION__, ROOT_INODE, pathSection);
#endif
      free(pathDup);
      return (resolveSectionReturn);
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
    free(pathDup);
    return (-1);
  }
  free(pathDup);
  return (0);
}

int splitFilenameAndPath(const char *path, char basename[FILENAME_SIZE], char dirname[MAX_DIR_PATH_SIZE]) {
  if (GetDirFromPath(path, dirname) == 0 || GetFilenameFromPath(path, basename) == 0) {
#if !defined(NDEBUG)
    fprintf(stderr, "Function: %s Failure with path: %s\n", __PRETTY_FUNCTION__, path);
#endif
    return (-1);
  }
  return (0);
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

