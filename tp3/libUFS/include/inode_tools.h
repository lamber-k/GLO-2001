#ifndef	INODE_TOOLS_H
# define INODE_TOOLS_H

# include	"UFS.h"

inline bool_t isINodeFree(const bool_t freeINodeBitmap[N_INODE_ON_DISK], const UINT16 inodeNum);

// Reserve an iNode on disk (call WriteBlock)
int reserveINode(iNodeEntry *reservedINode);

// Clear an iNode on disk (call WriteBlock)
int clearINode(const ino iNodeToClear);

// Save @toSave entry (call WriteBlock)
int saveINodeEntry(const iNodeEntry *toSave);

// Get the i-Node @iNodeNum informations on @entry
int getINodeEntry(const UINT16 iNodeNum, iNodeEntry *entry);

#endif // !INODE_TOOLS_H
