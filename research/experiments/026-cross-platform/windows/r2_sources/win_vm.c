#define WIN32_LEAN_AND_MEAN
#include <windows.h>
#include <stdint.h>
#include "win_vm.h"

void *fd_win_reserve(size_t bytes) {
    const size_t gran = 65536;
    if (bytes > SIZE_MAX - (gran - 1)) return NULL;
    size_t rounded = (bytes + gran - 1) & ~(gran - 1);
    return VirtualAlloc(NULL, rounded, MEM_RESERVE, PAGE_NOACCESS);
}
bool fd_win_commit(void *base, size_t offset, size_t bytes) {
    return VirtualAlloc((char *)base + offset, bytes, MEM_COMMIT, PAGE_READWRITE) != NULL;
}
void fd_win_release(void *base) {
    VirtualFree(base, 0, MEM_RELEASE);
}
double fd_win_now(void) {
    LARGE_INTEGER freq, ticks;
    QueryPerformanceFrequency(&freq); QueryPerformanceCounter(&ticks);
    return (double)ticks.QuadPart / (double)freq.QuadPart;
}
