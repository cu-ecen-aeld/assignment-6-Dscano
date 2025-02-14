#include <sys/types.h>
#include <sys/stat.h>
#include <fcntl.h>
#include <unistd.h>
#include <stdio.h>
#include <stdlib.h>

int file_create(char *pathname, ...);

int file_write(int fd, char *buffer, int size);

int file_seek(int fd, off_t offset, int whence);

int file_size(int fd);

int file_read(int fd, char *buffer, int size);

void file_delete(char *file);
