#ifndef IO_OPERATION_H_
#define IO_OPERATION_H_

int _bd_write(const char *pFilename, const char *buffer, int offset, int numbytes);
int _bd_read(const char *pFilename, char *buffer, int offset, int numbytes);
int _bd_truncate(const char *pFilename, int offset);
#endif