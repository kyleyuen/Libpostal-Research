// Windows counterpart of research/tools/bench_ext.c. One parsing thread, same input/order/API work.
// usage: bench_ext_win target.tsv passes [other-dataset-warmup.tsv]
#define WIN32_LEAN_AND_MEAN
#include <windows.h>
#include <psapi.h>
#include <tlhelp32.h>
#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include "libpostal.h"

#define MAXC 256
static LARGE_INTEGER freq;
static double now_us(void) { LARGE_INTEGER t; QueryPerformanceCounter(&t); return (double)t.QuadPart * 1e6 / (double)freq.QuadPart; }
static uint64_t filetime_us(FILETIME t) { ULARGE_INTEGER u; u.LowPart = t.dwLowDateTime; u.HighPart = t.dwHighDateTime; return u.QuadPart / 10; }
static uint64_t cpu_us(void) {
    FILETIME created, exited, kernel, user;
    if (!GetProcessTimes(GetCurrentProcess(), &created, &exited, &kernel, &user)) return 0;
    return filetime_us(kernel) + filetime_us(user);
}
static unsigned thread_count(void) {
    unsigned n = 0; DWORD pid = GetCurrentProcessId();
    HANDLE snap = CreateToolhelp32Snapshot(TH32CS_SNAPTHREAD, 0);
    if (snap == INVALID_HANDLE_VALUE) return 0;
    THREADENTRY32 e = { .dwSize = sizeof e };
    if (Thread32First(snap, &e)) do { if (e.th32OwnerProcessID == pid) n++; } while (Thread32Next(snap, &e));
    CloseHandle(snap); return n;
}
static char *read_line(FILE *f) {
    size_t n = 0, cap = 256; int c; char *s = malloc(cap);
    if (!s) return NULL;
    while ((c = fgetc(f)) != EOF && c != '\n') {
        if (n + 1 == cap) { char *p = realloc(s, cap *= 2); if (!p) { free(s); return NULL; } s = p; }
        s[n++] = (char)c;
    }
    if (c == EOF && n == 0) { free(s); return NULL; }
    if (n && s[n - 1] == '\r') n--;
    s[n] = 0; return s;
}
static int warmup(const char *path, libpostal_address_parser_options_t o) {
    FILE *f = fopen(path, "rb"); if (!f) return -1;
    int count = 0; char *line;
    while ((line = read_line(f))) {
        char *tab = strchr(line, '\t');
        if (tab) { libpostal_address_parser_response_t *r = libpostal_parse_address(tab + 1, o);
            if (!r) { free(line); fclose(f); return -1; }
            libpostal_address_parser_response_destroy(r); count++; }
        free(line);
    }
    fclose(f); return count;
}
int main(int argc, char **argv) {
    if (argc != 3 && argc != 4) return 2;
    QueryPerformanceFrequency(&freq);
    int passes = atoi(argv[2]); if (passes < 1) return 2;
    FILE *f = fopen(argv[1], "rb"); if (!f) return 2;
    char **addr = NULL, cnames[MAXC][8] = {{0}}; int *cid = NULL;
    size_t n = 0, cap = 0; int nc = 0; char *line;
    while ((line = read_line(f))) {
        char *tab = strchr(line, '\t'); if (!tab) { free(line); return 2; }
        *tab++ = 0;
        int c = -1; for (int i = 0; i < nc; i++) if (!strcmp(cnames[i], line)) { c = i; break; }
        if (c < 0) { if (nc == MAXC) { free(line); return 2; } strncpy(cnames[nc], line, 7); c = nc++; }
        if (n == cap) {
            size_t next = cap ? cap * 2 : 1024;
            char **a = realloc(addr, next * sizeof *addr); if (!a) return 2; addr = a;
            int *ids = realloc(cid, next * sizeof *cid); if (!ids) return 2; cid = ids; cap = next;
        }
        addr[n] = strdup(tab); if (!addr[n]) return 2; cid[n++] = c; free(line);
    }
    fclose(f); if (!n) return 2;
    double s0 = now_us();
    if (!libpostal_setup() || !libpostal_setup_parser()) return 2;
    printf("setup_seconds=%.3f corpus=%zu countries=%d\n", (now_us() - s0) / 1e6, n, nc);
    libpostal_address_parser_options_t o = libpostal_get_address_parser_default_options();
    if (argc == 4) { int w = warmup(argv[3], o); if (w < 0) return 2; printf("warmup_addresses=%d file=%s\n", w, argv[3]); }
    double *ct = calloc((size_t)nc, sizeof *ct); size_t *cn = calloc((size_t)nc, sizeof *cn);
    if (!ct || !cn) return 2;
    uint64_t comps = 0; unsigned max_threads = thread_count();
    for (int p = 1; p <= passes; p++) {
        memset(ct, 0, (size_t)nc * sizeof *ct); memset(cn, 0, (size_t)nc * sizeof *cn);
        uint64_t c0 = cpu_us(); double t0 = now_us();
        for (size_t i = 0; i < n; i++) {
            double a = now_us();
            libpostal_address_parser_response_t *r = libpostal_parse_address(addr[i], o);
            if (!r || !r->num_components) return 3;
            comps += r->num_components; libpostal_address_parser_response_destroy(r);
            ct[cid[i]] += now_us() - a; cn[cid[i]]++;
        }
        double total = now_us() - t0; uint64_t cpu = cpu_us() - c0;
        unsigned threads = thread_count(); if (threads > max_threads) max_threads = threads;
        printf("pass=%d us_per_addr=%.3f addresses_per_second=%.1f cpu_us_per_addr=%.3f threads=%u\n",
               p, total / n, n * 1e6 / total, (double)cpu / n, threads);
        if (p == 1 || p == passes) for (int c = 0; c < nc; c++)
            printf("country pass=%d cc=%s n=%zu us_per_addr=%.3f\n", p, cnames[c], cn[c], ct[c] / cn[c]);
    }
    PROCESS_MEMORY_COUNTERS_EX m = { .cb = sizeof m };
    if (!GetProcessMemoryInfo(GetCurrentProcess(), (PROCESS_MEMORY_COUNTERS *)&m, sizeof m)) return 4;
    printf("components=%llu peak_working_set_bytes=%zu working_set_bytes=%zu peak_commit_bytes=%zu private_usage_bytes=%zu max_threads_sampled=%u\n",
           (unsigned long long)comps, (size_t)m.PeakWorkingSetSize, (size_t)m.WorkingSetSize,
           (size_t)m.PeakPagefileUsage, (size_t)m.PrivateUsage, max_threads);
    return 0;
}
