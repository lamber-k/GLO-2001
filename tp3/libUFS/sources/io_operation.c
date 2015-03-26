#include <string.h>
#include <stdio.h>
#include "utils.h"
#include "disque.h"

static int addBlock(iNodeEntry *fileEntry)
{
	BOOL	blockBitmap[BLOCK_SIZE];
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
			fileEntry->Block[fileEntry->iNodeStat.st_blocks] = current;
			++(fileEntry->iNodeStat.st_blocks);
			return 0;
		}
		++current;
	}
	return -1;
}

static int freeBlock()
{
	return -1;
}

static int _find_block_by_offset(iNodeEntry *fileEntry, int offset)
{
	if (offset > fileEntry->iNodeStat.st_size) {
		offset = fileEntry->iNodeStat.st_size;
	}
	int position = offset / BLOCK_SIZE;

	if (position > N_BLOCK_PER_INODE) {
#if !defined(NDEBUG)
		fprintf(stderr, "Function: %s: offset(%d) is out of range\n", __PRETTY_FUNCTION__, offset);
#endif
		return -1;
	}
	return position;
}

static int _write_data(iNodeEntry *fileEntry, const char *buffer, int offset, int numbytes)
{
	int currentBlockPosition = -1;
	int numbytes_write = 0;
	char writeBuffer[BLOCK_SIZE];
	char isNewBlock = 0;
	int error;

	if (fileEntry->iNodeStat.st_blocks != 0 && (currentBlockPosition = _find_block_by_offset(fileEntry, offset)) == -1) {
		return -1;
	}
	fileEntry->iNodeStat.st_size = offset;
	while (numbytes_write != numbytes) {
		isNewBlock = 0;
		if (BLOCK_SIZE * fileEntry->iNodeStat.st_blocks == fileEntry->iNodeStat.st_size) {
			if ((error = addBlock(fileEntry)) == -1) {
#if !defined(NDEBUG)
				fprintf(stderr, "Function: %s: cannot add block to current file\n", __PRETTY_FUNCTION__);
#endif
				return error;
			}
			++currentBlockPosition;
			isNewBlock = 1;
		}
		int localOffset = offset % BLOCK_SIZE;
		int writeSize = numbytes - numbytes_write > BLOCK_SIZE ? BLOCK_SIZE - localOffset : numbytes - numbytes_write;
		if (!isNewBlock) {
			if (ReadBlock(fileEntry->Block[currentBlockPosition], writeBuffer) == -1) {
#if !defined(NDEBUG)
				fprintf(stderr, "Function: %s: ReadBlock(%d) Failure\n", __PRETTY_FUNCTION__, fileEntry->Block[currentBlockPosition]);
#endif
				return -1;
			}

		}
		memcpy(writeBuffer + localOffset,
		       buffer + numbytes_write,
		       writeSize);
		if (WriteBlock(fileEntry->Block[currentBlockPosition], writeBuffer) == -1) {
#if !defined(NDEBUG)
			fprintf(stderr, "Function: %s: WriteBlock(%d) Failure\n", __PRETTY_FUNCTION__, fileEntry->Block[currentBlockPosition]);
#endif
			return -1;
		}
		offset = 0;
		numbytes_write += writeSize;
		fileEntry->iNodeStat.st_size += writeSize;
	}
	if (saveINodeEntry(fileEntry) == -1) {
		return -1;
	}
	return numbytes;
}

int _bd_write(const char *pFilename, const char *buffer, int offset, int numbytes)
{
	iNodeEntry fileEntry;
	char *path = strdup(pFilename);

	if (path == NULL) {
#if defined(NDEBUG)
		fprintf(stderr, "Function: %s: strdup failure", __PRETTY_FUNCTION__);
#endif
		return -1;
	}
	if (resolvePath(path, &fileEntry) == -1) {
		free(path);
#if defined(NDEBUG)
		fprintf(stderr, "Function: %s: resolvePath(%s) failure", __PRETTY_FUNCTION__, path);
#endif
		return -1;
	}
	free(path);
	if (G_ISDIR(fileEntry.iNodeStat.st_mode)) {
#if defined(NDEBUG)
		fprintf(stderr, "Function: %s: %s is a directory", __PRETTY_FUNCTION__, path);
#endif
		return -2;
	}
	if (offset >= fileEntry.iNodeStat.st_size) {
#if defined(NDEBUG)
		fprintf(stderr, "Function: %s: offset: %d is out of range", __PRETTY_FUNCTION__, offset);
#endif
		return -3;
	}
	return _write_data(&fileEntry, buffer, offset, numbytes);
}

static int _read_data(iNodeEntry *fileEntry, char *buffer, int offset, int numbytes)
{
	char readBuffer[BLOCK_SIZE];
	int currentBlockPosition;
	int numbytes_read = 0;

	while (numbytes_read != numbytes) {
		if ((currentBlockPosition = _find_block_by_offset(fileEntry, offset)) == -1) {
			return -1;
		}
		if (ReadBlock(fileEntry->Block[currentBlockPosition], readBuffer) == -1) {
#if !defined(NDEBUG)
			fprintf(stderr, "Function: %s: ReadBlock(%d) Failure\n", __PRETTY_FUNCTION__, fileEntry->Block[currentBlockPosition]);
#endif
			return -1;
		}
		int localOffset = offset % BLOCK_SIZE;
		int readSize = (numbytes - numbytes_read > BLOCK_SIZE ? BLOCK_SIZE - localOffset : numbytes - numbytes_read);
		memcpy(buffer + numbytes_read,
		       readBuffer + localOffset,
		       readSize);
		numbytes_read += readSize;
		offset += readSize;
	}
	return numbytes_read;
}

int _bd_read(const char *pFilename, char *buffer, int offset, int numbytes)
{
	iNodeEntry fileEntry;
	char *path = strdup(pFilename);

	if (path == NULL) {
#if defined(NDEBUG)
		fprintf(stderr, "Function: %s: strdup failure", __PRETTY_FUNCTION__);
#endif
		return -1;
	}
	if (resolvePath(path, &fileEntry) == -1) {
		free(path);
#if defined(NDEBUG)
		fprintf(stderr, "Function: %s: resolvePath(%s) failure", __PRETTY_FUNCTION__, path);
#endif
		return -1;
	}
	free(path);
	if (G_ISDIR(fileEntry.iNodeStat.st_mode)) {
#if defined(NDEBUG)
		fprintf(stderr, "Function: %s: %s is a directory", __PRETTY_FUNCTION__, path);
#endif
		return -2;
	}
	if (offset >= fileEntry.iNodeStat.st_size) {
#if defined(NDEBUG)
		fprintf(stderr, "Function: %s: offset: %d is out of range", __PRETTY_FUNCTION__, offset);
#endif
		return -3;
	}
	return _read_data(&fileEntry, buffer, offset, (offset + numbytes > fileEntry.iNodeStat.st_size ? fileEntry.iNodeStat.st_size - offset : numbytes));
}
