#include <stdio.h>
#include <stdlib.h>
#include <stdint.h>
#include <string.h>
#include <time.h>
#include <unistd.h>
#include <sys/resource.h>
#include "libpostal.h"

static double now(clockid_t clock_id) {
    struct timespec t;
    if (clock_gettime(clock_id, &t)) exit(2);
    return t.tv_sec + t.tv_nsec / 1e9;
}

int main(int argc, char **argv) {
    if (argc != 3) {
        fprintf(stderr, "usage: %s addresses.txt seconds\n", argv[0]);
        return 2;
    }
    double duration = strtod(argv[2], NULL);
    if (duration <= 0) return 2;
    FILE *f = fopen(argv[1], "r");
    if (!f) return 2;
    char **addresses = NULL, *line = NULL;
    size_t n = 0, cap = 0, address_capacity = 0;
    ssize_t len;
    while ((len = getline(&line, &cap, f)) >= 0) {
        while (len && (line[len-1] == '\n' || line[len-1] == '\r')) line[--len] = 0;
        if (!len) continue;
        if (n == address_capacity) {
            size_t next_capacity = address_capacity ? address_capacity * 2 : 1024;
            char **next = realloc(addresses, next_capacity * sizeof(*addresses));
            if (!next) return 2;
            addresses = next;
            address_capacity = next_capacity;
        }
        if (!(addresses[n] = strdup(line))) return 2;
        n++;
    }
    free(line);
    fclose(f);
    if (!n || !libpostal_setup() || !libpostal_setup_parser()) return 2;
    libpostal_address_parser_options_t options = libpostal_get_address_parser_default_options();
    uint64_t count = 0, components = 0;
    for (size_t i = 0; i < n; i++) {
        libpostal_address_parser_response_t *r = libpostal_parse_address(addresses[i], options);
        if (!r || !r->num_components) return 3;
        libpostal_address_parser_response_destroy(r);
    }
    fprintf(stderr, "READY pid=%d corpus=%zu\n", getpid(), n);
    fflush(stderr);
    double cpu0 = now(CLOCK_PROCESS_CPUTIME_ID), start = now(CLOCK_MONOTONIC), end;
    do {
        for (size_t i = 0; i < n; i++) {
            libpostal_address_parser_response_t *r = libpostal_parse_address(addresses[i], options);
            if (!r || !r->num_components) return 3;
            components += r->num_components;
            libpostal_address_parser_response_destroy(r);
            count++;
        }
        end = now(CLOCK_MONOTONIC);
    } while (end - start < duration);
    double cpu = now(CLOCK_PROCESS_CPUTIME_ID) - cpu0;
    struct rusage usage;
    if (getrusage(RUSAGE_SELF, &usage)) return 2;
    printf("addresses=%llu wall_seconds=%.6f cpu_seconds=%.6f addresses_per_second=%.2f us_per_address=%.2f peak_rss_bytes=%ld components=%llu\n",
        (unsigned long long)count, end-start, cpu, count/(end-start), 1e6*(end-start)/count,
        usage.ru_maxrss, (unsigned long long)components);
    libpostal_teardown_parser();
    libpostal_teardown();
    for (size_t i = 0; i < n; i++) free(addresses[i]);
    free(addresses);
    return 0;
}
