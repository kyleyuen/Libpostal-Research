#ifndef FD_WIN_VM_H
#define FD_WIN_VM_H
#include <stdbool.h>
#include <stddef.h>
void *fd_win_reserve(size_t bytes);
bool fd_win_commit(void *base, size_t offset, size_t bytes);
void fd_win_release(void *base);
double fd_win_now(void);
#endif
