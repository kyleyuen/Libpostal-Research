// 041: does this macOS/arm64 machine grant superpages (larger than 16 KB) to user allocations?
#include <stdio.h>
#include <mach/mach.h>
#include <mach/mach_vm.h>
#include <mach/vm_statistics.h>
#include <sys/mman.h>
int main(void) {
    mach_vm_address_t a = 0; kern_return_t kr;
    kr = mach_vm_allocate(mach_task_self(), &a, 64ull << 20, VM_FLAGS_ANYWHERE | VM_FLAGS_SUPERPAGE_SIZE_ANY);
    printf("mach_vm_allocate SUPERPAGE_SIZE_ANY: kr=%d (%s)\n", kr, mach_error_string(kr));
#ifdef VM_FLAGS_SUPERPAGE_SIZE_2MB
    a = 0; kr = mach_vm_allocate(mach_task_self(), &a, 64ull << 20, VM_FLAGS_ANYWHERE | VM_FLAGS_SUPERPAGE_SIZE_2MB);
    printf("mach_vm_allocate SUPERPAGE_SIZE_2MB: kr=%d (%s)\n", kr, mach_error_string(kr));
#else
    printf("VM_FLAGS_SUPERPAGE_SIZE_2MB not defined on this platform\n");
#endif
    void *p = mmap(NULL, 64ull << 20, PROT_READ | PROT_WRITE, MAP_ANON | MAP_PRIVATE, VM_FLAGS_SUPERPAGE_SIZE_ANY, 0);
    printf("mmap fd=VM_FLAGS_SUPERPAGE_SIZE_ANY: %s\n", p == MAP_FAILED ? "MAP_FAILED" : "ok");
    return 0;
}
