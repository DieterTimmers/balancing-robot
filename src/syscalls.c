#include <sys/types.h>
#include <sys/stat.h>
#include <errno.h>
#include "stm32f4xx.h"

/* Retarget newlib's _write -> ITM stimulus port 0 (SWO).
 * Viewed with: st-trace, OpenOCD `tpiu config`, or Segger's SWO viewer. */
int _write(int fd, const char *buf, int len) {
    (void)fd;
    for (int i = 0; i < len; ++i) {
        ITM_SendChar((uint32_t)buf[i]);
    }
    return len;
}

/* Minimal newlib stubs so the linker is happy (we don't use files/heap/signals). */
int   _close(int fd)                        { (void)fd; return -1; }
int   _fstat(int fd, struct stat *s)        { (void)fd; (void)s; return 0; }
int   _isatty(int fd)                       { (void)fd; return 1; }
off_t _lseek(int fd, off_t o, int w)        { (void)fd; (void)o; (void)w; return 0; }
int   _read(int fd, char *b, int l)         { (void)fd; (void)b; (void)l; return 0; }
void  _exit(int code)                       { (void)code; while (1) {} }
int   _kill(int pid, int sig)               { (void)pid; (void)sig; errno = EINVAL; return -1; }
int   _getpid(void)                         { return 1; }
