#include <sys/types.h>
#include <sys/stat.h>
#include <fcntl.h>
#include <unistd.h>
#include <stdio.h>
#include <stdlib.h>
#include "aesdsocket.h" 
#include <stdarg.h>
#include <errno.h>

#define file_close tcp_close

int file_create(char *pathname, ...) {

    int fd, flags = 0, number;
    va_list ap;

    va_start(ap, pathname);

    number = va_arg(ap, int);
    while (number-- > 0) {
        flags |= va_arg(ap, int);
    }

    fd = open(pathname, flags, S_IRWXU | S_IRWXG | S_IRWXO);
    if (fd == -1) {
        ERROR_HANDLER(open);
    }

    va_end(ap);
    return fd;
}

int file_seek(int fd, off_t offset, int whence) {

    int rc;
    rc = (int) lseek(fd, offset, whence);
    if (rc == -1) {
        //fprintf(stdout, "err: %d, %s\n", errno, strerror(errno));
        ERROR_HANDLER();
    }
    return rc;
}

int file_write(int fd, char *buffer, int size) {

    ssize_t s;

    file_seek(fd, 0, SEEK_END);
    s = write(fd, buffer, size);
    if (s != size) {
        ERROR_HANDLER(write);
    }
    return s;
}

int file_size(int fd) {

    int front, end;

    end = file_seek(fd, 0, SEEK_END);
    front = file_seek(fd, 0, SEEK_SET);

    return end - front;
}

int file_read(int fd, char *buffer, int fsize) {

    int rc = 0;
    if (rc == -1) {
        ERROR_HANDLER(lseek); 
    }
    file_seek(fd, 0, SEEK_SET);
    rc = read(fd, buffer, fsize);
#if 0
    printf("fd: %d\n", fd);
    printf("[%s] %s", __func__, buffer);
    printf("rc: %d, size: %d\n", rc, fsize);
#endif

    return rc;
}

void file_delete(char *file) {

    int rc = 0;
    struct stat status;

    memset(&status, 0, sizeof(struct stat));

    rc = stat(file, &status);
    if (rc != -1) {
        remove(file);
    } else {
        perror("stat ");
	}
    return;
}
