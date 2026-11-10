// Minimal newlib system call stubs: the firmware has no file I/O.
#include <errno.h>
#include <sys/stat.h>

int _close(int fd) {
    return -1;
}

int _fstat(int fd, struct stat *st) {
    st->st_mode = S_IFCHR;
    return 0;
}

int _isatty(int fd) {
    return 1;
}

int _lseek(int fd, int offset, int whence) {
    return 0;
}

int _read(int fd, char *buf, int len) {
    return 0;
}

int _write(int fd, const char *buf, int len) {
    return len;
}

int _getpid(void) {
    return 1;
}

int _kill(int pid, int sig) {
    errno = EINVAL;
    return -1;
}
