#include	<stdio.h>
#include	"block_tools.h"
#include	"disque.h"

int			addBlock(iNodeEntry *fileEntry)
{
  bool_t	blockBitmap[BLOCK_SIZE];
  UINT16	current = BASE_BLOCK;

  if (fileEntry->iNodeStat.st_blocks == N_BLOCK_PER_INODE) {
#if !defined(NDEBUG)
    fprintf(stderr, "Function: %s: write is beyond the capacity of the file system", __PRETTY_FUNCTION__);
#endif
    return -4;
  }
  if (ReadBlock(FREE_BLOCK_BITMAP, (char *)blockBitmap) == -1) {
#if !defined(NDEBUG)
    fprintf(stderr, "Function: %s: ReadBlock(%d) Failure\n", __PRETTY_FUNCTION__, FREE_BLOCK_BITMAP);
#endif
    return -1;
  }
  while (current < N_BLOCK_ON_DISK) {
    if (blockBitmap[current] == BLOCK_FREE) {
      blockBitmap[current] = BLOCK_NOT_FREE;
      if (WriteBlock(FREE_BLOCK_BITMAP, (char *)blockBitmap) == -1) {
#if !defined(NDEBUG)
	fprintf(stderr, "Function: %s: WriteBlock(%d) Failure\n", __PRETTY_FUNCTION__, FREE_BLOCK_BITMAP);
#endif
	return -1;
      }
      printf("GLOFS: Saisie bloc %d\n", current);
      fileEntry->Block[fileEntry->iNodeStat.st_blocks] = current;
      ++(fileEntry->iNodeStat.st_blocks);
      return 0;
    }
    ++current;
  }
  return -1;
}

int			freeBlock(iNodeEntry *fileEntry,
				  int number)
{
  bool_t	blockBitmap[BLOCK_SIZE];
  UINT16	current;

  if (number > fileEntry->iNodeStat.st_blocks) {
    return -1;
  }
  if (ReadBlock(FREE_BLOCK_BITMAP, (char *)blockBitmap) == -1) {
#if !defined(NDEBUG)
    fprintf(stderr, "Function: %s: ReadBlock(%d) Failure\n", __PRETTY_FUNCTION__, FREE_BLOCK_BITMAP);
#endif
    return -1;
  }
  while (number != 0) {
    current = fileEntry->Block[fileEntry->iNodeStat.st_blocks - 1];
    blockBitmap[current] = BLOCK_FREE;
    printf("GLOFS: Relache bloc %d\n", current);
    --(fileEntry->iNodeStat.st_blocks);
    --number;
  }
  fileEntry->iNodeStat.st_size = (BLOCK_SIZE * fileEntry->iNodeStat.st_blocks);
  if (WriteBlock(FREE_BLOCK_BITMAP, (char *)blockBitmap) == -1) {
#if !defined(NDEBUG)
    fprintf(stderr, "Function: %s: WriteBlock(%d) Failure\n", __PRETTY_FUNCTION__, FREE_BLOCK_BITMAP);
#endif
    return -1;
  }
  return 0;
}
